/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

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

#include "RPiCamEditor.h"
#include "RPiCam.h"

#include <stdio.h>

#define MIN(a, b) ((a) < (b)) ? (a) : (b)))


RPiCamEditor::RPiCamEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors=true)
    : GenericEditor(parentNode, useDefaultParameterEditors)

{
	desiredWidth = 280;

	RPiCam *p= (RPiCam *)getProcessor();

    portLabel = new Label("Port", "Port:");
    portLabel->setBounds(5,25,65,25);
    addAndMakeVisible(portLabel);

	portEdit = new Label("Port", String(p->getPort()));
    portEdit->setBounds(75,25,50,20);
    portEdit->setFont(Font("Default", 15, Font::plain));
    portEdit->setColour(Label::textColourId, Colours::white);
	portEdit->setColour(Label::backgroundColourId, Colours::grey);
    portEdit->setEditable(true);
    portEdit->addListener(this);
	addAndMakeVisible(portEdit);

	connectButton = new UtilityButton("Connect", Font("Default", 15, Font::plain));
    connectButton->setBounds(130,25,65,20);
    connectButton->addListener(this);
    addAndMakeVisible(connectButton);

    addressLabel = new Label("Address", "Address:");
    addressLabel->setBounds(5,50,65,25);
    addAndMakeVisible(addressLabel);

	addressEdit = new Label("Address", p->getAddress());
    addressEdit->setBounds(75,50,120+80,20);
    addressEdit->setFont(Font("Default", 15, Font::plain));
    addressEdit->setColour(Label::textColourId, Colours::white);
	addressEdit->setColour(Label::backgroundColourId, Colours::grey);
    addressEdit->setEditable(true);
    addressEdit->addListener(this);
	addAndMakeVisible(addressEdit);

	// frame rate (values depend on camera format)
    fpsLabel = new Label("FPS", "FPS:");
    fpsLabel->setBounds(185,75,65,20);
    addAndMakeVisible(fpsLabel);

	fpsCombo = new ComboBox();
    fpsCombo->setBounds(225,75,50,20);
    fpsCombo->addListener(this);
	addAndMakeVisible(fpsCombo);

	// camera formats; for available video modes see https://picamera.readthedocs.io/en/release-1.13/fov.html
	camFormats = Array<RPiCamFormat>();
	camFormats.add(RPiCamFormat(2592, 1944, 10, 1, 15));
	camFormats.add(RPiCamFormat(1920, 1080, 10, 1, 15));
	camFormats.add(RPiCamFormat(1296, 972, 30, 1, 42));
	camFormats.add(RPiCamFormat(1296, 730, 30, 1, 49));
	camFormats.add(RPiCamFormat(1280, 720, 30, 1, 49));
 	camFormats.add(RPiCamFormat(1024, 768, 30, 1, 60));
	camFormats.add(RPiCamFormat(800, 600, 30, 1, 60));
	camFormats.add(RPiCamFormat(640, 480, 30, 1, 90));

    resolutionLabel = new Label("Resolution", "Resolution:");
    resolutionLabel->setBounds(5,75,60,20);
    addAndMakeVisible(resolutionLabel);

    resolutionCombo = new ComboBox();
    resolutionCombo->setBounds(75,75,95,20);
    resolutionCombo->addListener(this);
    for (int i=0; i<camFormats.size(); i++)
    {
		String str(String(camFormats[i].width) + "x" + String(camFormats[i].height));
        resolutionCombo->addItem(str, i+1);
    }
	resolutionCombo->setSelectedItemIndex(camFormats.size()-1, sendNotification);
	addAndMakeVisible(resolutionCombo);

	// re-initialization
	resetButton = new UtilityButton("R", Font("Default", 15, Font::plain));
    resetButton->setBounds(210,25,20,20);
    resetButton->addListener(this);
	resetButton->setTooltip("Reinitialize automatic gain and white level balance");
    addAndMakeVisible(resetButton);

	// horizontal flip
	hflipButton = new UtilityButton("H", Font("Default", 15, Font::plain));
    hflipButton->setBounds(233,25,20,20);
    hflipButton->setClickingTogglesState(true);
    hflipButton->setToggleState(p->getHflip(), dontSendNotification);
    hflipButton->addListener(this);
	hflipButton->setTooltip("Enable/disable horizontal flip");
    addAndMakeVisible(hflipButton);

	// vertical flip
	vflipButton = new UtilityButton("V", Font("Default", 15, Font::plain));
    vflipButton->setBounds(255,25,20,20);
    vflipButton->setClickingTogglesState(true);
    vflipButton->setToggleState(p->getVflip(), dontSendNotification);
    vflipButton->addListener(this);
	vflipButton->setTooltip("Enable/disable vertical flip");
    addAndMakeVisible(vflipButton);

	// zoom buttons
    zoomLabel = new Label("Zoom", "Zoom:");
    zoomLabel->setBounds(5,100,65,25);
	zoomLabel->setTooltip("(left, bottom, right, top) in percent");
    addAndMakeVisible(zoomLabel);

	int zoom[4];
	p->getZoom(&zoom[0]);

	int x0 = 85;
	int y0 = 100;
	int dx = 42;
	int dy = 12;
	TriangleButton *b;
	Label *l;

	for (int i=0; i<4; i++)
	{
		b = new TriangleButton(1);
		b->addListener(this);
		b->setBounds(x0+i*dx, y0+2, 10, 8);
		addAndMakeVisible(b);
		upButtons.add(b);

		b = new TriangleButton(2);
		b->addListener(this);
		b->setBounds(x0+i*dx, y0+2+dy, 10, 8);
		addAndMakeVisible(b);
		downButtons.add(b);

		l = new Label("Zoom" + String(i), String(zoom[i]));
		l->setBounds(x0+i*dx-32,100,35,25);
		l->setEditable(true);
		l->setJustificationType(Justification::right);
		l->addListener(this);
		addAndMakeVisible(l);
		zoomValues.add(l);
	}
}


RPiCamEditor::~RPiCamEditor()
{

}


void RPiCamEditor::updateValues()
{
	RPiCam *p= (RPiCam *)getProcessor();

	Value val = portEdit->getTextValue();
	int pp = val.getValue();
	if (p->getPort() != pp)
	{
		portEdit->setText(String(p->getPort()), dontSendNotification);
	}

	if (p->getAddress() != addressEdit->getText())
	{
		addressEdit->setText(p->getAddress(), dontSendNotification);
	}
}


void RPiCamEditor::enableControls(bool state)
{
	resolutionCombo->setEnabled(state);
	fpsCombo->setEnabled(state);
}

void RPiCamEditor::buttonEvent(Button* button)
{
	RPiCam *p= (RPiCam *)getProcessor();

	if (button == connectButton)
	{		
		p->closeSocket();
		p->openSocket();
		p->sendCameraParameters();
	}
	else if (button == resetButton)
	{
		p->resetGains();
	}
	else if (button == hflipButton)
	{
		p->setHflip(button->getToggleState());
	}
	else if (button == vflipButton)
	{
		p->setVflip(button->getToggleState());
	}
	else
	{
		// this is not particularly efficient ...
		int zoom[4];
		p->getZoom(&zoom[0]);

		for (int i=0; i<4; i++)
		{
			int currentValue = zoomValues[i]->getText().getIntValue();
			int increment = 0;
			if (button == upButtons[i])
			{
				increment = 1;
			}

			if (button == downButtons[i])
			{
				increment = -1;
			}

			if (increment != 0)
			{
				int newValue = currentValue + increment;
				if (newValue >= 1 && newValue <= 100)
				{
					if ((i < 2 && newValue < zoom[i+2]) || (i >= 2 && newValue > zoom[i-2]))
					{
						zoom[i] = newValue;
						zoomValues[i]->setText(String(zoom[i]), dontSendNotification);

						p->setZoom(&zoom[0]);
						break;
					}
				}
			}
		}
	}
}


void RPiCamEditor::setLabelColor(juce::Colour color)
{
	addressEdit->setColour(Label::backgroundColourId, color);
	portEdit->setColour(Label::backgroundColourId, color);
}


void RPiCamEditor::labelTextChanged(juce::Label *label)
{
	RPiCam *p= (RPiCam *)getProcessor();

	if (label == portEdit)
	{
	    Value val = label->getTextValue();
		p->setPort(val.getValue());
	}
	else if (label == addressEdit)
	{
		p->setAddress(label->getText());
	}
	else
	{
		// again, not particularly elegant ...
		int zoom[4];
		p->getZoom(&zoom[0]);

		for (int i=0; i<4; i++)
		{
			if (label == zoomValues[i])
			{
				int newValue = label->getText().getIntValue();
				if (newValue >= 1 && newValue <= 100)
				{
					if ((i < 2 && newValue < zoom[i+2]) || (i >= 2 && newValue > zoom[i-2]))
					{
						zoom[i] = newValue;
						p->setZoom(&zoom[0]);
						break;
					}
					else
					{
						// invalid value -> keep old value
						label->setText(String(zoom[i]), dontSendNotification);
					}
				}
			}
		}
	}
}


void RPiCamEditor::comboBoxChanged(ComboBox* cb)
{
	RPiCam *p= (RPiCam*)getProcessor();

	if (cb == resolutionCombo)
	{
		int index = cb->getSelectedItemIndex();
		int w = camFormats[index].width;
		int h = camFormats[index].height;
		p->setResolution(w, h);

		fpsCombo->clear();
		RPiCamFormat fmt = camFormats[index];
		for (int i=0; i<fmt.framerate_max-fmt.framerate_min+1; i++)
		{
			fpsCombo->addItem(String(fmt.framerate_min + i), i+1);
		}
		fpsCombo->setSelectedItemIndex(fmt.framerate-fmt.framerate_min, sendNotification);
	}
	else if (cb == fpsCombo)
	{
		int index = cb->getSelectedItemIndex();
		int fps = cb->getItemText(index).getIntValue();
		p->setFramerate(fps);
	}
}

