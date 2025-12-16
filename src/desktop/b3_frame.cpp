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

#include "b3_frame.h"
#include "b3_canvas_ogl.h"
#include "b3_canvas_soft.h"
#include "input_dialog.h"
#include "path_dialog.h"

enum FrameEvent {
    INSERT_CART = 1,
    EJECT_CART,
    QUIT,
    PAUSE,
    RESTART,
    STOP,
    FPS_LIMITER,
    CART_AUTO_BOOT,
    THREADED_GPU,
    GPU_RENDER_SOFT,
    GPU_RENDER_OGL,
    PATH_SETTINGS,
    INPUT_BINDINGS,
    UPDATE_JOYSTICK
};

wxBEGIN_EVENT_TABLE(b3Frame, wxFrame)
EVT_MENU(INSERT_CART, b3Frame::insertCart)
EVT_MENU(EJECT_CART, b3Frame::ejectCart)
EVT_MENU(QUIT, b3Frame::quit)
EVT_MENU(PAUSE, b3Frame::pause)
EVT_MENU(RESTART, b3Frame::restart)
EVT_MENU(STOP, b3Frame::stop)
EVT_MENU(FPS_LIMITER, b3Frame::fpsLimiter)
EVT_MENU(CART_AUTO_BOOT, b3Frame::cartAutoBoot)
EVT_MENU(THREADED_GPU, b3Frame::threadedGpu)
EVT_MENU(GPU_RENDER_SOFT, b3Frame::gpuRenderer<0>)
EVT_MENU(GPU_RENDER_OGL, b3Frame::gpuRenderer<1>)
EVT_MENU(PATH_SETTINGS, b3Frame::pathSettings)
EVT_MENU(INPUT_BINDINGS, b3Frame::inputBindings)
EVT_TIMER(UPDATE_JOYSTICK, b3Frame::updateJoystick)
EVT_CLOSE(b3Frame::close)
wxEND_EVENT_TABLE()

b3Frame::b3Frame(): wxFrame(nullptr, wxID_ANY, "3Beans") {
    // Set up the file menu
    fileMenu = new wxMenu();
    fileMenu->Append(INSERT_CART, "&Insert Cart ROM");
    fileMenu->Append(EJECT_CART, "&Eject Cart ROM");
    fileMenu->AppendSeparator();
    fileMenu->Append(QUIT, "&Quit");
    fileMenu->Enable(EJECT_CART, false);

    // Set up the system menu
    systemMenu = new wxMenu();
    systemMenu->Append(PAUSE, "&Pause");
    systemMenu->Append(RESTART, "&Restart");
    systemMenu->Append(STOP, "&Stop");

    // Set up the renderer submenu
    wxMenu *renderMenu = new wxMenu();
    renderMenu->AppendRadioItem(GPU_RENDER_SOFT, "&Software");
    renderMenu->AppendRadioItem(GPU_RENDER_OGL, "&OpenGL");

    // Set up the settings menu
    wxMenu *settingsMenu = new wxMenu();
    settingsMenu->AppendCheckItem(FPS_LIMITER, "&FPS Limiter");
    settingsMenu->AppendCheckItem(CART_AUTO_BOOT, "&Cart Auto-Boot");
    settingsMenu->AppendSeparator();
    settingsMenu->AppendCheckItem(THREADED_GPU, "&Threaded GPU");
    settingsMenu->AppendSubMenu(renderMenu, "&GPU Renderer");
    settingsMenu->AppendSeparator();
    settingsMenu->Append(PATH_SETTINGS, "&Path Settings");
    settingsMenu->Append(INPUT_BINDINGS, "&Input Bindings");

    // Set up the menu bar
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(systemMenu, "&System");
    menuBar->Append(settingsMenu, "&Settings");
    SetMenuBar(menuBar);

    // Set up and show the window
    stopCore(true);
    SetClientSize(MIN_SIZE);
    SetBackgroundColour(*wxBLACK);
    Centre();
    Show(true);

    // Create a canvas for drawing the screens
    try {
        // Use an OpenGL canvas if supported
        canvas = new b3CanvasOgl(this);
    }
    catch (CanvasError e) {
        // Fall back to software and disable OpenGL
        canvas = new b3CanvasSoft(this);
        renderMenu->Enable(GPU_RENDER_OGL, false);
        Settings::gpuRenderer = 0;
        glSupport = false;
    }

    // Add the canvas to the frame
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(canvas, 1, wxEXPAND);
    SetSizer(sizer);

    // Set the initial setting states
    settingsMenu->Check(FPS_LIMITER, Settings::fpsLimiter);
    settingsMenu->Check(CART_AUTO_BOOT, Settings::cartAutoBoot);
    settingsMenu->Check(THREADED_GPU, Settings::threadedGpu);
    renderMenu->Check(GPU_RENDER_SOFT + std::min(Settings::gpuRenderer, 1), true);

    // Prepare a joystick if one is connected
    joystick = new wxJoystick();
    if (joystick->IsOk()) {
        // Save initial axis values so inputs can be detected as offsets from resting
        for (int i = 0; i < joystick->GetNumberAxes(); i++)
            axisBases.push_back(joystick->GetPosition(i));

        // Start a timer to update joystick input, since wxJoystickEvents are unreliable
        timer = new wxTimer(this, UPDATE_JOYSTICK);
        timer->Start(10);
    }
    else {
        // Don't use a joystick if one isn't connected
        delete joystick;
        joystick = nullptr;
        timer = nullptr;
    }
}

void b3Frame::Refresh() {
    // Override the refresh function and enforce minimum frame size
    wxFrame::Refresh();
    SetMinClientSize(MIN_SIZE);

    // Display current FPS in the title bar if running
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
    // Fully stop and restart the core, or handle errors
    if (full) {
        stopCore(true);
        try {
            mutex.lock();
            core = new Core(cartPath, glSupport ? &((b3CanvasOgl*)canvas)->contextFunc : nullptr);
            mutex.unlock();
        }
        catch (CoreError e) {
            core = nullptr;
            mutex.unlock();
            wxMessageDialog(this, "One of the boot ROMs is missing! Check the path settings to configure them. "
                "You probably also want a NAND dump from GodMode9 and a FAT-formatted SD image file.",
                "Boot ROMs Missing", wxICON_NONE).ShowModal();
            return;
        }
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

uint32_t *b3Frame::getFrame() {
    // Track refresh rate and update the swap interval every second
    refreshRate++;
    std::chrono::duration<double> rateTime = std::chrono::steady_clock::now() - lastRateTime;
    if (rateTime.count() >= 1.0f) {
        swapInterval = (refreshRate + 5) / 60; // Margin of 5
        refreshRate = 0;
        lastRateTime = std::chrono::steady_clock::now();
    }

    // Wait until the swap interval is reached
    if (++frameCount < swapInterval)
        return nullptr;

    // Get a new frame from the core, or make an empty one if inactive
    uint32_t *frame;
    mutex.lock();
    if (core) {
        frame = core->pdc.getFrame();
        mutex.unlock();
    }
    else {
        mutex.unlock();
        frame = new uint32_t[400 * 480];
        memset(frame, 0, 400 * 480 * sizeof(uint32_t));
    }
    frameCount = 0;
    return frame;
}

void b3Frame::pressKey(int key) {
    // Handle a key press based on its type
    mutex.lock();
    if (core) {
        if (key < 12)
            core->input.pressKey(key);
        else if (key < 17)
            stickKeys[key - 12] = true, updateKeyStick();
        else
            core->input.pressHome();
    }
    mutex.unlock();
}

void b3Frame::releaseKey(int key) {
    // Handle a key release based on its type
    mutex.lock();
    if (core) {
        if (key < 12)
            core->input.releaseKey(key);
        else if (key < 17)
            stickKeys[key - 12] = false, updateKeyStick();
        else
            core->input.releaseHome();
    }
    mutex.unlock();
}

void b3Frame::updateKeyStick() {
    // Apply the base stick movement from pressed keys
    int stickX = 0, stickY = 0;
    if (stickKeys[0]) stickX -= 0x7FF;
    if (stickKeys[1]) stickX += 0x7FF;
    if (stickKeys[2]) stickY += 0x7FF;
    if (stickKeys[3]) stickY -= 0x7FF;

    // Scale diagonals to create a round boundary
    if (stickX && stickY) {
        stickX = stickX * 0x5FF / 0x7FF;
        stickY = stickY * 0x5FF / 0x7FF;
    }

    // Halve coordinates if the modifier is active
    if (stickKeys[4]) {
        stickX /= 2;
        stickY /= 2;
    }

    // Send key-stick coordinates to the core
    core->input.setLStick(stickX, stickY);
}

void b3Frame::pressScreen(int x, int y) {
    // Send a screen press to the core
    mutex.lock();
    if (core) core->input.pressScreen(x, y);
    mutex.unlock();
}

void b3Frame::releaseScreen() {
    // Send a screen release to the core
    mutex.lock();
    if (core) core->input.releaseScreen();
    mutex.unlock();
}

void b3Frame::insertCart(wxCommandEvent &event) {
    // Open a file browser for cartridge ROMs
    wxFileDialog romSelect(this, "Select Cart ROM", "", "",
        "3DS Cart ROMs (*.3ds, *.cci)|*.3ds;*.cci", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (romSelect.ShowModal() == wxID_CANCEL) return;

    // Set the cartridge path and start or restart the core
    cartPath = (const char*)romSelect.GetPath().mb_str(wxConvUTF8);
    startCore(true);
    fileMenu->Enable(EJECT_CART, true);
}

void b3Frame::ejectCart(wxCommandEvent &event) {
    // Clear the cartridge path and restart the core if started
    cartPath = "";
    if (core) startCore(true);
    fileMenu->Enable(EJECT_CART, false);
}

void b3Frame::quit(wxCommandEvent &event) {
    // Close the program
    Close(true);
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

void b3Frame::fpsLimiter(wxCommandEvent &event) {
    // Toggle the FPS limiter setting
    Settings::fpsLimiter = !Settings::fpsLimiter;
    Settings::save();
}

void b3Frame::cartAutoBoot(wxCommandEvent &event) {
    // Toggle the cart auto-boot setting
    Settings::cartAutoBoot = !Settings::cartAutoBoot;
    Settings::save();
}

void b3Frame::threadedGpu(wxCommandEvent &event) {
    // Toggle the threaded GPU setting
    Settings::threadedGpu = !Settings::threadedGpu;
    Settings::save();
}

template <int i> void b3Frame::gpuRenderer(wxCommandEvent &event) {
    // Set the GPU renderer to a specific value
    Settings::gpuRenderer = i;
    Settings::save();
}

void b3Frame::pathSettings(wxCommandEvent &event) {
    // Show the path settings dialog
    PathDialog pathDialog;
    pathDialog.ShowModal();
}

void b3Frame::inputBindings(wxCommandEvent &event) {
    // Pause joystick updates and show the input bindings dialog
    if (timer) timer->Stop();
    InputDialog inputDialog(joystick);
    inputDialog.ShowModal();
    if (timer) timer->Start(10);
}

void b3Frame::updateJoystick(wxTimerEvent &event) {
    // Check the status of mapped joystick inputs
    int stickX = 0, stickY = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
        if (b3App::keyBinds[i] >= 3000 && joystick->GetNumberAxes() > b3App::keyBinds[i] - 3000) { // Axis -
            int j = b3App::keyBinds[i] - 3000;
            switch (i) {
            case 12: // Stick Right
                // Scale the axis position and apply it to the stick in the right direction
                if (joystick->GetPosition(j) < axisBases[j])
                    stickX -= (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetXMax();
                continue;

            case 13: // Stick Left
                // Scale the axis position and apply it to the stick in the left direction
                if (joystick->GetPosition(j) < axisBases[j])
                    stickX += (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetXMin();
                continue;

            case 14: // Stick Up
                // Scale the axis position and apply it to the stick in the up direction
                if (joystick->GetPosition(j) < axisBases[j])
                    stickY += (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetYMin();
                continue;

            case 15: // Stick Down
                // Scale the axis position and apply it to the stick in the down direction
                if (joystick->GetPosition(j) < axisBases[j])
                    stickY -= (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetYMax();
                continue;

            default:
                // Trigger a key press or release based on the axis position
                if (joystick->GetPosition(j) < axisBases[j] - joystick->GetXMax() / 2)
                    pressKey(i);
                else
                    releaseKey(i);
                continue;
            }
        }
        else if (b3App::keyBinds[i] >= 2000 && joystick->GetNumberAxes() > b3App::keyBinds[i] - 2000) { // Axis +
            int j = b3App::keyBinds[i] - 2000;
            switch (i) {
            case 12: // Stick Right
                // Scale the axis position and apply it to the stick in the right direction
                if (joystick->GetPosition(j) > axisBases[j])
                    stickX -= (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetXMax();
                continue;

            case 13: // Stick Left
                // Scale the axis position and apply it to the stick in the left direction
                if (joystick->GetPosition(j) > axisBases[j])
                    stickX += (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetXMin();
                continue;

            case 14: // Stick Up
                // Scale the axis position and apply it to the stick in the up direction
                if (joystick->GetPosition(j) > axisBases[j])
                    stickY += (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetYMin();
                continue;

            case 15: // Stick Down
                // Scale the axis position and apply it to the stick in the down direction
                if (joystick->GetPosition(j) > axisBases[j])
                    stickY -= (joystick->GetPosition(j) - axisBases[j]) * 0x7FF / joystick->GetYMax();
                continue;

            default:
                // Trigger a key press or release based on the axis position
                if (joystick->GetPosition(j) > axisBases[j] + joystick->GetXMax() / 2)
                    pressKey(i);
                else
                    releaseKey(i);
                continue;
            }
        }
        else if (b3App::keyBinds[i] >= 1000 && joystick->GetNumberButtons() > b3App::keyBinds[i] - 1000) { // Button
            // Trigger a key press or release based on the button status
            if (joystick->GetButtonState(b3App::keyBinds[i] - 1000))
                pressKey(i);
            else
                releaseKey(i);
        }
    }

    // Halve stick coordinates if the modifier is active
    if (stickKeys[4]) {
        stickX /= 2;
        stickY /= 2;
    }

    // Send coordinates to the core if key-stick is inactive
    mutex.lock();
    if (core && !stickKeys[0] && !stickKeys[1] && !stickKeys[2] && !stickKeys[3])
        core->input.setLStick(stickX, stickY);
    mutex.unlock();
}

void b3Frame::close(wxCloseEvent &event) {
    // Clean up the joystick if used
    if (joystick) {
        timer->Stop();
        delete joystick;
    }

    // Stop the core
    stopCore(true);
    event.Skip(true);
}
