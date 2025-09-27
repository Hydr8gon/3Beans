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
#include "cartridge.h"
#include "cdma.h"
#include "cp15.h"
#include "csnd.h"
#include "defines.h"
#include "dsp.h"
#include "gpu.h"
#include "gpu_render_soft.h"
#include "i2c.h"
#include "input.h"
#include "interrupts.h"
#include "memory.h"
#include "ndma.h"
#include "pdc.h"
#include "pxi.h"
#include "rsa.h"
#include "sd_mmc.h"
#include "settings.h"
#include "sha.h"
#include "teak_interp.h"
#include "timers.h"
#include "vfp11_interp.h"
#include "wifi.h"

enum CoreError {
    ERROR_BOOTROM
};

enum Task {
    RESET_CYCLES,
    END_FRAME,
    TOGGLE_RUN_FUNC,
    ARM_STOP_CYCLES,
    TEAK_STOP_CYCLES,
    ARM11A_INTERRUPT,
    ARM11B_INTERRUPT,
    ARM11C_INTERRUPT,
    ARM11D_INTERRUPT,
    ARM9_INTERRUPT,
    TEAK_INTERRUPT0,
    TEAK_INTERRUPT1,
    TEAK_INTERRUPT2,
    TEAK_INTERRUPT3,
    TMR11A_UNDERFLOW0,
    TMR11A_UNDERFLOW1,
    TMR11B_UNDERFLOW0,
    TMR11B_UNDERFLOW1,
    TMR11C_UNDERFLOW0,
    TMR11C_UNDERFLOW1,
    TMR11D_UNDERFLOW0,
    TMR11D_UNDERFLOW1,
    TMR9_OVERFLOW0,
    TMR9_OVERFLOW1,
    TMR9_OVERFLOW2,
    TMR9_OVERFLOW3,
    DSP_UNDERFLOW0,
    DSP_UNDERFLOW1,
    DSP_UNSIGNAL0,
    DSP_UNSIGNAL1,
    DSP_SEND_AUDIO,
    AES_UPDATE,
    CDMA0_UPDATE,
    CDMA1_UPDATE,
    XDMA_UPDATE,
    NDMA_UPDATE,
    SHA11_UPDATE,
    SHA9_UPDATE,
    GPU_SET0_READY,
    GPU_SET1_READY,
    GPU_CPY_READY,
    CSND_SAMPLE,
    SDMMC0_READ_BLOCK,
    SDMMC1_READ_BLOCK,
    SDMMC0_WRITE_BLOCK,
    SDMMC1_WRITE_BLOCK,
    WIFI_READ_BLOCK,
    WIFI_WRITE_BLOCK,
    NTR_WORD_READY,
    CTR_WORD_READY,
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
    Cartridge cartridge;
    Cdma cdmas[3];
    Cp15 cp15;
    Csnd csnd;
    Dsp dsp;
    Gpu gpu;
    GpuRenderSoft gpuRender;
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
    TeakInterp teak;
    Timers timers;
    Vfp11Interp vfp11s[MAX_CPUS - 1];
    Wifi wifi;

    std::atomic<bool> running;
    std::vector<Event> events;
    uint64_t globalCycles = 0;

    Core(std::string &cartPath);
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
