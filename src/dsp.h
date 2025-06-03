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

#include <cstdint>

class Core;

class Dsp {
public:
    Dsp(Core *core): core(core) {}

    uint16_t readData(uint16_t address);
    void writeData(uint16_t address, uint16_t value);

    uint16_t readPcfg() { return dspPcfg; }
    uint16_t readPsts() { return dspPsts; }
    uint16_t readPsem() { return dspPsem; }
    uint16_t readPmask() { return dspPmask; }
    uint16_t readSem() { return dspSem; }
    uint16_t readCmd(int i) { return dspCmd[i]; }
    uint16_t readRep(int i);

    void writePcfg(uint16_t mask, uint16_t value);
    void writePsem(uint16_t mask, uint16_t value);
    void writePmask(uint16_t mask, uint16_t value);
    void writePclear(uint16_t mask, uint16_t value);
    void writeCmd(int i, uint16_t mask, uint16_t value);

private:
    Core *core;

    uint16_t dspPcfg = 0x1;
    uint16_t dspPsts = 0xE100;
    uint16_t dspPsem = 0;
    uint16_t dspPmask = 0xFFFF;
    uint16_t dspSem = 0;
    uint16_t dspCmd[3] = {};
    uint16_t dspRep[3] = {};

    uint16_t hpiMask = 0xFFFF;
    uint16_t hpiCfg = 0;
    uint16_t hpiSts = 0xE0;
    uint16_t miuIoBase = 0x8000;

    void updateArmSemIrq();
    void updateDspSemIrq();

    uint16_t readHpiCmd(int i);
    void writeHpiRep(int i, uint16_t value);
    void writeHpiSem(uint16_t value);
    void writeHpiMask(uint16_t value);
    void writeHpiClear(uint16_t value);
    void writeHpiCfg(uint16_t value);
    void writeIoBase(uint16_t value);
};
