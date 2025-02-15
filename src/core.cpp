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

#include <algorithm>
#include "core.h"

Core::Core(): aes(this), arms { ArmInterp(this, ARM11A), ArmInterp(this, ARM11B),
        ArmInterp(this, ARM11C), ArmInterp(this, ARM11D), ArmInterp(this, ARM9) }, cp15(this),
        gpu(this), i2c(this), input(this), interrupts(this), memory(this), ndma(this), pdc(this),
        pxi(this), rsa(this), sdMmc(this), shas { Sha(this, false), Sha(this, true) }, timers(this) {
    // Initialize things that need to be done after construction
    n3dsMode = sdMmc.init();
    memory.init();
    for (int i = 0; i < MAX_CPUS; i++)
        arms[i].init();
    LOG_INFO("Running in %s 3DS mode\n", n3dsMode ? "new" : "old");

    // Define the tasks that can be scheduled
    tasks[RESET_CYCLES] = std::bind(&Core::resetCycles, this);
    tasks[END_FRAME] = std::bind(&Core::endFrame, this);
    tasks[TOGGLE_RUN_FUNC] = std::bind(&Core::toggleRunFunc, this);
    tasks[ARM11A_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11A);
    tasks[ARM11B_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11B);
    tasks[ARM11C_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11C);
    tasks[ARM11D_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11D);
    tasks[ARM9_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM9);
    tasks[ARM11A_OVERFLOW0] = std::bind(&Timers::overflowMp, &timers, ARM11A, 0);
    tasks[ARM11A_OVERFLOW1] = std::bind(&Timers::overflowMp, &timers, ARM11A, 1);
    tasks[ARM11B_OVERFLOW0] = std::bind(&Timers::overflowMp, &timers, ARM11B, 0);
    tasks[ARM11B_OVERFLOW1] = std::bind(&Timers::overflowMp, &timers, ARM11B, 1);
    tasks[ARM11C_OVERFLOW0] = std::bind(&Timers::overflowMp, &timers, ARM11C, 0);
    tasks[ARM11C_OVERFLOW1] = std::bind(&Timers::overflowMp, &timers, ARM11C, 1);
    tasks[ARM11D_OVERFLOW0] = std::bind(&Timers::overflowMp, &timers, ARM11D, 0);
    tasks[ARM11D_OVERFLOW1] = std::bind(&Timers::overflowMp, &timers, ARM11D, 1);
    tasks[ARM9_OVERFLOW0] = std::bind(&Timers::overflowTm, &timers, 0);
    tasks[ARM9_OVERFLOW1] = std::bind(&Timers::overflowTm, &timers, 1);
    tasks[ARM9_OVERFLOW2] = std::bind(&Timers::overflowTm, &timers, 2);
    tasks[ARM9_OVERFLOW3] = std::bind(&Timers::overflowTm, &timers, 3);
    tasks[AES_UPDATE] = std::bind(&Aes::update, &aes);
    tasks[NDMA_UPDATE] = std::bind(&Ndma::update, &ndma);
    tasks[SHA11_UPDATE] = std::bind(&Sha::update, &shas[0]);
    tasks[SHA9_UPDATE] = std::bind(&Sha::update, &shas[1]);
    tasks[SDMMC_READ_BLOCK] = std::bind(&SdMmc::readBlock, &sdMmc);

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
    // Break execution at the end of a frame and count it
    running.store(false);
    fpsCount++;

    // Update FPS and reset the counter every second
    std::chrono::duration<double> fpsTime = std::chrono::steady_clock::now() - lastFpsTime;
    if (fpsTime.count() >= 1.0f) {
        fps = fpsCount;
        fpsCount = 0;
        lastFpsTime = std::chrono::steady_clock::now();
    }

    // Draw a frame and schedule the next one
    pdc.drawFrame();
    schedule(END_FRAME, 268111856 / 60);
}

void Core::toggleRunFunc() {
    // Switch between 2-core and 4-core ARM11 run functions and break execution
    runFunc = (runFunc == &ArmInterp::runFrame<true>) ? &ArmInterp::runFrame<false> : &ArmInterp::runFrame<true>;
    running.store(false);
}

void Core::schedule(Task task, uint32_t cycles) {
    // Add a task to the scheduler, sorted by least to most cycles until execution
    Event event(&tasks[task], globalCycles + cycles);
    auto it = std::upper_bound(events.cbegin(), events.cend(), event);
    events.insert(it, event);
}
