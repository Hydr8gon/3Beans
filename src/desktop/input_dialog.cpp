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

#include "input_dialog.h"

enum InputEvent {
    REMAP_A = 1,
    REMAP_B,
    REMAP_SELECT,
    REMAP_START,
    REMAP_RIGHT,
    REMAP_LEFT,
    REMAP_UP,
    REMAP_DOWN,
    REMAP_R,
    REMAP_L,
    REMAP_X,
    REMAP_Y,
    REMAP_LRIGHT,
    REMAP_LLEFT,
    REMAP_LUP,
    REMAP_LDOWN,
    REMAP_LMOD,
    REMAP_HOME,
    CLEAR_MAP
};

wxBEGIN_EVENT_TABLE(InputDialog, wxDialog)
EVT_BUTTON(REMAP_A, InputDialog::remapKey<0>)
EVT_BUTTON(REMAP_B, InputDialog::remapKey<1>)
EVT_BUTTON(REMAP_SELECT, InputDialog::remapKey<2>)
EVT_BUTTON(REMAP_START, InputDialog::remapKey<3>)
EVT_BUTTON(REMAP_RIGHT, InputDialog::remapKey<4>)
EVT_BUTTON(REMAP_LEFT, InputDialog::remapKey<5>)
EVT_BUTTON(REMAP_UP, InputDialog::remapKey<6>)
EVT_BUTTON(REMAP_DOWN, InputDialog::remapKey<7>)
EVT_BUTTON(REMAP_R, InputDialog::remapKey<8>)
EVT_BUTTON(REMAP_L, InputDialog::remapKey<9>)
EVT_BUTTON(REMAP_X, InputDialog::remapKey<10>)
EVT_BUTTON(REMAP_Y, InputDialog::remapKey<11>)
EVT_BUTTON(REMAP_LRIGHT, InputDialog::remapKey<12>)
EVT_BUTTON(REMAP_LLEFT, InputDialog::remapKey<13>)
EVT_BUTTON(REMAP_LUP, InputDialog::remapKey<14>)
EVT_BUTTON(REMAP_LDOWN, InputDialog::remapKey<15>)
EVT_BUTTON(REMAP_LMOD, InputDialog::remapKey<16>)
EVT_BUTTON(REMAP_HOME, InputDialog::remapKey<17>)
EVT_BUTTON(CLEAR_MAP, InputDialog::clearMap)
EVT_BUTTON(wxID_OK, InputDialog::confirm)
EVT_CHAR_HOOK(InputDialog::pressKey)
wxEND_EVENT_TABLE()

std::string InputDialog::keyToStr(int key) {
    // Convert special keys to words representing their respective keys
    switch (key) {
        case 0: return "None";
        case WXK_BACK: return "Backspace";
        case WXK_TAB: return "Tab";
        case WXK_RETURN: return "Return";
        case WXK_ESCAPE: return "Escape";
        case WXK_SPACE: return "Space";
        case WXK_DELETE: return "Delete";
        case WXK_START: return "Start";
        case WXK_LBUTTON: return "Left Button";
        case WXK_RBUTTON: return "Right Button";
        case WXK_CANCEL: return "Cancel";
        case WXK_MBUTTON: return "Middle Button";
        case WXK_CLEAR: return "Clear";
        case WXK_SHIFT: return "Shift";
        case WXK_ALT: return "Alt";
        case WXK_RAW_CONTROL: return "Control";
        case WXK_MENU: return "Menu";
        case WXK_PAUSE: return "Pause";
        case WXK_CAPITAL: return "Caps Lock";
        case WXK_END: return "End";
        case WXK_HOME: return "Home";
        case WXK_LEFT: return "Left";
        case WXK_UP: return "Up";
        case WXK_RIGHT: return "Right";
        case WXK_DOWN: return "Down";
        case WXK_SELECT: return "Select";
        case WXK_PRINT: return "Print";
        case WXK_EXECUTE: return "Execute";
        case WXK_SNAPSHOT: return "Snapshot";
        case WXK_INSERT: return "Insert";
        case WXK_HELP: return "Help";
        case WXK_NUMPAD0: return "Numpad 0";
        case WXK_NUMPAD1: return "Numpad 1";
        case WXK_NUMPAD2: return "Numpad 2";
        case WXK_NUMPAD3: return "Numpad 3";
        case WXK_NUMPAD4: return "Numpad 4";
        case WXK_NUMPAD5: return "Numpad 5";
        case WXK_NUMPAD6: return "Numpad 6";
        case WXK_NUMPAD7: return "Numpad 7";
        case WXK_NUMPAD8: return "Numpad 8";
        case WXK_NUMPAD9: return "Numpad 9";
        case WXK_MULTIPLY: return "Multiply";
        case WXK_ADD: return "Add";
        case WXK_SEPARATOR: return "Separator";
        case WXK_SUBTRACT: return "Subtract";
        case WXK_DECIMAL: return "Decimal";
        case WXK_DIVIDE: return "Divide";
        case WXK_F1: return "F1";
        case WXK_F2: return "F2";
        case WXK_F3: return "F3";
        case WXK_F4: return "F4";
        case WXK_F5: return "F5";
        case WXK_F6: return "F6";
        case WXK_F7: return "F7";
        case WXK_F8: return "F8";
        case WXK_F9: return "F9";
        case WXK_F10: return "F10";
        case WXK_F11: return "F11";
        case WXK_F12: return "F12";
        case WXK_F13: return "F13";
        case WXK_F14: return "F14";
        case WXK_F15: return "F15";
        case WXK_F16: return "F16";
        case WXK_F17: return "F17";
        case WXK_F18: return "F18";
        case WXK_F19: return "F19";
        case WXK_F20: return "F20";
        case WXK_F21: return "F21";
        case WXK_F22: return "F22";
        case WXK_F23: return "F23";
        case WXK_F24: return "F24";
        case WXK_NUMLOCK: return "Numlock";
        case WXK_SCROLL: return "Scroll";
        case WXK_PAGEUP: return "Page Up";
        case WXK_PAGEDOWN: return "Page Down";
        case WXK_NUMPAD_SPACE: return "Numpad Space";
        case WXK_NUMPAD_TAB: return "Numpad Tab";
        case WXK_NUMPAD_ENTER: return "Numpad Enter";
        case WXK_NUMPAD_F1: return "Numpad F1";
        case WXK_NUMPAD_F2: return "Numpad F2";
        case WXK_NUMPAD_F3: return "Numpad F3";
        case WXK_NUMPAD_F4: return "Numpad F4";
        case WXK_NUMPAD_HOME: return "Numpad Home";
        case WXK_NUMPAD_LEFT: return "Numpad Left";
        case WXK_NUMPAD_UP: return "Numpad Up";
        case WXK_NUMPAD_RIGHT: return "Numpad Right";
        case WXK_NUMPAD_DOWN: return "Numpad Down";
        case WXK_NUMPAD_PAGEUP: return "Numpad Page Up";
        case WXK_NUMPAD_PAGEDOWN: return "Numpad Page Down";
        case WXK_NUMPAD_END: return "Numpad End";
        case WXK_NUMPAD_BEGIN: return "Numpad Begin";
        case WXK_NUMPAD_INSERT: return "Numpad Insert";
        case WXK_NUMPAD_DELETE: return "Numpad Delete";
        case WXK_NUMPAD_EQUAL: return "Numpad Equal";
        case WXK_NUMPAD_MULTIPLY: return "Numpad Multiply";
        case WXK_NUMPAD_ADD: return "Numpad Add";
        case WXK_NUMPAD_SEPARATOR: return "Numpad Separator";
        case WXK_NUMPAD_SUBTRACT: return "Numpad Subtract";
        case WXK_NUMPAD_DECIMAL: return "Numpad Decimal";
        case WXK_NUMPAD_DIVIDE: return "Numpad Divide";

    default:
        // Directly use the key character for regular keys
        std::string regular;
        regular = (char)key;
        return regular;
    }
}

InputDialog::InputDialog(): wxDialog(nullptr, wxID_ANY, "Input Bindings") {
    // Use the height of a button as a unit to scale pixel values based on DPI/font
    wxButton *dummy = new wxButton(this, wxID_ANY, "");
    int size = dummy->GetSize().y;
    delete dummy;

    // Load the current key bindings and define strings
    memcpy(keyBinds, b3App::keyBinds, sizeof(keyBinds));
    const char *strs[] = {
        "A Button:", "B Button:", "Select Button:", "Start Button:",
        "D-Pad Right:", "D-Pad Left:", "D-Pad Up:", "D-Pad Down",
        "R Button:", "L Button:", "X Button:", "Y Button:",
        "Stick Right:", "Stick Left:", "Stick Up:", "Stick Down:",
        "Stick Mod:", "Home Button:"
    };

    // Set up the button settings
    wxBoxSizer *btnSizers[MAX_KEYS];
    for (int i = 0; i < MAX_KEYS; i++) {
        btnSizers[i] = new wxBoxSizer(wxHORIZONTAL);
        btnSizers[i]->Add(new wxStaticText(this, wxID_ANY, strs[i]), 1, wxALIGN_CENTRE | wxRIGHT, size / 16);
        btnSizers[i]->Add(keyBtns[i] = new wxButton(this, REMAP_A + i, keyToStr(keyBinds[i]),
            wxDefaultPosition, wxSize(size * 4, size)), 0, wxLEFT, size / 16);
    }

    // Combine all the left settings
    wxBoxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(btnSizers[0], 1, wxEXPAND | wxALL, size / 8);
    leftSizer->Add(btnSizers[1], 1, wxEXPAND | wxALL, size / 8);
    leftSizer->Add(btnSizers[10], 1, wxEXPAND | wxALL, size / 8);
    leftSizer->Add(btnSizers[11], 1, wxEXPAND | wxALL, size / 8);
    leftSizer->Add(btnSizers[3], 1, wxEXPAND | wxALL, size / 8);
    leftSizer->Add(btnSizers[2], 1, wxEXPAND | wxALL, size / 8);

    // Combine all the middle settings
    wxBoxSizer *middleSizer = new wxBoxSizer(wxVERTICAL);
    middleSizer->Add(btnSizers[6], 1, wxEXPAND | wxALL, size / 8);
    middleSizer->Add(btnSizers[7], 1, wxEXPAND | wxALL, size / 8);
    middleSizer->Add(btnSizers[5], 1, wxEXPAND | wxALL, size / 8);
    middleSizer->Add(btnSizers[4], 1, wxEXPAND | wxALL, size / 8);
    middleSizer->Add(btnSizers[9], 1, wxEXPAND | wxALL, size / 8);
    middleSizer->Add(btnSizers[8], 1, wxEXPAND | wxALL, size / 8);

    // Combine all the right settings
    wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(btnSizers[14], 1, wxEXPAND | wxALL, size / 8);
    rightSizer->Add(btnSizers[15], 1, wxEXPAND | wxALL, size / 8);
    rightSizer->Add(btnSizers[13], 1, wxEXPAND | wxALL, size / 8);
    rightSizer->Add(btnSizers[12], 1, wxEXPAND | wxALL, size / 8);
    rightSizer->Add(btnSizers[16], 1, wxEXPAND | wxALL, size / 8);
    rightSizer->Add(btnSizers[17], 1, wxEXPAND | wxALL, size / 8);

    // Combine all the settings
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(leftSizer, 1, wxEXPAND | wxRIGHT, size / 8);
    buttonSizer->Add(middleSizer, 1, wxEXPAND, size / 8);
    buttonSizer->Add(rightSizer, 1, wxEXPAND | wxLEFT, size / 8);

    // Set up the clear, cancel, and confirm buttons
    wxBoxSizer *naviSizer = new wxBoxSizer(wxHORIZONTAL);
    naviSizer->Add(new wxStaticText(this, wxID_ANY, ""), 1);
    naviSizer->Add(new wxButton(this, CLEAR_MAP, "Clear"), 0, wxRIGHT, size / 16);
    naviSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxLEFT | wxRIGHT, size / 16);
    naviSizer->Add(new wxButton(this, wxID_OK, "Confirm"), 0, wxLEFT, size / 16);

    // Combine all the contents
    wxBoxSizer *contents = new wxBoxSizer(wxVERTICAL);
    contents->Add(buttonSizer, 1, wxEXPAND | wxALL, size / 8);
    contents->Add(naviSizer, 0, wxEXPAND | wxALL, size / 8);

    // Add a final border around everything
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(contents, 1, wxEXPAND | wxALL, size / 8);
    SetSizerAndFit(sizer);
}

void InputDialog::resetLabels() {
    // Reset the button labels
    for (int i = 0; i < MAX_KEYS; i++)
        keyBtns[i]->SetLabel(keyToStr(keyBinds[i]));
}

template <int i> void InputDialog::remapKey(wxCommandEvent &event) {
    // Select an input to remap when a key is pressed
    resetLabels();
    keyBtns[i]->SetLabel("Press a key");
    current = keyBtns[i];
    keyIndex = i;
}

void InputDialog::clearMap(wxCommandEvent &event) {
    // Clear the selected mapping only, or all if none are selected
    if (current) { // Selected
        keyBinds[keyIndex] = 0;
        current->SetLabel(keyToStr(keyBinds[keyIndex]));
        current = nullptr;
    }
    else { // All
        memset(keyBinds, 0, sizeof(keyBinds));
        resetLabels();
    }
}

void InputDialog::confirm(wxCommandEvent &event) {
    // Save the modified key bindings
    memcpy(b3App::keyBinds, keyBinds, sizeof(keyBinds));
    Settings::save();
    event.Skip(true);
}

void InputDialog::pressKey(wxKeyEvent &event) {
    // Map the selected input to the pressed key
    if (!current) return;
    keyBinds[keyIndex] = event.GetKeyCode();
    current->SetLabel(keyToStr(keyBinds[keyIndex]));
    current = nullptr;
}
