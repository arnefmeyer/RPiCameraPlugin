#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3
"""
    Script to control the RPi camera.

    Two commands are available:

        - "plugin": run in plugin mode. Use this with the open-ephys RPi camera
                    open-ephys plugin (or any other program via zmq)
        - "standalone": run without open-ephys plugin. Recording stops after
                        a user-defined timeout or user interrupt (ctrl+c).

    In both cases, the script will generate the following files:

        - *.h264: the video data in h264 format
        - *_info.json: a json file with video parameters, e.g., resolution
        - *_timestamps.txt: a text file (csv) with frame/TTL timestamps

"""

from __future__ import print_function

import argparse
import sys
import os
import os.path as op
import time

try:
    from rpicamera.controller import Controller, ZmqThread
except ImportError:
    sys.path.append(op.join(op.split(__file__)[0], '..'))
    from rpicamera.controller import Controller, ZmqThread


def run_plugin(output=None,
               width=640,
               height=480,
               framerate=30.,
               name='rpicamera_video',
               quality=23,
               strobe_pin=11,
               zoom=(0, 0, 1, 1),
               sync_mode='strobe',
               **kwargs):

    assert sync_mode in ['strobe', 'trigger']

    if output is None:
        output = op.join(op.split(op.realpath(__file__))[0], 'RPiCameraVideos')
    else:
        output = op.realpath(op.expanduser(output))

    if not op.exists(output):
        os.makedirs(output)

    print("Output path:", output)

    print("Opening controller and camera")
    controller = Controller(output,
                            framerate=framerate,
                            resolution=(width, height),
                            strobe_pin=strobe_pin,
                            zoom=zoom,
                            sync_mode=sync_mode,
                            **kwargs)

    print("Starting preview and warming up camera for 2 seconds")
    controller.start_preview(warmup=2., fix_awb_gains=True)

    def start_cam(exp, rec, path):
        return controller.start_recording(experiment=exp,
                                          recording=rec,
                                          path=path,
                                          filename=name,
                                          quality=quality)

    def stop_cam():
        controller.stop_recording()

    def close_cam():
        controller.close()

    def set_parameter(param, value):

        if param == 'Framerate':
            print("Setting frame rate to: {} Hz".format(value)),
            controller.framerate = value

        elif param == 'Resolution':
            print("Setting resolution to: {}".format(value)),
            controller.resolution = value

        elif param == 'VFlip':
            print("Setting vflip to: {}".format(value)),
            controller.vflip = value

        elif param == 'HFlip':
            print("Setting hflip to: {}".format(value)),
            controller.hflip = value

        elif param == 'ResetGains':
            print("Resetting camera gains")
            controller.reset_gains()

        elif param == 'Stop':
            print("Stopping camera")
            controller.stop_recording()

        elif param == 'Close':
            print("Closing camera")
            controller.close()

        elif param == 'Zoom':
            print("Setting zoom to:", value)
            controller.zoom = value

    print("Starting ZMQ thread")
    thread = ZmqThread(start_cam, stop_cam, close_cam, set_parameter)
    thread.start()

    while not controller.closed:

        try:
            time.sleep(0.25)
        except KeyboardInterrupt:
            print("Keyboard interrupt. Cleaning up and exiting.")
            controller.close()
            sys.exit(0)  # workaround for stopping (daemon) zmq thread

    thread.join()


def run_standalone(output=None,
                   width=640,
                   height=480,
                   framerate=30.,
                   timeout=600,
                   strobe_pin=11,
                   name='test',
                   quality=23,
                   update_interval=5,
                   verbose=True,
                   zoom=(0, 0, 1, 1),
                   sync_mode='strobe',
                   wait_for_trigger=True,
                   **kwargs):
    """run camera in standalone mode (i.e. without open-ephys plugin)

        The recording will stop after a given timeout (default: 600 seconds)
        or after pressing ctrl+c. Use timeout < 0 for no timeout.

    """

    assert sync_mode in ['strobe', 'trigger']

    if output is None:
        output = op.join(op.split(__file__)[0], 'RPiCamera_test')
    else:
        output = op.realpath(op.expanduser(output))

    if not op.exists(output):
        os.makedirs(output)

    print("Data path:", output)

    print("Opening controller and camera")
    controller = Controller(output,
                            framerate=framerate,
                            resolution=(width, height),
                            strobe_pin=strobe_pin,
                            zoom=zoom,
                            sync_mode=sync_mode,
                            wait_for_trigger=wait_for_trigger,
                            **kwargs)

    print("Starting preview and warming up camera for 2 seconds")
    controller.start_preview(warmup=2.,
                             fix_awb_gains=True)
    time.sleep(2)

    print("Starting recording")
    filename = '{}_{}x{}_{}hz'.format(name,
                                      controller.resolution[0],
                                      controller.resolution[1],
                                      int(controller.framerate))
    controller.start_recording(experiment=1,
                               recording=0,
                               filename=name,
                               path=filename,
                               quality=quality)

    if sync_mode == 'trigger' and wait_for_trigger:
        # starting and stopping of recording will be controlled
        # by external hardware trigger
        time.sleep(1.)
        try:
            while controller.is_recording:
                time.sleep(.1)

        except KeyboardInterrupt:
            print("Keyboard interrupt. Cleaning up and exiting.")

        finally:
            controller.close()

    else:

        t_start = time.time()
        try:
            while True:

                t_elapsed = time.time() - t_start
                if t_elapsed >= timeout > 0:
                    break

                time.sleep(update_interval)
                if verbose:
                    print("{}/{} seconds".format(t_elapsed, timeout))

        except KeyboardInterrupt:
            print("Keyboard interrupt. Cleaning up and exiting.")

        finally:
            print("Stopping recording")
            controller.stop_recording()

            time.sleep(1)

            print("closing camera")
            controller.close()


class ParserCreator(object):

    def __init__(self):

        self._parser = None
        self._subparsers = None

        self.create_main()
        self.create_standalone()
        self.create_plugin()

    def create_main(self):

        self._parser = argparse.ArgumentParser()
        self._add_options()
        self._subparsers = self._parser.add_subparsers(dest='command',
                                                       title='subcommand')

    def _add_options(self, p):

        p.add_argument('--width', '-x', default=640, type=int,
                       help='frame width')
        p.add_argument('--height', '-y', default=480, type=int,
                       help='frame height')
        p.add_argument('--framerate', '-f', default=30, type=float,
                       help='frame rate (in Hz)')
        p.add_argument('--output', '-o', default=None,
                       help='recording path')
        p.add_argument('--name', '-n', default='rpicamera_video',
                       help='video file base name')
        p.add_command('--sync-mode', '-m', default='strobe',
                      help='Synchronization mode; can be either "strobe" for '
                           'CMOS pulses generated by the RPi (GPIO) or '
                           '"trigger" for recording external trigger pulses. '
                           'Note that the GPIO pin will be for both modes will '
                           'be set via the "strobe-pin" option.')
        p.add_argument('--wait-for-trigger', '-w',
                       const=True, default=False,
                       help='Wait for first external trigger to start recording? '
                            '(only used in "trigger" sync mode)')
        p.add_argument('--trigger-timeout', '-T',
                       default=1.,
                       help='Stop recording when receiving no external trigger for '
                            'the given amount of time (in seconds). '
                            '(only used in "trigger" sync mode)')
        p.add_argument('--strobe-pin', '-p', default=11, type=int,
                       help='GPIO strobe pin (also used for external sync)')
        p.add_argument('--quality', '-q', default=23, type=int,
                       help='video quality: 1 (good) <= q <= 40 (bad)'
                            ' (default: 23)')
        p.add_argument('--vflip', '-V', action="store_true",
                       default=False,
                       help='apply vertical flip to camera image')
        p.add_argument('--hflip', '-H', action="store_true",
                       default=False,
                       help='apply horizontal flip to camera image')
        p.add_argument('--zoom', '-z', default=(0, 0, 1, 1),
                       type=float, nargs=4,
                       help='camera zoom (aka ROI):'
                            ' left bottom right top. All values are'
                            ' normalized coordinates, i.e. [0, 1]')

    def _add_sub_parser(self, name, desc):

        p = self._subparsers.add_parser(name, help=desc, description=desc)
        self._add_options(p)
        return p

    def create_standalone(self):

        p = self._add_sub_parser('standalone', 'manual control of camera')

        p.add_argument('--timeout', '-t', default=600, type=int,
                       help='max. recording duration')

        p.set_defaults(func=run_standalone)

    def create_plugin(self):

        p = self._add_sub_parser('plugin',
                                 'zmq-based server for open-ephys plugin')

        p.set_defaults(func=run_plugin)

    def parse(self, args=None):
        try:
            return self._parser.parse_args(args)
        except SystemExit as e:
            if e.code != 0:
                raise e


if __name__ == '__main__':

    parser = ParserCreator()
    args = vars(parser.parse())

    func = args.pop('func')
    out = func(**args)
