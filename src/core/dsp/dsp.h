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

#include <cstdint>

class Core;

class Dsp {
public:
    virtual ~Dsp() {}
    virtual void resetCycles() = 0;
    virtual void setAudClock(DspClock clock) = 0;

    virtual uint16_t readPdata() = 0;
    virtual uint16_t readPcfg() = 0;
    virtual uint16_t readPsts() = 0;
    virtual uint16_t readPsem() = 0;
    virtual uint16_t readPmask() = 0;
    virtual uint16_t readSem() = 0;
    virtual uint16_t readCmd(int i) = 0;
    virtual uint16_t readRep(int i) = 0;

    virtual void writePdata(uint16_t mask, uint16_t value) = 0;
    virtual void writePadr(uint16_t mask, uint16_t value) = 0;
    virtual void writePcfg(uint16_t mask, uint16_t value) = 0;
    virtual void writePsem(uint16_t mask, uint16_t value) = 0;
    virtual void writePmask(uint16_t mask, uint16_t value) = 0;
    virtual void writePclear(uint16_t mask, uint16_t value) = 0;
    virtual void writeCmd(int i, uint16_t mask, uint16_t value) = 0;

protected:
    uint16_t dspPadr = 0;
    uint16_t dspPcfg = 0;
    uint16_t dspPsts = 0x100;
    uint16_t dspPsem = 0;
    uint16_t dspPmask = 0;
    uint16_t dspSem = 0;
    uint16_t dspCmd[3] = {};
    uint16_t dspRep[3] = {};
};
