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

enum DspState {
    STATE_OFF,
    STATE_HANDSHAKE,
    STATE_INIT,
    STATE_RECEIVE,
    STATE_REPLY,
    STATE_RUNNING
};

class Core;

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
    DspState state = STATE_OFF;
    uint32_t base = 0;
    uint32_t cycles = 0;
    bool scheduled = false;

    uint16_t readData(uint32_t address);
    void writeData(uint32_t address, uint16_t value);

    void sendResponse(int i, uint16_t value);
    void setSemaphore(int bit);
    void updateSemaIrq();
};
