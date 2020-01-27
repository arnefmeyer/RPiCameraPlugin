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

    def __init__(self, *args, **kwargs):

        super(VideoEncoderGPIO, self).__init__(*args, **kwargs)

        self.strobe_pin = 11
        self.frame_count = 0
        self.trigger_count = 0
        self.t_start = 0

    def set_strobe_pin(self, p):

        self.strobe_pin = p

    def start(self, output, motion_output=None):

        super(VideoEncoderGPIO, self).start(output, motion_output)

        self.t_start = time.time()

    def close(self):

        t_run = time.time() - self.t_start
        print("frame rate:", self.frame_count / t_run)
        print("trigger signals:", self.trigger_count)

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

                    GPIO.output(self.strobe_pin, True)
                    time.sleep(0.001)
                    GPIO.output(self.strobe_pin, False)

                    self.trigger_count += 1

                if buf.pts < 0:
                    # this usually happens if the video quality is set to
                    # a low value (= high quality). Try something in the range
                    # 20 to 25.
                    print("invalid time time stamp (buf.pts < 0):", buf.pts)

                self.parent.write_timestamps(buf.pts, current_ts)
                self.frame_count += 1

        return super(VideoEncoderGPIO, self)._callback_write(buf, **kwargs)


class CameraGPIO(picamera.PiCamera):

    def __init__(self,
                 framerate=30.,
                 resolution=(640, 480),
                 clock_mode='raw',
                 strobe_pin=11,
                 **kwargs):

        super(CameraGPIO, self).__init__(framerate=framerate,
                                         resolution=resolution,
                                         clock_mode=clock_mode)

        for (k, value) in kwargs.items():
            if hasattr(self, k):
                setattr(self, k, value)

        self.strobe_pin = strobe_pin

        if framerate != self.framerate:
            print("Camera: changing framerate from ", framerate,
                  " to ", self.framerate)

        self.ts_file = None

        if GPIO_AVAILABLE and self.strobe_pin is not None:
            print("Camera: setting GPIO strobe pin ", self.strobe_pin)
            GPIO.setup(self.strobe_pin, GPIO.OUT)
            GPIO.output(self.strobe_pin, False)

    def __del__(self):

        if GPIO_AVAILABLE:
            GPIO.cleanup()

    def _get_video_encoder(self, *args, **kwargs):

        encoder = VideoEncoderGPIO(self, *args, **kwargs)
        encoder.set_strobe_pin(self.strobe_pin)

        return encoder

    def start_recording(self, output, **kwargs):

        ts_path = op.splitext(output)[0] + '_timestamps.csv'
        try:
            self.ts_file = open(ts_path, 'w')
            self.ts_file.write('# frame timestamp, TTL timestamp\n')
            print("Saving timestamps to:", ts_path)

        except BaseException:
            print("Could not open time stamp file:", ts_path)
            traceback.print_exc()

        super(CameraGPIO, self).start_recording(output, **kwargs)

    def stop_recording(self):

        if self.ts_file is not None:

            # make sure all (buffered) data are being written
            self.ts_file.flush()
            os.fsync(self.ts_file.fileno())

            self.ts_file.close()
            self.ts_file = None

        try:
            # catch "ValueError: I/O operation on closed file" exception
            super(CameraGPIO, self).stop_recording()
        except BaseException:
            traceback.print_exc()

    def write_timestamps(self, pts, ets):

        if self.ts_file is not None:
            self.ts_file.write("{},{}\n".format(pts, ets))
