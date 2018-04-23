#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    Copy camera data from RPi to the recording computer

    The script recursively searches for event files created using the
    open-ephys plugin-GUI and copies RPi camera data (*.{h264,txt,json}).
    Currently works on Linux-based systems (e.g., Ubuntu) and with data
    recorded in kwik and binary format. See file "rpicamera/utils.py"
    for details.
"""

from __future__ import print_function

import os
import os.path as op
import argparse
import traceback
import sys

try:
    import rpicamera.util as rpu
except ImportError:
    sys.path.append(op.join(op.split(__file__)[0], '..'))
    import rpicamera.util as rpu


def copy_data(path=None, user=None, address=None):

    assert path is not None
    assert user is not None

    path = op.abspath(op.expanduser(path))

    if not op.exists(path):
        os.makedirs(path)

    for f in rpu.get_event_files(path):

        messages = rpu.load_messages_from_event_file(f)
        remote_data = rpu.parse_messages(messages)

        for dd in remote_data:

            if address is not None:
                remote_address = address
            else:
                remote_address = dd['address']

            try:
                rpu.scp(user, remote_address,
                        op.join(dd['path'], '*.{h264,csv,json}'),
                        f['recording_path'])
            except BaseException:
                traceback.print_exc()


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("path", help='data path')
    parser.add_argument('-u', '--user',
                        help='ssh user name', default='pi')
    parser.add_argument('-a', '--address',
                        help='IP address', default=None)
    args = vars(parser.parse_args())

    copy_data(**args)
