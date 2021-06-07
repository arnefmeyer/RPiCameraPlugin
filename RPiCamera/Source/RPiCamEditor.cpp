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
	desiredWidth = 350;

	RPiCam *p= (RPiCam *)getProcessor();

    addressLabel = new Label("Address", "Address");
    addressLabel->setFont(Font("Small Text", 12, Font::plain));
    addressLabel->setColour(Label::textColourId, Colours::darkgrey);
    addressLabel->setBounds(5,25,65,25);
    addAndMakeVisible(addressLabel);

	addressEdit = new Label("Address", p->getAddress());
    addressEdit->setBounds(72,25,120+23,20);
    addressEdit->setFont(Font("Default", 15, Font::plain));
    addressEdit->setColour(Label::textColourId, Colours::white);
	addressEdit->setColour(Label::backgroundColourId, Colours::grey);
    addressEdit->setEditable(true);
    addressEdit->addListener(this);
	addAndMakeVisible(addressEdit);

    portLabel = new Label("Port", "Port");
    portLabel->setFont(Font("Small Text", 12, Font::plain));
    portLabel->setColour(Label::textColourId, Colours::darkgrey);
    portLabel->setBounds(5,50,60,25);
    addAndMakeVisible(portLabel);

	portEdit = new Label("Port", String(p->getPort()));
    portEdit->setBounds(72,50,50+20,20);
    portEdit->setFont(Font("Default", 15, Font::plain));
    portEdit->setColour(Label::textColourId, Colours::white);
	portEdit->setColour(Label::backgroundColourId, Colours::grey);
    portEdit->setEditable(true);
    portEdit->addListener(this);
	addAndMakeVisible(portEdit);

	connectButton = new UtilityButton("Connect", Font("Default", 15, Font::plain));
    connectButton->setBounds(130+20,50,65,20);
    connectButton->addListener(this);
    addAndMakeVisible(connectButton);

	// frame rate (values depend on camera format)
    fpsLabel = new Label("FPS", "FPS");
    fpsLabel->setFont(Font("Small Text", 12, Font::plain));
    fpsLabel->setColour(Label::textColourId, Colours::darkgrey);
    fpsLabel->setBounds(177,75,65,20);
    addAndMakeVisible(fpsLabel);

	fpsCombo = new ComboBox();
    fpsCombo->setBounds(215,75,50,20);
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

    resolutionLabel = new Label("Resolution", "Resolution");
    resolutionLabel->setFont(Font("Small Text", 12, Font::plain));
    resolutionLabel->setColour(Label::textColourId, Colours::darkgrey);
    resolutionLabel->setBounds(5,75,65,20);
    addAndMakeVisible(resolutionLabel);

    resolutionCombo = new ComboBox();
    resolutionCombo->setBounds(72,75,95,20);
    resolutionCombo->addListener(this);
    for (int i=0; i<camFormats.size(); i++)
    {
		String str(String(camFormats[i].width) + "x" + String(camFormats[i].height));
        resolutionCombo->addItem(str, i+1);
    }
	resolutionCombo->setSelectedItemIndex(camFormats.size()-1, sendNotification);
	addAndMakeVisible(resolutionCombo);

	// AWB gain settings
	awbLabel = new Label("Gains", "WB Gains");
    awbLabel->setFont(Font("Small Text", 12, Font::plain));
    awbLabel->setColour(Label::textColourId, Colours::darkgrey);
    awbLabel->setBounds(275,25,70,20);
    awbLabel->setTooltip("White level balance gains");
    addAndMakeVisible(awbLabel);

	// re-initialization
	resetButton = new UtilityButton("R", Font("Default", 15, Font::plain));
    resetButton->setBounds(280,50,20,20);
    resetButton->addListener(this);
	resetButton->setTooltip("Reinitialize automatic white level balance gains");
    addAndMakeVisible(resetButton);

	// retrieve current settings
	getGainButton = new UtilityButton("G", Font("Default", 15, Font::plain));
    getGainButton->setBounds(302,50,20,20);
    getGainButton->addListener(this);
	getGainButton->setTooltip("Get current white level balance gains");
    addAndMakeVisible(getGainButton);

	// set gains
	setGainButton = new UtilityButton("S", Font("Default", 15, Font::plain));
    setGainButton->setBounds(325,50,20,20);
    setGainButton->addListener(this);
	setGainButton->setTooltip("Set white level balance gains");
    addAndMakeVisible(setGainButton);

    // gain values
	gain1Edit = new Label("gain1", "");
    gain1Edit->setBounds(280,75,60,20);
    gain1Edit->setFont(Font("Default", 15, Font::plain));
    gain1Edit->setColour(Label::textColourId, Colours::white);
	gain1Edit->setColour(Label::backgroundColourId, Colours::grey);
    gain1Edit->setEditable(true);
    gain1Edit->addListener(this);
	addAndMakeVisible(gain1Edit);

	gain2Edit = new Label("gain2", "");
    gain2Edit->setBounds(280,100,60,20);
    gain2Edit->setFont(Font("Default", 15, Font::plain));
    gain2Edit->setColour(Label::textColourId, Colours::white);
	gain2Edit->setColour(Label::backgroundColourId, Colours::grey);
    gain2Edit->setEditable(true);
    gain2Edit->addListener(this);
	addAndMakeVisible(gain2Edit);

	updateGains(false);

	// zoom buttons
    zoomLabel = new Label("Zoom", "Zoom");
    zoomLabel->setFont(Font("Small Text", 12, Font::plain));
    zoomLabel->setColour(Label::textColourId, Colours::darkgrey);
    zoomLabel->setBounds(5,100,65,25);
	zoomLabel->setTooltip("(left, bottom, right, top) in percent");
    addAndMakeVisible(zoomLabel);

	int zoom[4];
	p->getZoom(&zoom[0]);

	int x0 = 85;
	int y0 = 100;
	int dx = 40;
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

	// horizontal flip
	hflipButton = new UtilityButton("H", Font("Default", 15, Font::plain));
    hflipButton->setBounds(233-10,25+75,20,20);
    hflipButton->setClickingTogglesState(true);
    hflipButton->setToggleState(p->getHflip(), dontSendNotification);
    hflipButton->addListener(this);
	hflipButton->setTooltip("Enable/disable horizontal flip");
    addAndMakeVisible(hflipButton);

	// vertical flip
	vflipButton = new UtilityButton("V", Font("Default", 15, Font::plain));
    vflipButton->setBounds(255-10,25+75,20,20);
    vflipButton->setClickingTogglesState(true);
    vflipButton->setToggleState(p->getVflip(), dontSendNotification);
    vflipButton->addListener(this);
	vflipButton->setTooltip("Enable/disable vertical flip");
    addAndMakeVisible(vflipButton);

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


void RPiCamEditor::updateGains(bool query)
{
	double gains[2];

	RPiCam *p= (RPiCam *)getProcessor();
	p->getGains(&gains[0], query);

	std::cout << "RPiCamEditor::updateGains " << gains[0] << " " << gains[1] << "\n";
	if (gains[0] >= 0 && gains[0] <= 8)
	{
		gain1Edit->setText(String(gains[0], 6), dontSendNotification);
	}
	if (gains[1] >= 0 && gains[1] <= 8)
	{
		gain2Edit->setText(String(gains[1], 6), dontSendNotification);
	}
}


void RPiCamEditor::setGainsFromUserInput()
{
	double gains[2];

	RPiCam *p= (RPiCam *)getProcessor();
	p->getGains(&gains[0], false);

	double gain1 = gain1Edit->getText().getDoubleValue();
	double gain2 = gain2Edit->getText().getDoubleValue();

	if ((gain1 >= 0) && (gain1 <= 8) && (gain2 >= 0) && (gain2 <= 8))
	{
		gains[0] = gain1;
		gains[1] = gain2;
		p->setGains(&gains[0], true);
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
		updateGains(true);
	}
	else if (button == resetButton)
	{
		p->resetGains();
	}
	else if (button == getGainButton)
	{
		updateGains(true);
	}
	else if (button == setGainButton)
	{
		setGainsFromUserInput();
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
				if (newValue >= 0 && newValue <= 100)
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
	else if (label == gain1Edit)
	{
		double g = label->getText().getDoubleValue();
		p->setGain(0, g, true);
	}
	else if (label == gain2Edit)
	{
		double g = label->getText().getDoubleValue();
		p->setGain(1, g, true);
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
				if (newValue >= 0 && newValue <= 100)
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
		int fps = p->getFramerate();

		p->setResolution(w, h);

		fpsCombo->clear();
		RPiCamFormat fmt = camFormats[index];
		for (int i=0; i<fmt.framerate_max-fmt.framerate_min+1; i++)
		{
			fpsCombo->addItem(String(fmt.framerate_min + i), i+1);
		}
		
		if ((fps >= fmt.framerate_min) && (fps <= fmt.framerate_max))
		{
			// use current frame rate
			fpsCombo->setSelectedItemIndex(fps - fmt.framerate_min, sendNotification);
		}
		else
		{
			// set default frame rate
			fpsCombo->setSelectedItemIndex(fmt.framerate - fmt.framerate_min, sendNotification);
		}
	}
	else if (cb == fpsCombo)
	{
		int index = cb->getSelectedItemIndex();
		int fps = cb->getItemText(index).getIntValue();
		p->setFramerate(fps);
	}
}

