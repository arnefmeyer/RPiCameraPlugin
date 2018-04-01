#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

from __future__ import print_function

from io import FileIO
import socket
import subprocess


class FileOutput(FileIO):

    def __init__(self, f, **kwargs):
        super(FileOutput, self).__init__(f, **kwargs)
        self.size = 0

    def write(self, s):
        self.size += len(s)
        super(FileOutput, self).write(s)

    def flush(self):
        print("# bytes:", self.size)
        super(FileOutput, self).flush()


class NetworkStreamOutput(object):
    """https://wiki.python.org/moin/TcpCommunication"""

    def __init__(self, address, port=12397, **kwargs):

        self.address = address
        self.port = port

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((address, port))
        self.size = 0

    def __del__(self):
        if self.socket is not None:
            self.socket.close()

    def write(self, s):
        self.size += len(s)
        self.socket.sendall(s)

    def flush(self):
        print("# bytes:", self.size)
        self.socket.close()
        self.socket = None


class NetworkStreamWriter(object):

    def __init__(self, address='', port=12397, verbose=True):

        self.address = address
        self.port = port
        self.verbose = verbose

        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.bind((address, port))

        if self.verbose:
            print("server address/port:", server.getsockname())

        server.listen(1)

        self.server = server

    def process(self, output=None, vlc=False):

        assert not (output is None and not vlc),\
            "Either output file or viewer must be valid"

        conn = None
        n_bytes = 0

        if output is not None:
            f = FileIO(output, "w")

        if vlc:
            # open pipe to vlc to view incoming video data
            cmdline = ['vlc', '--demux', 'h264',
                       '--network-caching=', '512',  # in ms
                       '--h264-fps', '30',
                       '-']
            player = subprocess.Popen(cmdline, stdin=subprocess.PIPE)

        try:
            conn, addr = self.server.accept()
            if self.verbose:
                print('Connection address:', addr)

            while 1:

                data = conn.recv(1048576)
                if not data:
                    break

                n_bytes += len(data)

                # write data to file and video stream
                if output is not None:
                    f.write(data)

                if vlc:
                    player.stdin.write(data)

                # indicate that we are ready for more data (move up?)
                conn.send("")

        finally:
            if conn is not None:
                conn.close()
            if output is not None:
                f.close()
            if vlc:
                player.terminate()

        if self.verbose:
            print(" total # bytes received:", n_bytes)
