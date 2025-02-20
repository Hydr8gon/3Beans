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

#include <wx/wx.h>

class PathDialog: public wxDialog {
public:
    PathDialog();

private:
    wxTextCtrl *boot11Path;
    wxTextCtrl *boot9Path;
    wxTextCtrl *nandPath;
    wxTextCtrl *sdPath;

    void boot11Browse(wxCommandEvent &event);
    void boot9Browse(wxCommandEvent &event);
    void nandBrowse(wxCommandEvent &event);
    void sdBrowse(wxCommandEvent &event);
    void openFolder(wxCommandEvent &event);
    void confirm(wxCommandEvent &event);
    wxDECLARE_EVENT_TABLE();
};
