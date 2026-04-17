/*
    Copyright 2023-2026 Hydr8gon

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

#include <queue>

#include "csnd.h"
#include "dsp.h"

class Core;

enum DspState {
    STATE_OFF,
    STATE_HANDSHAKE0,
    STATE_HANDSHAKE1,
    STATE_INIT0,
    STATE_INIT1,
    STATE_RECEIVE0,
    STATE_RECEIVE1,
    STATE_REPLY0,
    STATE_REPLY1,
    STATE_RUNNING,
    STATE_INTERRUPT,
    STATE_RESET0,
    STATE_RESET1,
    STATE_STOPPED0,
    STATE_STOPPED1
};

struct InputBuffer {
    uint32_t address;
    uint32_t count;
    uint16_t seqId;
    int16_t adpcmPrev[2];
};

struct InputState {
    uint16_t playFlags;
    uint16_t syncCount;
    uint16_t format;
    float rate;
    float volume[2];

    double position;
    InputBuffer current;
    InputBuffer buffers[5];
    int16_t adpcmCoeffs[8][2];
};

class DspHle: public Dsp {
public:
    DspHle(Core &core): core(core) {}
    void update();

    void resetCycles() {}
    void setAudClock(DspClock clock);

    uint16_t readPdata();
    uint16_t readPcfg() { return dspPcfg; }
    uint16_t readPsts() { return dspPsts; }
    uint16_t readPsem() { return dspPsem; }
    uint16_t readPmask() { return dspPmask; }
    uint16_t readSem() { return dspSem; }
    uint16_t readCmd(int i) { return dspCmd[i]; }
    uint16_t readRep(int i);

    void writePdata(uint16_t mask, uint16_t value);
    void writePadr(uint16_t mask, uint16_t value);
    void writePcfg(uint16_t mask, uint16_t value);
    void writePsem(uint16_t mask, uint16_t value);
    void writePmask(uint16_t mask, uint16_t value);
    void writePclear(uint16_t mask, uint16_t value);
    void writeCmd(int i, uint16_t mask, uint16_t value);

private:
    Core &core;

    std::queue<uint16_t> readFifo;
    uint32_t cycles = 0;
    bool scheduled = false;

    DspState state = STATE_OFF;
    InputState inputs[24] = {};
    float outVolume = 1.0f;

    uint32_t frameBase = 0x1FF40000;
    uint32_t saveAddress = 0x1FF68000;
    bool canRestore = false;

    void advanceState();
    void processFrame();

    uint16_t readData(uint16_t address);
    void writeData(uint16_t address, uint16_t value);

    void sendResponse(int i, uint16_t value);
    void setSemaphore(int bit);
    void updateSemaIrq();
};
