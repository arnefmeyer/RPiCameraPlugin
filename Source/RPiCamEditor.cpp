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

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

CustomUpDownButton::CustomUpDownButton(Parameter *param) : ParameterEditor(param)
{
	m_up = std::make_unique<TriangleButton>(1);
	m_up->setBounds(10, 0, 10, 10);
	m_up->addListener(this);
	addAndMakeVisible(m_up.get());

	m_label = std::make_unique<Label>(param->getName(), param->getName());
	m_label->setBounds(0, 13, 40, 10);
	Font f{10, Font::bold};
	m_label->setFont(f);
	m_label->setJustificationType(Justification::centred);
	addAndMakeVisible(m_label.get());

	m_down = std::make_unique<TriangleButton>(2);
	m_down->setBounds(10, 25, 10, 10);
	m_down->addListener(this);
	addAndMakeVisible(m_down.get());

	setBounds(0, 0, 100, 40);
}

void CustomUpDownButton::buttonClicked(Button *btn)
{
	if (btn == m_up.get())
	{
		++m_value;
	}
	if (btn == m_down.get())
	{
		--m_value;
	}
	param->setNextValue(m_value);
}

void CustomUpDownButton::resized(){};

CustomButton::CustomButton(Parameter *param) : ParameterEditor(param)
{
	m_btn = std::make_unique<UtilityButton>(param->getName(), Font::plain);
	m_btn->addListener(this);
	m_btn->setBounds(0, 0, 50, 15);
	addAndMakeVisible(m_btn.get());

	setBounds(0, 0, 80, 20);
}

void CustomButton::buttonClicked(Button *btn)
{
	if (btn == m_btn.get())
	{
		param->setNextValue(m_btn->getLabel());
	}
}

void CustomButton::setToolTip(const String &str)
{
	m_btn->setTooltip(str);
}

void CustomButton::resized() {}

RPiCamEditor::RPiCamEditor(GenericProcessor *parentNode)
	: GenericEditor(parentNode)

{
	desiredWidth = 280;

	RPiCam *p = (RPiCam *)getProcessor();

	addTextBoxParameterEditor("Port", 5, 25);
	addTextBoxParameterEditor("Address", 95, 25);
	addComboBoxParameterEditor("Resolution", 185, 25);
	addComboBoxParameterEditor("FPS", 185, 65);

	Parameter *connect = getProcessor()->getParameter("Connect");
	auto btn = new CustomButton(connect);
	btn->setToolTip("Connect to the RPi");
	addCustomParameterEditor(btn, 205, 105);

	Parameter *reInit = getProcessor()->getParameter("R");
	btn = new CustomButton(reInit);
	btn->setToolTip("Reinitialize automatic gain and white level balance");
	addCustomParameterEditor(btn, 5, 105);

	Parameter *hFlip = getProcessor()->getParameter("H");
	btn = new CustomButton(hFlip);
	btn->setToolTip("Enable/disable horizontal flip");
	addCustomParameterEditor(btn, 75, 105);

	Parameter *vFlip = getProcessor()->getParameter("V");
	btn = new CustomButton(vFlip);
	btn->setToolTip("Enable/disable vertical flip");
	addCustomParameterEditor(btn, 145, 105);

	auto zoomLabel = new Label("Zoom", "Zoom:");
	zoomLabel->setBounds(5, 70, 65, 25);
	addAndMakeVisible(zoomLabel);
	Parameter *left = getProcessor()->getParameter("Left");
	addCustomParameterEditor(new CustomUpDownButton(left), 45, 65);
	Parameter *bottom = getProcessor()->getParameter("Bottom");
	addCustomParameterEditor(new CustomUpDownButton(bottom), 75, 65);
	Parameter *width = getProcessor()->getParameter("Width");
	addCustomParameterEditor(new CustomUpDownButton(width), 105, 65);
	Parameter *height = getProcessor()->getParameter("Height");
	addCustomParameterEditor(new CustomUpDownButton(height), 135, 65);
}

RPiCamEditor::~RPiCamEditor()
{
}
/*
void RPiCamEditor::updateValues()
{
	RPiCam *p = (RPiCam *)getProcessor();

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

	// set camera format based on resolution
	int index = 0;
	for (RPiCamFormat *fmt = camFormats.begin(); fmt++; fmt != camFormats.end())
	{
		if (fmt->width == p->getWidth() && fmt->height == p->getHeight())
		{
			resolutionCombo->setSelectedItemIndex(index + 1, dontSendNotification);

			fpsCombo->clear();
			RPiCamFormat fmt = camFormats[index];
			for (int i = 0; i < fmt.framerate_max - fmt.framerate_min + 1; i++)
			{
				fpsCombo->addItem(String(fmt.framerate_min + i), i + 1);
			}
			fpsCombo->setSelectedItemIndex(p->getFramerate() - fmt.framerate_min - 1, dontSendNotification);

			break;
		}
		index++;
	}
}

void RPiCamEditor::enableControls(bool state)
{
	resolutionCombo->setEnabled(state);
	fpsCombo->setEnabled(state);
}

void RPiCamEditor::buttonEvent(Button *button)
{
	RPiCam *p = (RPiCam *)getProcessor();

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

		for (int i = 0; i < 4; i++)
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
					if ((i < 2 && newValue < zoom[i + 2]) || (i >= 2 && newValue > zoom[i - 2]))
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
	RPiCam *p = (RPiCam *)getProcessor();

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

		for (int i = 0; i < 4; i++)
		{
			if (label == zoomValues[i])
			{
				int newValue = label->getText().getIntValue();
				if (newValue >= 0 && newValue <= 100)
				{
					if ((i < 2 && newValue < zoom[i + 2]) || (i >= 2 && newValue > zoom[i - 2]))
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

void RPiCamEditor::comboBoxChanged(ComboBox *cb)
{
	RPiCam *p = (RPiCam *)getProcessor();

	if (cb == resolutionCombo)
	{
		int index = cb->getSelectedItemIndex();
		int w = camFormats[index].width;
		int h = camFormats[index].height;
		p->setResolution(w, h);

		fpsCombo->clear();
		RPiCamFormat fmt = camFormats[index];
		for (int i = 0; i < fmt.framerate_max - fmt.framerate_min + 1; i++)
		{
			fpsCombo->addItem(String(fmt.framerate_min + i), i + 1);
		}
		fpsCombo->setSelectedItemIndex(fmt.framerate - fmt.framerate_min, sendNotification);
	}
	else if (cb == fpsCombo)
	{
		int index = cb->getSelectedItemIndex();
		int fps = cb->getItemText(index).getIntValue();
		p->setFramerate(fps);
	}
}
*/