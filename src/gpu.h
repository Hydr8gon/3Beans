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

enum PrimMode {
    SAME_PRIM,
    TRIANGLES,
    TRI_STRIPS,
    TRI_FANS,
    GEO_PRIM
};

enum CullMode {
    CULL_NONE,
    CULL_FRONT,
    CULL_BACK
};

enum TexWrap {
    WRAP_CLAMP,
    WRAP_BORDER,
    WRAP_REPEAT,
    WRAP_MIRROR
};

enum CombSrc {
    COMB_PRIM,
    COMB_FRAG0,
    COMB_FRAG1,
    COMB_TEX0,
    COMB_TEX1,
    COMB_TEX2,
    COMB_TEX3,
    COMB_PRVBUF,
    COMB_CONST,
    COMB_PREV,
    COMB_UNK
};

enum OperFunc {
    OPER_ZERO,
    OPER_ONE,
    OPER_SRC,
    OPER_1MSRC,
    OPER_DST,
    OPER_1MDST,
    OPER_SRCA,
    OPER_1MSRCA,
    OPER_DSTA,
    OPER_1MDSTA,
    OPER_CONST,
    OPER_1MCON,
    OPER_CONSTA,
    OPER_1MCONA
};

enum CalcMode {
    MODE_REPLACE,
    MODE_MOD,
    MODE_ADD,
    MODE_ADDS,
    MODE_INTERP,
    MODE_SUB,
    MODE_DOT3,
    MODE_DOT3A,
    MODE_MULADD,
    MODE_ADDMUL,
    MODE_RSUB,
    MODE_MIN,
    MODE_MAX,
    MODE_UNK
};

enum TestFunc {
    TEST_NV,
    TEST_AL,
    TEST_EQ,
    TEST_NE,
    TEST_LT,
    TEST_LE,
    TEST_GT,
    TEST_GE
};

enum TexFmt {
    TEX_RGBA8,
    TEX_RGB8,
    TEX_RGB5A1,
    TEX_RGB565,
    TEX_RGBA4,
    TEX_LA8,
    TEX_RG8,
    TEX_L8,
    TEX_A8,
    TEX_LA4,
    TEX_L4,
    TEX_A4,
    TEX_ETC1,
    TEX_ETC1A4,
    TEX_UNK
};

enum ColbufFmt {
    COL_RGBA8,
    COL_RGB8,
    COL_RGB565,
    COL_RGB5A1,
    COL_RGBA4,
    COL_UNK
};

enum DepbufFmt {
    DEP_16,
    DEP_24,
    DEP_24S8,
    DEP_UNK
};

class Gpu {
public:
    Gpu(Core *core): core(core) {}

    uint32_t readCfg11GpuCnt() { return cfg11GpuCnt; }
    uint32_t readMemsetDstAddr(int i) { return gpuMemsetDstAddr[i]; }
    uint32_t readMemsetDstEnd(int i) { return gpuMemsetDstEnd[i]; }
    uint32_t readMemsetData(int i) { return gpuMemsetData[i]; }
    uint32_t readMemsetCnt(int i) { return gpuMemsetCnt[i]; }
    uint32_t readMemcpySrcAddr() { return gpuMemcpySrcAddr; }
    uint32_t readMemcpyDstAddr() { return gpuMemcpyDstAddr; }
    uint32_t readMemcpyDispDstSize() { return gpuMemcpyDispDstSize; }
    uint32_t readMemcpyDispSrcSize() { return gpuMemcpyDispSrcSize; }
    uint32_t readMemcpyFlags() { return gpuMemcpyFlags; }
    uint32_t readMemcpyCnt() { return gpuMemcpyCnt; }
    uint32_t readMemcpyTexSize() { return gpuMemcpyTexSize; }
    uint32_t readMemcpyTexSrcWidth() { return gpuMemcpyTexSrcWidth; }
    uint32_t readMemcpyTexDstWidth() { return gpuMemcpyTexDstWidth; }
    uint32_t readIrqAck(int i) { return gpuIrqReq[i]; }
    uint32_t readIrqCmp(int i) { return gpuIrqCmp[i]; }
    uint32_t readIrqMaskL() { return gpuIrqMask >> 0; }
    uint32_t readIrqMaskH() { return gpuIrqMask >> 32; }
    uint32_t readIrqStatL() { return gpuIrqStat >> 0; }
    uint32_t readIrqStatH() { return gpuIrqStat >> 32; }
    uint32_t readIrqAutostop() { return gpuIrqAutostop; }

    uint32_t readIrqReq(int i) { return gpuIrqReq[i]; }
    uint32_t readFaceCulling() { return gpuFaceCulling; }
    uint32_t readViewScaleH() { return gpuViewScaleH; }
    uint32_t readViewStepH() { return gpuViewStepH; }
    uint32_t readViewScaleV() { return gpuViewScaleV; }
    uint32_t readViewStepV() { return gpuViewStepV; }
    uint32_t readShdOutTotal() { return gpuShdOutTotal; }
    uint32_t readShdOutMap(int i) { return gpuShdOutMap[i]; }
    uint32_t readTexBorder(int i) { return gpuTexBorder[i]; }
    uint32_t readTexDim(int i) { return gpuTexDim[i]; }
    uint32_t readTexParam(int i) { return gpuTexParam[i]; }
    uint32_t readTexAddr1(int i) { return gpuTexAddr1[i]; }
    uint32_t readTexType(int i) { return gpuTexType[i]; }
    uint32_t readCombSrc(int i) { return gpuCombSrc[i]; }
    uint32_t readCombOper(int i) { return gpuCombOper[i]; }
    uint32_t readCombMode(int i) { return gpuCombMode[i]; }
    uint32_t readCombColor(int i) { return gpuCombColor[i]; }
    uint32_t readCombBufCol() { return gpuCombBufCol; }
    uint32_t readBlendFunc() { return gpuBlendFunc; }
    uint32_t readBlendColor() { return gpuBlendColor; }
    uint32_t readAlphaTest() { return gpuAlphaTest; }
    uint32_t readDepcolMask() { return gpuDepcolMask; }
    uint32_t readColbufWrite() { return gpuColbufWrite; }
    uint32_t readDepbufWrite() { return gpuDepbufWrite; }
    uint32_t readDepbufFmt() { return gpuDepbufFmt; }
    uint32_t readColbufFmt() { return gpuColbufFmt; }
    uint32_t readDepbufLoc() { return gpuDepbufLoc; }
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
    uint32_t readVshNumAttr() { return gpuVshNumAttr; }
    uint32_t readPrimConfig() { return gpuPrimConfig; }
    uint32_t readPrimRestart() { return gpuPrimRestart; }
    uint32_t readVshBools() { return gpuVshBools; }
    uint32_t readVshInts(int i) { return gpuVshInts[i]; }
    uint32_t readVshEntry() { return gpuVshEntry; }
    uint32_t readVshAttrIdsL() { return gpuVshAttrIds >> 0; }
    uint32_t readVshAttrIdsH() { return gpuVshAttrIds >> 32; }
    uint32_t readVshOutMask() { return gpuVshOutMask; }

    void writeCfg11GpuCnt(uint32_t mask, uint32_t value);
    void writeMemsetDstAddr(int i, uint32_t mask, uint32_t value);
    void writeMemsetDstEnd(int i, uint32_t mask, uint32_t value);
    void writeMemsetData(int i, uint32_t mask, uint32_t value);
    void writeMemsetCnt(int i, uint32_t mask, uint32_t value);
    void writeMemcpySrcAddr(uint32_t mask, uint32_t value);
    void writeMemcpyDstAddr(uint32_t mask, uint32_t value);
    void writeMemcpyDispDstSize(uint32_t mask, uint32_t value);
    void writeMemcpyDispSrcSize(uint32_t mask, uint32_t value);
    void writeMemcpyFlags(uint32_t mask, uint32_t value);
    void writeMemcpyCnt(uint32_t mask, uint32_t value);
    void writeMemcpyTexSize(uint32_t mask, uint32_t value);
    void writeMemcpyTexSrcWidth(uint32_t mask, uint32_t value);
    void writeMemcpyTexDstWidth(uint32_t mask, uint32_t value);
    void writeIrqAck(int i, uint32_t mask, uint32_t value);
    void writeIrqCmp(int i, uint32_t mask, uint32_t value);
    void writeIrqMaskL(uint32_t mask, uint32_t value);
    void writeIrqMaskH(uint32_t mask, uint32_t value);
    void writeIrqAutostop(uint32_t mask, uint32_t value);

    template <int i> void writeIrqReq(uint32_t mask, uint32_t value);
    void writeFaceCulling(uint32_t mask, uint32_t value);
    void writeViewScaleH(uint32_t mask, uint32_t value);
    void writeViewStepH(uint32_t mask, uint32_t value);
    void writeViewScaleV(uint32_t mask, uint32_t value);
    void writeViewStepV(uint32_t mask, uint32_t value);
    void writeShdOutTotal(uint32_t mask, uint32_t value);
    template <int i> void writeShdOutMap(uint32_t mask, uint32_t value);
    template <int i> void writeTexBorder(uint32_t mask, uint32_t value);
    template <int i> void writeTexDim(uint32_t mask, uint32_t value);
    template <int i> void writeTexParam(uint32_t mask, uint32_t value);
    template <int i> void writeTexAddr1(uint32_t mask, uint32_t value);
    template <int i> void writeTexType(uint32_t mask, uint32_t value);
    template <int i> void writeCombSrc(uint32_t mask, uint32_t value);
    template <int i> void writeCombOper(uint32_t mask, uint32_t value);
    template <int i> void writeCombMode(uint32_t mask, uint32_t value);
    template <int i> void writeCombColor(uint32_t mask, uint32_t value);
    void writeCombBufCol(uint32_t mask, uint32_t value);
    void writeBlendFunc(uint32_t mask, uint32_t value);
    void writeBlendColor(uint32_t mask, uint32_t value);
    void writeAlphaTest(uint32_t mask, uint32_t value);
    void writeDepcolMask(uint32_t mask, uint32_t value);
    void writeColbufWrite(uint32_t mask, uint32_t value);
    void writeDepbufWrite(uint32_t mask, uint32_t value);
    void writeDepbufFmt(uint32_t mask, uint32_t value);
    void writeColbufFmt(uint32_t mask, uint32_t value);
    void writeDepbufLoc(uint32_t mask, uint32_t value);
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
    void writeAttrFixedIdx(uint32_t mask, uint32_t value);
    void writeAttrFixedData(uint32_t mask, uint32_t value);
    template <int i> void writeCmdSize(uint32_t mask, uint32_t value);
    template <int i> void writeCmdAddr(uint32_t mask, uint32_t value);
    template <int i> void writeCmdJump(uint32_t mask, uint32_t value);
    void writeVshNumAttr(uint32_t mask, uint32_t value);
    void writePrimConfig(uint32_t mask, uint32_t value);
    void writePrimRestart(uint32_t mask, uint32_t value);
    void writeVshBools(uint32_t mask, uint32_t value);
    template <int i> void writeVshInts(uint32_t mask, uint32_t value);
    void writeVshEntry(uint32_t mask, uint32_t value);
    void writeVshAttrIdsL(uint32_t mask, uint32_t value);
    void writeVshAttrIdsH(uint32_t mask, uint32_t value);
    void writeVshOutMask(uint32_t mask, uint32_t value);
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

    bool restart = false;
    bool fixedDirty = false;
    float fixedBase[16][4] = {};
    uint32_t attrFixedData[31][3] = {};
    uint8_t attrFixedIdx = 0;
    uint32_t vshFloatData[4] = {};
    uint16_t vshFloatIdx = 0;
    bool vshFloat32 = false;

    uint32_t cfg11GpuCnt = 0;
    uint32_t gpuMemsetDstAddr[2] = {};
    uint32_t gpuMemsetDstEnd[2] = {};
    uint32_t gpuMemsetData[2] = {};
    uint32_t gpuMemsetCnt[2] = {};
    uint32_t gpuMemcpySrcAddr = 0;
    uint32_t gpuMemcpyDstAddr = 0;
    uint32_t gpuMemcpyDispDstSize = 0;
    uint32_t gpuMemcpyDispSrcSize = 0;
    uint32_t gpuMemcpyFlags = 0;
    uint32_t gpuMemcpyCnt = 0;
    uint32_t gpuMemcpyTexSize = 0;
    uint32_t gpuMemcpyTexSrcWidth = 0;
    uint32_t gpuMemcpyTexDstWidth = 0;
    uint32_t gpuIrqCmp[16] = {};
    uint64_t gpuIrqMask = 0;
    uint64_t gpuIrqStat = 0;
    uint32_t gpuIrqAutostop = 0;

    uint32_t gpuIrqReq[16] = {};
    uint32_t gpuFaceCulling = 0;
    uint32_t gpuViewScaleH = 0;
    uint32_t gpuViewStepH = 0;
    uint32_t gpuViewScaleV = 0;
    uint32_t gpuViewStepV = 0;
    uint32_t gpuShdOutTotal = 0;
    uint32_t gpuShdOutMap[7] = {};
    uint32_t gpuTexBorder[3] = {};
    uint32_t gpuTexDim[3] = {};
    uint32_t gpuTexParam[3] = {};
    uint32_t gpuTexAddr1[3] = {};
    uint32_t gpuTexType[3] = {};
    uint32_t gpuCombSrc[6] = {};
    uint32_t gpuCombOper[6] = {};
    uint32_t gpuCombMode[6] = {};
    uint32_t gpuCombColor[6] = {};
    uint32_t gpuCombBufCol = 0;
    uint32_t gpuBlendFunc = 0;
    uint32_t gpuBlendColor = 0;
    uint32_t gpuAlphaTest = 0;
    uint32_t gpuDepcolMask = 0;
    uint32_t gpuColbufWrite = 0;
    uint32_t gpuDepbufWrite = 0;
    uint32_t gpuDepbufFmt = 0;
    uint32_t gpuColbufFmt = 0;
    uint32_t gpuDepbufLoc = 0;
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
    uint32_t gpuVshNumAttr = 0;
    uint32_t gpuPrimConfig = 0;
    uint32_t gpuPrimRestart = 0;
    uint32_t gpuVshBools = 0;
    uint32_t gpuVshInts[4] = {};
    uint32_t gpuVshEntry = 0;
    uint64_t gpuVshAttrIds = 0;
    uint32_t gpuVshOutMask = 0;
    uint32_t gpuVshCodeIdx = 0;
    uint32_t gpuVshDescIdx = 0;

    bool checkInterrupt(int i);
    static uint32_t flt24e7to32e8(uint32_t value);
    static uint32_t flt32e7to32e8(uint32_t value);

    void runCommands();
    void drawAttrIdx(uint32_t idx);
    void updateOutMap();
};
