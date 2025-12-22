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

#include <cmath>
#include <cstring>

#include "../core.h"
#include "gpu_render.h"

// Lookup table for vertex shader instructions
void (GpuShaderInterp::*GpuShaderInterp::vshInstrs[])(uint32_t) {
    &GpuShaderInterp::shdAdd, &GpuShaderInterp::shdDp3, &GpuShaderInterp::shdDp4, &GpuShaderInterp::shdDph, // 0x00-0x03
    &GpuShaderInterp::vshUnk, &GpuShaderInterp::shdEx2, &GpuShaderInterp::shdLg2, &GpuShaderInterp::vshUnk, // 0x04-0x07
    &GpuShaderInterp::shdMul, &GpuShaderInterp::shdSge, &GpuShaderInterp::shdSlt, &GpuShaderInterp::shdFlr, // 0x08-0x0B
    &GpuShaderInterp::shdMax, &GpuShaderInterp::shdMin, &GpuShaderInterp::shdRcp, &GpuShaderInterp::shdRsq, // 0x0C-0x0F
    &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, &GpuShaderInterp::shdMova, &GpuShaderInterp::shdMov, // 0x10-0x13
    &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, // 0x14-0x17
    &GpuShaderInterp::shdDphi, &GpuShaderInterp::vshUnk, &GpuShaderInterp::shdSgei, &GpuShaderInterp::shdSlti, // 0x18-0x1B
    &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, // 0x1C-0x1F
    &GpuShaderInterp::shdBreak, &GpuShaderInterp::shdNop, &GpuShaderInterp::shdEnd, &GpuShaderInterp::shdBreakc, // 0x20-0x23
    &GpuShaderInterp::shdCall, &GpuShaderInterp::shdCallc, &GpuShaderInterp::shdCallu, &GpuShaderInterp::shdIfu, // 0x24-0x27
    &GpuShaderInterp::shdIfc, &GpuShaderInterp::shdLoop, &GpuShaderInterp::vshUnk, &GpuShaderInterp::vshUnk, // 0x28-0x2B
    &GpuShaderInterp::shdJmpc, &GpuShaderInterp::shdJmpu, &GpuShaderInterp::shdCmp, &GpuShaderInterp::shdCmp, // 0x2C-0x2F
    &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, // 0x30-0x33
    &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, // 0x34-0x37
    &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, // 0x38-0x3B
    &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad // 0x3C-0x3F
};

// Lookup table for geometry shader instructions
void (GpuShaderInterp::*GpuShaderInterp::gshInstrs[])(uint32_t) {
    &GpuShaderInterp::shdAdd, &GpuShaderInterp::shdDp3, &GpuShaderInterp::shdDp4, &GpuShaderInterp::shdDph, // 0x00-0x03
    &GpuShaderInterp::gshUnk, &GpuShaderInterp::shdEx2, &GpuShaderInterp::shdLg2, &GpuShaderInterp::gshUnk, // 0x04-0x07
    &GpuShaderInterp::shdMul, &GpuShaderInterp::shdSge, &GpuShaderInterp::shdSlt, &GpuShaderInterp::shdFlr, // 0x08-0x0B
    &GpuShaderInterp::shdMax, &GpuShaderInterp::shdMin, &GpuShaderInterp::shdRcp, &GpuShaderInterp::shdRsq, // 0x0C-0x0F
    &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, &GpuShaderInterp::shdMova, &GpuShaderInterp::shdMov, // 0x10-0x13
    &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, // 0x14-0x17
    &GpuShaderInterp::shdDphi, &GpuShaderInterp::gshUnk, &GpuShaderInterp::shdSgei, &GpuShaderInterp::shdSlti, // 0x18-0x1B
    &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, &GpuShaderInterp::gshUnk, // 0x1C-0x1F
    &GpuShaderInterp::shdBreak, &GpuShaderInterp::shdNop, &GpuShaderInterp::shdEnd, &GpuShaderInterp::shdBreakc, // 0x20-0x23
    &GpuShaderInterp::shdCall, &GpuShaderInterp::shdCallc, &GpuShaderInterp::shdCallu, &GpuShaderInterp::shdIfu, // 0x24-0x27
    &GpuShaderInterp::shdIfc, &GpuShaderInterp::shdLoop, &GpuShaderInterp::gshEmit, &GpuShaderInterp::gshSetemit, // 0x28-0x2B
    &GpuShaderInterp::shdJmpc, &GpuShaderInterp::shdJmpu, &GpuShaderInterp::shdCmp, &GpuShaderInterp::shdCmp, // 0x2C-0x2F
    &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, // 0x30-0x33
    &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, &GpuShaderInterp::shdMadi, // 0x34-0x37
    &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, // 0x38-0x3B
    &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad, &GpuShaderInterp::shdMad // 0x3C-0x3F
};

GpuShaderInterp::GpuShaderInterp(GpuRender &gpuRender): gpuRender(gpuRender) {
    // Map the constant shader source and destination registers
    for (int i = 0x0; i < 0x10; i++)
        gshRegs[i] = gshInput[i], dstRegs[i] = shdOut[i];
    for (int i = 0x10; i < 0x20; i++)
        vshRegs[i] = gshRegs[i] = dstRegs[i] = shdTmp[i - 0x10];
    for (int i = 0x20; i < 0x80; i++)
        vshRegs[i] = vshFloats[i - 0x20], gshRegs[i] = gshFloats[i - 0x20];

    // Set up the vertex shader by default
    srcRegs = vshRegs, shdDesc = vshDesc;
    shdInts = vshInts, shdBools = vshBools;
}

void GpuShaderInterp::startList() {
    // Increment the vertex tag to invalidate cache and reset on overflow to avoid false positives
    if (++vtxTag) return;
    memset(vtxCache, 0, sizeof(vtxCache));
    vtxTag = 1;
}

void GpuShaderInterp::processVtx(float (*input)[4], uint32_t idx) {
    // Get an entry in the vertex cache, up to the first 256 in a list
    idx = std::min<uint32_t>(0x100, idx);
    VertexCache &cache = vtxCache[idx];

    // Skip processing the vertex if it's cached and not outdated
    if (idx < 0x100 && cache.tag == vtxTag && !gshInCount)
        return gpuRender.submitVertex(cache.vtx);

    // Update source registers and run the vertex shader
    for (int i = 0x0; i < 0x10; i++)
        vshRegs[i] = input[i];
    runShader<false>();

    // Cache and submit a vertex from the output if geometry shader is disabled
    if (!gshInCount) {
        cache.tag = vtxTag;
        buildVertex(cache.vtx);
        return gpuRender.submitVertex(cache.vtx);
    }

    // Copy vertex output to geometry input until it's full
    for (int i = 0; i < gshInCount; i++) {
        memcpy(gshInput[gshInMap[gshInTotal]], shdOut[i], 4 * sizeof(float));
        if (++gshInTotal >= 16 || gshInMap[gshInTotal] >= 16) goto geometry;
    }
    return;

geometry:
    // Run the geometry shader and then switch back to vertex
    srcRegs = gshRegs, shdDesc = gshDesc;
    shdInts = gshInts, shdBools = gshBools;
    runShader<true>();
    srcRegs = vshRegs, shdDesc = vshDesc;
    shdInts = vshInts, shdBools = vshBools;
    gshInTotal = 0;
}

template <bool geo> void GpuShaderInterp::runShader() {
    // Configure constants that depend on shader type
    const uint16_t mask = (geo ? 0xFFF : 0x1FF);
    void (GpuShaderInterp::**instrs)(uint32_t) = (geo ? gshInstrs : vshInstrs);
    uint32_t *code = (geo ? gshCode : vshCode);

    // Set the initial PC and stop address for the shader
    shdPc = (geo ? gshEntry : vshEntry);
    shdStop = (geo ? gshEnd : vshEnd);

    // Reset the general shader state
    memset(shdTmp, 0, sizeof(shdTmp));
    memset(shdOut, 0, sizeof(shdOut));
    memset(shdAddr, 0, sizeof(shdAddr));
    memset(shdCond, 0, sizeof(shdCond));
    ifStack = callStack = {};

    // Execute the current shader until completion
    while (shdPc != shdStop) {
        // Run an opcode and increment the program counter
        uint32_t opcode = code[shdPc & mask];
        uint16_t cmpPc = ++shdPc;
        (this->*instrs[opcode >> 26])(opcode);

        // Check the program counter against flow stacks and pop on match
        while (!callStack.empty() && !((cmpPc ^ callStack.front()) & mask))
            shdPc = (callStack.front() >> 16), callStack.pop_front(); // Multiple checks
        if (!ifStack.empty() && !((cmpPc ^ ifStack.front()) & mask))
            shdPc = (ifStack.front() >> 16), ifStack.pop_front(); // Single check

        // Adjust the loop counter and loop again or end on program counter match
        if (!loopStack.empty() && !((cmpPc ^ loopStack.front()) & mask)) {
            opcode = vshCode[((loopStack.front() >> 12) - 1) & mask];
            shdAddr[2] += shdInts[(opcode >> 22) & 0x3][2];
            shdPc = (loopStack.front() >> 12) & 0xFFF;
            if (loopStack.front() >> 24)
                loopStack[loopStack.size() - 1] -= BIT(24);
            else
                loopStack.pop_front();
        }
    }
}

void GpuShaderInterp::buildVertex(SoftVertex &vertex) {
    // Build a vertex using the shader output map
    vertex.x = shdOut[outMap[0x0][0]][outMap[0x0][1]];
    vertex.y = shdOut[outMap[0x1][0]][outMap[0x1][1]];
    vertex.z = shdOut[outMap[0x2][0]][outMap[0x2][1]];
    vertex.w = shdOut[outMap[0x3][0]][outMap[0x3][1]];
    vertex.r = shdOut[outMap[0x8][0]][outMap[0x8][1]];
    vertex.g = shdOut[outMap[0x9][0]][outMap[0x9][1]];
    vertex.b = shdOut[outMap[0xA][0]][outMap[0xA][1]];
    vertex.a = shdOut[outMap[0xB][0]][outMap[0xB][1]];
    vertex.s0 = shdOut[outMap[0xC][0]][outMap[0xC][1]];
    vertex.t0 = shdOut[outMap[0xD][0]][outMap[0xD][1]];
    vertex.s1 = shdOut[outMap[0xE][0]][outMap[0xE][1]];
    vertex.t1 = shdOut[outMap[0xF][0]][outMap[0xF][1]];
    vertex.s2 = shdOut[outMap[0x16][0]][outMap[0x16][1]];
    vertex.t2 = shdOut[outMap[0x17][0]][outMap[0x17][1]];
}

float GpuShaderInterp::mult(float a, float b) {
    // Multiply floats, or return 0 for any number multiplied by 0 (including infinity)
    return ((a == 0.0f && !std::isnan(b)) || (b == 0.0f && !std::isnan(a))) ? 0.0f : (a * b);
}

float *GpuShaderInterp::getSrc(uint8_t src, uint32_t desc, uint8_t idx) {
    // Apply relative addressing for uniform registers if enabled
    float *value = getRegs[getIdx++ & 0x3];
    if (idx && src >= 0x20) {
        src += shdAddr[idx - 1];
        if (src < 0x20 || src >= 0x80) {
            value[0] = value[1] = value[2] = value[3] = 0.0f;
            return value;
        }
    }

    // Get a source register with swizzling and negation based on OPDESC bits
    if (desc & BIT(4)) { // Negative
        value[0] = -srcRegs[src][(desc >> 11) & 0x3];
        value[1] = -srcRegs[src][(desc >> 9) & 0x3];
        value[2] = -srcRegs[src][(desc >> 7) & 0x3];
        value[3] = -srcRegs[src][(desc >> 5) & 0x3];
    }
    else { // Positive
        value[0] = srcRegs[src][(desc >> 11) & 0x3];
        value[1] = srcRegs[src][(desc >> 9) & 0x3];
        value[2] = srcRegs[src][(desc >> 7) & 0x3];
        value[3] = srcRegs[src][(desc >> 5) & 0x3];
    }
    return value;
}

void GpuShaderInterp::setDst(uint8_t dst, uint32_t desc, float *value) {
    // Set a destination register with masking based on OPDESC bits
    if (desc & BIT(3)) dstRegs[dst][0] = value[0];
    if (desc & BIT(2)) dstRegs[dst][1] = value[1];
    if (desc & BIT(1)) dstRegs[dst][2] = value[2];
    if (desc & BIT(0)) dstRegs[dst][3] = value[3];
}

void GpuShaderInterp::shdAdd(uint32_t opcode) {
    // Add each component of two source registers with each other
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = src1[i] + src2[i];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdDp3(uint32_t opcode) {
    // Calculate the dot product of two 3-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = mult(src1[0], src2[0]) + mult(src1[1], src2[1]) + mult(src1[2], src2[2]);
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdDp4(uint32_t opcode) {
    // Calculate the dot product of two 4-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = mult(src1[0], src2[0]) + mult(src1[1], src2[1]) + mult(src1[2], src2[2]) + mult(src1[3], src2[3]);
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdDph(uint32_t opcode) {
    // Calculate the dot product of a 3-component and a 4-component source register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = mult(src1[0], src2[0]) + mult(src1[1], src2[1]) + mult(src1[2], src2[2]) + src2[3];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdEx2(uint32_t opcode) {
    // Calculate the base-2 exponent of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[3] = src1[2] = src1[1] = src1[0] = exp2f(src1[0]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdLg2(uint32_t opcode) {
    // Calculate the base-2 logarithm of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[3] = src1[2] = src1[1] = src1[0] = log2f(src1[0]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdMul(uint32_t opcode) {
    // Multiply each component of two source registers with each other
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = mult(src1[i], src2[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdSge(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are greater or equal to source 2's
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] >= src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdSlt(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are less than source 2's
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] < src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdFlr(uint32_t opcode) {
    // Set the destination components to the floor of the source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = floorf(src1[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdMax(uint32_t opcode) {
    // Set the destination components to the maximum of two source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = std::max(src1[i], src2[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdMin(uint32_t opcode) {
    // Set the destination components to the minimum of two source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = std::min(src1[i], src2[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdRcp(uint32_t opcode) {
    // Calculate the reciprocal of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[0] = 1.0f / src1[0];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdRsq(uint32_t opcode) {
    // Calculate the reverse square root of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[0] = 1.0f / sqrtf(src1[0]);
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdMova(uint32_t opcode) {
    // Move a value from a source register to the X/Y address register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    if (desc & BIT(3)) shdAddr[0] = src1[0];
    if (desc & BIT(2)) shdAddr[1] = src1[1];
}

void GpuShaderInterp::shdMov(uint32_t opcode) {
    // Move a value from a source register to a destination register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdDphi(uint32_t opcode) {
    // Calculate the dot product of a 3-component and a 4-component source register (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    src1[0] = mult(src1[0], src2[0]) + mult(src1[1], src2[1]) + mult(src1[2], src2[2]) + src2[3];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdSgei(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are greater or equal to source 2's (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] >= src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdSlti(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are less than source 2's (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] < src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdBreak(uint32_t opcode) {
    // Jump to the end of a loop and finish it
    if (loopStack.empty()) return;
    shdPc = (loopStack.front() & 0xFFF);
    loopStack.pop_front();
}

void GpuShaderInterp::shdNop(uint32_t opcode) {
    // Do nothing
}

void GpuShaderInterp::shdEnd(uint32_t opcode) {
    // Finish shader execution
    shdPc = shdStop;
}

void GpuShaderInterp::shdBreakc(uint32_t opcode) {
    // Evaluate the X/Y condition values using reference values
    bool refX = (opcode & BIT(25)), refY = (opcode & BIT(24));
    switch ((opcode >> 22) & 0x3) {
        case 0x0: if (shdCond[0] == refX || shdCond[1] == refY) break; return; // OR
        case 0x1: if (shdCond[0] == refX && shdCond[1] == refY) break; return; // AND
        case 0x2: if (shdCond[0] == refX) break; return; // X
        default: if (shdCond[1] == refY) break; return; // Y
    }

    // If true, jump to the end of a loop and finish it
    if (loopStack.empty()) return;
    shdPc = (loopStack.front() & 0xFFF);
    loopStack.pop_front();
}

void GpuShaderInterp::shdCall(uint32_t opcode) {
    // Jump to an address and run a set number of opcodes
    uint16_t dst = (opcode >> 10) & 0xFFF;
    callStack.push_front((shdPc << 16) | (dst + (opcode & 0xFF)));
    if (callStack.size() > 4) callStack.pop_back(); // 4-deep
    shdPc = dst;
}

void GpuShaderInterp::shdCallc(uint32_t opcode) {
    // Evaluate the X/Y condition values using reference values
    bool refX = (opcode & BIT(25)), refY = (opcode & BIT(24));
    switch ((opcode >> 22) & 0x3) {
        case 0x0: if (shdCond[0] == refX || shdCond[1] == refY) break; return; // OR
        case 0x1: if (shdCond[0] == refX && shdCond[1] == refY) break; return; // AND
        case 0x2: if (shdCond[0] == refX) break; return; // X
        default: if (shdCond[1] == refY) break; return; // Y
    }

    // If true, jump to an address and run a set number of opcodes
    uint16_t dst = (opcode >> 10) & 0xFFF;
    callStack.push_front((shdPc << 16) | (dst + (opcode & 0xFF)));
    if (callStack.size() > 4) callStack.pop_back(); // 4-deep
    shdPc = dst;
}

void GpuShaderInterp::shdCallu(uint32_t opcode) {
    // If a uniform bool is true, jump to an address and run a set number of opcodes
    if (!shdBools[(opcode >> 22) & 0xF]) return;
    uint16_t dst = (opcode >> 10) & 0xFFF;
    callStack.push_front((shdPc << 16) | (dst + (opcode & 0xFF)));
    if (callStack.size() > 4) callStack.pop_back(); // 4-deep
    shdPc = dst;
}

void GpuShaderInterp::shdIfu(uint32_t opcode) {
    // If a uniform bool is true, run until an address then jump past it; otherwise jump to the address
    uint16_t dst = (opcode >> 10) & 0xFFF;
    if (shdBools[(opcode >> 22) & 0xF]) {
        ifStack.push_front(((dst + (opcode & 0xFF)) << 16) | dst);
        if (ifStack.size() > 8) ifStack.pop_back(); // 8-deep
    }
    else {
        shdPc = dst;
    }
}

void GpuShaderInterp::shdIfc(uint32_t opcode) {
    // Evaluate the X/Y condition values using reference values
    bool cond, refX = (opcode & BIT(25)), refY = (opcode & BIT(24));
    switch ((opcode >> 22) & 0x3) {
        case 0x0: cond = (shdCond[0] == refX || shdCond[1] == refY); break; // OR
        case 0x1: cond = (shdCond[0] == refX && shdCond[1] == refY); break; // AND
        case 0x2: cond = (shdCond[0] == refX); break; // X
        default: cond = (shdCond[1] == refY); break; // Y
    }

    // If true, run until an address then jump past it; otherwise jump to the address
    uint16_t dst = (opcode >> 10) & 0xFFF;
    if (cond) {
        ifStack.push_front(((dst + (opcode & 0xFF)) << 16) | dst);
        if (ifStack.size() > 8) ifStack.pop_back(); // 8-deep
    }
    else {
        shdPc = dst;
    }
}

void GpuShaderInterp::shdLoop(uint32_t opcode) {
    // Reload the loop counter and set up a loop between the next opcode and an address
    uint8_t *ints = shdInts[(opcode >> 22) & 0x3];
    shdAddr[2] = ints[1]; // Loop counter
    uint16_t dst = ((opcode >> 10) + 1) & 0xFFF;
    loopStack.push_front((ints[0] << 24) | ((shdPc & 0xFFF) << 12) | dst);
    if (loopStack.size() > 4) loopStack.pop_back(); // 4-deep
}

void GpuShaderInterp::gshEmit(uint32_t opcode) {
    // Build a vertex from the current output and check the primitive bit
    buildVertex(gshBuffer[emitParam >> 2]);
    if (~emitParam & BIT(1)) return;

    // Submit a triangle from the geometry buffer based on the winding bit
    if (emitParam & BIT(0)) {
        gpuRender.submitVertex(gshBuffer[2]);
        gpuRender.submitVertex(gshBuffer[1]);
        gpuRender.submitVertex(gshBuffer[0]);
    }
    else {
        gpuRender.submitVertex(gshBuffer[0]);
        gpuRender.submitVertex(gshBuffer[1]);
        gpuRender.submitVertex(gshBuffer[2]);
    }
}

void GpuShaderInterp::gshSetemit(uint32_t opcode) {
    // Update the geometry emit parameters
    emitParam = (opcode >> 22) & 0xF;
}

void GpuShaderInterp::shdJmpc(uint32_t opcode) {
    // Evaluate the X/Y condition values using reference values
    bool refX = (opcode & BIT(25)), refY = (opcode & BIT(24));
    switch ((opcode >> 22) & 0x3) {
        case 0x0: if (shdCond[0] == refX || shdCond[1] == refY) break; return; // OR
        case 0x1: if (shdCond[0] == refX && shdCond[1] == refY) break; return; // AND
        case 0x2: if (shdCond[0] == refX) break; return; // X
        default: if (shdCond[1] == refY) break; return; // Y
    }

    // If true, jump to an address
    shdPc = (opcode >> 10) & 0xFFF;
}

void GpuShaderInterp::shdJmpu(uint32_t opcode) {
    // If a uniform bool is true, jump to an address
    if (!shdBools[(opcode >> 22) & 0xF]) return;
    shdPc = (opcode >> 10) & 0xFFF;
}

void GpuShaderInterp::shdCmp(uint32_t opcode) {
    // Set the X/Y condition values by comparing X/Y of two source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 2; i++) {
        switch ((opcode >> (24 - i * 3)) & 0x7) {
            case 0x0: shdCond[i] = (src1[i] == src2[i]); continue; // EQ
            case 0x1: shdCond[i] = (src1[i] != src2[i]); continue; // NE
            case 0x2: shdCond[i] = (src1[i] < src2[i]); continue; // LT
            case 0x3: shdCond[i] = (src1[i] <= src2[i]); continue; // LE
            case 0x4: shdCond[i] = (src1[i] > src2[i]); continue; // GT
            case 0x5: shdCond[i] = (src1[i] >= src2[i]); continue; // GE
            default: shdCond[i] = true; continue;
        }
    }
}

void GpuShaderInterp::shdMadi(uint32_t opcode) {
    // Multiply each component of two source registers and add a third one (alternate)
    uint32_t desc = shdDesc[opcode & 0x1F];
    float *src1 = getSrc((opcode >> 17) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 12) & 0x1F, desc >> 9, 0);
    float *src3 = getSrc((opcode >> 5) & 0x7F, desc >> 18, (opcode >> 22) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = mult(src1[i], src2[i]) + src3[i];
    setDst((opcode >> 24) & 0x1F, desc, src1);
}

void GpuShaderInterp::shdMad(uint32_t opcode) {
    // Multiply each component of two source registers and add a third one
    uint32_t desc = shdDesc[opcode & 0x1F];
    float *src1 = getSrc((opcode >> 17) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 10) & 0x7F, desc >> 9, (opcode >> 22) & 0x3);
    float *src3 = getSrc((opcode >> 5) & 0x1F, desc >> 18, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = mult(src1[i], src2[i]) + src3[i];
    setDst((opcode >> 24) & 0x1F, desc, src1);
}

void GpuShaderInterp::vshUnk(uint32_t opcode) {
    // Handle an unknown vertex shader opcode
    LOG_CRIT("Unknown GPU vertex shader opcode: 0x%X\n", opcode);
}

void GpuShaderInterp::gshUnk(uint32_t opcode) {
    // Handle an unknown geometry shader opcode
    LOG_CRIT("Unknown GPU geometry shader opcode: 0x%X\n", opcode);
}

void GpuShaderInterp::setOutMap(uint8_t (*map)[2]) {
    // Set the map of shader outputs to fixed semantics
    memcpy(outMap, map, sizeof(outMap));
}

void GpuShaderInterp::setGshInMap(uint8_t *map) {
    // Set the map of vertex shader outputs to geometry shader inputs
    memcpy(gshInMap, map, sizeof(gshInMap));
}

void GpuShaderInterp::setVshEntry(uint16_t entry, uint16_t end) {
    // Set the vertex shader entry and end points
    vshEntry = entry;
    vshEnd = end;
}

void GpuShaderInterp::setVshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) {
    // Set a group of 3 vertex shader integers
    vshInts[i][0] = int0;
    vshInts[i][1] = int1;
    vshInts[i][2] = int2;
}

void GpuShaderInterp::setVshFloats(int i, float *floats) {
    // Set a group of 4 vertex shader floats
    memcpy(vshFloats[i], floats, 4 * sizeof(float));
}

void GpuShaderInterp::setGshEntry(uint16_t entry, uint16_t end) {
    // Set the geometry shader entry and end points
    gshEntry = entry;
    gshEnd = end;
}

void GpuShaderInterp::setGshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) {
    // Set a group of 3 geometry shader integers
    gshInts[i][0] = int0;
    gshInts[i][1] = int1;
    gshInts[i][2] = int2;
}

void GpuShaderInterp::setGshFloats(int i, float *floats) {
    // Set a group of 4 geometry shader floats
    memcpy(gshFloats[i], floats, 4 * sizeof(float));
}
