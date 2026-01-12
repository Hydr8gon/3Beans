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

class Timers {
public:
    Timers(Core &core): core(core) {}

    void resetCycles();
    void setMpScale(int scale);
    void underflowMp(CpuId id, int i);
    void overflowTm(int i);

    uint32_t readMpReload(CpuId id, int i) { return mpReload[id][i]; }
    uint32_t readMpCounter(CpuId id, int i);
    uint32_t readMpTmcnt(CpuId id, int i) { return mpTmcnt[id][i]; }
    uint32_t readMpTmirq(CpuId id, int i) { return mpTmirq[id][i]; }
    uint16_t readTmCntL(int i);
    uint16_t readTmCntH(int i) { return tmCntH[i]; }

    void writeMpReload(CpuId id, int i, uint32_t mask, uint32_t value);
    void writeMpCounter(CpuId id, int i, uint32_t mask, uint32_t value);
    void writeMpTmcnt(CpuId id, int i, uint32_t mask, uint32_t value);
    void writeMpTmirq(CpuId id, int i, uint32_t mask, uint32_t value);
    void writeTmCntL(int i, uint16_t mask, uint16_t value);
    void writeTmCntH(int i, uint16_t mask, uint16_t value);

private:
    Core &core;
    int mpScale = 1;

    uint64_t endCyclesMp[MAX_CPUS - 1][2] = {};
    uint64_t endCyclesTm[4] = {};
    uint16_t timers[4] = {};
    uint8_t shifts[4] = { 1, 1, 1, 1 };
    bool countUp[4] = {};

    uint32_t mpReload[MAX_CPUS - 1][2] = {};
    uint32_t mpCounter[MAX_CPUS - 1][2] = {};
    uint32_t mpTmcnt[MAX_CPUS - 1][2] = {};
    uint32_t mpTmirq[MAX_CPUS - 1][2] = {};
    uint16_t tmCntL[4] = {};
    uint16_t tmCntH[4] = {};

    void scheduleMp(CpuId id, int i);
};
