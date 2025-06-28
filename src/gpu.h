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

class Gpu {
public:
    Gpu(Core *core): core(core) {}

    uint32_t readCfg11GpuCnt() { return cfg11GpuCnt; }
    uint32_t readMemfillDstAddr(int i) { return gpuMemfillDstAddr[i]; }
    uint32_t readMemfillDstEnd(int i) { return gpuMemfillDstEnd[i]; }
    uint32_t readMemfillData(int i) { return gpuMemfillData[i]; }
    uint32_t readMemfillCnt(int i) { return gpuMemfillCnt[i]; }
    uint32_t readMemcopySrcAddr() { return gpuMemcopySrcAddr; }
    uint32_t readMemcopyDstAddr() { return gpuMemcopyDstAddr; }
    uint32_t readMemcopyFlags() { return gpuMemcopyFlags; }
    uint32_t readMemcopyCnt() { return gpuMemcopyCnt; }
    uint32_t readMemcopyTexSize() { return gpuMemcopyTexSize; }
    uint32_t readMemcopyTexSrcWidth() { return gpuMemcopyTexSrcWidth; }
    uint32_t readMemcopyTexDstWidth() { return gpuMemcopyTexDstWidth; }
    uint32_t readIrqAck(int i) { return gpuIrqReq[i]; }
    uint32_t readIrqCmp(int i) { return gpuIrqCmp[i]; }
    uint32_t readIrqMaskL() { return gpuIrqMask >> 0; }
    uint32_t readIrqMaskH() { return gpuIrqMask >> 32; }
    uint32_t readIrqStatL() { return gpuIrqStat >> 0; }
    uint32_t readIrqStatH() { return gpuIrqStat >> 32; }
    uint32_t readIrqAutostop() { return gpuIrqAutostop; }

    uint32_t readIrqReq(int i) { return gpuIrqReq[i]; }
    uint32_t readCmdbufSize(int i) { return gpuCmdbufSize[i]; }
    uint32_t readCmdbufAddr(int i) { return gpuCmdbufAddr[i]; }

    void writeCfg11GpuCnt(uint32_t mask, uint32_t value);
    void writeMemfillDstAddr(int i, uint32_t mask, uint32_t value);
    void writeMemfillDstEnd(int i, uint32_t mask, uint32_t value);
    void writeMemfillData(int i, uint32_t mask, uint32_t value);
    void writeMemfillCnt(int i, uint32_t mask, uint32_t value);
    void writeMemcopySrcAddr(uint32_t mask, uint32_t value);
    void writeMemcopyDstAddr(uint32_t mask, uint32_t value);
    void writeMemcopyFlags(uint32_t mask, uint32_t value);
    void writeMemcopyCnt(uint32_t mask, uint32_t value);
    void writeMemcopyTexSize(uint32_t mask, uint32_t value);
    void writeMemcopyTexSrcWidth(uint32_t mask, uint32_t value);
    void writeMemcopyTexDstWidth(uint32_t mask, uint32_t value);
    void writeIrqAck(int i, uint32_t mask, uint32_t value);
    void writeIrqCmp(int i, uint32_t mask, uint32_t value);
    void writeIrqMaskL(uint32_t mask, uint32_t value);
    void writeIrqMaskH(uint32_t mask, uint32_t value);
    void writeIrqAutostop(uint32_t mask, uint32_t value);

    template <int i> void writeIrqReq(uint32_t mask, uint32_t value);
    template <int i> void writeCmdbufSize(uint32_t mask, uint32_t value);
    template <int i> void writeCmdbufAddr(uint32_t mask, uint32_t value);
    template <int i> void writeCmdbufJump(uint32_t mask, uint32_t value);
    void writeUnkCmd(uint32_t mask, uint32_t value);

private:
    Core *core;

    static void (Gpu::*cmdWrites[0x400])(uint32_t, uint32_t);
    static uint32_t maskTable[0x10];

    uint32_t cmdAddr = -1;
    uint32_t cmdEnd = 0;
    uint16_t curCmd = 0;

    uint32_t cfg11GpuCnt = 0;
    uint32_t gpuMemfillDstAddr[2] = {};
    uint32_t gpuMemfillDstEnd[2] = {};
    uint32_t gpuMemfillData[2] = {};
    uint32_t gpuMemfillCnt[2] = {};
    uint32_t gpuMemcopySrcAddr = 0;
    uint32_t gpuMemcopyDstAddr = 0;
    uint32_t gpuMemcopyFlags = 0;
    uint32_t gpuMemcopyCnt = 0;
    uint32_t gpuMemcopyTexSize = 0;
    uint32_t gpuMemcopyTexSrcWidth = 0;
    uint32_t gpuMemcopyTexDstWidth = 0;
    uint32_t gpuIrqCmp[16] = {};
    uint64_t gpuIrqMask = 0;
    uint64_t gpuIrqStat = 0;
    uint32_t gpuIrqAutostop = 0;

    uint32_t gpuIrqReq[16] = {};
    uint32_t gpuCmdbufSize[2] = {};
    uint32_t gpuCmdbufAddr[2] = {};

    void runCommands();
    bool checkInterrupt(int i);
};
