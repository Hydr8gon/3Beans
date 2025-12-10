/*
    Copyright 2023-2025 Hydr8gon

    This file is part of 3Beans.

    3Beans is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    3Beans is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with 3Beans. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <wx/wx.h>
#include "b3_app.h"

class InputDialog: public wxDialog {
public:
    InputDialog(wxJoystick *joystick);
    ~InputDialog();

private:
    wxButton *keyBtns[MAX_KEYS];
    wxJoystick *joystick;
    wxTimer *timer;

    int keyBinds[MAX_KEYS];
    std::vector<int> axisBases;
    wxButton *current = nullptr;
    int keyIndex = 0;

    static std::string keyToStr(int key);
    void resetLabels();

    template <int i> void remapKey(wxCommandEvent &event);
    void clearMap(wxCommandEvent &event);
    void updateJoystick(wxTimerEvent &event);
    void confirm(wxCommandEvent &event);
    void pressKey(wxKeyEvent &event);
    wxDECLARE_EVENT_TABLE();
};
