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

#include <epoxy/gl.h>
#include <string>
#include <vector>

#include "gpu_shader.h"

class GpuRenderOgl;

struct ShaderCache {
    GLuint program;
    GLint floatsLoc, intsLoc, boolsLoc;
    uint32_t codeCrc, descCrc, mapCrc;
    uint32_t entryEnd;
};

struct ShaderFunc {
    std::string name;
    uint16_t entry, end;
};

class GpuShaderGlsl: public GpuShader {
public:
    GpuShaderGlsl(GpuRenderOgl &gpuRender, float (*input)[4]);
    ~GpuShaderGlsl();

    void startList() {}
    void processVtx(uint32_t idx = -1);

    void setOutMap(uint8_t (*map)[2]);
    void setGshInMap(uint8_t *map) {}
    void setGshInCount(uint8_t count) {}

    void setVshCode(int i, uint32_t value);
    void setVshDesc(int i, uint32_t value);
    void setVshEntry(uint16_t entry, uint16_t end);
    void setVshBool(int i, bool value);
    void setVshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2);
    void setVshFloats(int i, float *floats);

    void setGshCode(int i, uint32_t value) {}
    void setGshDesc(int i, uint32_t value) {}
    void setGshEntry(uint16_t entry, uint16_t end) {}
    void setGshBool(int i, bool value) {}
    void setGshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) {}
    void setGshFloats(int i, float *floats) {}

private:
    GpuRenderOgl &gpuRender;
    float (*input)[4];
    GLuint vao, vbo;

    static void (GpuShaderGlsl::*vshInstrs[0x40])(std::string&, uint32_t);
    static const char *vtxBase;

    std::vector<ShaderCache> shaderCache;
    std::vector<ShaderFunc> shaderFuncs;
    std::vector<uint32_t> ifStack, loopStack, jmpStack;

    ShaderCache *current = nullptr;
    uint32_t *shdDesc = vshDesc;
    uint16_t shdPc, shdStop;
    bool shaderDirty = false;

    uint8_t outMap[0x18][2] = {};
    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    uint16_t vshEntry = 0;
    uint16_t vshEnd = 0;
    GLint vshBools[16] = {};
    GLint vshInts[4][3] = {};
    float vshFloats[96][4] = {};

    static uint32_t calcCrc32(uint8_t *data, uint32_t size);
    static std::string getSrc(uint8_t src, uint32_t desc, uint8_t idx = 0);
    static std::string setDst(uint8_t dst, uint32_t desc, std::string value, bool single = false);

    void updateUniforms();
    void emitFuncBody(std::string &code, uint16_t entry, uint16_t end, bool full = true);

    void shdAdd(std::string &code, uint32_t opcode);
    void shdDp3(std::string &code, uint32_t opcode);
    void shdDp4(std::string &code, uint32_t opcode);
    void shdDph(std::string &code, uint32_t opcode);
    void shdEx2(std::string &code, uint32_t opcode);
    void shdLg2(std::string &code, uint32_t opcode);
    void shdMul(std::string &code, uint32_t opcode);
    void shdSge(std::string &code, uint32_t opcode);
    void shdSlt(std::string &code, uint32_t opcode);
    void shdFlr(std::string &code, uint32_t opcode);
    void shdMax(std::string &code, uint32_t opcode);
    void shdMin(std::string &code, uint32_t opcode);
    void shdRcp(std::string &code, uint32_t opcode);
    void shdRsq(std::string &code, uint32_t opcode);
    void shdMova(std::string &code, uint32_t opcode);
    void shdMov(std::string &code, uint32_t opcode);
    void shdDphi(std::string &code, uint32_t opcode);
    void shdSgei(std::string &code, uint32_t opcode);
    void shdSlti(std::string &code, uint32_t opcode);
    void shdBreak(std::string &code, uint32_t opcode);
    void shdNop(std::string &code, uint32_t opcode);
    void shdEnd(std::string &code, uint32_t opcode);
    void shdBreakc(std::string &code, uint32_t opcode);
    void shdCall(std::string &code, uint32_t opcode);
    void shdCallc(std::string &code, uint32_t opcode);
    void shdCallu(std::string &code, uint32_t opcode);
    void shdIfu(std::string &code, uint32_t opcode);
    void shdIfc(std::string &code, uint32_t opcode);
    void shdLoop(std::string &code, uint32_t opcode);
    void shdJmpc(std::string &code, uint32_t opcode);
    void shdJmpu(std::string &code, uint32_t opcode);
    void shdCmp(std::string &code, uint32_t opcode);
    void shdMadi(std::string &code, uint32_t opcode);
    void shdMad(std::string &code, uint32_t opcode);
    void vshUnk(std::string &code, uint32_t opcode);
};
