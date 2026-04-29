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

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>

class Core;
class GpuRender;
class GpuShader;

enum PrimMode {
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

enum CombOper {
    OPER_SRC,
    OPER_1MSRC,
    OPER_SRCA,
    OPER_1MSRCA,
    OPER_SRCR,
    OPER_1MSRCR,
    OPER_SRCG,
    OPER_1MSRCG,
    OPER_SRCB,
    OPER_1MSRCB
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

enum StenOper {
    STEN_KEEP,
    STEN_ZERO,
    STEN_REPLACE,
    STEN_INCR,
    STEN_DECR,
    STEN_INVERT,
    STEN_INCWR,
    STEN_DECWR
};

enum BlendOper {
    BLND_ZERO,
    BLND_ONE,
    BLND_SRC,
    BLND_1MSRC,
    BLND_DST,
    BLND_1MDST,
    BLND_SRCA,
    BLND_1MSRCA,
    BLND_DSTA,
    BLND_1MDSTA,
    BLND_CONST,
    BLND_1MCON,
    BLND_CONSTA,
    BLND_1MCONA,
    BLND_ALPHSAT
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

enum LutInput {
    INPUT_NH,
    INPUT_VH,
    INPUT_NV,
    INPUT_LN,
    INPUT_LP,
    INPUT_CP,
    INPUT_UNK
};

enum LutId {
    LUT_D0 = 0x00,
    LUT_D1 = 0x01,
    LUT_FR = 0x03,
    LUT_RB = 0x04,
    LUT_RG = 0x05,
    LUT_RR = 0x06,
    LUT_SP0 = 0x08,
    LUT_SP1 = 0x09,
    LUT_SP2 = 0x0A,
    LUT_SP3 = 0x0B,
    LUT_SP4 = 0x0C,
    LUT_SP5 = 0x0D,
    LUT_SP6 = 0x0E,
    LUT_SP7 = 0x0F,
    LUT_DA0 = 0x10,
    LUT_DA1 = 0x11,
    LUT_DA2 = 0x12,
    LUT_DA3 = 0x13,
    LUT_DA4 = 0x14,
    LUT_DA5 = 0x15,
    LUT_DA6 = 0x16,
    LUT_DA7 = 0x17
};

enum GpuTaskType {
    TASK_CMD,
    TASK_FILL,
    TASK_COPY
};

struct GpuFillRegs {
    uint32_t dstAddr = 0;
    uint32_t dstEnd = 0;
    uint32_t data = 0;
    uint32_t cnt = 0;
};

struct GpuCopyRegs {
    uint32_t srcAddr = 0;
    uint32_t dstAddr = 0;
    uint32_t dispDstSize = 0;
    uint32_t dispSrcSize = 0;
    uint32_t flags = 0;
    uint32_t cnt = 0;
    uint32_t texSize = 0;
    uint32_t texSrcWidth = 0;
    uint32_t texDstWidth = 0;
};

struct GpuThreadTask {
    GpuTaskType type;
    void *data;
};

class Gpu {
public:
    Gpu(Core &core, std::function<void()> *contextFunc);
    ~Gpu();

    void syncRender();
    void endFill(int i);
    void endCopy();

    uint32_t readCfg11GpuCnt() { return cfg11GpuCnt; }
    uint32_t readFillDstAddr(int i) { return gpuFill[i].dstAddr; }
    uint32_t readFillDstEnd(int i) { return gpuFill[i].dstEnd; }
    uint32_t readFillData(int i) { return gpuFill[i].data; }
    uint32_t readFillCnt(int i) { return gpuFill[i].cnt; }
    uint32_t readCopySrcAddr() { return gpuCopy.srcAddr; }
    uint32_t readCopyDstAddr() { return gpuCopy.dstAddr; }
    uint32_t readCopyDispDstSize() { return gpuCopy.dispDstSize; }
    uint32_t readCopyDispSrcSize() { return gpuCopy.dispSrcSize; }
    uint32_t readCopyFlags() { return gpuCopy.flags; }
    uint32_t readCopyCnt() { return gpuCopy.cnt; }
    uint32_t readCopyTexSize() { return gpuCopy.texSize; }
    uint32_t readCopyTexSrcWidth() { return gpuCopy.texSrcWidth; }
    uint32_t readCopyTexDstWidth() { return gpuCopy.texDstWidth; }
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
    uint32_t readViewXY() { return gpuViewXY; }
    uint32_t readTexBorder(int i) { return gpuTexBorder[i]; }
    uint32_t readTexDim(int i) { return gpuTexDim[i]; }
    uint32_t readTexParam(int i) { return gpuTexParam[i]; }
    uint32_t readTexAddr1(int i) { return gpuTexAddr1[i]; }
    uint32_t readTexType(int i) { return gpuTexType[i]; }
    uint32_t readCombSrc(int i) { return gpuCombSrc[i]; }
    uint32_t readCombOper(int i) { return gpuCombOper[i]; }
    uint32_t readCombMode(int i) { return gpuCombMode[i]; }
    uint32_t readCombColor(int i) { return gpuCombColor[i]; }
    uint32_t readCombBufUpd() { return gpuCombBufUpd; }
    uint32_t readCombBufCol() { return gpuCombBufCol; }
    uint32_t readBlendFunc() { return gpuBlendFunc; }
    uint32_t readBlendColor() { return gpuBlendColor; }
    uint32_t readAlphaTest() { return gpuAlphaTest; }
    uint32_t readStencilTest() { return gpuStencilTest; }
    uint32_t readStencilOp() { return gpuStencilOp; }
    uint32_t readDepcolMask() { return gpuDepcolMask; }
    uint32_t readColbufWrite() { return gpuColbufWrite; }
    uint32_t readDepbufWrite() { return gpuDepbufWrite; }
    uint32_t readDepbufFmt() { return gpuDepbufFmt; }
    uint32_t readColbufFmt() { return gpuColbufFmt; }
    uint32_t readDepbufLoc() { return gpuDepbufLoc; }
    uint32_t readColbufLoc() { return gpuColbufLoc; }
    uint32_t readBufferDim() { return gpuBufferDim; }
    uint32_t readLightSpec0(int i) { return gpuLightSpec0[i]; }
    uint32_t readLightSpec1(int i) { return gpuLightSpec1[i]; }
    uint32_t readLightDiff(int i) { return gpuLightDiff[i]; }
    uint32_t readLightAmb(int i) { return gpuLightAmb[i]; }
    uint32_t readLightVecL(int i) { return gpuLightVecL[i]; }
    uint32_t readLightVecH(int i) { return gpuLightVecH[i]; }
    uint32_t readLightSpotL(int i) { return gpuLightSpotL[i]; }
    uint32_t readLightSpotH(int i) { return gpuLightSpotH[i]; }
    uint32_t readLightConfig(int i) { return gpuLightConfig[i]; }
    uint32_t readLightAtnBias(int i) { return gpuLightAtnBias[i]; }
    uint32_t readLightAtnScl(int i) { return gpuLightAtnScl[i]; }
    uint32_t readLightBaseAmb() { return gpuLightBaseAmb; }
    uint32_t readLightTotal() { return gpuLightTotal; }
    uint32_t readLightConfig0() { return gpuLightConfig0; }
    uint32_t readLightConfig1() { return gpuLightConfig1; }
    uint32_t readLightLutIdx() { return gpuLightLutIdx; }
    uint32_t readLightLutData();
    uint32_t readLightLutAbs() { return gpuLightLutAbs; }
    uint32_t readLightLutSel() { return gpuLightLutSel; }
    uint32_t readLightLutScl() { return gpuLightLutScl; }
    uint32_t readLightIds() { return gpuLightIds; }
    uint32_t readAttrBase() { return gpuAttrBase; }
    uint32_t readAttrFmtL() { return gpuAttrFmt >> 0; }
    uint32_t readAttrFmtH() { return gpuAttrFmt >> 32; }
    uint32_t readAttrOfs(int i) { return gpuAttrOfs[i]; }
    uint32_t readAttrCfgL(int i) { return gpuAttrCfg[i] >> 0; }
    uint32_t readAttrCfgH(int i) { return gpuAttrCfg[i] >> 32; }
    uint32_t readAttrIdxList() { return gpuAttrIdxList; }
    uint32_t readAttrNumVerts() { return gpuAttrNumVerts; }
    uint32_t readGshConfig() { return gpuGshConfig; }
    uint32_t readAttrFirstIdx() { return gpuAttrFirstIdx; }
    uint32_t readCmdSize(int i) { return gpuCmdSize[i]; }
    uint32_t readCmdAddr(int i) { return gpuCmdAddr[i]; }
    uint32_t readVshNumAttr() { return gpuVshNumAttr; }
    uint32_t readVshOutTotal() { return gpuVshOutTotal; }
    uint32_t readPrimConfig() { return gpuPrimConfig; }
    uint32_t readPrimRestart() { return gpuPrimRestart; }
    uint32_t readGshBools() { return gpuGshBools; }
    uint32_t readGshInts(int i) { return gpuGshInts[i]; }
    uint32_t readGshInputCfg() { return gpuGshInputCfg; }
    uint32_t readGshEntry() { return gpuGshEntry; }
    uint32_t readGshAttrIdsL() { return gpuGshAttrIds >> 0; }
    uint32_t readGshAttrIdsH() { return gpuGshAttrIds >> 32; }
    uint32_t readGshOutMask() { return gpuGshOutMask; }
    uint32_t readVshBools() { return gpuVshBools; }
    uint32_t readVshInts(int i) { return gpuVshInts[i]; }
    uint32_t readVshEntry() { return gpuVshEntry; }
    uint32_t readVshAttrIdsL() { return gpuVshAttrIds >> 0; }
    uint32_t readVshAttrIdsH() { return gpuVshAttrIds >> 32; }
    uint32_t readVshOutMask() { return gpuVshOutMask; }

    void writeCfg11GpuCnt(uint32_t mask, uint32_t value);
    void writeFillDstAddr(int i, uint32_t mask, uint32_t value);
    void writeFillDstEnd(int i, uint32_t mask, uint32_t value);
    void writeFillData(int i, uint32_t mask, uint32_t value);
    void writeFillCnt(int i, uint32_t mask, uint32_t value);
    void writeCopySrcAddr(uint32_t mask, uint32_t value);
    void writeCopyDstAddr(uint32_t mask, uint32_t value);
    void writeCopyDispDstSize(uint32_t mask, uint32_t value);
    void writeCopyDispSrcSize(uint32_t mask, uint32_t value);
    void writeCopyFlags(uint32_t mask, uint32_t value);
    void writeCopyCnt(uint32_t mask, uint32_t value);
    void writeCopyTexSize(uint32_t mask, uint32_t value);
    void writeCopyTexSrcWidth(uint32_t mask, uint32_t value);
    void writeCopyTexDstWidth(uint32_t mask, uint32_t value);
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
    void writeViewXY(uint32_t mask, uint32_t value);
    template <int i> void writeTexBorder(uint32_t mask, uint32_t value);
    template <int i> void writeTexDim(uint32_t mask, uint32_t value);
    template <int i> void writeTexParam(uint32_t mask, uint32_t value);
    template <int i> void writeTexAddr1(uint32_t mask, uint32_t value);
    template <int i> void writeTexType(uint32_t mask, uint32_t value);
    template <int i> void writeCombSrc(uint32_t mask, uint32_t value);
    template <int i> void writeCombOper(uint32_t mask, uint32_t value);
    template <int i> void writeCombMode(uint32_t mask, uint32_t value);
    template <int i> void writeCombColor(uint32_t mask, uint32_t value);
    void writeCombBufUpd(uint32_t mask, uint32_t value);
    void writeCombBufCol(uint32_t mask, uint32_t value);
    void writeBlendFunc(uint32_t mask, uint32_t value);
    void writeBlendColor(uint32_t mask, uint32_t value);
    void writeAlphaTest(uint32_t mask, uint32_t value);
    void writeStencilTest(uint32_t mask, uint32_t value);
    void writeStencilOp(uint32_t mask, uint32_t value);
    void writeDepcolMask(uint32_t mask, uint32_t value);
    void writeColbufWrite(uint32_t mask, uint32_t value);
    void writeDepbufWrite(uint32_t mask, uint32_t value);
    void writeDepbufFmt(uint32_t mask, uint32_t value);
    void writeColbufFmt(uint32_t mask, uint32_t value);
    void writeDepbufLoc(uint32_t mask, uint32_t value);
    void writeColbufLoc(uint32_t mask, uint32_t value);
    void writeBufferDim(uint32_t mask, uint32_t value);
    template <int i> void writeLightSpec0(uint32_t mask, uint32_t value);
    template <int i> void writeLightSpec1(uint32_t mask, uint32_t value);
    template <int i> void writeLightDiff(uint32_t mask, uint32_t value);
    template <int i> void writeLightAmb(uint32_t mask, uint32_t value);
    template <int i> void writeLightVecL(uint32_t mask, uint32_t value);
    template <int i> void writeLightVecH(uint32_t mask, uint32_t value);
    template <int i> void writeLightSpotL(uint32_t mask, uint32_t value);
    template <int i> void writeLightSpotH(uint32_t mask, uint32_t value);
    template <int i> void writeLightConfig(uint32_t mask, uint32_t value);
    template <int i> void writeLightAtnBias(uint32_t mask, uint32_t value);
    template <int i> void writeLightAtnScl(uint32_t mask, uint32_t value);
    void writeLightBaseAmb(uint32_t mask, uint32_t value);
    void writeLightTotal(uint32_t mask, uint32_t value);
    void writeLightConfig0(uint32_t mask, uint32_t value);
    void writeLightConfig1(uint32_t mask, uint32_t value);
    void writeLightLutIdx(uint32_t mask, uint32_t value);
    void writeLightLutData(uint32_t mask, uint32_t value);
    void writeLightLutAbs(uint32_t mask, uint32_t value);
    void writeLightLutSel(uint32_t mask, uint32_t value);
    void writeLightLutScl(uint32_t mask, uint32_t value);
    void writeLightIds(uint32_t mask, uint32_t value);
    void writeAttrBase(uint32_t mask, uint32_t value);
    void writeAttrFmtL(uint32_t mask, uint32_t value);
    void writeAttrFmtH(uint32_t mask, uint32_t value);
    template <int i> void writeAttrOfs(uint32_t mask, uint32_t value);
    template <int i> void writeAttrCfgL(uint32_t mask, uint32_t value);
    template <int i> void writeAttrCfgH(uint32_t mask, uint32_t value);
    void writeAttrIdxList(uint32_t mask, uint32_t value);
    void writeAttrNumVerts(uint32_t mask, uint32_t value);
    void writeGshConfig(uint32_t mask, uint32_t value);
    void writeAttrFirstIdx(uint32_t mask, uint32_t value);
    void writeAttrDrawArrays(uint32_t mask, uint32_t value);
    void writeAttrDrawElems(uint32_t mask, uint32_t value);
    void writeAttrFixedIdx(uint32_t mask, uint32_t value);
    void writeAttrFixedData(uint32_t mask, uint32_t value);
    template <int i> void writeCmdSize(uint32_t mask, uint32_t value);
    template <int i> void writeCmdAddr(uint32_t mask, uint32_t value);
    template <int i> void writeCmdJump(uint32_t mask, uint32_t value);
    void writeVshNumAttr(uint32_t mask, uint32_t value);
    void writeVshOutTotal(uint32_t mask, uint32_t value);
    void writePrimConfig(uint32_t mask, uint32_t value);
    void writePrimRestart(uint32_t mask, uint32_t value);
    void writeGshBools(uint32_t mask, uint32_t value);
    template <int i> void writeGshInts(uint32_t mask, uint32_t value);
    void writeGshInputCfg(uint32_t mask, uint32_t value);
    void writeGshEntry(uint32_t mask, uint32_t value);
    void writeGshAttrIdsL(uint32_t mask, uint32_t value);
    void writeGshAttrIdsH(uint32_t mask, uint32_t value);
    void writeGshOutMask(uint32_t mask, uint32_t value);
    void writeGshFloatIdx(uint32_t mask, uint32_t value);
    void writeGshFloatData(uint32_t mask, uint32_t value);
    void writeGshCodeIdx(uint32_t mask, uint32_t value);
    void writeGshCodeData(uint32_t mask, uint32_t value);
    void writeGshDescIdx(uint32_t mask, uint32_t value);
    void writeGshDescData(uint32_t mask, uint32_t value);
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
    Core &core;
    std::function<void()> *contextFunc;

    GpuRender *gpuRender = nullptr;
    GpuShader *gpuShader = nullptr;
    int renderType = -1, shaderType = -1;

    static void (Gpu::*cmdWrites[0x400])(uint32_t, uint32_t);
    static uint32_t maskTable[0x10];

    GpuThreadTask taskBuffer[0x1000];
    std::atomic<uint16_t> taskStart{0};
    std::atomic<uint16_t> taskEnd{0};
    std::atomic<bool> running{false};
    std::thread *thread;

    uint32_t cmdAddr = -1;
    uint32_t cmdEnd = 0;
    uint16_t curCmd = 0;

    bool shdMapDirty = false;
    bool fixedDirty = false;
    float fixedBase[16][4] = {};
    float shdInput[16][4] = {};
    uint32_t attrFixedData[31][3] = {};
    uint8_t attrFixedIdx = 0;

    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    float vshFloats[96 * 4] = {};
    uint32_t vshFloatData[4] = {};
    uint16_t vshFloatIdx = 0;
    bool vshFloat32 = false;

    uint32_t gshCode[0x1000] = {};
    uint32_t gshDesc[0x80] = {};
    float gshFloats[96 * 4] = {};
    uint32_t gshFloatData[4] = {};
    uint16_t gshFloatIdx = 0;
    bool gshFloat32 = false;

    uint32_t cfg11GpuCnt = 0;
    GpuFillRegs gpuFill[2];
    GpuCopyRegs gpuCopy;
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
    uint32_t gpuViewXY = 0;
    uint32_t gpuTexBorder[3] = {};
    uint32_t gpuTexDim[3] = {};
    uint32_t gpuTexParam[3] = {};
    uint32_t gpuTexAddr1[3] = {};
    uint32_t gpuTexType[3] = {};
    uint32_t gpuCombSrc[6] = {};
    uint32_t gpuCombOper[6] = {};
    uint32_t gpuCombMode[6] = {};
    uint32_t gpuCombColor[6] = {};
    uint32_t gpuCombBufUpd = 0;
    uint32_t gpuCombBufCol = 0;
    uint32_t gpuBlendFunc = 0;
    uint32_t gpuBlendColor = 0;
    uint32_t gpuAlphaTest = 0;
    uint32_t gpuStencilTest = 0;
    uint32_t gpuStencilOp = 0;
    uint32_t gpuDepcolMask = 0;
    uint32_t gpuColbufWrite = 0;
    uint32_t gpuDepbufWrite = 0;
    uint32_t gpuDepbufFmt = 0;
    uint32_t gpuColbufFmt = 0;
    uint32_t gpuDepbufLoc = 0;
    uint32_t gpuColbufLoc = 0;
    uint32_t gpuBufferDim = 0;
    uint32_t gpuLightSpec0[8] = {};
    uint32_t gpuLightSpec1[8] = {};
    uint32_t gpuLightDiff[8] = {};
    uint32_t gpuLightAmb[8] = {};
    uint32_t gpuLightVecL[8] = {};
    uint32_t gpuLightVecH[8] = {};
    uint32_t gpuLightSpotL[8] = {};
    uint32_t gpuLightSpotH[8] = {};
    uint32_t gpuLightConfig[8] = {};
    uint32_t gpuLightAtnBias[8] = {};
    uint32_t gpuLightAtnScl[8] = {};
    uint32_t gpuLightBaseAmb = 0;
    uint32_t gpuLightTotal = 0;
    uint32_t gpuLightConfig0 = 0;
    uint32_t gpuLightConfig1 = 0;
    uint32_t gpuLightLutIdx = 0;
    uint32_t gpuLightLutD0[0x100] = {};
    uint32_t gpuLightLutD1[0x100] = {};
    uint32_t gpuLightLutFr[0x100] = {};
    uint32_t gpuLightLutRb[0x100] = {};
    uint32_t gpuLightLutRg[0x100] = {};
    uint32_t gpuLightLutRr[0x100] = {};
    uint32_t gpuLightLutSp[8][0x100] = {};
    uint32_t gpuLightLutDa[8][0x100] = {};
    uint32_t gpuLightLutAbs = 0;
    uint32_t gpuLightLutSel = 0;
    uint32_t gpuLightLutScl = 0;
    uint32_t gpuLightIds = 0;
    uint32_t gpuAttrBase = 0;
    uint64_t gpuAttrFmt = 0;
    uint32_t gpuAttrOfs[12] = {};
    uint64_t gpuAttrCfg[12] = {};
    uint32_t gpuAttrIdxList = 0;
    uint32_t gpuAttrNumVerts = 0;
    uint32_t gpuGshConfig = 0;
    uint32_t gpuAttrFirstIdx = 0;
    uint32_t gpuCmdSize[2] = {};
    uint32_t gpuCmdAddr[2] = {};
    uint32_t gpuVshNumAttr = 0;
    uint32_t gpuVshOutTotal = 0;
    uint32_t gpuPrimConfig = 0;
    uint32_t gpuPrimRestart = 0;
    uint32_t gpuGshBools = 0;
    uint32_t gpuGshInts[4] = {};
    uint32_t gpuGshInputCfg = 0;
    uint32_t gpuGshEntry = 0;
    uint64_t gpuGshAttrIds = 0;
    uint32_t gpuGshOutMask = 0;
    uint32_t gpuGshCodeIdx = 0;
    uint32_t gpuGshDescIdx = 0;
    uint32_t gpuVshBools = 0;
    uint32_t gpuVshInts[4] = {};
    uint32_t gpuVshEntry = 0;
    uint64_t gpuVshAttrIds = 0;
    uint32_t gpuVshOutMask = 0;
    uint32_t gpuVshCodeIdx = 0;
    uint32_t gpuVshDescIdx = 0;

    void createRender();
    void destroyRender();

    void addThreadTask(GpuTaskType type, void *data);
    void runThreaded();
    bool checkInterrupt(int i);

    uint32_t getDispSrcOfs(uint32_t x, uint32_t y, uint32_t width);
    uint32_t getDispDstOfs(uint32_t x, uint32_t y, uint32_t width);
    void startFill(GpuFillRegs &regs);
    void startCopy(GpuCopyRegs &regs);

    static uint32_t flt16e5to32e8(uint16_t value);
    static uint32_t flt20e7to32e8(uint32_t value);
    static uint32_t flt24e7to32e8(uint32_t value);
    static uint32_t flt32e7to32e8(uint32_t value);

    void runCommands();
    void drawAttrIdx(uint32_t idx);
    void updateShdMaps();
    void updateLightMap();
    void updateLutMask();
    uint32_t *getLightLut(LutId id);
};
