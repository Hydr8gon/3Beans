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

#include <cstring>

#include "../defines.h"
#include "gpu_shader_glsl.h"
#include "gpu_render_ogl.h"

// Lookup table for vertex shader instructions
void (GpuShaderGlsl::*GpuShaderGlsl::vshInstrs[])(std::string&, uint32_t) {
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x00-0x03
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x04-0x07
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x08-0x0B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x0C-0x0F
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x10-0x13
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x14-0x17
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x18-0x1B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x1C-0x1F
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x20-0x23
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x24-0x27
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x28-0x2B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x2C-0x2F
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x30-0x33
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x34-0x37
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x38-0x3B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk // 0x3C-0x3F
};

enum ShaderLoc {
    LOC_IN_REGS = 0
};

const char *GpuShaderGlsl::vtxBase = R"(
    #version 330

    layout(location = 0) in vec4 inRegs[16];
    out vec4 vtxColor;
    out vec3 vtxCoordsS;
    out vec3 vtxCoordsT;

    uniform vec4 posScale;
    uniform vec4 floats[96];
    uniform ivec3 ints[4];
    uniform bool bools[16];

    vec4 tmpRegs[16];
    vec4 outRegs[16];
    ivec3 addrReg;
    bvec2 condReg;
    vec4 pos;

    void main() {
)";

GpuShaderGlsl::GpuShaderGlsl(GpuRenderOgl &gpuRender, float (*input)[4]): gpuRender(gpuRender), input(input) {
    // Create array and buffer objects for JITed shaders
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Configure input attributes for JITed shaders
    glVertexAttribPointer(LOC_IN_REGS, 16 * 4, GL_FLOAT, GL_FALSE, sizeof(float[16][4]), nullptr);
    glEnableVertexAttribArray(LOC_IN_REGS);
}

GpuShaderGlsl::~GpuShaderGlsl() {
    // Clean up resources that were generated
    for (int i = 0; i < shaderCache.size(); i++)
        glDeleteProgram(shaderCache[i].program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

uint32_t GpuShaderGlsl::calcCrc32(uint8_t *data, uint32_t size) {
    // Calculate a CRC32 for the given data
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 0x1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
    }
    return crc;
}

void GpuShaderGlsl::processVtx(uint32_t idx) {
    // Submit input and check if the shader should update
    if (!shaderDirty) return gpuRender.submitInput(input);
    gpuRender.flushVertices();
    gpuRender.submitInput(input);
    shaderDirty = false;

    // Calculate comparison values for the current shader
    ShaderCache s;
    s.codeCrc = calcCrc32((uint8_t*)vshCode, sizeof(vshCode));
    s.descCrc = calcCrc32((uint8_t*)vshDesc, sizeof(vshDesc));
    s.mapCrc = calcCrc32((uint8_t*)outMap, sizeof(outMap));
    s.entryEnd = (vshEntry << 16) | vshEnd;

    // Use a cached shader program if one is found
    for (int i = 0; i < shaderCache.size(); i++) {
        ShaderCache &c = shaderCache[i];
        if (c.codeCrc == s.codeCrc && c.descCrc == s.descCrc && c.mapCrc == s.mapCrc && c.entryEnd == s.entryEnd)
            return gpuRender.setProgram((current = &c)->program);
    }

    // Initialize a vertex shader and translate opcodes to GLSL
    std::string code = vtxBase;
    for (uint16_t pc = vshEntry; pc != vshEnd; pc = (pc + 1) & 0x1FF) {
        uint32_t opcode = vshCode[pc];
        (this->*vshInstrs[opcode >> 26])(code, opcode);
    }

    // Write code to map vertex shader outputs to fragment shader inputs
    code += "pos.x = outRegs[" + std::to_string(outMap[0x0][0]) + "][" + std::to_string(outMap[0x0][1]) + "];";
    code += "pos.y = outRegs[" + std::to_string(outMap[0x1][0]) + "][" + std::to_string(outMap[0x1][1]) + "];";
    code += "pos.z = outRegs[" + std::to_string(outMap[0x2][0]) + "][" + std::to_string(outMap[0x2][1]) + "];";
    code += "pos.w = outRegs[" + std::to_string(outMap[0x3][0]) + "][" + std::to_string(outMap[0x3][1]) + "];";
    code += "vtxColor.x = outRegs[" + std::to_string(outMap[0x8][0]) + "][" + std::to_string(outMap[0x8][1]) + "];";
    code += "vtxColor.y = outRegs[" + std::to_string(outMap[0x9][0]) + "][" + std::to_string(outMap[0x9][1]) + "];";
    code += "vtxColor.z = outRegs[" + std::to_string(outMap[0xA][0]) + "][" + std::to_string(outMap[0xA][1]) + "];";
    code += "vtxColor.w = outRegs[" + std::to_string(outMap[0xB][0]) + "][" + std::to_string(outMap[0xB][1]) + "];";
    code += "vtxCoordsS.x = outRegs[" + std::to_string(outMap[0xC][0]) + "][" + std::to_string(outMap[0xC][1]) + "];";
    code += "vtxCoordsT.x = outRegs[" + std::to_string(outMap[0xD][0]) + "][" + std::to_string(outMap[0xD][1]) + "];";
    code += "vtxCoordsS.y = outRegs[" + std::to_string(outMap[0xE][0]) + "][" + std::to_string(outMap[0xE][1]) + "];";
    code += "vtxCoordsT.y = outRegs[" + std::to_string(outMap[0xF][0]) + "][" + std::to_string(outMap[0xF][1]) + "];";
    code += "vtxCoordsS.z = outRegs[" + std::to_string(outMap[0x16][0]) + "][" + std::to_string(outMap[0x16][1]) + "];";
    code += "vtxCoordsT.z = outRegs[" + std::to_string(outMap[0x17][0]) + "][" + std::to_string(outMap[0x17][1]) + "];";
    code += "gl_Position = pos * posScale; }";

    // Compile and cache a program from the finished vertex code
    LOG_INFO("Caching GLSL shader with CRCs 0x%X, 0x%X, and 0x%X\n", s.codeCrc, s.descCrc, s.mapCrc);
    s.program = gpuRender.makeProgram(code.c_str());
    gpuRender.setProgram(s.program);
    s.floatsLoc = glGetUniformLocation(s.program, "floats");
    s.intsLoc = glGetUniformLocation(s.program, "ints");
    s.boolsLoc = glGetUniformLocation(s.program, "bools");
    shaderCache.push_back(s);
    current = &shaderCache[shaderCache.size() - 1];
}

void GpuShaderGlsl::vshUnk(std::string &code, uint32_t opcode) {
    // Handle an unknown vertex shader opcode
    LOG_CRIT("Unknown vertex shader GLSL JIT opcode: 0x%X\n", opcode);
}

void GpuShaderGlsl::setOutMap(uint8_t (*map)[2]) {
    // Set the map of shader outputs to fixed semantics
    memcpy(outMap, map, sizeof(outMap));
    shaderDirty = true;
}

void GpuShaderGlsl::setVshCode(int i, uint32_t value) {
    // Set one of the vertex shader opcodes
    vshCode[i] = value;
    shaderDirty = true;
}

void GpuShaderGlsl::setVshDesc(int i, uint32_t value) {
    // Set one of the vertex shader descriptors
    vshDesc[i] = value;
    shaderDirty = true;
}

void GpuShaderGlsl::setVshEntry(uint16_t entry, uint16_t end) {
    // Set the vertex shader entry and end points
    vshEntry = entry;
    vshEnd = end;
    shaderDirty = true;
}

void GpuShaderGlsl::setVshBool(int i, bool value) {
    // Update one of the vertex shader boolean uniforms
    if (current) glUniform1i(current->boolsLoc + i, value);
}

void GpuShaderGlsl::setVshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) {
    // Update one of the vertex shader integer uniforms
    if (current) glUniform3i(current->intsLoc + i, int0, int1, int2);
}

void GpuShaderGlsl::setVshFloats(int i, float *floats) {
    // Update one of the vertex shader float uniforms
    if (current) glUniform4fv(current->floatsLoc + i, 1, floats);
}
