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
    Interrupts(Core &core): core(core) {}

    void sendInterrupt(CpuId id, int type);
    void checkInterrupt(CpuId id) { sendInterrupt(id, -1); }
    void interrupt(CpuId id);
    void halt(CpuId id, uint8_t type = 0);

    uint16_t readCfg11Socinfo();
    uint32_t readCfg11MpClkcnt() { return cfg11MpClkcnt; }
    uint8_t readCfg11MpBootcnt(int i);
    uint32_t readMpScuConfig();
    uint32_t readMpIle(CpuId id) { return mpIle[id]; }
    uint32_t readMpPrioMask(CpuId id) { return mpPrioMask[id]; }
    uint32_t readMpAck(CpuId id);
    uint32_t readMpPending(CpuId id);
    uint32_t readMpIge() { return mpIge; }
    uint32_t readMpCtrlType();
    uint32_t readMpIe(int i) { return mpIe[i]; }
    uint32_t readMpIp(CpuId id, int i) { return mpIp[id][i]; }
    uint32_t readMpIa(CpuId id, int i) { return mpIa[id][i]; }
    uint8_t readMpPriorityL(CpuId id, int i) { return mpPriorityL[id][i]; }
    uint8_t readMpPriorityG(int i) { return mpPriorityG[i - 0x20]; }
    uint8_t readMpTarget(CpuId id, int i);
    uint32_t readIrqIe() { return irqIe; }
    uint32_t readIrqIf() { return irqIf; }

    void writeCfg11MpClkcnt(uint32_t mask, uint32_t value);
    void writeCfg11MpBootcnt(int i, uint8_t value);
    void writeMpIle(CpuId id, uint32_t mask, uint32_t value);
    void writeMpPrioMask(CpuId id, uint32_t mask, uint32_t value);
    void writeMpEoi(CpuId id, uint32_t mask, uint32_t value);
    void writeMpIge(uint32_t mask, uint32_t value);
    void writeMpIeSet(int i, uint32_t mask, uint32_t value);
    void writeMpIeClear(int i, uint32_t mask, uint32_t value);
    void writeMpIpSet(int i, uint32_t mask, uint32_t value);
    void writeMpIpClear(int i, uint32_t mask, uint32_t value);
    void writeMpPriorityL(CpuId id, int i, uint8_t value);
    void writeMpPriorityG(int i, uint8_t value);
    void writeMpTarget(int i, uint8_t value);
    void writeMpSoftIrq(CpuId id, uint32_t mask, uint32_t value);
    void writeIrqIe(uint32_t mask, uint32_t value);
    void writeIrqIf(uint32_t mask, uint32_t value);

private:
    Core &core;

    bool scheduled[MAX_CPUS] = {};
    uint8_t sources[MAX_CPUS - 1][0x10] = {};

    uint32_t cfg11MpClkcnt = 0;
    uint8_t cfg11MpBootcnt[2] = {};
    uint32_t mpIle[MAX_CPUS - 1] = {};
    uint32_t mpPrioMask[MAX_CPUS - 1] = { 0xF0, 0xF0, 0xF0, 0xF0 };
    uint32_t mpIge = 0;
    uint32_t mpIe[4] = { 0xFFFF };
    uint32_t mpIp[MAX_CPUS - 1][4] = {};
    uint32_t mpIa[MAX_CPUS - 1][4] = {};
    uint8_t mpPriorityL[MAX_CPUS - 1][0x20] = {};
    uint8_t mpPriorityG[0x60] = {};
    uint8_t mpTarget[0x80] = {};
    uint32_t irqIe = 0;
    uint32_t irqIf = 0;
};
