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
    float x = 0, y = 0, z = 0, w = 0;
    float r = 0, g = 0, b = 0, a = 0;
};

class GpuRenderSoft {
public:
    GpuRenderSoft(Core *core);
    void runShader(float (*input)[4], PrimMode mode);

    void writeVshCode(uint16_t address, uint32_t value);
    void writeVshDesc(uint16_t address, uint32_t value);
    void setVshEntry(uint16_t entry, uint16_t end);
    void setVshBool(int i, bool value);
    void setVshInt(int i, int j, uint8_t value);
    void setVshFloat(int i, int j, float value);

    void setOutMap(uint8_t (*map)[2]);
    void setViewScaleH(float scale);
    void setViewStepH(float step);
    void setViewScaleV(float scale);
    void setViewStepV(float step);
    void setBufferDims(uint16_t width, uint16_t height, bool mirror);
    void setColbufAddr(uint32_t address);
    void setColbufFmt(ColbufFmt format);
    void setColbufMask(uint8_t mask);
    void setDepbufAddr(uint32_t address);
    void setDepbufFmt(DepbufFmt format);
    void setDepbufMask(uint8_t mask);
    void setDepthFunc(DepthFunc func);

private:
    Core *core;
    static void (GpuRenderSoft::*vshInstrs[0x40])(uint32_t);

    SoftVertex vertices[3];
    uint32_t vtxCount = 0;
    PrimMode curMode = SAME_PRIM;

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

    uint8_t outMap[0x18][2] = {};
    float viewScaleH = 0;
    float viewStepH = 0;
    float viewScaleV = 0;
    float viewStepV = 0;
    float signY = 0;
    uint16_t bufWidth = 0;
    uint16_t bufHeight = 0;
    uint32_t colbufAddr = 0;
    ColbufFmt colbufFmt = COL_UNK;
    uint8_t colbufMask = 0;
    uint32_t depbufAddr = 0;
    DepbufFmt depbufFmt = DEP_UNK;
    uint8_t depbufMask = 0;
    DepthFunc depthFunc = DEPTH_AL;

    static float interpolate(float v1, float v2, float x1, float x, float x2);
    static SoftVertex interpolate(SoftVertex &v1, SoftVertex &v2, float x1, float x, float x2);

    void drawPixel(SoftVertex &p);
    void drawTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c);

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
