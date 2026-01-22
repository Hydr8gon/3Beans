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
#include <wx/joystick.h>

#include "../core/core.h"

#define MIN_SIZE wxSize(400, 480)

class b3Frame: public wxFrame {
public:
    Core *core = nullptr;
    std::atomic<bool> running{false};
    std::mutex mutex;

    b3Frame();
    void Refresh();

    uint32_t *getFrame();
    void pressKey(int key);
    void releaseKey(int key);
    void pressScreen(int x, int y);
    void releaseScreen();

private:
    wxMenu *fileMenu, *systemMenu, *shaderMenu;
    wxWindow *canvas;
    wxJoystick *joystick;
    wxTimer *timer;

    std::thread *thread;
    std::string cartPath;
    std::vector<int> axisBases;
    bool stickKeys[5] = {};

    std::chrono::steady_clock::time_point lastRateTime;
    int frameCount = 0;
    int swapInterval = 0;
    int refreshRate = 0;
    bool glSupport = true;

    void runCore();
    void startCore(bool full);
    void stopCore(bool full);
    void updateKeyStick();

    void insertCart(wxCommandEvent &event);
    void ejectCart(wxCommandEvent &event);
    void quit(wxCommandEvent &event);
    void pause(wxCommandEvent &event);
    void restart(wxCommandEvent &event);
    void stop(wxCommandEvent &event);
    void fpsLimiter(wxCommandEvent &event);
    void cartAutoBoot(wxCommandEvent &event);
    void threadedGpu(wxCommandEvent &event);
    template <int i> void gpuRenderer(wxCommandEvent &event);
    template <int i> void gpuShader(wxCommandEvent &event);
    void pathSettings(wxCommandEvent &event);
    void inputBindings(wxCommandEvent &event);
    void updateJoystick(wxTimerEvent &event);
    void close(wxCloseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
