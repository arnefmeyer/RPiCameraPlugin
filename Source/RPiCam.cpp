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
#include "../../plugin-GUI/Source/Utils/Utils.h"

const int MAX_MESSAGE_LENGTH = 16000;

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

RPiCam::RPiCam()
	: GenericProcessor("RPiCamera"), address(""), port(5555), context(NULL), socket(NULL), rpiRecPath(""), sendRecPathEvent(false), width(640), height(480), framerate(30), vflip(false), hflip(false), isRecording(false), zoom{0, 0, 100, 100}

{
	for (const auto &format : camFormats)
		m_resolutions.add(String(format.width) + "x" + String(format.height));

	auto nFormats = camFormats.size();
	for (int i = camFormats[nFormats - 1].framerate_min; i < camFormats[nFormats - 1].framerate_max; ++i)
		m_fps.add(String(i));

	addIntParameter(Parameter::GLOBAL_SCOPE, "Port", "RPi port to connect to", 5555, 0, 10000, true);
	addStringParameter(Parameter::GLOBAL_SCOPE, "Address", "RPi address to connect to", "128.40.51.152", true);
	addCategoricalParameter(Parameter::GLOBAL_SCOPE, "Resolution", "RPi camera resolution", m_resolutions, m_resolutions.size() - 1, true);
	addCategoricalParameter(Parameter::GLOBAL_SCOPE, "FPS", "Frames per second", m_fps, 0, true);
	addIntParameter(Parameter::GLOBAL_SCOPE, "Left", "Left edge of cameras ROI", 0, 0, 100, true);
	addIntParameter(Parameter::GLOBAL_SCOPE, "Bottom", "Bottom edge of cameras ROI", 0, 0, 100, true);
	addIntParameter(Parameter::GLOBAL_SCOPE, "Width", "Width of cameras ROI", 100, 0, 100, true);
	addIntParameter(Parameter::GLOBAL_SCOPE, "Height", "Height of cameras ROI", 100, 0, 100, true);
	addStringParameter(Parameter::GLOBAL_SCOPE, "Connect", "Connect to the RPi", String());
	addBooleanParameter(Parameter::GLOBAL_SCOPE, "R", "Reinitialize automatic gain and white level balance", true, true);
	addBooleanParameter(Parameter::GLOBAL_SCOPE, "H", "Enable/disable horizontal flip", true, true);
	addBooleanParameter(Parameter::GLOBAL_SCOPE, "V", "Enable/disable vertical flip", true, true);
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

void RPiCam::parameterValueChanged(Parameter *param)
{
	if (param->getName().equalsIgnoreCase("Port"))
	{
		setPort((int)param->getValue());
	}
	else if (param->getName().equalsIgnoreCase("Address"))
	{
		setAddress(param->getValueAsString());
	}
	else if (param->getName().equalsIgnoreCase("Resolution"))
	{
		CategoricalParameter *cparam = (CategoricalParameter *)param;
		auto index = cparam->getSelectedIndex();
		RPiCamFormat format = camFormats[index];
		setResolution(format.width, format.height);
		auto fpsParam = (CategoricalParameter *)getParameter("FPS");
		StringArray fps{};
		for (int i = format.framerate_min; i < format.framerate_max + 1; ++i)
			fps.add(String(i));
		fpsParam->setCategories(fps);
	}
	else if (param->getName().equalsIgnoreCase("FPS"))
	{
		setFramerate((int)param->getValue());
	}
	else if (param->getName().equalsIgnoreCase("Left"))
	{
		changeZoom();
	}
	else if (param->getName().equalsIgnoreCase("Bottom"))
	{
		changeZoom();
	}
	else if (param->getName().equalsIgnoreCase("Width"))
	{
		changeZoom();
	}
	else if (param->getName().equalsIgnoreCase("Height"))
	{
		changeZoom();
	}
	else if (param->getName().equalsIgnoreCase("Connect"))
	{
		address = getParameter("Address")->getValueAsString();
		port = (int)getParameter("Port")->getValue();
		LOGC("address: ", address);
		LOGC("port: ", port);
		closeSocket();
		openSocket();
		sendCameraParameters();
	}
	else if (param->getName().equalsIgnoreCase("R"))
	{
		resetGains();
	}
	else if (param->getName().equalsIgnoreCase("H"))
	{
		setHflip((bool)param->getValue());
	}
	else if (param->getName().equalsIgnoreCase("V"))
	{
		setVflip((bool)param->getValue());
	}
}

void RPiCam::changeZoom()
{
	int zoom[4];
	zoom[0] = (int)(getParameter("Left")->getValue());
	zoom[1] = (int)(getParameter("Bottom")->getValue());
	zoom[2] = (int)(getParameter("Width")->getValue());
	zoom[3] = (int)(getParameter("Height")->getValue());
	setZoom(zoom);
}

void RPiCam::updateSettings()
{
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
		sendMessage("VFlip " + String((int)status), 1000);
	}
}

void RPiCam::setHflip(bool status)
{
	hflip = status;
	if (!isRecording)
	{
		sendMessage("HFlip " + String((int)status), 1000);
	}
}

void RPiCam::setZoom(int z[4])
{
	zoom[0] = z[0];
	zoom[1] = z[1];
	zoom[2] = z[2];
	zoom[3] = z[3];

	String msg("Zoom ");
	for (int i = 0; i < 4; i++)
	{
		// convert from percent to normalized coordinates
		msg += String(zoom[i] / 100., 2);
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
		LOGC("RPiCam destroying context ... ");
		zmq_ctx_destroy(context);
		context = NULL;
		LOGC("done");
	}
}

void RPiCam::openSocket()
{
	if (socket == NULL)
	{
		socket = zmq_socket(context, ZMQ_REQ);
		String url = String("tcp://") + String(address) + ":" + String(port);
		LOGC("RPiCam connecting to ", url.toStdString(), " ... ");

		int rc = zmq_connect(socket, url.toRawUTF8());

		if (rc != 0)
		{
			LOGC("failed to open socket: ", zmq_strerror(zmq_errno()));
			socket = NULL;
		}
		else
		{
			LOGC("done");
		}
	}
	else
	{
		LOGC("Socket already opened");
	}
}

bool RPiCam::closeSocket()
{
	if (socket != NULL)
	{
		LOGC("RPiCam closing socket ...");
		zmq_close(socket);
		socket = NULL;
		LOGC("done");
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

	LOGC("RPiCam sending message: ", msg.toStdString(), " ... ");

	if (socket != NULL)
	{
		zmq_setsockopt(socket, ZMQ_RCVTIMEO, (const void *)&timeout, sizeof(int));
		zmq_send(socket, msg.getCharPointer(), msg.length(), 0);

		unsigned char *buffer = new unsigned char[MAX_MESSAGE_LENGTH];
		int result = zmq_recv(socket, buffer, MAX_MESSAGE_LENGTH - 1, 0);
		if (result < 0)
		{
			response = String(""); // responder died.
		}
		else
		{
			response = String(reinterpret_cast<char *>(buffer), result);
		}
		delete buffer;
	}
	else
	{
		response = String("not connected");
	}

	LOGC("the RPi answered: ", response.toStdString());

	return response;
}

AudioProcessorEditor *RPiCam::createEditor()
{
	editor = std::make_unique<RPiCamEditor>(this);

	return editor.get();
}

bool RPiCam::startAcquisition()
{
	return true;
}

void RPiCam::startRecording()
{
	isRecording = true;

	auto nodeId = getNodeId();
	int expNumber = CoreServices::RecordNode::getExperimentNumber(nodeId);
	int recNumber = CoreServices::RecordNode::getRecordingNumber(nodeId) + 1;
	String recPath = CoreServices::RecordNode::getRecordingDirectory(nodeId).getFullPathName();

	String msg("Start");
	msg += String(" Experiment=") + String(expNumber);
	msg += String(" Recording=") + String(recNumber);

	// make sure to include a unique recording directory name to avoid overwriting data on the RPi
	if (recPath.length() == 0)
	{
		recPath = CoreServices::getRecordingDirectoryName();
	}
	msg += String(" Path=") + recPath;

	rpiRecPath = sendMessage(msg);
	sendRecPathEvent = true;
}

void RPiCam::stopRecording()
{
	isRecording = false;

	sendMessage(String("Stop"), 1000);
}

void RPiCam::process(AudioSampleBuffer &buffer)
{
	if (rpiRecPath.isNotEmpty() && sendRecPathEvent)
	{
		juce::int64 timestamp_software = timer.getHighResolutionTicks();
		String msg1("RPiCam Address=" + address + " RecPath=" + rpiRecPath);

		uint8 *msg1_raw = new uint8[msg1.length() + 1];
		memcpy(msg1_raw, msg1.toRawUTF8(), msg1.length());
		*(msg1_raw + msg1.length()) = '\0';
		String msgOut = msg1 + " Timestamp=" + String(timestamp_software);
		broadcastMessage(msgOut);

		delete[] msg1_raw;

		sendRecPathEvent = false;
	}
}

void RPiCam::enabledState(bool t)
{
	isEnabled = t;
}

void RPiCam::saveCustomParametersToXml(XmlElement *xml)
{
	xml->setAttribute("Left", (int)getParameter("Left")->getValue());
	xml->setAttribute("Bottom", (int)getParameter("Bottom")->getValue());
	xml->setAttribute("Width", (int)getParameter("Width")->getValue());
	xml->setAttribute("Height", (int)getParameter("Height")->getValue());
}

void RPiCam::loadCustomParametersFromXml(XmlElement *xml)
{
	int left = xml->getIntAttribute("Left", 0);
	int bottom = xml->getIntAttribute("Bottom", 0);
	int width = xml->getIntAttribute("Width", 100);
	int height = xml->getIntAttribute("Height", 100);

	auto param = getParameter("Left");
	param->setNextValue(left);
	param = getParameter("Bottom");
	param->setNextValue(bottom);
	param = getParameter("Width");
	param->setNextValue(width);
	param = getParameter("Height");
	param->setNextValue(height);
}