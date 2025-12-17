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
#include <wx/glcanvas.h>

class b3Frame;

enum CanvasError {
    OPENGL_FAIL
};

struct CanvasVtx {
    float x, y;
    float s, t;
};

class b3CanvasOgl: public wxGLCanvas {
public:
    b3CanvasOgl(b3Frame *frame);
    std::function<void()> contextFunc;

private:
    b3Frame *frame;
    wxGLContext *contexts[2];
    wxGLCanvas *coreCanvas;
    GLint winSizeLoc;

    static const char *vtxCode;
    static const char *fragCode;

    bool inited = false;
    bool toggle = false;

    float scrW = 0, scrH = 0;
    float scrX = 0, scrY = 0;

    void coreContext();
    void glInit();

    void draw(wxPaintEvent &event);
    void resize(wxSizeEvent &event);
    void pressKey(wxKeyEvent &event);
    void releaseKey(wxKeyEvent &event);
    void pressScreen(wxMouseEvent &event);
    void releaseScreen(wxMouseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
