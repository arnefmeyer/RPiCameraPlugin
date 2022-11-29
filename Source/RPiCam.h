/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2015 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __RPICAM__
#define __RPICAM__

#ifdef ZEROMQ

#ifdef WIN32
#include <zmq.h>
#include <zmq_utils.h>
#else
#include <zmq.h>
#endif

#endif

#include <ProcessorHeaders.h>

/**

 Control Rapsberry PI camera via zmq messages

  @see GenericProcessor

*/
class RPiCamFormat
{
public:
    RPiCamFormat() : width(0), height(0), framerate(0), framerate_min(0), framerate_max(0)
    {
    }

    RPiCamFormat(int w, int h, int r, int r_min, int r_max)
    {
        width = w;
        height = h;
        framerate = r;
        framerate_min = r_min;
        framerate_max = r_max;
    }

    int width;
    int height;
    int framerate;
    int framerate_min;
    int framerate_max;
};

// camera formats; for available video modes see https://picamera.readthedocs.io/en/release-1.13/fov.html
auto const camFormats = Array<RPiCamFormat>(
    {RPiCamFormat(2592, 1944, 10, 1, 15),
     RPiCamFormat(1920, 1080, 10, 1, 15),
     RPiCamFormat(1296, 972, 30, 1, 42),
     RPiCamFormat(1296, 730, 30, 1, 49),
     RPiCamFormat(1280, 720, 30, 1, 49),
     RPiCamFormat(1024, 768, 30, 1, 60),
     RPiCamFormat(800, 600, 30, 1, 60),
     RPiCamFormat(640, 480, 30, 1, 90)});

class RPiCam : public GenericProcessor
{
public:
    RPiCam();
    ~RPiCam();
    AudioProcessorEditor *createEditor();
    void process(AudioSampleBuffer &buffer) override;

    void parameterValueChanged(Parameter *);

    bool isSource() { return true; };

    void updateSettings();

    bool isReady() { return true; }
    float getDefaultSampleRate() { return 30000.; };
    int getDefaultNumOutputs() { return 1; };
    void enabledState(bool t);
    int getNumEventChannels() { return 1; };

    bool startAcquisition() override;
    void startRecording() override;
    void stopRecording() override;

    void setPort(int port, bool connect = false, bool update = true);
    int getPort();
    void setAddress(String s, bool connect = false, bool update = true);
    String getAddress();
    void setResolution(int w, int h);
    int getWidth() { return width; }
    int getHeight() { return height; }
    void setFramerate(int fps);
    int getFramerate() { return framerate; }
    void setVflip(bool status);
    bool getVflip() { return vflip; }
    void setHflip(bool status);
    bool getHflip() { return hflip; }
    void changeZoom();
    void setZoom(int z[4]);
    void getZoom(int *z);
    void resetGains();

    void sendCameraParameters();

    void openSocket();
    bool closeSocket();

    String sendMessage(String msg, int timeout = -1);

    void saveCustomParametersToXml(XmlElement *xml);
    void loadCustomParametersFromXml(XmlElement *xml);

private:
    void createContext();
    void destroyContext();

    StringArray m_resolutions{};
    StringArray m_fps{};

    void *context;
    void *socket;
    int port;
    String address;

    bool sendRecPathEvent;
    String rpiRecPath;

    int width;
    int height;
    int framerate;
    bool vflip;
    bool hflip;
    int zoom[4];
    bool isRecording;

    Time timer;

    CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RPiCam);
};

#endif // RPICAM
