#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    test client for the rpicamera plugin
"""


import sys
import zmq
import time


def run_example(rec_path, duration=10, address='128.40.50.22', port=5555):

    url = "tcp://%s:%d" % (address, port)

    width = 640
    height = 480
    framerate = 30

    with zmq.Context() as context:

        with context.socket(zmq.REQ) as socket:

            socket.connect(url)

            try:

                msg = "Resolution {} {}".format(width, height)
                socket.send(msg)
                print socket.recv()
                time.sleep(1)

                msg = "Framerate {}".format(framerate)
                socket.send(msg)
                print socket.recv()
                time.sleep(1)

                msg = "Start"
                msg += " Experiment={}".format(1)
                msg += " Recording={}".format(1)
                msg += " Path={}".format(rec_path)
                socket.send(msg)
                print socket.recv()

                print "recording for {} seconds".format(duration)
                time.sleep(duration)

                socket.send("Stop")
                print socket.recv()

                socket.send("Close")
                print socket.recv()

            except KeyboardInterrupt:
                print "exiting client"


if __name__ == '__main__':

    address = '128.40.50.22'
    rec_path = 'SomeRecordingName'

    # rec_path = sys.argv[1]

    duration = 10
    if len(sys.argv) > 1:
        duration = float(sys.argv[1])

    run_example(rec_path, duration=duration, address=address)
