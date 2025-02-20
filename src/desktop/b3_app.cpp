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

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include "b3_app.h"

enum AppEvent {
    UPDATE = 1
};

wxBEGIN_EVENT_TABLE(b3App, wxApp)
EVT_TIMER(UPDATE, b3App::update)
wxEND_EVENT_TABLE()

int b3App::keyBinds[] = { 'L', 'K', 'G', 'H', 'D', 'A', 'W', 'S', 'P', 'Q', 'O', 'I' };

bool b3App::OnInit() {
    // Define and add platform-specific settings
    std::vector<Setting> platSettings = {
        Setting("keyA", &keyBinds[0], false),
        Setting("keyB", &keyBinds[1], false),
        Setting("keySelect", &keyBinds[2], false),
        Setting("keyStart", &keyBinds[3], false),
        Setting("keyRight", &keyBinds[4], false),
        Setting("keyLeft", &keyBinds[5], false),
        Setting("keyUp", &keyBinds[6], false),
        Setting("keyDown", &keyBinds[7], false),
        Setting("keyR", &keyBinds[8], false),
        Setting("keyL", &keyBinds[9], false),
        Setting("keyX", &keyBinds[10], false),
        Setting("keyY", &keyBinds[11], false)
    };
    Settings::add(platSettings);

    // Try to load the settings file
    if (FILE *file = fopen("3beans.ini", "r")) {
        // Load from the working directory if a file exists
        fclose(file);
        Settings::load();
    }
    else {
        // Load from the system-specific application settings directory
        std::string settingsDir;
        wxStandardPaths &paths = wxStandardPaths::Get();
#if defined(WINDOWS) || defined(MACOS) || !wxCHECK_VERSION(3, 1, 0)
        settingsDir = paths.GetUserDataDir().mb_str(wxConvUTF8);
#else
        paths.SetFileLayout(wxStandardPaths::FileLayout_XDG);
        settingsDir = paths.GetUserConfigDir().mb_str(wxConvUTF8);
        settingsDir += "/3beans";
#endif
        Settings::load(settingsDir);
    }

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
