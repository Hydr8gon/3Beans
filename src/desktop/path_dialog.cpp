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

#include "path_dialog.h"
#include "../core/settings.h"

enum PathEvent {
    BOOT11_BROWSE = 1,
    BOOT9_BROWSE,
    NAND_BROWSE,
    SD_BROWSE,
    OPEN_FOLDER
};

wxBEGIN_EVENT_TABLE(PathDialog, wxDialog)
EVT_BUTTON(BOOT11_BROWSE, PathDialog::boot11Browse)
EVT_BUTTON(BOOT9_BROWSE, PathDialog::boot9Browse)
EVT_BUTTON(NAND_BROWSE, PathDialog::nandBrowse)
EVT_BUTTON(SD_BROWSE, PathDialog::sdBrowse)
EVT_BUTTON(OPEN_FOLDER, PathDialog::openFolder)
EVT_BUTTON(wxID_OK, PathDialog::confirm)
wxEND_EVENT_TABLE()

PathDialog::PathDialog(): wxDialog(nullptr, wxID_ANY, "Path Settings") {
    // Use the height of a button as a unit to scale pixel values based on DPI/font
    wxButton *dummy = new wxButton(this, wxID_ANY, "");
    int size = dummy->GetSize().y;
    delete dummy;

    // Set up the ARM11 boot ROM path setting
    wxBoxSizer *boot11Sizer = new wxBoxSizer(wxHORIZONTAL);
    boot11Sizer->Add(new wxStaticText(this, wxID_ANY, "Boot11 ROM:"), 1, wxALIGN_CENTRE | wxRIGHT, size / 8);
    boot11Path = new wxTextCtrl(this, wxID_ANY, Settings::boot11Path, wxDefaultPosition, wxSize(size * 8, size));
    boot11Sizer->Add(boot11Path);
    boot11Sizer->Add(new wxButton(this, BOOT11_BROWSE, "Browse"), 0, wxLEFT, size / 8);

    // Set up the ARM9 boot ROM path setting
    wxBoxSizer *boot9Sizer = new wxBoxSizer(wxHORIZONTAL);
    boot9Sizer->Add(new wxStaticText(this, wxID_ANY, "Boot9 ROM:"), 1, wxALIGN_CENTRE | wxRIGHT, size / 8);
    boot9Path = new wxTextCtrl(this, wxID_ANY, Settings::boot9Path, wxDefaultPosition, wxSize(size * 8, size));
    boot9Sizer->Add(boot9Path);
    boot9Sizer->Add(new wxButton(this, BOOT9_BROWSE, "Browse"), 0, wxLEFT, size / 8);

    // Set up the NAND dump path setting
    wxBoxSizer *nandSizer = new wxBoxSizer(wxHORIZONTAL);
    nandSizer->Add(new wxStaticText(this, wxID_ANY, "NAND Dump:"), 1, wxALIGN_CENTRE | wxRIGHT, size / 8);
    nandPath = new wxTextCtrl(this, wxID_ANY, Settings::nandPath, wxDefaultPosition, wxSize(size * 8, size));
    nandSizer->Add(nandPath);
    nandSizer->Add(new wxButton(this, NAND_BROWSE, "Browse"), 0, wxLEFT, size / 8);

    // Set up the SD image path setting
    wxBoxSizer *sdSizer = new wxBoxSizer(wxHORIZONTAL);
    sdSizer->Add(new wxStaticText(this, wxID_ANY, "SD Image:"), 1, wxALIGN_CENTRE | wxRIGHT, size / 8);
    sdPath = new wxTextCtrl(this, wxID_ANY, Settings::sdPath, wxDefaultPosition, wxSize(size * 8, size));
    sdSizer->Add(sdPath);
    sdSizer->Add(new wxButton(this, SD_BROWSE, "Browse"), 0, wxLEFT, size / 8);

    // Set up the open folder, cancel, and confirm buttons
    wxBoxSizer *naviSizer = new wxBoxSizer(wxHORIZONTAL);
    naviSizer->Add(new wxButton(this, OPEN_FOLDER, "Open Folder"));
    naviSizer->Add(new wxStaticText(this, wxID_ANY, ""), 1);
    naviSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, size / 16);
    naviSizer->Add(new wxButton(this, wxID_OK, "Confirm"), 0, wxLEFT, size / 16);

    // Combine all the contents
    wxBoxSizer *contents = new wxBoxSizer(wxVERTICAL);
    contents->Add(boot11Sizer, 1, wxEXPAND | wxALL, size / 8);
    contents->Add(boot9Sizer, 1, wxEXPAND | wxALL, size / 8);
    contents->Add(nandSizer, 1, wxEXPAND | wxALL, size / 8);
    contents->Add(sdSizer, 1, wxEXPAND | wxALL, size / 8);
    contents->Add(naviSizer, 1, wxEXPAND | wxALL, size / 8);

    // Add a final border around everything
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(contents, 1, wxEXPAND | wxALL, size / 8);
    SetSizerAndFit(sizer);
}

void PathDialog::boot11Browse(wxCommandEvent &event) {
    // Show the Boot11 file browser
    wxFileDialog boot11Select(this, "Select Boot11 ROM File", "", "",
        "Binary files (*.bin)|*.bin", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (boot11Select.ShowModal() == wxID_CANCEL) return;

    // Update the path
    boot11Path->Clear();
    *boot11Path << boot11Select.GetPath();
}

void PathDialog::boot9Browse(wxCommandEvent &event) {
    // Show the Boot9 file browser
    wxFileDialog boot9Select(this, "Select Boot9 ROM File", "", "",
        "Binary files (*.bin)|*.bin", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (boot9Select.ShowModal() == wxID_CANCEL) return;

    // Update the path
    boot9Path->Clear();
    *boot9Path << boot9Select.GetPath();
}

void PathDialog::nandBrowse(wxCommandEvent &event) {
    // Show the NAND file browser
    wxFileDialog nandSelect(this, "Select NAND Dump", "", "",
        "Binary files (*.bin)|*.bin", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (nandSelect.ShowModal() == wxID_CANCEL) return;

    // Update the path
    nandPath->Clear();
    *nandPath << nandSelect.GetPath();
}

void PathDialog::sdBrowse(wxCommandEvent &event) {
    // Show the SD file browser
    wxFileDialog sdImageSelect(this, "Select SD Image File", "", "",
        "Image files (*.img)|*.img", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (sdImageSelect.ShowModal() == wxID_CANCEL) return;

    // Update the path
    sdPath->Clear();
    *sdPath << sdImageSelect.GetPath();
}

void PathDialog::openFolder(wxCommandEvent &event) {
    // Open the folder containing settings and other files
    wxLaunchDefaultApplication(Settings::basePath);
}

void PathDialog::confirm(wxCommandEvent &event) {
    // Update and save the path settings
    Settings::boot11Path = boot11Path->GetValue().ToStdString();
    Settings::boot9Path = boot9Path->GetValue().ToStdString();
    Settings::nandPath = nandPath->GetValue().ToStdString();
    Settings::sdPath = sdPath->GetValue().ToStdString();
    Settings::save();
    event.Skip(true);
}
