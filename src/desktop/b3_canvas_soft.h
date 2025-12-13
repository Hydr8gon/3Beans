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

class b3Frame;

class b3CanvasSoft: public wxPanel {
public:
    b3CanvasSoft(b3Frame *frame);

private:
    b3Frame *frame;
    wxBitmap bitmap;

    int scrW = 0, scrH = 0;
    int scrX = 0, scrY = 0;

    void draw(wxPaintEvent &event);
    void resize(wxSizeEvent &event);
    void pressKey(wxKeyEvent &event);
    void releaseKey(wxKeyEvent &event);
    void pressScreen(wxMouseEvent &event);
    void releaseScreen(wxMouseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
