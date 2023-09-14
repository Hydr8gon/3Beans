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

#include <wx/wx.h>
#include "../core.h"

class b3App: public wxApp
{
    private:
        bool OnInit();
};

bool b3App::OnInit()
{
    // Print the first boot ROM opcodes to ensure memory works
    Core *core = new Core();
    uint32_t opcode11 = core->memory.read<uint32_t>(false, 0xFFFF0000);
    uint32_t opcode9 = core->memory.read<uint32_t>(true, 0xFFFF0000);
    printf("First opcodes: 0x%08X, 0x%08X\n", opcode11, opcode9);

    // Make a useless window because there's nothing else to show :)
    SetAppName("3Beans");
    wxFrame *frame = new wxFrame(nullptr, wxID_ANY, "3Beans");
    frame->Show(true);
    return true;
}

// Let wxWidgets handle the main function
wxIMPLEMENT_APP(b3App);
