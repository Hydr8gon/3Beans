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
#include <chrono>
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
#include "ndma.h"
#include "pdc.h"
#include "pxi.h"
#include "rsa.h"
#include "sd_mmc.h"
#include "sha.h"
#include "timers.h"

enum Task {
    RESET_CYCLES,
    END_FRAME,
    TOGGLE_RUN_FUNC,
    ARM11A_INTERRUPT,
    ARM11B_INTERRUPT,
    ARM11C_INTERRUPT,
    ARM11D_INTERRUPT,
    ARM9_INTERRUPT,
    ARM11A_OVERFLOW0,
    ARM11A_OVERFLOW1,
    ARM11B_OVERFLOW0,
    ARM11B_OVERFLOW1,
    ARM11C_OVERFLOW0,
    ARM11C_OVERFLOW1,
    ARM11D_OVERFLOW0,
    ARM11D_OVERFLOW1,
    ARM9_OVERFLOW0,
    ARM9_OVERFLOW1,
    ARM9_OVERFLOW2,
    ARM9_OVERFLOW3,
    AES_UPDATE,
    NDMA_UPDATE,
    SHA11_UPDATE,
    SHA9_UPDATE,
    SDMMC0_READ_BLOCK,
    SDMMC1_READ_BLOCK,
    MAX_TASKS
};

struct Event {
    std::function<void()> *task;
    uint64_t cycles;

    Event(std::function<void()> *task, uint64_t cycles): task(task), cycles(cycles) {}
    bool operator<(const Event &event) const { return cycles < event.cycles; }
};

class Core {
public:
    int fps = 0;
    bool n3dsMode = false;

    Aes aes;
    ArmInterp arms[MAX_CPUS];
    Cp15 cp15;
    Gpu gpu;
    I2c i2c;
    Input input;
    Interrupts interrupts;
    Memory memory;
    Ndma ndma;
    Pdc pdc;
    Pxi pxi;
    Rsa rsa;
    SdMmc sdMmcs[2];
    Sha shas[2];
    Timers timers;

    std::atomic<bool> running;
    std::vector<Event> events;
    uint64_t globalCycles = 0;

    Core();
    void runFrame() { (*runFunc)(this); }
    void schedule(Task task, uint64_t cycles);

private:
    void (*runFunc)(Core*) = &ArmInterp::runFrame<false>;
    std::function<void()> tasks[MAX_TASKS];
    std::chrono::steady_clock::time_point lastFpsTime;
    int fpsCount = 0;

    void resetCycles();
    void endFrame();
    void toggleRunFunc();
};
