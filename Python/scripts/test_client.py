#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    test client for the rpicamera plugin
"""

from __future__ import print_function

import sys
import zmq
import time
import click


def run_example(name='recording_name',
                duration=10,
                address='128.40.50.22',
                port=5555):

    url = "tcp://%s:%d" % (address, port)

    width = 640
    height = 480
    framerate = 30

    with zmq.Context() as context:

        with context.socket(zmq.REQ) as socket:

            socket.connect(url)

            if sys.version_info.major == 3:
                send_func = socket.send_string
            else:
                send_func = socket.send

            try:

                msg = "Resolution {} {}".format(width, height)
                send_func(msg)
                print(socket.recv())
                time.sleep(1)

                msg = "Framerate {}".format(framerate)
                send_func(msg)
                print(socket.recv())
                time.sleep(1)

                msg = "Start"
                msg += " Experiment={}".format(1)
                msg += " Recording={}".format(1)
                msg += " Path={}".format(name)
                send_func(msg)
                print(socket.recv())

                print("recording for {} seconds".format(duration))
                time.sleep(duration)

                send_func("Stop")
                print(socket.recv())

                send_func("Close")
                print(socket.recv())

            except KeyboardInterrupt:
                print("exiting client")


@click.command()
@click.option('--address', '-a', default='1.2.3.4',
              help='IP address of RPi')
@click.option('--port', '-p', default=5555)
@click.option('--name', '-n', default='recording_name',
              help='Name of recording')
@click.option('--duration', '-d', default=10.)
def cli(**kwargs):

    run_example(**kwargs)


if __name__ == '__main__':
    cli()
