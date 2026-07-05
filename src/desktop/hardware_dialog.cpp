/*
    Copyright 2023-2026 Hydr8gon

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

#include "hardware_dialog.h"
#include "../core/settings.h"

enum HardwareEvent {
    SYSTEM_AUTO = 1,
    SYSTEM_O3DS,
    SYSTEM_N3DS,
    UNIT_RETAIL,
    UNIT_DEV1,
    UNIT_DEV2,
    UNIT_DEV3
};

wxBEGIN_EVENT_TABLE(HardwareDialog, wxDialog)
EVT_RADIOBUTTON(SYSTEM_AUTO, HardwareDialog::systemType<0>)
EVT_RADIOBUTTON(SYSTEM_O3DS, HardwareDialog::systemType<1>)
EVT_RADIOBUTTON(SYSTEM_N3DS, HardwareDialog::systemType<2>)
EVT_RADIOBUTTON(UNIT_RETAIL, HardwareDialog::unitType<0>)
EVT_RADIOBUTTON(UNIT_DEV1, HardwareDialog::unitType<1>)
EVT_RADIOBUTTON(UNIT_DEV2, HardwareDialog::unitType<2>)
EVT_RADIOBUTTON(UNIT_DEV3, HardwareDialog::unitType<3>)
EVT_BUTTON(wxID_CANCEL, HardwareDialog::cancel)
EVT_BUTTON(wxID_OK, HardwareDialog::confirm)
wxEND_EVENT_TABLE()

HardwareDialog::HardwareDialog(): wxDialog(nullptr, wxID_ANY, "Set Hardware") {
    // Remember previous settings in case changes are discarded
    prevSettings[0] = Settings::systemType;
    prevSettings[1] = Settings::unitType;

    // Use the height of a button as a unit to scale pixel values based on DPI/font
    wxButton *dummy = new wxButton(this, wxID_ANY, "");
    int size = dummy->GetSize().y;
    delete dummy;

    // Set up the system type selections
    wxBoxSizer *systemSizer = new wxBoxSizer(wxHORIZONTAL);
    systemSizer->Add(new wxStaticText(this, wxID_ANY, "System Type:",
        wxDefaultPosition, wxSize(wxDefaultSize.GetWidth(), size)));
    systemSizer->Add(systemBtns[0] = new wxRadioButton(this, SYSTEM_AUTO, "Automatic",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxLEFT, size / 4);
    systemSizer->Add(systemBtns[1] = new wxRadioButton(this, SYSTEM_O3DS, "Old 3DS"), 0, wxLEFT, size / 4);
    systemSizer->Add(systemBtns[2] = new wxRadioButton(this, SYSTEM_N3DS, "New 3DS"), 0, wxLEFT, size / 4);

    // Set up the unit type selections
    wxBoxSizer *unitSizer = new wxBoxSizer(wxHORIZONTAL);
    unitSizer->Add(new wxStaticText(this, wxID_ANY, "Unit Type:",
        wxDefaultPosition, wxSize(wxDefaultSize.GetWidth(), size)));
    unitSizer->Add(unitBtns[0] = new wxRadioButton(this, UNIT_RETAIL, "Retail",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxLEFT, size / 4);
    unitSizer->Add(unitBtns[1] = new wxRadioButton(this, UNIT_DEV1, "Dev 1"), 0, wxLEFT, size / 4);
    unitSizer->Add(unitBtns[2] = new wxRadioButton(this, UNIT_DEV2, "Dev 2"), 0, wxLEFT, size / 4);
    unitSizer->Add(unitBtns[3] = new wxRadioButton(this, UNIT_DEV3, "Dev 3"), 0, wxLEFT, size / 4);

    // Set the initial setting states
    systemBtns[std::min(Settings::systemType, 2)]->SetValue(true);
    unitBtns[std::min(Settings::unitType, 3)]->SetValue(true);

    // Set up the cancel and confirm buttons
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxStaticText(this, wxID_ANY, ""), 1);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, size / 16);
    buttonSizer->Add(new wxButton(this, wxID_OK, "Confirm"), 0, wxLEFT, size / 16);

    // Combine all the contents with a warning
    wxBoxSizer *contents = new wxBoxSizer(wxVERTICAL);
    contents->Add(new wxStaticText(this, wxID_ANY, "Only change these if your NAND isn't booting."), 1, wxEXPAND);
    contents->Add(systemSizer, 1, wxEXPAND);
    contents->Add(unitSizer, 1, wxEXPAND);
    contents->Add(buttonSizer, 1, wxEXPAND);

    // Add a final border around everything
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(contents, 1, wxEXPAND | wxALL, size / 4);
    SetSizerAndFit(sizer);
}

template <int i> void HardwareDialog::systemType(wxCommandEvent &event) {
    // Set the system type to a specific value
    Settings::systemType = i;
}

template <int i> void HardwareDialog::unitType(wxCommandEvent &event) {
    // Set the unit type to a specific value
    Settings::unitType = i;
}

void HardwareDialog::cancel(wxCommandEvent &event) {
    // Reset settings to their previous values
    Settings::systemType = prevSettings[0];
    Settings::unitType = prevSettings[1];
    event.Skip(true);
}

void HardwareDialog::confirm(wxCommandEvent &event) {
    // Save the modified settings
    Settings::save();
    event.Skip(true);
}
