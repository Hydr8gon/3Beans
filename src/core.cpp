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

#include <algorithm>
#include "core.h"

Core::Core(): aes(this), cpus { Interpreter(this, ARM11A), Interpreter(this, ARM11B), Interpreter(this, ARM9) },
    gpu(this), interrupts(this), memory(this), pxi(this), sdMmc(this), shas { Sha(this), Sha(this) }
{
    // Initialize memory and the CPUs
    memory.loadFiles();
    for (int i = 0; i < MAX_CPUS; i++)
        cpus[i].init();

    // Define the tasks that can be scheduled
    tasks[RESET_CYCLES] = std::bind(&Core::resetCycles, this);
    tasks[END_FRAME] = std::bind(&Core::endFrame, this);

    // Schedule the initial tasks
    schedule(RESET_CYCLES, 0x7FFFFFFF);
    schedule(END_FRAME, 268111856 / 60);
}

void Core::resetCycles()
{
    // Reset the global cycle count periodically to prevent overflow
    for (size_t i = 0; i < events.size(); i++)
        events[i].cycles -= globalCycles;
    for (int i = 0; i < MAX_CPUS; i++)
        cpus[i].resetCycles();
    globalCycles -= globalCycles;
    schedule(RESET_CYCLES, 0x7FFFFFFF);
}

void Core::endFrame()
{
    // Break execution at the end of a frame
    gpu.drawFrame();
    running.store(false);
    schedule(END_FRAME, 268111856 / 60);
}

void Core::schedule(Task task, uint32_t cycles)
{
    // Add a task to the scheduler, sorted by least to most cycles until execution
    Event event(&tasks[task], globalCycles + cycles);
    auto it = std::upper_bound(events.cbegin(), events.cend(), event);
    events.insert(it, event);
}
