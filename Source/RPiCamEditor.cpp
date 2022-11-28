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

CustomUpDownButton::CustomUpDownButton(Parameter *param, int initValue = 0) : ParameterEditor(param), m_value(initValue)
{
	m_up = std::make_unique<TriangleButton>(1);
	m_up->setBounds(10, 0, 10, 10);
	m_up->addListener(this);
	addAndMakeVisible(m_up.get());

	auto str = param->getName();
	str = str.substring(0, 1) + String(":") + String(m_value);
	m_label = std::make_unique<Label>(param->getName(), str);
	m_label->setBounds(0, 13, 40, 10);
	Font f{12, Font::bold};
	m_label->setFont(f);
	m_label->setJustificationType(Justification::left);
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
	updateLabel();
}

void CustomUpDownButton::updateLabel()
{
	auto str = param->getName();
	str = str.substring(0, 1) + String(":") + String(m_value);
	m_label->setText(str, dontSendNotification);
}

void CustomUpDownButton::setToolTip(const String &str)
{
	m_label->setTooltip(str);
}

void CustomUpDownButton::resized(){};

void CustomUpDownButton::labelTextChanged(Label *lbl)
{
}

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
		auto val = param->getValue();
		if (val.isBool()) {
			bool newval = (bool)val;
			param->setNextValue(!newval);
		}
		auto name = param->getName();
		if (name == "Connect") {
			param->setNextValue(1);
		}
	}
}

void CustomButton::setToolTip(const String &str)
{
	m_btn->setTooltip(str);
}

void CustomButton::resized()
{
}

RPiCamEditor::RPiCamEditor(GenericProcessor *parentNode)
	: GenericEditor(parentNode)

{
	desiredWidth = 320;

	addTextBoxParameterEditor("Port", 5, 20);
	addTextBoxParameterEditor("Address", 95, 20);
	addComboBoxParameterEditor("Resolution", 235, 20);
	addComboBoxParameterEditor("FPS", 235, 55);

	Parameter *connect = getProcessor()->getParameter("Connect");
	auto btn = new CustomButton(connect);
	btn->setToolTip(connect->getDescription());
	addCustomParameterEditor(btn, 215, 105);

	Parameter *reInit = getProcessor()->getParameter("R");
	btn = new CustomButton(reInit);
	btn->setToolTip(reInit->getDescription());
	addCustomParameterEditor(btn, 5, 105);

	Parameter *hFlip = getProcessor()->getParameter("H");
	btn = new CustomButton(hFlip);
	btn->setToolTip(hFlip->getDescription());
	addCustomParameterEditor(btn, 75, 105);

	Parameter *vFlip = getProcessor()->getParameter("V");
	btn = new CustomButton(vFlip);
	btn->setToolTip(vFlip->getDescription());
	addCustomParameterEditor(btn, 145, 105);

	auto zoomLabel = new Label("Zoom", "Zoom:");
	zoomLabel->setBounds(5, 70, 65, 25);
	addAndMakeVisible(zoomLabel);

	Parameter *left = getProcessor()->getParameter("Left");
	auto cbtn = new CustomUpDownButton(left);
	cbtn->setToolTip(left->getDescription());
	addCustomParameterEditor(cbtn, 50, 65);

	Parameter *bottom = getProcessor()->getParameter("Bottom");
	cbtn = new CustomUpDownButton(bottom);
	cbtn->setToolTip(bottom->getDescription());
	addCustomParameterEditor(cbtn, 95, 65);

	Parameter *width = getProcessor()->getParameter("Width");
	cbtn = new CustomUpDownButton(width, 100);
	cbtn->setToolTip(width->getDescription());
	addCustomParameterEditor(cbtn, 140, 65);

	Parameter *height = getProcessor()->getParameter("Height");
	cbtn = new CustomUpDownButton(height, 100);
	cbtn->setToolTip(height->getDescription());
	addCustomParameterEditor(cbtn, 185, 65);

	// adjust the address parameter editor to be a bit bigger
	for (const auto &ed : parameterEditors)
	{
		if (ed->getParameterName().equalsIgnoreCase("Address"))
		{
			auto rect = ed->getBounds();
			rect.setWidth(rect.getWidth() + 40);
			ed->setBounds(rect);
		}
	}
}

RPiCamEditor::~RPiCamEditor()
{
}