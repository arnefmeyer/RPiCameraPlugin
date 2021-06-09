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

#include <stdio.h>
#include "RPiCam.h"
#include "RPiCamEditor.h"


//const int MAX_MESSAGE_LENGTH = 64000;
const int MAX_MESSAGE_LENGTH = 16000;


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


String generateDateString()
{
    // adapted from RecordNode.cpp

	Time t = Time::getCurrentTime();

	String s;

    s += String(t.getYear());
    s += '-';

    int m = t.getMonth() + 1;
    if (m < 10)
    {
        s += "0";
       }
    s += String(m);
	s += "-";

    int day = t.getDayOfMonth();
    if (day < 10)
    {
        s += "0";
    }
	s += String(day);

	s += "_";

	int hrs, mins, secs;
	hrs = t.getHours();
	mins = t.getMinutes();
	secs = t.getSeconds();

    if (hrs < 10)
    {
        s += "0";
    }
	s += hrs;
    s += "-";

	if (mins < 10)
	{
		s += "0";
	}
	s += mins;
    s += "-";

	if (secs < 10)
	{
		s += "0";
	}
	s += secs;

	return s;
}


RPiCam::RPiCam()
    : GenericProcessor("RPiCamera"), address(""), port(5555), context(NULL), socket(NULL), rpiRecPath(""), sendRecPathEvent(false), width(640), height(480), framerate(30), vflip(false), hflip(false), isRecording(false), zoom{0, 0, 100, 100}

{
    setProcessorType(PROCESSOR_TYPE_SOURCE);

    createContext();

	if (!address.isEmpty())
  	{
  	    openSocket();
  	}

    sendSampleCount = false;
}


RPiCam::~RPiCam()
{
	sendMessage(String("Close"), 1000);
    closeSocket();
}


void RPiCam::createEventChannels()
{
	EventChannel* chan = new EventChannel(EventChannel::TEXT, 1, MAX_MESSAGE_LENGTH, CoreServices::getGlobalSampleRate(), this);
	chan->setName("RPiCam Messages");
	chan->setDescription("Messages received through the RPiCam plugin");
	chan->setIdentifier("external.network.rawData");
	chan->addEventMetaData(new MetaDataDescriptor(MetaDataDescriptor::INT64, 1, "Software timestamp",
		"OS high resolution timer count when the event was received", "timestamp.software"));
	eventChannelArray.add(chan);
	messageChannel = chan;
}


void RPiCam::setPort(int p, bool connect, bool update)
{
	if (p != port)
	{
		port = p;

		if (connect || socket != NULL)
		{
			closeSocket();
			openSocket();
		}

        if (update)
        {
          RPiCamEditor* e = (RPiCamEditor*)getEditor();
          e->updateValues();
        }
	}
}


void RPiCam::setAddress(String s, bool connect, bool update)
{
	if (!s.equalsIgnoreCase(address))
	{
		address = s;

		if (connect || socket != NULL)
		{
			closeSocket();
			openSocket();
		}

        if (update)
        {
                RPiCamEditor* e = (RPiCamEditor*)getEditor();
                e->updateValues();
        }
	}
}


void RPiCam::setResolution(int w, int h)
{
	width = w;
	height = h;

	if (!isRecording)
	{
		String msg("Resolution");
		msg += String(" ") + String(width);
		msg += String(" ") + String(height);
		sendMessage(msg, 1000);
	}
}


void RPiCam::setFramerate(int fps)
{
	framerate = fps;

	if (!isRecording)
	{
		String msg("Framerate ");
		msg += String(framerate);
		sendMessage(msg, 1000);
	}
}


void RPiCam::setVflip(bool status)
{
	vflip = status;
	if (!isRecording)
	{
		sendMessage("VFlip " + String(status), 1000);
	}
}

void RPiCam::setHflip(bool status)
{
	hflip = status;
	if (!isRecording)
	{
		sendMessage("HFlip " + String(status), 1000);
	}
}


void RPiCam::setZoom(int z[4])
{
	zoom[0] = z[0];
	zoom[1] = z[1];
	zoom[2] = z[2];
	zoom[3] = z[3];

	String msg("Zoom ");
	for (int i=0; i<4; i++)
	{
		// convert from percent to normalized coordinates
		msg += String(zoom[i]/100., 2);
		if (i < 3)
			msg += " ";
	}
	sendMessage(msg, 1000);
}

void RPiCam::getZoom(int *z)
{
	z[0] = zoom[0];
	z[1] = zoom[1];
	z[2] = zoom[2];
	z[3] = zoom[3];
}


void RPiCam::sendCameraParameters()
{
	if (!isRecording)
	{
		setResolution(width, height);
		setFramerate(framerate);
	}
}


void RPiCam::resetGains()
{
	if (!isRecording)
	{
		String msg("ResetGains");
		sendMessage(msg, 1000);
	}
}


void RPiCam::createContext()
{
    if (context == NULL)
        context = zmq_ctx_new();
}


void RPiCam::destroyContext()
{
	if (context != NULL)
	{
		std::cout << "RPiCam destroying context ... ";
		zmq_ctx_destroy(context);
		context = NULL;
		std::cout << "done\n";
	}
}


void RPiCam::openSocket()
{
	if (socket == NULL)
	{
		socket = zmq_socket(context, ZMQ_REQ);
		String url = String("tcp://") + String(address) + ":" + String(port);
		std::cout << "RPiCam connecting to " << url.toStdString() << " ... ";

		int rc = zmq_connect(socket, url.toRawUTF8());

		if (rc != 0)
		{
			std::cout << "failed to open socket: " << zmq_strerror(zmq_errno()) << "\n";
			socket = NULL;
		}
		else
		{
			std::cout << "done\n";
		}
	}
	else
	{
		std::cout << "Socket already opened.\n";
	}
}


bool RPiCam::closeSocket()
{
	if (socket != NULL)
	{
		std::cout << "RPiCam closing socket ...";
    	zmq_close(socket);
		socket = NULL;
		std::cout << "done\n";
	}

    return true;
}


int RPiCam::getPort()
{
	return port;
}


String RPiCam::getAddress()
{
	return address;
}


String RPiCam::sendMessage(String msg, int timeout)
{
	String response;

	std::cout << "RPiCam sending message: "  << msg.toStdString() << " ... ";

	if (socket != NULL)
	{
		zmq_setsockopt(socket, ZMQ_RCVTIMEO, (const void*) &timeout, sizeof(int));
		zmq_send(socket, msg.getCharPointer(), msg.length(), 0);

		unsigned char* buffer = new unsigned char[MAX_MESSAGE_LENGTH];
		int result = zmq_recv(socket, buffer, MAX_MESSAGE_LENGTH-1, 0);

		if (result < 0)
		{
            response = String("");  // responder died.
		}
		else
		{
			response = String(reinterpret_cast<char*>(buffer), result);
		}

		delete buffer;

	}
	else
	{
		response = String("not connected");
	}

	std::cout << "the RPi answered: " << response.toStdString() << "\n";

	return response;
}


void RPiCam::updateSettings()
{
	if (editor != NULL)
	{
		editor->update();
	}
}


AudioProcessorEditor* RPiCam::createEditor(
)
{
    editor = new RPiCamEditor(this, true);

    return editor;
}


void RPiCam::setParameter(int parameterIndex, float newValue)
{
}


void RPiCam::handleEvent(int eventType, juce::MidiMessage& event, int samplePosition)
{
}


void RPiCam::startRecording()
{
	isRecording = true;

	int expNumber = CoreServices::RecordNode::getExperimentNumber();
	int recNumber = CoreServices::RecordNode::getRecordingNumber() + 1;
	String recPath = CoreServices::RecordNode::getRecordingPath().getFullPathName();

	String msg("Start");
	msg += String(" Experiment=") + String(expNumber);
	msg += String(" Recording=") + String(recNumber);

    // make sure to include a unique recording directory name to avoid overwriting data on the RPi
    if (recPath.length() == 0)
    {
        recPath = generateDateString();
    }
    msg += String(" Path=") + recPath;

	rpiRecPath = sendMessage(msg);
    sendRecPathEvent = true;

	RPiCamEditor* e = (RPiCamEditor*)getEditor();
	e->enableControls(false);
}


void RPiCam::stopRecording()
{
	isRecording  = false;

	sendMessage(String("Stop"), 1000);

	RPiCamEditor* e = (RPiCamEditor*)getEditor();
	e->enableControls(true);
}


void RPiCam::process(AudioSampleBuffer& buffer)
{

    if (rpiRecPath.isNotEmpty() && sendRecPathEvent)
    {
        juce::int64 timestamp_software = timer.getHighResolutionTicks();
        String msg1("RPiCam Address=" + address + " RecPath=" + rpiRecPath);

        uint8* msg1_raw = new uint8[msg1.length()+1];
        memcpy(msg1_raw, msg1.toRawUTF8(), msg1.length());
		*(msg1_raw+msg1.length()) = '\0';

	    MetaDataValueArray md;
	    md.add(new MetaDataValue(MetaDataDescriptor::INT64, 1, &timestamp_software));
	    TextEventPtr event = TextEvent::createTextEvent(messageChannel, CoreServices::getGlobalTimestamp(), String::fromUTF8(reinterpret_cast<const char*>((uint8*)msg1_raw), msg1.length()+1), md);
	    addEvent(messageChannel, event, 0);

//        addEvent(events,
//                 MESSAGE,
//                 0,
//                 0,
//                 0,
//                 msg1.length()+1,
//                 (uint8*)msg1_raw);
        delete[] msg1_raw;

        sendRecPathEvent = false;
    }
}


void RPiCam::enabledState(bool t)
{
    isEnabled = t;
}


void RPiCam::saveCustomParametersToXml(XmlElement* parentElement)
{
    XmlElement* mainNode = parentElement->createNewChildElement("RPiCam");
	mainNode->setAttribute("address", address);
    mainNode->setAttribute("port", port);
    mainNode->setAttribute("width", width);
    mainNode->setAttribute("height", height);
    mainNode->setAttribute("framerate", framerate);
	mainNode->setAttribute("hflip", hflip);
	mainNode->setAttribute("vflip", vflip);
	mainNode->setAttribute("x1", zoom[0]);
	mainNode->setAttribute("y1", zoom[1]);
	mainNode->setAttribute("x2", zoom[2]);
	mainNode->setAttribute("y2", zoom[3]);
}


void RPiCam::loadCustomParametersFromXml()
{
    if (parametersAsXml != nullptr)
    {
        forEachXmlChildElement(*parametersAsXml, mainNode)
        {
            if (mainNode->hasTagName("RPiCam"))
            {
                // std::cout << "RPiCam setting attribute address: " << mainNode->getStringAttribute("address") << "\n";
                setAddress(mainNode->getStringAttribute("address"), false, false);
				setPort(mainNode->getIntAttribute("port"), false, false);

                if (mainNode->hasAttribute("width"))
                {
                    width = mainNode->getIntAttribute("width");
                }

                if (mainNode->hasAttribute("height"))
      			{
      			    height = mainNode->getIntAttribute("height");
      			}

                if (mainNode->hasAttribute("framerate"))
                {
                    framerate = mainNode->getIntAttribute("framerate");
                }

                if (mainNode->hasAttribute("hflip"))
                {
                    hflip = mainNode->getBoolAttribute("hflip");
                }

      			if (mainNode->hasAttribute("vflip"))
      			{
      			    vflip = mainNode->getBoolAttribute("vflip");
      			}

      			if (mainNode->hasAttribute("x1"))
      			{
      			    zoom[0] = mainNode->getIntAttribute("x1");
      			    zoom[1] = mainNode->getIntAttribute("y1");
      			    zoom[2] = mainNode->getIntAttribute("x2");
      			    zoom[3] = mainNode->getIntAttribute("y2");
      			}

                RPiCamEditor* e = (RPiCamEditor*)getEditor();
                e->updateValues();
            }
        }
    }
}
