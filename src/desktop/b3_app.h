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

#include <portaudio.h>
#include "b3_frame.h"

#define MAX_KEYS 17

class b3App: public wxApp {
public:
    static int keyBinds[MAX_KEYS];

private:
    b3Frame *frame;
    wxTimer *timer;
    PaStream *stream;

    bool OnInit();
    int OnExit();

    static int audioCallback(const void *in, void *out, unsigned long count,
        const PaStreamCallbackTimeInfo *info, PaStreamCallbackFlags flags, void *data);

    void update(wxTimerEvent &event);
    wxDECLARE_EVENT_TABLE();
};
