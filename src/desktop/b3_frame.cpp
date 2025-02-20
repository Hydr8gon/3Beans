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

#include <thread>
#include "b3_frame.h"
#include "b3_canvas.h"

enum FrameEvent {
    PAUSE = 1,
    RESTART,
    STOP
};

wxBEGIN_EVENT_TABLE(b3Frame, wxFrame)
EVT_MENU(PAUSE, b3Frame::pause)
EVT_MENU(RESTART, b3Frame::restart)
EVT_MENU(STOP, b3Frame::stop)
EVT_CLOSE(b3Frame::close)
wxEND_EVENT_TABLE()

b3Frame::b3Frame(): wxFrame(nullptr, wxID_ANY, "3Beans") {
    // Set up the System menu
    systemMenu = new wxMenu();
    systemMenu->Append(PAUSE, "&Pause");
    systemMenu->Append(RESTART, "&Restart");
    systemMenu->Append(STOP, "&Stop");

    // Set up the menu bar
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(systemMenu, "&System");
    SetMenuBar(menuBar);

    // Start emulation automatically
    running.store(false);
    startCore(true);

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
}

void b3Frame::Refresh() {
    // Override the refresh function to also update the FPS counter
    wxFrame::Refresh();
    wxString label = "3Beans";
    mutex.lock();
    if (running.load())
        label += wxString::Format(" - %d FPS", core->fps);
    mutex.unlock();
    SetLabel(label);
}

void b3Frame::runCore() {
    // Run the emulator until stopped
    while (running.load())
        core->runFrame();
}

void b3Frame::startCore(bool full) {
    // Fully stop and restart the core
    if (full) {
        stopCore(true);
        mutex.lock();
        core = new Core();
        mutex.unlock();
    }

    // Start the core thread if not already running
    if (running.load()) return;
    running.store(true);
    thread = new std::thread(&b3Frame::runCore, this);

    // Update the system menu for running
    systemMenu->SetLabel(PAUSE, "&Pause");
    systemMenu->SetLabel(RESTART, "&Restart");
    systemMenu->Enable(PAUSE, true);
    systemMenu->Enable(STOP, true);
}

void b3Frame::stopCore(bool full) {
    // Stop the core thread if it was running
    if (running.load()) {
        running.store(false);
        thread->join();
        delete thread;
    }

    // Update the system menu for pausing or stopping
    systemMenu->SetLabel(PAUSE, "&Resume");
    if (!full) return;
    systemMenu->SetLabel(RESTART, "&Start");
    systemMenu->Enable(PAUSE, false);
    systemMenu->Enable(STOP, false);

    // Fully stop and remove the core
    mutex.lock();
    if (core) {
        delete core;
        core = nullptr;
    }
    mutex.unlock();
}

void b3Frame::pause(wxCommandEvent &event) {
    // Pause or resume the core
    running.load() ? stopCore(false) : startCore(false);
}

void b3Frame::restart(wxCommandEvent &event) {
    // Restart the core
    startCore(true);
}

void b3Frame::stop(wxCommandEvent &event) {
    // Stop the core
    stopCore(true);
}

void b3Frame::close(wxCloseEvent &event) {
    // Stop the canvas and the core
    canvas->finish();
    stopCore(true);
    event.Skip(true);
}
