# rpicamera

A Python package to control the camera on the RPi.


## Dependencies

The rpicamera package requires a number of other packages, e.g., [picamera](https://picamera.readthedocs.io) and [zeromq](http://zeromq.org/bindings:python). These packages can be installed via pip:

* pip install picamera
* pip install pyzmq

It it also recommended to install the [RPi.GPIO](https://pypi.python.org/pypi/RPi.GPIO) package which is required to send frame trigger pulses to the open-ephys acquisition board:

* pip install RPi.GPIO


## Installation

There are two ways to use the package:

1. **Without installation:** copy the content of the "Python" folder to the RPi and simply run the "rpi_host.py" script, e.g., "python Python/scripts/rpi_host.py"

2. **With installation:** copy the content of the "Python" folder to the RPi and install the package via "python setup.py install". Copy the "rpi_host.py" script to some location on the RPi (e.g., the home folder). You can delete the "Python" folder afterwards.


## Modes

1. Plugin mode (for open-ephys plugin): "rpi_host.py plugin"

2. Standalone mode: "rpi_host.py standalone"


## Options

Run "python rpi_host.py --help" to see all options, e.g. frame width/height, framerate, and the GPIO pin used to send trigger pulses to the open-ephys board.


## Streaming

The current version stores video data on the SD card of the RPi. There are some classes that allow streaming of video data over ethernet/wireless network (see rpicamera/streams.py).


## Copying data to recording computer

The script "scripts/copy_data.py" can be used to copy from the RPi to recording computer running open-ephys. It will recursively search for event files and automatically copy video data to the recording directory. At the moment, only the kwik file format is supported but other formats (original recording, binary, nwb) will be supported quite soon.

