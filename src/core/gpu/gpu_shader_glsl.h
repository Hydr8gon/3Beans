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
    ShaderCache *current = nullptr;
    bool shaderDirty = false;

    uint8_t outMap[0x18][2] = {};
    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    uint16_t vshEntry = 0;
    uint16_t vshEnd = 0;

    static uint32_t calcCrc32(uint8_t *data, uint32_t size);
    void vshUnk(std::string &code, uint32_t opcode);
};
