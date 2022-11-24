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

#ifndef __RPICAMEDITOR_H__
#define __RPICAMEDITOR_H__

#include <EditorHeaders.h>

class RPiCam;

/**

  User interface for the "RPiCam" plugin

  @see RPiCam

*/

class CustomUpDownButton : public ParameterEditor,
						   public Button::Listener,
						   public Label::Listener
{
public:
	CustomUpDownButton(Parameter *, int);
	virtual ~CustomUpDownButton() {}
	void buttonClicked(Button *label) override;
	void labelTextChanged(Label *) override;
	void updateView(){};
	void resized() override;
	void setToolTip(const String &);
	void updateLabel();

private:
	std::unique_ptr<TriangleButton> m_up;
	std::unique_ptr<TriangleButton> m_down;
	std::unique_ptr<Label> m_label;
	int m_value = 0;
};

class CustomButton : public ParameterEditor,
					 public Button::Listener
{
public:
	CustomButton(Parameter *);
	virtual ~CustomButton(){};
	void buttonClicked(Button *) override;
	void updateView(){};
	void resized() override;
	void setToolTip(const String &);

private:
	std::unique_ptr<UtilityButton> m_btn;
};

class RPiCamEditor : public GenericEditor
{
public:
	RPiCamEditor(GenericProcessor *parentNode);
	virtual ~RPiCamEditor();

	// void buttonEvent(Button *button);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RPiCamEditor);
};

#endif // __RPICAMEDITOR_H__
