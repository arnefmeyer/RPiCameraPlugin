#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    Control-related functionality.

    For RPi GPIO pinout see https://pinout.xyz. Note that this script uses
    the "Board" pin mode as the the "BCM" mode might differ between different
    RPi versions.
"""

from __future__ import print_function

import os
import os.path as op
import time
import traceback
import json
import threading
import zmq

from .camera import CameraGPIO


class ZmqThread(threading.Thread):
    """Handle communication with an open-ephys plugin (or any other zmq client)

    """

    def __init__(self, start_callback, stop_callback, close_callback,
                 parameter_callback):

        super(ZmqThread, self).__init__()

        self.start_callback = start_callback
        self.stop_callback = stop_callback
        self.close_callback = close_callback
        self.parameter_callback = parameter_callback

        self.url = 'tcp://*:5555'
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind(self.url)

        self.is_running = False
        self.daemon = True

    def stop_running(self):

        self.is_running = False

    def run(self):

        self.is_running = True

        socket = self.socket

        while self.is_running:

            msg = socket.recv()
            parts = msg.split()
            cmd = parts[0]
            if cmd == 'Start':

                experiment = 0
                recording = 1
                path = None

                for p in parts[1:]:
                    name, value = p.split('=')
                    if name == 'Experiment':
                        experiment = int(value)
                    elif name == 'Recording':
                        recording = int(value)
                    elif name == 'Path':
                        path = value

                rec_path = self.start_callback(experiment, recording, path)

                if rec_path is None:
                    rec_path = ''

                socket.send(rec_path)

            elif cmd == 'Stop':

                self.parameter_callback('Stop', None)
                socket.send('Stopped')

            elif cmd == 'Close':

                self.parameter_callback('Close', None)
                socket.send('Closing')
                break

            elif cmd == 'Resolution':

                width = int(parts[1])
                height = int(parts[2])
                self.parameter_callback('Resolution', (width, height))
                socket.send("Done")

            elif cmd == 'Framerate':

                fps = float(parts[1])
                self.parameter_callback('Framerate', fps)
                socket.send("Done")

            elif cmd == 'ResetGains':

                self.parameter_callback('ResetGains', None)
                socket.send("Done")

            elif cmd == 'VFlip':

                self.parameter_callback('VFlip', int(parts[1]) > 0)
                socket.send("Done")

            elif cmd == 'HFlip':

                self.parameter_callback('HFlip', int(parts[1]) > 0)
                socket.send("Done")

            elif cmd == 'Zoom':

                self.parameter_callback('Zoom', parts[1:])
                socket.send("Done")

            else:
                socket.send("Not handled")


class Controller(object):

    def __init__(self, data_path, **kwargs):

        super(Controller, self).__init__()

        self.data_path = data_path
        self.closed = False

        try:
            self.camera = CameraGPIO(**kwargs)

        except BaseException:
            traceback.print_exc()
            self.camera = None

    def __del__(self):

        self.cleanup()

    @property
    def framerate(self):

        if self.camera is not None:
            return self.camera.framerate

    @framerate.setter
    def framerate(self, fps):

        if self.camera is not None and not self.camera.recording:
            self.camera.framerate = fps

    @property
    def resolution(self):

        if self.camera is not None:
            return self.camera.resolution

    @resolution.setter
    def resolution(self, xy):

        if self.camera is not None and not self.camera.recording:
            self.camera.resolution = xy

    @property
    def vflip(self):

        if self.camera is not None:
            return self.camera.vflip

    @vflip.setter
    def vflip(self, status):

        if self.camera is not None and not self.camera.recording:
            self.camera.vflip = status

    @property
    def hflip(self):

        if self.camera is not None:
            return self.camera.hflip

    @hflip.setter
    def hflip(self, status):

        if self.camera is not None and not self.camera.recording:
            self.camera.hflip = status

    @property
    def zoom(self):

        if self.camera is not None:
            return self.camera.zoom

    @zoom.setter
    def zoom(self, coords):

        if self.camera is not None:
            if len(coords) == 4 and min(coords) >= 0 and max(coords) <= 1:
                self.camera.zoom = coords

    def cleanup(self):

        if self.camera is not None:

            if self.camera.recording:
                print("Controller: stopping recording ")
                self.camera.stop_recording()

            if self.camera.previewing:
                print("Controller: stopping preview")
                self.camera.stop_preview()

            if not self.camera.closed:
                print("Camera: closing")
                self.camera.close()

            print("Controller: deleting camera")
            del self.camera
            self.camera = None

        self.closed = True

    def start_preview(self, warmup=2., fix_awb_gains=True,
                      fix_exposure_speed=True):

        if self.camera is not None:

            self.camera.awb_mode = 'auto'
            self.camera.exposure_mode = 'auto'

            self.camera.start_preview()

            # wait for camera to "warm up"
            time.sleep(warmup)

            if fix_awb_gains:
                gains = self.camera.awb_gains
                self.camera.awb_mode = 'off'
                self.camera.awb_gains = gains

            if fix_exposure_speed:
                self.camera.shutter_speed = self.camera.exposure_speed
                self.camera.exposure_mode = 'off'

    def reset_gains(self, warmup=2., fix_awb_gains=True,
                    fix_exposure_speed=True):

        if self.camera is not None:

            if self.camera.recording:
                self.camera.stop_recording()

            was_previewing = self.camera.previewing
            if self.camera.previewing:
                self.camera.stop_preview()

            if was_previewing:
                self.start_preview(warmup=warmup,
                                   fix_awb_gains=fix_awb_gains,
                                   fix_exposure_speed=fix_exposure_speed)

    def start_recording(self, filename='rpicamera_video', experiment=0,
                        recording=1, path='None', quality=23):

        # TODO: add network stream output
        if self.camera is not None and not self.camera.recording:

            rec_datetime = path.split(op.sep)[-1]
            name = '{}_experiment_{}_recording_{}'.format(rec_datetime,
                                                          experiment,
                                                          recording)

            rec_path = op.join(self.data_path, name)
            if not op.exists(rec_path):
                os.makedirs(rec_path)

            print("Saving data to:", rec_path)

            file_base = op.join(rec_path, filename)
            video_path = file_base + '.h264'
            param_file = file_base + '_params.json'

            # parameters and path information
            params = {'rec_path': rec_path,
                      'experiment': experiment,
                      'recording': recording,
                      'video_path': video_path,
                      'width': self.camera.resolution.width,
                      'height': self.camera.resolution.height,
                      'framerate': float(self.camera.framerate)}

            with open(param_file, 'w') as f:
                json.dump(params, f, indent=4,
                          sort_keys=True, separators=(',', ': '))

            print("Starting recording to video file:", video_path)
            self.camera.start_recording(video_path,
                                        format='h264',
                                        quality=quality)

        else:
            rec_path = None

        return rec_path

    def stop_recording(self):

        if self.camera is not None and self.camera.recording:

            print("Controller: stopping recording")
            self.camera.stop_recording()

    def close(self):

        print("Controller: cleaning up")
        self.cleanup()
