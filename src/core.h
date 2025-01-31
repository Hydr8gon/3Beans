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

#include <atomic>
#include <functional>
#include <vector>

#include "aes.h"
#include "arm_interp.h"
#include "cp15.h"
#include "defines.h"
#include "gpu.h"
#include "i2c.h"
#include "input.h"
#include "interrupts.h"
#include "memory.h"
#include "pdc.h"
#include "pxi.h"
#include "rsa.h"
#include "sd_mmc.h"
#include "sha.h"
#include "timers.h"

enum Task {
    RESET_CYCLES,
    END_FRAME,
    ARM11A_INTERRUPT,
    ARM11B_INTERRUPT,
    ARM9_INTERRUPT,
    TIMER0_OVERFLOW,
    TIMER1_OVERFLOW,
    TIMER2_OVERFLOW,
    TIMER3_OVERFLOW,
    MAX_TASKS
};

struct Event {
    std::function<void()> *task;
    uint32_t cycles;

    Event(std::function<void()> *task, uint32_t cycles): task(task), cycles(cycles) {}
    bool operator<(const Event &event) const { return cycles < event.cycles; }
};

class Core {
public:
    Aes aes;
    ArmInterp arms[MAX_CPUS];
    Gpu gpu;
    I2c i2c;
    Input input;
    Interrupts interrupts;
    Memory memory;
    Pdc pdc;
    Pxi pxi;
    Rsa rsa;
    SdMmc sdMmc;
    Sha shas[2];
    Timers timers;

    std::atomic<bool> running;
    std::vector<Event> events;
    uint32_t globalCycles = 0;

    Core();
    void runFrame() { ArmInterp::runFrame(this); }
    void schedule(Task task, uint32_t cycles);

private:
    std::function<void()> tasks[MAX_TASKS];

    void resetCycles();
    void endFrame();
};
