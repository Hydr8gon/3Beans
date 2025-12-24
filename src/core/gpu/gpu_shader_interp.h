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

struct SourceDesc {
    uint8_t map[4];
    bool negate;
};

struct ShaderDesc {
    SourceDesc src[3];
    bool dstMask[4];
};

struct ShaderCode {
    void (GpuShaderInterp::*instr)(ShaderCode&);
    ShaderDesc *desc;
    float *dst;
    uint8_t src[3];
    uint8_t idx;
    uint32_t value;
};

class GpuShaderInterp {
public:
    GpuShaderInterp(GpuRender &gpuRender);

    void startList();
    void processVtx(float (*input)[4], uint32_t idx = -1);

    void setOutMap(uint8_t (*map)[2]);
    void setGshInMap(uint8_t *map);
    void setGshInCount(uint8_t count) { gshInCount = count; }

    void setVshCode(int i, uint32_t value) { cacheCode(vshCode[i], value, false); }
    void setVshDesc(int i, uint32_t value) { cacheDesc(vshDesc[i], value); }
    void setVshEntry(uint16_t entry, uint16_t end);
    void setVshBool(int i, bool value) { vshBools[i] = value; }
    void setVshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2);
    void setVshFloats(int i, float *floats);

    void setGshCode(int i, uint32_t value) { cacheCode(gshCode[i], value, true); }
    void setGshDesc(int i, uint32_t value) { cacheDesc(gshDesc[i], value); }
    void setGshEntry(uint16_t entry, uint16_t end);
    void setGshBool(int i, bool value) { gshBools[i] = value; }
    void setGshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2);
    void setGshFloats(int i, float *floats);

private:
    GpuRender &gpuRender;

    static void (GpuShaderInterp::*vshInstrs[0x40])(ShaderCode&);
    static void (GpuShaderInterp::*gshInstrs[0x40])(ShaderCode&);

    VertexCache vtxCache[0x101] = {};
    uint32_t vtxTag = 1;

    float **srcRegs;
    float *dstRegs[0x20];
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

    ShaderCode vshCode[0x200] = {};
    ShaderDesc vshDesc[0x80] = {};
    uint16_t vshEntry = 0;
    uint16_t vshEnd = 0;
    bool vshBools[16] = {};
    uint8_t vshInts[4][3] = {};
    float vshFloats[96][4] = {};
    float *vshRegs[0x80] = {};

    ShaderCode gshCode[0x1000] = {};
    ShaderDesc gshDesc[0x80] = {};
    uint16_t gshEntry = 0;
    uint16_t gshEnd = 0;
    bool gshBools[16] = {};
    uint8_t gshInts[4][3] = {};
    float gshFloats[96][4] = {};
    float *gshRegs[0x80] = {};

    void cacheCode(ShaderCode &code, uint32_t value, bool geo);
    void cacheDesc(ShaderDesc &desc, uint32_t value);

    template <bool geo> void runShader();
    void buildVertex(SoftVertex &vertex);

    static float mult(float a, float b);
    template <bool relative> float *getSrc(ShaderCode &op, int i);
    void setDst(ShaderCode &op, float *value);

    void shdAdd(ShaderCode &op);
    void shdDp3(ShaderCode &op);
    void shdDp4(ShaderCode &op);
    void shdDph(ShaderCode &op);
    void shdEx2(ShaderCode &op);
    void shdLg2(ShaderCode &op);
    void shdMul(ShaderCode &op);
    void shdSge(ShaderCode &op);
    void shdSlt(ShaderCode &op);
    void shdFlr(ShaderCode &op);
    void shdMax(ShaderCode &op);
    void shdMin(ShaderCode &op);
    void shdRcp(ShaderCode &op);
    void shdRsq(ShaderCode &op);
    void shdMova(ShaderCode &op);
    void shdMov(ShaderCode &op);
    void shdDphi(ShaderCode &op);
    void shdSgei(ShaderCode &op);
    void shdSlti(ShaderCode &op);
    void shdBreak(ShaderCode &op);
    void shdNop(ShaderCode &op);
    void shdEnd(ShaderCode &op);
    void shdBreakc(ShaderCode &op);
    void shdCall(ShaderCode &op);
    void shdCallc(ShaderCode &op);
    void shdCallu(ShaderCode &op);
    void shdIfu(ShaderCode &op);
    void shdIfc(ShaderCode &op);
    void shdLoop(ShaderCode &op);
    void gshEmit(ShaderCode &op);
    void gshSetemit(ShaderCode &op);
    void shdJmpc(ShaderCode &op);
    void shdJmpu(ShaderCode &op);
    void shdCmp(ShaderCode &op);
    void shdMadi(ShaderCode &op);
    void shdMad(ShaderCode &op);
    void vshUnk(ShaderCode &op);
    void gshUnk(ShaderCode &op);
};
