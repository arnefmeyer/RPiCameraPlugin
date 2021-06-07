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


class RPiCamEditor : public GenericEditor, public Label::Listener, public ComboBox::Listener
{
public:
    RPiCamEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors);
    virtual ~RPiCamEditor();

    void buttonEvent(Button* button);
	void labelTextChanged(juce::Label *);
	void setLabelColor(juce::Colour color);
	void comboBoxChanged(ComboBox* cb);

	void updateValues();
	void updateGains(bool query = true);
	void setGainsFromUserInput();

	void enableControls(bool state);

private:

	ScopedPointer<UtilityButton> restartConnection;

    ScopedPointer<Label> portLabel;
	ScopedPointer<Label> portEdit;

    ScopedPointer<Label> addressLabel;
	ScopedPointer<Label> addressEdit;

	Array<RPiCamFormat> camFormats;
	ScopedPointer<Label> resolutionLabel;
	ScopedPointer<ComboBox> resolutionCombo;

	ScopedPointer<Label> fpsLabel;
	ScopedPointer<ComboBox> fpsCombo;

	ScopedPointer<UtilityButton> connectButton;
	ScopedPointer<UtilityButton> vflipButton;
	ScopedPointer<UtilityButton> hflipButton;

	ScopedPointer<Label> awbLabel;
	ScopedPointer<UtilityButton> resetButton;
	ScopedPointer<UtilityButton> getGainButton;
	ScopedPointer<UtilityButton> setGainButton;
	ScopedPointer<Label> gain1Edit;
	ScopedPointer<Label> gain2Edit;

	ScopedPointer<Label> zoomLabel;
	OwnedArray<Label> zoomValues;
	OwnedArray<TriangleButton> upButtons;
	OwnedArray<TriangleButton> downButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RPiCamEditor);

};



#endif  // __RPICAMEDITOR_H__

