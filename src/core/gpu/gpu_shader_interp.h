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

class GpuRender;

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

class GpuShaderInterp {
public:
    GpuShaderInterp(GpuRender &gpuRender);

    void startList();
    void processVtx(float (*input)[4], uint32_t idx = -1);

    void setOutMap(uint8_t (*map)[2]);
    void setGshInMap(uint8_t *map);
    void setGshInCount(uint8_t count) { gshInCount = count; }

    void writeVshCode(int i, uint32_t value) { vshCode[i] = value; }
    void writeVshDesc(int i, uint32_t value) { vshDesc[i] = value; }
    void setVshEntry(uint16_t entry, uint16_t end);
    void setVshBool(int i, bool value) { vshBools[i] = value; }
    void setVshInt(int i, int j, uint8_t value) { vshInts[i][j] = value; }
    void setVshFloat(int i, int j, float value) { vshFloats[i][j] = value; }

    void writeGshCode(int i, uint32_t value) { gshCode[i] = value; }
    void writeGshDesc(int i, uint32_t value) { gshDesc[i] = value; }
    void setGshEntry(uint16_t entry, uint16_t end);
    void setGshBool(int i, bool value) { gshBools[i] = value; }
    void setGshInt(int i, int j, uint8_t value) { gshInts[i][j] = value; }
    void setGshFloat(int i, int j, float value) { gshFloats[i][j] = value; }

private:
    GpuRender &gpuRender;

    static void (GpuShaderInterp::*vshInstrs[0x40])(uint32_t);
    static void (GpuShaderInterp::*gshInstrs[0x40])(uint32_t);

    VertexCache vtxCache[0x101] = {};
    uint32_t vtxTag = 1;

    float **srcRegs;
    float *dstRegs[0x20];
    uint32_t *shdDesc;
    uint8_t (*shdInts)[3];
    bool *shdBools;

    float shdTmp[16][4];
    float shdOut[16][4];
    int16_t shdAddr[3];
    bool shdCond[2];
    uint16_t shdPc;
    uint16_t shdStop;

    std::deque<uint32_t> loopStack;
    std::deque<uint32_t> ifStack;
    std::deque<uint32_t> callStack;
    float getRegs[4][4];
    uint8_t getIdx = 0;

    float gshInput[16][4] = {};
    SoftVertex gshBuffer[4] = {};
    uint8_t gshInTotal = 0;
    uint8_t emitParam = 0;

    uint8_t outMap[0x18][2] = {};
    uint8_t gshInMap[0x10] = {};
    uint8_t gshInCount = 0;

    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    uint16_t vshEntry = 0;
    uint16_t vshEnd = 0;
    bool vshBools[16] = {};
    uint8_t vshInts[4][3] = {};
    float vshFloats[96][4] = {};
    float *vshRegs[0x80] = {};

    uint32_t gshCode[0x1000] = {};
    uint32_t gshDesc[0x80] = {};
    uint16_t gshEntry = 0;
    uint16_t gshEnd = 0;
    bool gshBools[16] = {};
    uint8_t gshInts[4][3] = {};
    float gshFloats[96][4] = {};
    float *gshRegs[0x80] = {};

    template <bool geo> void runShader();
    void buildVertex(SoftVertex &vertex);

    static float mult(float a, float b);
    float *getSrc(uint8_t src, uint32_t desc, uint8_t idx);
    void setDst(uint8_t dst, uint32_t desc, float *value);

    void shdAdd(uint32_t opcode);
    void shdDp3(uint32_t opcode);
    void shdDp4(uint32_t opcode);
    void shdDph(uint32_t opcode);
    void shdEx2(uint32_t opcode);
    void shdLg2(uint32_t opcode);
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
    void gshEmit(uint32_t opcode);
    void gshSetemit(uint32_t opcode);
    void shdJmpc(uint32_t opcode);
    void shdJmpu(uint32_t opcode);
    void shdCmp(uint32_t opcode);
    void shdMadi(uint32_t opcode);
    void shdMad(uint32_t opcode);
    void vshUnk(uint32_t opcode);
    void gshUnk(uint32_t opcode);
};
