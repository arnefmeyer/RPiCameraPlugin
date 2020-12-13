#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import subprocess
import glob
import argparse


def convert_data(src, 
                ext="h264",
                destExt="mp4",
                src_fps=30,
                dest_fps=30):
    """
    This function perform in place conversion of files.

    Input:
        - src: source path
        - ext: default extention to search in source path
        - destExt: extension for the output file
        - src_fps: fps of the source file (only take a global one for the moment)
        - dest_fps: fps of destination file

    """
    assert src is not None
    fullpath = src + '/**/*.' + ext
    allFile = glob.glob(fullpath, recursive=True)

    for curFile in allFile:
        destFile = curFile.replace(ext,destExt)
        if not os.path.isfile(destFile):
            subprocess.check_output(['ffmpeg', '-r', str(src_fps), '-i', curFile , '-r'  , str(dest_fps), destFile])



if __name__ == '__main__':
    '''
    This function recursively search for all the file with a specified
     extention and frame rate in a given folder and convert them to a specified
     format and framerate. It will not erase existing files.

    If called as a script this function will take 
    batch_convert_video.py path [-arg]
    Input:
            - path: path where to search video files

    Args:
            -e [--ext]: default extention to search in source path (default: h264)
            -d [--destExt]: extension for the output file
            -sf [--src_fps]: fps of the source file
            -df [--dest_fps]: fps of destination files

    Author: Bourboulou Romain, 
    Date: 28/10/20

    To do: 
    other destination folder
    prompt to erase existing file if they exist
    '''

    parser = argparse.ArgumentParser()

    parser.add_argument("src", help="Source folder")
    parser.add_argument('-e' , "--ext" , help="Source extension", default="h264")
    parser.add_argument('-sf' , "--src_fps" , help="Fps of source file", default=30)
    parser.add_argument('-d', "--destExt" , help="Destination extension", default="mp4")
    parser.add_argument('-df' , "--dest_fps" , help="Fps of Dest file", default=30)

    args = vars(parser.parse_args())

    convert_data(**args)