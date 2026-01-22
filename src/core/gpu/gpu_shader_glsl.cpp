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
    &GpuShaderGlsl::shdAdd, &GpuShaderGlsl::shdDp3, &GpuShaderGlsl::shdDp4, &GpuShaderGlsl::shdDph, // 0x00-0x03
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::shdEx2, &GpuShaderGlsl::shdLg2, &GpuShaderGlsl::vshUnk, // 0x04-0x07
    &GpuShaderGlsl::shdMul, &GpuShaderGlsl::shdSge, &GpuShaderGlsl::shdSlt, &GpuShaderGlsl::shdFlr, // 0x08-0x0B
    &GpuShaderGlsl::shdMax, &GpuShaderGlsl::shdMin, &GpuShaderGlsl::shdRcp, &GpuShaderGlsl::shdRsq, // 0x0C-0x0F
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::shdMova, &GpuShaderGlsl::shdMov, // 0x10-0x13
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x14-0x17
    &GpuShaderGlsl::shdDphi, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::shdSgei, &GpuShaderGlsl::shdSlti, // 0x18-0x1B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x1C-0x1F
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::shdNop, &GpuShaderGlsl::shdEnd, &GpuShaderGlsl::vshUnk, // 0x20-0x23
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x24-0x27
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, // 0x28-0x2B
    &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::vshUnk, &GpuShaderGlsl::shdCmp, &GpuShaderGlsl::shdCmp, // 0x2C-0x2F
    &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, // 0x30-0x33
    &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, &GpuShaderGlsl::shdMadi, // 0x34-0x37
    &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad, // 0x38-0x3B
    &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad, &GpuShaderGlsl::shdMad // 0x3C-0x3F
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
    shdPc = vshEntry, shdStop = vshEnd;
    while (shdPc != shdStop) {
        uint32_t opcode = vshCode[shdPc];
        shdPc = (shdPc + 1) & 0x1FF;
        (this->*vshInstrs[opcode >> 26])(code, opcode);
    }

    // Emit code to map vertex shader outputs to fragment shader inputs
    code += "\ngl_Position = posScale * vec4(";
    code += "outRegs[" + std::to_string(outMap[0x0][0]) + "][" + std::to_string(outMap[0x0][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x1][0]) + "][" + std::to_string(outMap[0x1][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x2][0]) + "][" + std::to_string(outMap[0x2][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x3][0]) + "][" + std::to_string(outMap[0x3][1]) + "]);";
    code += "\nvtxColor = vec4(";
    code += "outRegs[" + std::to_string(outMap[0x8][0]) + "][" + std::to_string(outMap[0x8][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x9][0]) + "][" + std::to_string(outMap[0x9][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0xA][0]) + "][" + std::to_string(outMap[0xA][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0xB][0]) + "][" + std::to_string(outMap[0xB][1]) + "]);";
    code += "\nvtxCoordsS = vec3(";
    code += "outRegs[" + std::to_string(outMap[0xC][0]) + "][" + std::to_string(outMap[0xC][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0xE][0]) + "][" + std::to_string(outMap[0xE][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x16][0]) + "][" + std::to_string(outMap[0x16][1]) + "]);";
    code += "\nvtxCoordsT = vec3(";
    code += "outRegs[" + std::to_string(outMap[0xD][0]) + "][" + std::to_string(outMap[0xD][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0xF][0]) + "][" + std::to_string(outMap[0xF][1]) + "], ";
    code += "outRegs[" + std::to_string(outMap[0x17][0]) + "][" + std::to_string(outMap[0x17][1]) + "]);";
    code += "\n}";

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

std::string GpuShaderGlsl::getSrc(uint8_t src, uint32_t desc, uint8_t idx) {
    // Build the base for a source register using its sign and index
    std::string str = (desc & BIT(4)) ? "-" : "";
    if (src < 0x10)
        str += "inRegs[" + std::to_string(src) + "].";
    else if (src < 0x20)
        str += "tmpRegs[" + std::to_string(src - 0x10) + "].";
    else if (src < 0x80) {
        str += "floats[";
        if (idx) str += "addrReg[" + std::to_string(idx - 1) + "] + ";
        str += std::to_string(src - 0x20) + "].";
    }

    // Append swizzled components to the source string
    static const char comps[] = { 'x', 'y', 'z', 'w' };
    for (int i = 11; i >= 5; i -= 2)
        str += comps[(desc >> i) & 0x3];
    return str;
}

std::string GpuShaderGlsl::setDst(uint8_t dst, uint32_t desc, std::string value, bool single) {
    // Build the base for a destination register using its index
    std::string str, swiz;
    if (dst < 0x10)
        str = "outRegs[" + std::to_string(dst) + "].";
    else if (dst < 0x20)
        str = "tmpRegs[" + std::to_string(dst - 0x10) + "].";

    // Get swizzled components and output an assignment operation
    static const char comps[] = { 'w', 'z', 'y', 'x' };
    for (int i = 3; i >= 0; i--)
        if (desc & BIT(i)) swiz += comps[i];
    str += swiz + " = (" + value + ")";
    if (!single) str += "." + swiz;
    return str + ";\n";
}

void GpuShaderGlsl::shdAdd(std::string &code, uint32_t opcode) {
    // Emit code to add two source values together
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, s1 + " + " + s2);
}

void GpuShaderGlsl::shdDp3(std::string &code, uint32_t opcode) {
    // Emit code to get the dot product of two 3-component source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "dot((" + s1 + ").xyz, (" + s2 + ").xyz)", true);
}

void GpuShaderGlsl::shdDp4(std::string &code, uint32_t opcode) {
    // Emit code to get the dot product of two 4-component source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "dot(" + s1 + ", " + s2 + ")", true);
}

void GpuShaderGlsl::shdDph(std::string &code, uint32_t opcode) {
    // Emit code to get the dot product of a 3-component and a 4-component source value
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "dot(vec4((" + s1 + ").xyz, 0), " + s2 + ")", true);
}

void GpuShaderGlsl::shdEx2(std::string &code, uint32_t opcode) {
    // Emit code to get the base-2 exponent of a source value's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "exp2((" + s1 + ").x)", true);
}

void GpuShaderGlsl::shdLg2(std::string &code, uint32_t opcode) {
    // Emit code to get the base-2 logarithm of a source value's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "log2((" + s1 + ").x)", true);
}

void GpuShaderGlsl::shdMul(std::string &code, uint32_t opcode) {
    // Emit code to multiply two source values together
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, s1 + " * " + s2);
}

void GpuShaderGlsl::shdSge(std::string &code, uint32_t opcode) {
    // Emit code to perform a greater or equal comparison on two source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "vec4(greaterThanEqual(" + s1 + ", " + s2 + "))");
}

void GpuShaderGlsl::shdSlt(std::string &code, uint32_t opcode) {
    // Emit code to perform a less than comparison on two source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "vec4(lessThan(" + s1 + ", " + s2 + "))");
}

void GpuShaderGlsl::shdFlr(std::string &code, uint32_t opcode) {
    // Emit code to get the floor of a source value's components
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "floor(" + s1 + ")");
}

void GpuShaderGlsl::shdMax(std::string &code, uint32_t opcode) {
    // Emit code to get the maximum components of two source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "max(" + s1 + ", " + s2 + ")");
}

void GpuShaderGlsl::shdMin(std::string &code, uint32_t opcode) {
    // Emit code to get the minimum components of two source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string s2 = getSrc((opcode >> 7) & 0x1F, desc >> 9);
    code += setDst((opcode >> 21) & 0x1F, desc, "min(" + s1 + ", " + s2 + ")");
}

void GpuShaderGlsl::shdRcp(std::string &code, uint32_t opcode) {
    // Emit code to get the reciprocal of a source value's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "1 / (" + s1 + ").x", true);
}

void GpuShaderGlsl::shdRsq(std::string &code, uint32_t opcode) {
    // Emit code to get the reverse square root of a source value's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "1 / sqrt((" + s1 + ").x)", true);
}

void GpuShaderGlsl::shdMova(std::string &code, uint32_t opcode) {
    // Emit code to move a source value to the address register's X/Y components
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    std::string swiz = std::string((desc & BIT(3)) ? "x" : "") + ((desc & BIT(2)) ? "y" : "");
    code += "addrReg." + swiz + " = ivec4(" + s1 + ")." + swiz + ";\n";
}

void GpuShaderGlsl::shdMov(std::string &code, uint32_t opcode) {
    // Emit code to move a source value to a destination register
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, s1);
}

void GpuShaderGlsl::shdDphi(std::string &code, uint32_t opcode) {
    // Emit code to get the dot product of a 3-component and a 4-component source value (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 14) & 0x1F, desc);
    std::string s2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "dot(vec4((" + s1 + ").xyz, 0), " + s2 + ")", true);
}

void GpuShaderGlsl::shdSgei(std::string &code, uint32_t opcode) {
    // Emit code to perform a greater or equal comparison on two source values (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 14) & 0x1F, desc);
    std::string s2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "vec4(greaterThanEqual(" + s1 + ", " + s2 + "))");
}

void GpuShaderGlsl::shdSlti(std::string &code, uint32_t opcode) {
    // Emit code to perform a less than comparison on two source values (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = getSrc((opcode >> 14) & 0x1F, desc);
    std::string s2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    code += setDst((opcode >> 21) & 0x1F, desc, "vec4(lessThan(" + s1 + ", " + s2 + "))");
}

void GpuShaderGlsl::shdNop(std::string &code, uint32_t opcode) {
    // Do nothing
}

void GpuShaderGlsl::shdEnd(std::string &code, uint32_t opcode) {
    // Finish shader emission
    shdPc = shdStop;
}

void GpuShaderGlsl::shdCmp(std::string &code, uint32_t opcode) {
    // Emit code to compare the X/Y components of two source values
    uint32_t desc = shdDesc[opcode & 0x7F];
    std::string s1 = "(" + getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3) + ").";
    std::string s2 = "(" + getSrc((opcode >> 7) & 0x1F, desc >> 9) + ").";
    code += "condReg = bvec2(";
    for (int i = 0; i < 2; i++) {
        char x = (!i ? 'x' : 'y');
        switch ((opcode >> (24 - i * 3)) & 0x7) {
            case 0x0: code += s1 + x + " == " + s2 + x; break; // EQ
            case 0x1: code += s1 + x + " != " + s2 + x; break; // NE
            case 0x2: code += s1 + x + " < " + s2 + x; break; // LT
            case 0x3: code += s1 + x + " <= " + s2 + x; break; // LE
            case 0x4: code += s1 + x + " > " + s2 + x; break; // GT
            case 0x5: code += s1 + x + " >= " + s2 + x; break; // GE
            default: code += "true"; break;
        }
        code += !i ? ", " : ");\n";
    }
}

void GpuShaderGlsl::shdMadi(std::string &code, uint32_t opcode) {
    // Emit code to multiply two source values together and add a third one (alternate)
    uint32_t desc = shdDesc[opcode & 0x1F];
    std::string s1 = getSrc((opcode >> 17) & 0x1F, desc, 0);
    std::string s2 = getSrc((opcode >> 12) & 0x1F, desc >> 9, 0);
    std::string s3 = getSrc((opcode >> 5) & 0x7F, desc >> 18, (opcode >> 22) & 0x3);
    code += setDst((opcode >> 24) & 0x1F, desc, s1 + " * " + s2 + " + " + s3);
}

void GpuShaderGlsl::shdMad(std::string &code, uint32_t opcode) {
    // Emit code to multiply two source values together and add a third one
    uint32_t desc = shdDesc[opcode & 0x1F];
    std::string s1 = getSrc((opcode >> 17) & 0x1F, desc);
    std::string s2 = getSrc((opcode >> 10) & 0x7F, desc >> 9, (opcode >> 22) & 0x3);
    std::string s3 = getSrc((opcode >> 5) & 0x1F, desc >> 18);
    code += setDst((opcode >> 24) & 0x1F, desc, s1 + " * " + s2 + " + " + s3);
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
