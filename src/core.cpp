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

Core::Core(std::string &cartPath): aes(this), arms { ArmInterp(this, ARM11A), ArmInterp(this, ARM11B), ArmInterp(this,
        ARM11C), ArmInterp(this, ARM11D), ArmInterp(this, ARM9) }, cartridge(this, cartPath), cdmas { Cdma(this, CDMA0),
        Cdma(this, CDMA1), Cdma(this, XDMA) }, cp15(this), csnd(this), dsp(this), gpu(this), gpuRender(this), i2c(this),
        input(this), interrupts(this), memory(this), ndma(this), pdc(this), pxi(this), rsa(this), sdMmcs { SdMmc(this),
        SdMmc(this) }, shas { Sha(this, false), Sha(this, true) }, teak(this), timers(this), vfp11s { Vfp11Interp(this,
        ARM11A), Vfp11Interp(this, ARM11B), Vfp11Interp(this, ARM11C), Vfp11Interp(this, ARM11D) }, wifi(this) {
    // Initialize things that need to be done after construction
    n3dsMode = sdMmcs[0].init(sdMmcs[1]);
    if (!memory.init())
        throw ERROR_BOOTROM;
    for (int i = 0; i < MAX_CPUS; i++)
        arms[i].init();
    LOG_INFO("Running in %s 3DS mode\n", n3dsMode ? "new" : "old");

    // Define the tasks that can be scheduled
    tasks[RESET_CYCLES] = std::bind(&Core::resetCycles, this);
    tasks[END_FRAME] = std::bind(&Core::endFrame, this);
    tasks[TOGGLE_RUN_FUNC] = std::bind(&Core::toggleRunFunc, this);
    tasks[ARM_STOP_CYCLES] = std::bind(&ArmInterp::stopCycles, this);
    tasks[ARM11A_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11A);
    tasks[ARM11B_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11B);
    tasks[ARM11C_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11C);
    tasks[ARM11D_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM11D);
    tasks[ARM9_INTERRUPT] = std::bind(&Interrupts::interrupt, &interrupts, ARM9);
    tasks[TEAK_INTERRUPT0] = std::bind(&TeakInterp::interrupt, &teak, 0);
    tasks[TEAK_INTERRUPT1] = std::bind(&TeakInterp::interrupt, &teak, 1);
    tasks[TEAK_INTERRUPT2] = std::bind(&TeakInterp::interrupt, &teak, 2);
    tasks[TEAK_INTERRUPT3] = std::bind(&TeakInterp::interrupt, &teak, 3);
    tasks[TMR11A_UNDERFLOW0] = std::bind(&Timers::underflowMp, &timers, ARM11A, 0);
    tasks[TMR11A_UNDERFLOW1] = std::bind(&Timers::underflowMp, &timers, ARM11A, 1);
    tasks[TMR11B_UNDERFLOW0] = std::bind(&Timers::underflowMp, &timers, ARM11B, 0);
    tasks[TMR11B_UNDERFLOW1] = std::bind(&Timers::underflowMp, &timers, ARM11B, 1);
    tasks[TMR11C_UNDERFLOW0] = std::bind(&Timers::underflowMp, &timers, ARM11C, 0);
    tasks[TMR11C_UNDERFLOW1] = std::bind(&Timers::underflowMp, &timers, ARM11C, 1);
    tasks[TMR11D_UNDERFLOW0] = std::bind(&Timers::underflowMp, &timers, ARM11D, 0);
    tasks[TMR11D_UNDERFLOW1] = std::bind(&Timers::underflowMp, &timers, ARM11D, 1);
    tasks[TMR9_OVERFLOW0] = std::bind(&Timers::overflowTm, &timers, 0);
    tasks[TMR9_OVERFLOW1] = std::bind(&Timers::overflowTm, &timers, 1);
    tasks[TMR9_OVERFLOW2] = std::bind(&Timers::overflowTm, &timers, 2);
    tasks[TMR9_OVERFLOW3] = std::bind(&Timers::overflowTm, &timers, 3);
    tasks[DSP_UNDERFLOW0] = std::bind(&Dsp::underflowTmr, &dsp, 0);
    tasks[DSP_UNDERFLOW1] = std::bind(&Dsp::underflowTmr, &dsp, 1);
    tasks[DSP_UNSIGNAL0] = std::bind(&Dsp::unsignalTmr, &dsp, 0);
    tasks[DSP_UNSIGNAL1] = std::bind(&Dsp::unsignalTmr, &dsp, 1);
    tasks[DSP_SEND_AUDIO] = std::bind(&Dsp::sendAudio, &dsp);
    tasks[AES_UPDATE] = std::bind(&Aes::update, &aes);
    tasks[CDMA0_UPDATE] = std::bind(&Cdma::update, &cdmas[CDMA0]);
    tasks[CDMA1_UPDATE] = std::bind(&Cdma::update, &cdmas[CDMA1]);
    tasks[XDMA_UPDATE] = std::bind(&Cdma::update, &cdmas[XDMA]);
    tasks[NDMA_UPDATE] = std::bind(&Ndma::update, &ndma);
    tasks[SHA11_UPDATE] = std::bind(&Sha::update, &shas[0]);
    tasks[SHA9_UPDATE] = std::bind(&Sha::update, &shas[1]);
    tasks[CSND_SAMPLE] = std::bind(&Csnd::runSample, &csnd);
    tasks[SDMMC0_READ_BLOCK] = std::bind(&SdMmc::readBlock, &sdMmcs[0]);
    tasks[SDMMC1_READ_BLOCK] = std::bind(&SdMmc::readBlock, &sdMmcs[1]);
    tasks[SDMMC0_WRITE_BLOCK] = std::bind(&SdMmc::writeBlock, &sdMmcs[0]);
    tasks[SDMMC1_WRITE_BLOCK] = std::bind(&SdMmc::writeBlock, &sdMmcs[1]);
    tasks[WIFI_READ_BLOCK] = std::bind(&Wifi::readBlock, &wifi);
    tasks[WIFI_WRITE_BLOCK] = std::bind(&Wifi::writeBlock, &wifi);
    tasks[NTR_WORD_READY] = std::bind(&Cartridge::ntrWordReady, &cartridge);
    tasks[CTR_WORD_READY] = std::bind(&Cartridge::ctrWordReady, &cartridge);

    // Schedule the initial tasks
    schedule(RESET_CYCLES, 0x7FFFFFFFFFFFFFFF);
    schedule(END_FRAME, 268111856 / 60);
    schedule(CSND_SAMPLE, 2048);
}

void Core::resetCycles() {
    // Reset the global cycle count eventually to prevent overflow
    for (uint32_t i = 0; i < events.size(); i++)
        events[i].cycles -= globalCycles;
    for (int i = 0; i < MAX_CPUS; i++)
        arms[i].resetCycles();
    dsp.resetCycles();
    teak.resetCycles();
    timers.resetCycles();
    globalCycles -= globalCycles;
    schedule(RESET_CYCLES, 0x7FFFFFFFFFFFFFFF);
}

void Core::endFrame() {
    // Break execution at the end of a frame and count it
    running.store(false);
    fpsCount++;

    // Update the save file and FPS counter every second
    std::chrono::duration<double> fpsTime = std::chrono::steady_clock::now() - lastFpsTime;
    if (fpsTime.count() >= 1.0f) {
        cartridge.updateSave();
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

void Core::schedule(Task task, uint64_t cycles) {
    // Add a task to the scheduler, sorted by least to most cycles until execution
    Event event(&tasks[task], globalCycles + cycles);
    auto it = std::upper_bound(events.cbegin(), events.cend(), event);
    events.insert(it, event);
}
