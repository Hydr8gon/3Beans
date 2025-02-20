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

#include <mutex>
#include <thread>
#include <wx/wx.h>
#include "../core.h"

#define MIN_SIZE wxSize(400, 480)
class b3Canvas;

class b3Frame: public wxFrame {
public:
    Core *core = nullptr;
    std::atomic<bool> running;
    std::mutex mutex;

    b3Frame();
    void Refresh();

private:
    b3Canvas *canvas;
    wxMenu *systemMenu;
    std::thread *thread;

    void runCore();
    void startCore(bool full);
    void stopCore(bool full);

    void pause(wxCommandEvent &event);
    void restart(wxCommandEvent &event);
    void stop(wxCommandEvent &event);
    void close(wxCloseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
