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
#include "defines.h"

class Core;

struct MmuMap {
    uint32_t addr;
    uint32_t tag;
};

class Cp15 {
public:
    uint32_t exceptAddrs[MAX_CPUS] = {};

    Cp15(Core *core): core(core) {}

    template <typename T> T read(CpuId id, uint32_t address);
    template <typename T> void write(CpuId id, uint32_t address, T value);

    uint32_t readReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp);
    void writeReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value);

private:
    Core *core;

    MmuMap mmuMaps[MAX_CPUS - 1][0x100000] = {};
    uint32_t mmuTags[MAX_CPUS - 1] = { 1, 1, 1, 1 };
    bool mmuEnables[MAX_CPUS - 1] = {};

    bool dtcmRead = false, dtcmWrite = false;
    bool itcmRead = false, itcmWrite = false;
    uint32_t dtcmAddr = 0, dtcmSize = 0;
    uint32_t itcmSize = 0;

    uint8_t itcm[0x8000] = {}; // 32KB ARM9 ITCM
    uint8_t dtcm[0x4000] = {}; // 16KB ARM9 DTCM

    uint32_t ctrlRegs[MAX_CPUS] = { 0x54078, 0x54078, 0x78 };
    uint32_t tlbBase0Regs[MAX_CPUS - 1] = {};
    uint32_t tlbBase1Regs[MAX_CPUS - 1] = {};
    uint32_t tlbCtrlRegs[MAX_CPUS - 1] = {};
    uint32_t physAddrRegs[MAX_CPUS - 1] = {};
    uint32_t threadIdRegs[MAX_CPUS - 1][3] = {};
    uint32_t dtcmReg = 0;
    uint32_t itcmReg = 0;

    uint32_t mmuTranslate(CpuId id, uint32_t address);
    void mmuInvalidate(CpuId id);

    void writeCtrl11(CpuId id, uint32_t value);
    void writeCtrl9(CpuId id, uint32_t value);
    void writeTlbBase0(CpuId id, uint32_t value);
    void writeTlbBase1(CpuId id, uint32_t value);
    void writeTlbCtrl(CpuId id, uint32_t value);
    void writeAddrTrans(CpuId id, uint32_t value);
    void writeThreadId(CpuId id, int i, uint32_t value);
    void writeWfi(CpuId id, uint32_t value);
    void writeDtcm(CpuId id, uint32_t value);
    void writeItcm(CpuId id, uint32_t value);
};
