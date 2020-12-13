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


class RPiCam : public GenericProcessor
{
public:
    RPiCam();
    ~RPiCam();
    AudioProcessorEditor* createEditor();
    void process(AudioSampleBuffer& buffer);
    void setParameter(int parameterIndex, float newValue);
	void createEventChannels();

    bool isSource() { return true; };

    void updateSettings();

    bool isReady() { return true; }
    float getDefaultSampleRate() { return 30000.; };
    int getDefaultNumOutputs() { return 1; };
    void enabledState(bool t);
    int getNumEventChannels() { return 1; };

	void startRecording();
	void stopRecording();

    void setPort(int port, bool connect = false);
	int getPort();
	void setAddress(String s, bool connect = false);
	String getAddress();
	void setResolution(int w, int h);
	void setFramerate(int fps);
	int getFramerate() { return framerate; }
	void setVflip(bool status);
	bool getVflip() { return vflip; }
	void setHflip(bool status);
	bool getHflip() { return hflip; }
	void setZoom(int z[4]);
	void getZoom(int *z);
	void resetGains();

	void sendCameraParameters();

	void openSocket();
    bool closeSocket();

	String sendMessage(String msg, int timeout=-1);

    void saveCustomParametersToXml(XmlElement* parentElement);
    void loadCustomParametersFromXml();

private:
    void handleEvent(int eventType, MidiMessage& event, int samplePos);

    void createContext();
	void destroyContext();

    void* context;
	void* socket;
    int port;
	String address;

    bool sendRecPath;
    String rpiRecPath;

	int width;
	int height;
	int framerate;
	bool vflip;
	bool hflip;
	int zoom[4];
	bool isRecording;

	const EventChannel* messageChannel{ nullptr };
	Time timer;

    CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RPiCam);

};

#endif  // RPICAM

