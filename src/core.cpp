/*
    Copyright 2023-2024 Hydr8gon

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

Core::Core(): aes(this), arms { ArmInterp(this, ARM11A), ArmInterp(this, ARM11B),
        ArmInterp(this, ARM9) }, i2c(this), interrupts(this), memory(this), pdc(this),
        pxi(this), rsa(this), sdMmc(this), shas { Sha(this), Sha(this) }, timers(this) {
    // Initialize memory and the CPUs
    memory.loadFiles();
    sdMmc.loadFiles();
    for (int i = 0; i < MAX_CPUS; i++)
        arms[i].init();

    // Define the tasks that can be scheduled
    tasks[RESET_CYCLES] = std::bind(&Core::resetCycles, this);
    tasks[END_FRAME] = std::bind(&Core::endFrame, this);
    tasks[ARM11A_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11A);
    tasks[ARM11B_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11B);
    tasks[ARM9_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM9);
    tasks[TIMER0_OVERFLOW] = std::bind(&Timers::overflow, &timers, 0);
    tasks[TIMER1_OVERFLOW] = std::bind(&Timers::overflow, &timers, 1);
    tasks[TIMER2_OVERFLOW] = std::bind(&Timers::overflow, &timers, 2);
    tasks[TIMER3_OVERFLOW] = std::bind(&Timers::overflow, &timers, 3);

    // Schedule the initial tasks
    schedule(RESET_CYCLES, 0x7FFFFFFF);
    schedule(END_FRAME, 268111856 / 60);
}

void Core::resetCycles() {
    // Reset the global cycle count periodically to prevent overflow
    for (uint32_t i = 0; i < events.size(); i++)
        events[i].cycles -= globalCycles;
    for (int i = 0; i < MAX_CPUS; i++)
        arms[i].resetCycles();
    timers.resetCycles();
    globalCycles -= globalCycles;
    schedule(RESET_CYCLES, 0x7FFFFFFF);
}

void Core::endFrame() {
    // Break execution at the end of a frame
    pdc.drawFrame();
    running.store(false);
    schedule(END_FRAME, 268111856 / 60);
}

void Core::schedule(Task task, uint32_t cycles) {
    // Add a task to the scheduler, sorted by least to most cycles until execution
    Event event(&tasks[task], globalCycles + cycles);
    auto it = std::upper_bound(events.cbegin(), events.cend(), event);
    events.insert(it, event);
}
