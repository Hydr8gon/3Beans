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

class Interrupts {
public:
    Interrupts(Core *core): core(core) {}
    void sendInterrupt(bool arm9, int type);
    void checkInterrupt(bool arm9) { sendInterrupt(arm9, -1); }
    void interrupt(CpuId id);

    uint32_t readMpIle(CpuId id) { return mpIle[id]; }
    uint32_t readMpAck(CpuId id);
    uint32_t readMpPending(CpuId id);
    uint32_t readMpIge() { return mpIge; }
    uint32_t readMpIe(int i) { return mpIe[i]; }
    uint32_t readMpIp(CpuId id, int i) { return mpIp[id][i]; }
    uint32_t readMpIa(CpuId id, int i) { return mpIa[id][i]; }
    uint32_t readIrqIe() { return irqIe; }
    uint32_t readIrqIf() { return irqIf; }

    void writeMpIle(CpuId id, uint32_t mask, uint32_t value);
    void writeMpEoi(CpuId id, uint32_t mask, uint32_t value);
    void writeMpIge(uint32_t mask, uint32_t value);
    void writeMpIeSet(int i, uint32_t mask, uint32_t value);
    void writeMpIeClear(int i, uint32_t mask, uint32_t value);
    void writeIrqIe(uint32_t mask, uint32_t value);
    void writeIrqIf(uint32_t mask, uint32_t value);

private:
    Core *core;

    uint32_t mpIle[2] = {};
    uint32_t mpIge = 0;
    uint32_t mpIe[4] = { 0xFFFF };
    uint32_t mpIp[2][4] = {};
    uint32_t mpIa[2][4] = {};
    uint32_t irqIe = 0;
    uint32_t irqIf = 0;
};
