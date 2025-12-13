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

#include <wx/rawbmp.h>
#include "b3_canvas_soft.h"
#include "b3_app.h"

wxBEGIN_EVENT_TABLE(b3CanvasSoft, wxPanel)
EVT_PAINT(b3CanvasSoft::draw)
EVT_SIZE(b3CanvasSoft::resize)
EVT_KEY_DOWN(b3CanvasSoft::pressKey)
EVT_KEY_UP(b3CanvasSoft::releaseKey)
EVT_LEFT_DOWN(b3CanvasSoft::pressScreen)
EVT_MOTION(b3CanvasSoft::pressScreen)
EVT_LEFT_UP(b3CanvasSoft::releaseScreen)
wxEND_EVENT_TABLE()

b3CanvasSoft::b3CanvasSoft(b3Frame *frame): wxPanel(frame), frame(frame) {
    // Initialize data and set focus for key presses
    bitmap = wxBitmap(400, 480, 24);
    frame->SendSizeEvent();
    SetFocus();
}

void b3CanvasSoft::draw(wxPaintEvent &event) {
    // Update bitmap data with a new framebuffer if one is available
    if (uint32_t *fb = frame->getFrame()) {
        wxNativePixelData data(bitmap);
        wxNativePixelData::Iterator iter(data);
        for (int y = 0; y < 480; y++) {
            wxNativePixelData::Iterator pixel = iter;
            for (int x = 0; x < 400; x++, pixel++) {
                uint32_t color = fb[y * 400 + x];
                pixel.Red() = ((color >> 0) & 0xFF);
                pixel.Green() = ((color >> 8) & 0xFF);
                pixel.Blue() = ((color >> 16) & 0xFF);
            }
            iter.OffsetY(data, 1);
        }
        delete[] fb;
    }

    // Scale the bitmap and draw it
    wxPaintDC dc(this);
    wxImage image = bitmap.ConvertToImage();
    image.Rescale(scrW, scrH, wxIMAGE_QUALITY_BILINEAR);
    wxBitmap scaled = wxBitmap(image);
    dc.DrawBitmap(scaled, wxPoint(scrX, scrY));
}

void b3CanvasSoft::resize(wxSizeEvent &event) {
    // Set the layout to be centered and as large as possible
    wxSize size = GetSize();
    if ((float(size.x) / size.y) > (400.0f / 480)) { // Wide
        scrW = (400.0f * size.y) / 480;
        scrH = size.y;
        scrX = (size.x - scrW) / 2;
        scrY = 0;
    }
    else { // Tall
        scrW = size.x;
        scrH = 480 * size.x / 400;
        scrX = 0;
        scrY = (size.y - scrH) / 2;
    }
}

void b3CanvasSoft::pressKey(wxKeyEvent &event) {
    // Trigger a key press if a mapped key was pressed
    for (int i = 0; i < MAX_KEYS; i++)
        if (event.GetKeyCode() == b3App::keyBinds[i])
            return frame->pressKey(i);
}

void b3CanvasSoft::releaseKey(wxKeyEvent &event) {
    // Trigger a key release if a mapped key was released
    for (int i = 0; i < MAX_KEYS; i++)
        if (event.GetKeyCode() == b3App::keyBinds[i])
            return frame->releaseKey(i);
}

void b3CanvasSoft::pressScreen(wxMouseEvent &event) {
    // Trigger a screen touch relative to the bottom screen if clicked
    if (!event.LeftIsDown()) return;
    int x = (event.GetX() - scrX) * 400 / scrW - 40;
    int y = (event.GetY() - scrY) * 480 / scrH - 240;
    frame->pressScreen(x, y);
}

void b3CanvasSoft::releaseScreen(wxMouseEvent &event) {
    // Trigger a screen release
    frame->releaseScreen();
}
