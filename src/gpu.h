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

enum ColbufFmt {
    RGBA8 = 0,
    RGB565,
    RGB5A1,
    RGBA4,
    UNK_FMT
};

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
    uint32_t readMemcopyDispSize() { return gpuMemcopyDispSize; }
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
    uint32_t readViewScaleH() { return gpuViewScaleH; }
    uint32_t readViewScaleV() { return gpuViewScaleV; }
    uint32_t readShdOutTotal() { return gpuShdOutTotal; }
    uint32_t readShdOutMap(int i) { return gpuShdOutMap[i]; }
    uint32_t readColbufFmt() { return gpuColbufFmt; }
    uint32_t readColbufLoc() { return gpuColbufLoc; }
    uint32_t readBufferDim() { return gpuBufferDim; }
    uint32_t readAttrBase() { return gpuAttrBase; }
    uint32_t readAttrFmtL() { return gpuAttrFmt >> 0; }
    uint32_t readAttrFmtH() { return gpuAttrFmt >> 32; }
    uint32_t readAttrOfs(int i) { return gpuAttrOfs[i]; }
    uint32_t readAttrCfgL(int i) { return gpuAttrCfg[i] >> 0; }
    uint32_t readAttrCfgH(int i) { return gpuAttrCfg[i] >> 32; }
    uint32_t readAttrIdxList() { return gpuAttrIdxList; }
    uint32_t readAttrNumVerts() { return gpuAttrNumVerts; }
    uint32_t readAttrFirstIdx() { return gpuAttrFirstIdx; }
    uint32_t readCmdSize(int i) { return gpuCmdSize[i]; }
    uint32_t readCmdAddr(int i) { return gpuCmdAddr[i]; }
    uint32_t readVshBools() { return gpuVshBools; }
    uint32_t readVshInts(int i) { return gpuVshInts[i]; }
    uint32_t readVshEntry() { return gpuVshEntry; }
    uint32_t readVshAttrIdsL() { return gpuVshAttrIds >> 0; }
    uint32_t readVshAttrIdsH() { return gpuVshAttrIds >> 32; }

    void writeCfg11GpuCnt(uint32_t mask, uint32_t value);
    void writeMemfillDstAddr(int i, uint32_t mask, uint32_t value);
    void writeMemfillDstEnd(int i, uint32_t mask, uint32_t value);
    void writeMemfillData(int i, uint32_t mask, uint32_t value);
    void writeMemfillCnt(int i, uint32_t mask, uint32_t value);
    void writeMemcopySrcAddr(uint32_t mask, uint32_t value);
    void writeMemcopyDstAddr(uint32_t mask, uint32_t value);
    void writeMemcopyDispSize(uint32_t mask, uint32_t value);
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
    void writeViewScaleH(uint32_t mask, uint32_t value);
    void writeViewScaleV(uint32_t mask, uint32_t value);
    void writeShdOutTotal(uint32_t mask, uint32_t value);
    template <int i> void writeShdOutMap(uint32_t mask, uint32_t value);
    void writeColbufFmt(uint32_t mask, uint32_t value);
    void writeColbufLoc(uint32_t mask, uint32_t value);
    void writeBufferDim(uint32_t mask, uint32_t value);
    void writeAttrBase(uint32_t mask, uint32_t value);
    void writeAttrFmtL(uint32_t mask, uint32_t value);
    void writeAttrFmtH(uint32_t mask, uint32_t value);
    template <int i> void writeAttrOfs(uint32_t mask, uint32_t value);
    template <int i> void writeAttrCfgL(uint32_t mask, uint32_t value);
    template <int i> void writeAttrCfgH(uint32_t mask, uint32_t value);
    void writeAttrIdxList(uint32_t mask, uint32_t value);
    void writeAttrNumVerts(uint32_t mask, uint32_t value);
    void writeAttrFirstIdx(uint32_t mask, uint32_t value);
    void writeAttrDrawArrays(uint32_t mask, uint32_t value);
    void writeAttrDrawElems(uint32_t mask, uint32_t value);
    template <int i> void writeCmdSize(uint32_t mask, uint32_t value);
    template <int i> void writeCmdAddr(uint32_t mask, uint32_t value);
    template <int i> void writeCmdJump(uint32_t mask, uint32_t value);
    void writeVshBools(uint32_t mask, uint32_t value);
    template <int i> void writeVshInts(uint32_t mask, uint32_t value);
    void writeVshEntry(uint32_t mask, uint32_t value);
    void writeVshAttrIdsL(uint32_t mask, uint32_t value);
    void writeVshAttrIdsH(uint32_t mask, uint32_t value);
    void writeVshFloatIdx(uint32_t mask, uint32_t value);
    void writeVshFloatData(uint32_t mask, uint32_t value);
    void writeVshCodeIdx(uint32_t mask, uint32_t value);
    void writeVshCodeData(uint32_t mask, uint32_t value);
    void writeVshDescIdx(uint32_t mask, uint32_t value);
    void writeVshDescData(uint32_t mask, uint32_t value);
    void writeUnkCmd(uint32_t mask, uint32_t value);

private:
    Core *core;

    static void (Gpu::*cmdWrites[0x400])(uint32_t, uint32_t);
    static uint32_t maskTable[0x10];

    uint32_t cmdAddr = -1;
    uint32_t cmdEnd = 0;
    uint16_t curCmd = 0;

    uint32_t vshFloatData[4] = {};
    uint16_t vshFloatIdx = 0;
    bool vshFloat32 = false;

    uint32_t cfg11GpuCnt = 0;
    uint32_t gpuMemfillDstAddr[2] = {};
    uint32_t gpuMemfillDstEnd[2] = {};
    uint32_t gpuMemfillData[2] = {};
    uint32_t gpuMemfillCnt[2] = {};
    uint32_t gpuMemcopySrcAddr = 0;
    uint32_t gpuMemcopyDstAddr = 0;
    uint32_t gpuMemcopyDispSize = 0;
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
    uint32_t gpuViewScaleH = 0;
    uint32_t gpuViewScaleV = 0;
    uint32_t gpuShdOutTotal = 0;
    uint32_t gpuShdOutMap[7] = {};
    uint32_t gpuColbufFmt = 0;
    uint32_t gpuColbufLoc = 0;
    uint32_t gpuBufferDim = 0;
    uint32_t gpuAttrBase = 0;
    uint64_t gpuAttrFmt = 0;
    uint32_t gpuAttrOfs[12] = {};
    uint64_t gpuAttrCfg[12] = {};
    uint32_t gpuAttrIdxList = 0;
    uint32_t gpuAttrNumVerts = 0;
    uint32_t gpuAttrFirstIdx = 0;
    uint32_t gpuCmdSize[2] = {};
    uint32_t gpuCmdAddr[2] = {};
    uint32_t gpuVshBools = 0;
    uint32_t gpuVshInts[4] = {};
    uint32_t gpuVshEntry = 0;
    uint64_t gpuVshAttrIds = 0;
    uint32_t gpuVshCodeIdx = 0;
    uint32_t gpuVshDescIdx = 0;

    bool checkInterrupt(int i);
    static uint32_t float24To32(uint32_t value);

    void runCommands();
    void drawAttrIdx(uint32_t idx);
    void updateOutMap();
};
