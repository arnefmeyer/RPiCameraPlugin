#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    Camera and encoder based on picamera.

    For RPi GPIO pinout see https://pinout.xyz. Note that this script uses
    the "Board" pin mode as the the "BCM" mode might differ between different
    RPi versions.
"""

from __future__ import print_function

import time
import os
import os.path as op
import traceback
from functools import partial

import picamera
from picamera import mmal

try:
    from RPi import GPIO
    GPIO.setmode(GPIO.BOARD)
    GPIO_AVAILABLE = True

except ImportError:
    print("Could not import RPi.GPIO module. Strobe capability not available.")
    GPIO_AVAILABLE = False


class VideoEncoderGPIO(picamera.PiVideoEncoder):

    def __init__(self, *args,
                 use_strobe=True,
                 strobe_pin=11,
                 trigger_length=.001,
                 **kwargs):

        super(VideoEncoderGPIO, self).__init__(*args, **kwargs)

        self.use_strobe = use_strobe
        self.strobe_mode = strobe_mode
        self.strobe_pin = strobe_pin
        self.strobe_length = trigger_length

        self.frame_count = 0
        self.strobe_count = 0
        self.t_start = 0

    def start(self, output,
              motion_output=None):

        super(VideoEncoderGPIO, self).start(output, motion_output)

        self.t_start = time.time()

    def close(self):

        if self.use_strobe:
            t_run = time.time() - self.t_start
            print("frame rate:", self.frame_count / t_run)
            print("trigger signals:", self.strobe_count)

        super(VideoEncoderGPIO, self).close()

    def _callback_write(self, buf, **kwargs):

        if isinstance(buf, picamera.mmalobj.MMALBuffer):
            # for firmware >= 4.4.8
            flags = buf.flags
        else:
            # for firmware < 4.4.8
            flags = buf[0].flags

        if not (flags & mmal.MMAL_BUFFER_HEADER_FLAG_CONFIG):

            if flags & mmal.MMAL_BUFFER_HEADER_FLAG_FRAME_END:

                current_ts = self.parent.timestamp

                if GPIO_AVAILABLE and self.strobe_pin is not None:

                    if self.use_strobe:

                        GPIO.output(self.strobe_pin, True)
                        time.sleep(self.trigger_length)
                        GPIO.output(self.strobe_pin, False)

                        self.strobe_count += 1

                if buf.pts < 0:
                    # this usually happens if the video quality is set to
                    # a low value (= high quality). Try something in the range
                    # 23 <= quality <= 28
                    print("invalid time time stamp (buf.pts < 0):", buf.pts)

                self.parent.write_timestamps(buf.pts, current_ts)
                self.frame_count += 1

        return super(VideoEncoderGPIO, self)._callback_write(buf, **kwargs)


class TimerThread(threading.Thread):

    def __init__(self, event, callback, interval=.1):

        threading.Thread.__init__(self)

        self.stop_event = event
        self.callback = callback
        self.interval = interval

    def run(self):

        while not self.stop_event.wait(self.interval):
            self.callback()


class CameraGPIO(picamera.PiCamera):

    def __init__(self,
                 framerate=30.,
                 resolution=(640, 480),
                 clock_mode='raw',
                 sync_mode='strobe',
                 strobe_pin=11,
                 wait_for_trigger=True,
                 trigger_timeout=1.,
                 **kwargs):

        super(CameraGPIO, self).__init__(framerate=framerate,
                                         resolution=resolution,
                                         clock_mode=clock_mode)

        for k, value in kwargs.iteritems():
            if hasattr(self, k):
                setattr(self, k, value)

        self.sync_mode = sync_mode
        self.strobe_pin = strobe_pin
        self.wait_for_trigger = wait_for_trigger
        self.trigger_timeout = trigger_timeout

        if framerate != self.framerate:
            print("Camera: changing framerate from ", framerate,
                  " to ", self.framerate)

        self.ts_file = None
        self.trigger_file = None

        self._start_on_next_trigger = False
        self._last_trigger_ts = None
        self._timer_thread = None
        self._stop_event = None
        self.lock = threading.Lock()

    def __del__(self):

        if GPIO_AVAILABLE and self.use_strobe:
            GPIO.cleanup()

    def _get_video_encoder(self, *args, **kwargs):

        encoder = VideoEncoderGPIO(self, *args,
                                   use_strobe=self.use_strobe,
                                   strobe_pin=self.strobe_pin,
                                   **kwargs)

        return encoder

    def start_recording(self, output, **kwargs):

        assert self.sync_mode in ['strobe', 'trigger']

        if GPIO_AVAILABLE:

            assert strobe_pin is not None

            if self.sync_mode == 'strobe':
                print("Camera: using GPIO pin {} as strobe output".format(self.strobe_pin))
                GPIO.setup(self.strobe_pin, GPIO.OUT,
                           initial=GPIO.LOW)

            elif self.sync_mode == 'trigger':
                print("Camera: using GPIO pin {} as sync input".format(self.strobe_pin))
                GPIO.setup(self.strobe_pin, GPIO.IN)
                GPIO.setup(channel, GPIO.IN,
                           pull_up_down=GPIO.PUD_DOWN)
                GPIO.add_event_detect(self.strobe_pin,
                                      GPIO.RISING,
                                      callback=self._trigger_callback)

        # file for camera frame timestamps (and strobe timestamps
        # if sync_mode == 'internal')
        ts_path = op.splitext(output)[0] + '_timestamps.csv'
        try:
            self.ts_file = open(ts_path, 'w')
            self.ts_file.write('# frame timestamp, TTL timestamp\n')
            print("Saving timestamps to:", ts_path)

        except BaseException:
            print("Could not open time stamp file:", ts_path)
            traceback.print_exc()

        if self.sync_mode == 'strobe':
            # RPi will generate a strobe sync signal
            super(CameraGPIO, self).start_recording(output, **kwargs)

        else:
            # use external trigger pulses for synchronization
            trigger_file_path = op.splitext(self.file_path)[0] + '_timestamps_trigger.csv'
            try:
                self.trigger_file = open(trigger_file_path, 'w')
                self.trigger_file.write(
                    '# external sync time stamp (system clock), external sync time stamp (camera clock)\n')
                print("Saving external trigger timestamps to:", trigger_file_path)

            except BaseException:
                print("Could not open time stamp file:", trigger_file_path)
                traceback.print_exc()

            if self.wait_for_trigger:
                # wait for trigger to start recording and stop recording when
                # time between external triggers exceeds "trigger_timeout";
                # triggers will be handled by the GPIO event detection callback
                # and we will use a separate thread to take care of the trigger
                # timeout.
                self._start_on_next_trigger = True
                self._last_trigger_ts = None

                self._stop_event = threading.Event()
                self._timer_thread = TimerThread(interval=.1)

            else:
                self._start_on_next_trigger = False
                super(CameraGPIO, self).start_recording(output, **kwargs)

    def _trigger_callback(self, output, pin):

        # get current time stamp from camera firmware and append to file
        t0 = self.timestamp
        self.write_trigger_timestamp(t0)
        with self.lock:
            self._last_trigger_ts = t0

        if self.wait_for_trigger and self._start_on_next_trigger:
            super(CameraGPIO, self).start_recording(output, **kwargs)
            self._start_on_next_trigger = False

    def get_time_since_last_trigger(self):

        if self._last_trigger_ts is None:
            t = None
        else:
            # convert to seconds
            t = (self.timestamp - self._last_trigger_ts) / 1000000.

        return t

    def check_trigger_condition(self):

        if self.wait_for_trigger:

            t = self.get_time_since_last_trigger()

            if t > self.trigger_timeout:

                if self._stop_event is not None:
                    self._stop_event.set()

                self.stop_recording()

    def stop_recording(self):

        if GPIO_AVAILABLE and self.sync_mode == 'trigger':
            GPIO.remove_event_detect(self.strobe_pin)

        if self._stop_event is not None:
            self._stop_event.set()

        if self.ts_file is not None:

            # make sure all (buffered) data are being written
            self.ts_file.flush()
            os.fsync(self.ts_file.fileno())
            time.sleep(.1)

            self.ts_file.close()
            self.ts_file = None

        if self.trigger_file is not None:

            # make sure all (buffered) data are being written
            self.trigger_file.flush()
            os.fsync(self.trigger_file.fileno())
            time.sleep(.1)

            self.trigger_file.close()
            self.trigger_file = None

        try:
            # catch "ValueError: I/O operation on closed file" exception
            super(CameraGPIO, self).stop_recording()

        except BaseException:
            traceback.print_exc()

    def write_timestamps(self, pts, ets):
        # TODO: write timestamps in batches for efficiency

        if self.ts_file is not None:
            self.ts_file.write("{},{}\n".format(pts, ets))

    def write_trigger_timestamp(self, cam_ts):
        # TODO: write timestamps in batches for efficiency

        if self.trigger_file is not None:
            self.trigger_file.write("{}\n".format(cam_ts))
