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
#include <deque>

class Core;

struct SoftVertex {
    float x, y, z, w;
    float r, g, b, a;
    float s0, s1, s2;
    float t0, t1, t2;
};

struct VertexCache {
    SoftVertex vtx;
    uint32_t tag;
};

class GpuRenderSoft {
public:
    GpuRenderSoft(Core *core);

    void runShader(float (*input)[4], PrimMode mode, uint32_t idx = -1);
    void startList();

    void writeVshCode(int i, uint32_t value) { vshCode[i] = value; }
    void writeVshDesc(int i, uint32_t value) { vshDesc[i] = value; }
    void setVshEntry(uint16_t entry, uint16_t end);
    void setVshBool(int i, bool value) { vshBools[i] = value; }
    void setVshInt(int i, int j, uint8_t value) { vshInts[i][j] = value; }
    void setVshFloat(int i, int j, float value) { vshFloats[i][j] = value; }

    void setTexAddr(int i, uint32_t address);
    void setTexDims(int i, uint16_t width, uint16_t height);
    void setTexBorder(int i, float r, float g, float b, float a);
    void setTexFmt(int i, TexFmt format);
    void setTexWrapS(int i, TexWrap wrap) { texWrapS[i] = wrap; }
    void setTexWrapT(int i, TexWrap wrap) { texWrapT[i] = wrap; }
    void setCombSrc(int i, int j, CombSrc src);
    void setCombOper(int i, int j, OperFunc oper);
    void setCombMode(int i, int j, CalcMode mode);
    void setCombColor(int i, float r, float g, float b, float a);
    void setCombBufColor(float r, float g, float b, float a);
    void setBlendOper(int i, OperFunc oper) { blendOpers[i] = oper; }
    void setBlendMode(int i, CalcMode mode) { blendModes[i] = mode; }
    void setBlendColor(float r, float g, float b, float a);
    void setAlphaFunc(TestFunc func) { alphaFunc = func; }
    void setAlphaValue(float value) { alphaValue = value; }
    void setStencilTest(TestFunc Func, bool enable);
    void setStencilOps(StenOper fail, StenOper depFail, StenOper depPass);
    void setStencilMasks(uint8_t bufMask, uint8_t refMask);
    void setStencilValue(uint8_t value) { stencilValue = value; }

    void setOutMap(uint8_t (*map)[2]);
    void setCullMode(CullMode mode) { cullMode = mode; }
    void setViewScaleH(float scale) { viewScaleH = scale; }
    void setViewStepH(float step) { viewStepH = step; }
    void setViewScaleV(float scale) { viewScaleV = scale; }
    void setViewStepV(float step) { viewStepV = step; }
    void setBufferDims(uint16_t width, uint16_t height, bool flip);
    void setColbufAddr(uint32_t address) { colbufAddr = address; }
    void setColbufFmt(ColbufFmt format) { colbufFmt = format; }
    void setColbufMask(uint8_t mask) { colbufMask = mask; }
    void setDepbufAddr(uint32_t address) { depbufAddr = address; }
    void setDepbufFmt(DepbufFmt format) { depbufFmt = format; }
    void setDepbufMask(uint8_t mask) { depbufMask = mask; }
    void setDepthFunc(TestFunc func) { depthFunc = func; }

private:
    Core *core;

    static void (GpuRenderSoft::*vshInstrs[0x40])(uint32_t);
    static int16_t etc1Tables[8][4];

    uint8_t outMap[0x18][2] = {};
    VertexCache vtxCache[0x101] = {};
    SoftVertex vertices[3] = {};
    uint32_t vtxCount = 0;
    uint32_t vtxTag = 1;
    PrimMode primMode = SAME_PRIM;
    CullMode cullMode = CULL_NONE;

    float rc[6], gc[6], bc[6], ac[6];
    float rt[3], gt[3], bt[3], at[3];
    int64_t lastU[3] = { -1, -1, -1 };
    int64_t lastV[3] = { -1, -1, -1 };
    uint8_t combStart = -1;
    uint8_t combMask = 0;

    float *srcRegs[0x80];
    float *dstRegs[0x20];
    uint32_t *shdDesc;
    uint8_t (*shdInts)[3];
    bool *shdBools;

    float shdTmp[16][4];
    float shdOut[8][4];
    int16_t shdAddr[3];
    bool shdCond[2];
    uint16_t shdPc;

    std::deque<uint32_t> loopStack;
    std::deque<uint32_t> ifStack;
    std::deque<uint32_t> callStack;
    float getRegs[4][4];
    uint8_t getIdx = 0;

    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    uint16_t vshEntry = 0;
    uint16_t vshEnd = 0;
    bool vshBools[16] = {};
    uint8_t vshInts[4][3] = {};
    float vshFloats[96][4] = {};

    uint32_t texAddrs[3] = {};
    uint16_t texWidths[3] = {};
    uint16_t texHeights[3] = {};
    float texBorders[3][4] = {};
    TexFmt texFmts[3] = {};
    TexWrap texWrapS[3] = {};
    TexWrap texWrapT[3] = {};
    CombSrc combSrcs[6][6] = {};
    OperFunc combOpers[6][6] = {};
    CalcMode combModes[6][2] = {};
    float combColors[6][4] = {};
    float combBufColor[4] = {};
    OperFunc blendOpers[4] = {};
    CalcMode blendModes[2] = {};
    float blendR = 0, blendG = 0, blendB = 0, blendA = 0;
    TestFunc alphaFunc = TEST_AL;
    float alphaValue = 0;

    float viewScaleH = 0;
    float viewStepH = 0;
    float viewScaleV = 0;
    float viewStepV = 0;
    bool flipY = false;
    uint16_t bufWidth = 0;
    uint16_t bufHeight = 0;
    uint32_t colbufAddr = 0;
    ColbufFmt colbufFmt = COL_UNK;
    uint8_t colbufMask = 0;
    uint32_t depbufAddr = 0;
    DepbufFmt depbufFmt = DEP_UNK;
    uint8_t depbufMask = 0;
    TestFunc depthFunc = TEST_AL;
    TestFunc stencilFunc = TEST_NV;
    StenOper stencilFail = STEN_KEEP;
    StenOper stenDepFail = STEN_KEEP;
    StenOper stenDepPass = STEN_KEEP;
    uint8_t stencilMasks[2] = {};
    uint8_t stencilValue = 0;
    bool stencilEnable = false;

    template <bool doX> static SoftVertex interpolate(SoftVertex &v1, SoftVertex &v2, float x1, float x, float x2);
    static SoftVertex intersect(SoftVertex &v1, SoftVertex &v2, float x1, float x2);
    uint8_t stencilOp(uint8_t value, StenOper oper);

    void getTexel(float &r, float &g, float &b, float &a, float s, float t, int i);
    void getSource(float &r, float &g, float &b, float &a, SoftVertex &v, int i, int j);
    void getCombine(float &r, float &g, float &b, float &a, SoftVertex &v, int i);

    void drawPixel(SoftVertex &p);
    void drawTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c);
    void clipTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c);

    float *getSrc(uint8_t src, uint32_t desc, uint8_t idx);
    void setDst(uint8_t dst, uint32_t desc, float *value);

    void shdAdd(uint32_t opcode);
    void shdDp3(uint32_t opcode);
    void shdDp4(uint32_t opcode);
    void shdDph(uint32_t opcode);
    void shdMul(uint32_t opcode);
    void shdSge(uint32_t opcode);
    void shdSlt(uint32_t opcode);
    void shdFlr(uint32_t opcode);
    void shdMax(uint32_t opcode);
    void shdMin(uint32_t opcode);
    void shdRcp(uint32_t opcode);
    void shdRsq(uint32_t opcode);
    void shdMova(uint32_t opcode);
    void shdMov(uint32_t opcode);
    void shdDphi(uint32_t opcode);
    void shdSgei(uint32_t opcode);
    void shdSlti(uint32_t opcode);
    void shdBreak(uint32_t opcode);
    void shdNop(uint32_t opcode);
    void shdEnd(uint32_t opcode);
    void shdBreakc(uint32_t opcode);
    void shdCall(uint32_t opcode);
    void shdCallc(uint32_t opcode);
    void shdCallu(uint32_t opcode);
    void shdIfu(uint32_t opcode);
    void shdIfc(uint32_t opcode);
    void shdLoop(uint32_t opcode);
    void shdJmpc(uint32_t opcode);
    void shdJmpu(uint32_t opcode);
    void shdCmp(uint32_t opcode);
    void shdMadi(uint32_t opcode);
    void shdMad(uint32_t opcode);
    void vshUnk(uint32_t opcode);
};
