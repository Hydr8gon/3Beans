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

#include "b3_app.h"

enum AppEvent {
    UPDATE = 1
};

wxBEGIN_EVENT_TABLE(b3App, wxApp)
EVT_TIMER(UPDATE, b3App::update)
wxEND_EVENT_TABLE()

int b3App::keyBinds[] = { 'L', 'K', 'G', 'H', 'D', 'A', 'W', 'S', 'P', 'Q', 'O', 'I' };

bool b3App::OnInit() {
    // Create the program's frame
    SetAppName("3Beans");
    frame = new b3Frame();

    // Set up the update timer
    timer = new wxTimer(this, UPDATE);
    timer->Start(6);
    return true;
}

void b3App::update(wxTimerEvent &event) {
    // Continuously refresh the frame
    frame->Refresh();
}
