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
import glob
import shutil


try:
    import rpicamera.util as rpu
except ImportError:
    sys.path.append(op.join(op.split(__file__)[0], '..'))
    import rpicamera.util as rpu


def copy_local(localpath, path, dest_dir, verbose=True):
    # recursively search for matching directory and copy data

    rec_name = path.split(op.sep)[-1]
    for root, dirs, files in os.walk(localpath):

        if rec_name in dirs:

            local_dir = op.join(root, rec_name)
            for ext in ['h264', 'csv', 'json']:

                for f in glob.glob(op.join(local_dir, '*.' + ext)):

                    if verbose:
                        print("copying file")
                        print("  source=" + f)
                        print("  dest=" + dest_dir)

                    shutil.copy(f, dest_dir)


def copy_data(path=None,
              user=None,
              address=None,
              verbose=False,
              localpath=None,
              **kargs):

    assert path is not None
    assert user is not None

    path = op.realpath(op.expanduser(path))

    if localpath is not None:
        localpath = op.realpath(op.expanduser(localpath))

    if not op.exists(path):
        os.makedirs(path)

    for f in rpu.get_event_files(path):

        messages = rpu.load_messages_from_event_file(f)
        remote_data = rpu.parse_messages(messages)

        for dd in remote_data:

            try:
                if localpath is not None:
                    # copy data from local path (see function "copy_local")
                    copy_local(localpath, dd['path'],
                               f['recording_path'])
                else:
                    # copy data from remote location (i.e. Raspberry Pi)
                    rpu.scp(user,
                            dd['address'] if address is None else address,
                            op.join(dd['path'], '*.{h264,csv,json}'),
                            f['recording_path'],
                            verbose=verbose)

            except BaseException:
                traceback.print_exc()


def copy_func(args):

    try:
        rpu.scp(args[0], args[1], args[2], args[3], verbose=args[4])
    except BaseException:
        traceback.print_exc()


def copy_data_parallel(path=None,
                       user=None,
                       address=None,
                       workers=4,
                       verbose=False):
    # the RPi's slow wifi can be a bottleneck so let's do things in parallel

    from multiprocessing import Pool

    assert path is not None
    assert user is not None

    path = op.realpath(op.expanduser(path))

    if not op.exists(path):
        os.makedirs(path)

    all_data = []
    for f in rpu.get_event_files(path):

        messages = rpu.load_messages_from_event_file(f)
        remote_data = rpu.parse_messages(messages)

        for dd in remote_data:

            if address is not None:
                remote_address = address
            else:
                remote_address = dd['address']

            all_data.append((user,
                             remote_address,
                             op.join(dd['path'], '*.{h264,csv,json}'),
                             f['recording_path'],
                             verbose))

    pool = Pool(processes=workers)
    pool.map(copy_func, all_data)


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    parser.add_argument("path", help='data path')
    parser.add_argument('-u', '--user',
                        help='ssh user name', default='pi')
    parser.add_argument('-a', '--address',
                        help='IP address', default=None)
    parser.add_argument('-p', '--parallel', action='store_true')
    parser.add_argument('-w', '--workers', default=4)
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-l', '--localpath', default=None)

    args = vars(parser.parse_args())

    print(args)

    parallel = args.pop('parallel')
    local_path = args.pop('localpath')
    if parallel:
        if local_path is not None:
            raise Exception('The parallel option can only be used for remote locations')
        copy_data_parallel(**args)
    else:
        copy_data(localpath=local_path,
                  **args)
