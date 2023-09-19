/*
    Copyright 2023 Hydr8gon

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

#include <thread>
#include "b3_frame.h"
#include "b3_canvas.h"

wxBEGIN_EVENT_TABLE(b3Frame, wxFrame)
EVT_CLOSE(b3Frame::close)
wxEND_EVENT_TABLE()

b3Frame::b3Frame(): wxFrame(nullptr, wxID_ANY, "3Beans")
{
    // Set up and show the window
    SetClientSize(MIN_SIZE);
    SetMinClientSize(MIN_SIZE);
    Centre();
    Show(true);

    // Set up a canvas for drawing the framebuffer
    canvas = new b3Canvas(this);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(canvas, 1, wxEXPAND);
    SetSizer(sizer);

    // Start running the emulator on a separate thread
    core = new Core();
    std::thread *thread = new std::thread(&b3Frame::runCore, this);
}

void b3Frame::runCore()
{
    // Run the emulator
    while (core)
        core->runFrame();
}

void b3Frame::close(wxCloseEvent &event)
{
    // Tell the canvas to stop drawing
    canvas->finish();
    event.Skip(true);
}
