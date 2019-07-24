#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Arne F. Meyer <arne.f.meyer@gmail.com>
# License: GPLv3

"""
    some helper functions for the raspberry pi camera open-ephys plugin
"""

from __future__ import print_function

import os
import os.path as op
import glob
import numpy as np
import json
import platform
import subprocess
import traceback


# -----------------------------------------------------------------------------
# Loading/reading of video data
# -----------------------------------------------------------------------------


def get_video_files(path, absolute=False, recursive=False):

    rec_files = []

    if recursive:

        for root, _, _ in os.walk(path, topdown=False):
            files = get_video_files(root, absolute=absolute, recursive=False)
            if files is not None:
                if isinstance(files, list):
                    rec_files.extend(files)
                else:
                    rec_files.append(files)

    else:
        video_files = glob.glob(op.join(path, '*.h264'))
        if len(video_files) > 0:

            for video_file in video_files:

                # each file belonging to this recording should have the same
                # base name
                basename = op.splitext(video_file)[0]
                ts_file = basename + '_timestamps.csv'
                param_file = basename + '_params.json'
                if op.exists(ts_file) and op.exists(param_file):
                    dd = {'video': video_file,
                          'timestamps': ts_file,
                          'parameters': param_file}
                    rec_files.append(dd)

    return rec_files


def read_timestamp_deltas(path):

    if op.isfile(path):
        files = [path]
    elif op.isdir(path):
        files = glob.glob(op.join(path, '*timestamps.txt'))
    else:
        raise ValueError('given path neither file nor directory')

    if len(files) > 0:
        ts_file = files[0]

        try:
            ts = np.genfromtxt(ts_file, delimiter=',')

        except ValueError:
            # this can happen if the file has been closed before the value of the 2nd column was written
            ts = []
            with open(ts_file, 'r') as f:
                for line in f:
                    if not line.startswith('#'):
                        if ',' in line:
                            values = [int(x) for x in line.split(',')]
                            if len(values) >= 2:
                                ts.append(values)
            ts = np.asarray(ts)

        except BaseException:
            # some other error
            traceback.print_exc()

        if ts.shape[1] > 2:
            # only use first two columns
            ts = ts[:, :2]

        dts = np.diff(ts, axis=1).ravel() / 1000000.  # usec -> sec

        # in very rare cases, the encoder doesn't attach a valid timestamp
        # to some frames, e.g., if the video quality setting is set too high.
        # return -1 for these timestamps.
        dropped = ts[:, 0] < 0
        dts[dropped] = -1

    else:
        dts = None

    return dts


def interpolate_missing_timestamps(ts, deltas, fps=30):
    """simple interpolation of missing timestamps (not multiple in a row)"""

    missing = np.where(deltas < 0)[0]

    if 1 in np.diff(missing):
        raise ValueError("two successive missing timestamps!")
    elif len(ts) <= 2:
        raise ValueError("need at least 3 timestamp values")

    dt = 1. / fps
    ts_corrected = ts - deltas

    for i in missing:

        if i == 0:
            ts_corrected[i] = ts[i+1] - deltas[i+1] - dt
        else:
            ts_corrected[i] = ts[i-1] - deltas[i-1] + dt

    return ts_corrected


def load_video_parameters(path, pattern='*video*'):

    w, h = 640, 480
    fps = 30.
    ts = None

    if op.isfile(path):
        rec_path, filepath = op.split(path)
        filename = op.splitext(filepath)[0]

    elif op.isdir(path):
        rec_path = path
        filename = pattern

    else:
        raise ValueError("file pr path does not exist: {}".format(path))

    if len(glob.glob(op.join(rec_path, filename + '.npz'))) > 0:
        # converted format

        files = glob.glob(op.join(rec_path, filename + '.npz'))
        rec_info = np.load(files[0])

        if 'width' in rec_info:

            fps = rec_info['framerate'].item()
            w = rec_info['width'].item()
            h = rec_info['height'].item()
            ts = rec_info['timestamps']
            print(ts)

        else:

            framerate = rec_info['framerate']
            fps = int(float(framerate[0]) / framerate[1])
            w, h = rec_info['resolution']
            ts = rec_info['timestamps']

    elif len(glob.glob(op.join(rec_path, pattern + '.json'))) > 0:
        # data written by rpicam host

        files = glob.glob(op.join(rec_path, pattern + '.json'))
        json_file = files[0]
        with open(json_file, 'r') as f:
            params = json.load(f)

        w, h = params['width'], params['height']
        fps = params['framerate']
        ts = None

    else:
        print("No parameter file found. Returning default video parameters.")

    return {'fps': fps,
            'width': w,
            'height': h,
            'timestamps': ts}

# -----------------------------------------------------------------------------
# Copying of video data
# -----------------------------------------------------------------------------


def get_event_files(path, verbose=True):
    """recursive search for event files

        currently supported: kwik (kwe) and binary format (text.npy)

        TODO: add support for open-ephys and nwb formats
    """

    event_files = []
    for root, dirs, files in os.walk(op.abspath(path), topdown=False):

        for f in files:

            if f.endswith('.kwe'):

                event_files.append({'filepath': op.join(root, f),
                                    'format': 'kwik',
                                    'recording_path': root})

            elif f == 'structure.oebin':

                with open(op.join(root, 'structure.oebin'), 'r') as ff:
                    S = json.load(ff)

                for proc in S['events']:

                    if proc['source_processor'].startswith('RPiCamera'):
                        msg_file = op.join(root,
                                           'events',
                                           proc['folder_name'],
                                           'text.npy')

                        if op.exists(msg_file):
                            event_files.append({'filepath': msg_file,
                                                'format': 'binary',
                                                'recording_path': root})

    if verbose:
        print("found {} event files:".format(len(event_files)))
        for ef in event_files:
            print("format:", ef['format'])
            print("file path:", ef['filepath'])
            print("recording path:", ef['recording_path'])
            print("")

    return event_files


def load_messages_from_event_file(event_file):
    """parse remote path and address from a kwik or binary format event file"""

    import h5py

    if event_file['format'] == 'kwik':
        # kwik event file
        with h5py.File(event_file['path'], 'r') as f:
            messages = \
                f['event_types']['Messages']['events']['user_data']['Text']

    elif event_file['format'] == 'binary':
        # binary format network event file
        messages = [msg.strip()
                    for msg in np.load(event_file['filepath']).tolist()]

    return messages


def parse_messages(messages):

    remote_data = []

    for msg in messages:

        if isinstance(msg, bytes):
            msg = msg.decode()

        parts = msg.split()

        if parts[0] == 'RPiCam':

            i1 = msg.find('Address=')
            i2 = msg.find('RecPath=')

            remote_address = msg[i1+len('Address='):i2-1]
            remote_address = ''.join(e for e in remote_address
                                     if e.isdigit() or e == '.')
            remote_address = ''.join(e[:min(len(e), 3)] + '.'
                                     for e in remote_address.split('.'))
            remote_address = remote_address[:-1]
            print("address:", remote_address)

            remote_path = msg[i2+len('RecPath='):]
            remote_path = ''.join(e for e in remote_path
                                  if e.isalnum() or
                                  e in ['/', '-', '_'])
            print("remote path:", remote_path)

            remote_data.append({'address': remote_address,
                                'path': remote_path})

    return remote_data


def scp(user, address, src_file, dest_dir, verbose=True):
    """transfer file(s) using scp

        Linux: scp
        Windows: pscp -> https://the.earth.li/~sgtatham/putty/0.60/htmldoc/Chapter5.html
        Mac: ?
    """

    src_cmd = '{}@{}:{}'.format(user, address, src_file)

    if verbose:
        print("running commmand: {}".format(src_cmd))

    if platform.system() == "Linux":
        # use scp command; disable strict filename checking (option -T)
        if verbose:
            out = subprocess.check_output(['scp', '-T', '-v', src_cmd, dest_dir])
        else:
            out = subprocess.check_output(['scp', '-T', src_cmd, dest_dir])

    elif platform.system() == "Windows":
        raise NotImplementedError()

    elif platform.system() == "Darwin":
        raise NotImplementedError()

    if verbose:
        print(out)
