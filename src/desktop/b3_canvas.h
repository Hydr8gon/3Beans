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

#include <chrono>
#include <wx/wx.h>
#include <wx/glcanvas.h>

class b3Frame;

class b3Canvas: public wxGLCanvas {
public:
    b3Canvas(b3Frame *frame);
    void finish();

private:
    b3Frame *frame;
    wxGLContext *context;
    bool stickKeys[5] = {};

    std::chrono::steady_clock::time_point lastRateTime;
    int frameCount = 0;
    int swapInterval = 0;
    int refreshRate = 0;
    bool finished = false;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t x = 0;
    uint32_t y = 0;

    void updateKeyStick();

    void draw(wxPaintEvent &event);
    void resize(wxSizeEvent &event);
    void pressKey(wxKeyEvent &event);
    void releaseKey(wxKeyEvent &event);
    void pressScreen(wxMouseEvent &event);
    void releaseScreen(wxMouseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
