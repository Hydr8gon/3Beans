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
#include "core.h"

// Lookup table for vertex shader instructions
void (GpuRenderSoft::*GpuRenderSoft::vshInstrs[0x40])(uint32_t) {
    &GpuRenderSoft::shdAdd, &GpuRenderSoft::shdDp3, &GpuRenderSoft::shdDp4, &GpuRenderSoft::vshUnk, // 0x00-0x03
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x04-0x07
    &GpuRenderSoft::shdMul, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x08-0x0B
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdRcp, &GpuRenderSoft::shdRsq, // 0x0C-0x0F
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdMov, // 0x10-0x13
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x14-0x17
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x18-0x1B
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x1C-0x1F
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdNop, &GpuRenderSoft::shdEnd, &GpuRenderSoft::vshUnk, // 0x20-0x23
    &GpuRenderSoft::shdCall, &GpuRenderSoft::shdCallc, &GpuRenderSoft::shdCallu, &GpuRenderSoft::shdIfu, // 0x24-0x27
    &GpuRenderSoft::shdIfc, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x28-0x2B
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdCmp, &GpuRenderSoft::shdCmp, // 0x2C-0x2F
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x30-0x33
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x34-0x37
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x38-0x3B
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk // 0x3C-0x3F
};

GpuRenderSoft::GpuRenderSoft(Core *core): core(core) {
    // Map the constant shader source and destination registers
    for (int i = 0x0; i < 0x10; i++)
        dstRegs[i] = shdOut[std::min(7, i)];
    for (int i = 0x10; i < 0x20; i++)
        srcRegs[i] = dstRegs[i] = shdTmp[i - 0x10];
    for (int i = 0x20; i < 0x80; i++)
        srcRegs[i] = vshFloats[i - 0x20];
    shdDesc = vshDesc;
    shdBools = vshBools;
}

void GpuRenderSoft::runShader(uint16_t entry, float (*input)[4]) {
    // Update source registers and reset the shader state
    for (int i = 0x0; i < 0x10; i++)
        srcRegs[i] = input[i];
    memset(shdTmp, 0, sizeof(shdTmp));
    memset(shdOut, 0, sizeof(shdOut));
    memset(shdCond, 0, sizeof(shdCond));
    ifStack = callStack = {};
    shdPc = entry;
    running = true;

    // Execute the vertex shader until completion
    while (running) {
        // Run an opcode and increment the program counter
        uint32_t opcode = vshCode[shdPc & 0x1FF];
        uint16_t cmpPc = ++shdPc;
        (this->*vshInstrs[opcode >> 26])(opcode);

        // Check the program counter against flow stacks and pop on match
        if (!ifStack.empty() && !((cmpPc ^ ifStack.front()) & 0x1FF))
            shdPc = (ifStack.front() >> 16), ifStack.pop_front();
        while (!callStack.empty() && !((cmpPc ^ callStack.front()) & 0x1FF))
            shdPc = (callStack.front() >> 16), callStack.pop_front();
    }

    // Draw a white pixel representing a vertex to an 8x8 framebuffer tile
    // TODO: actually render things properly
    float w = shdOut[outMap[0x3][0]][outMap[0x3][1]];
    if (w == 0) return;
    int x = shdOut[outMap[0x0][0]][outMap[0x0][1]] / w * viewScaleH + viewScaleH;
    int y = shdOut[outMap[0x1][0]][outMap[0x1][1]] / w * viewScaleV + viewScaleV;
    if (x < 0 || x >= bufWidth || y < 0 || y >= bufHeight) return;
    uint32_t tile = (y >> 3) * (bufWidth >> 3) + (x >> 3);
    if (colbufFmt == RGBA8)
        core->memory.write<uint32_t>(ARM11, colbufAddr + (tile << 8) + ((y & 0x7) << 5) + ((x & 0x7) << 2), -1);
    else if (colbufFmt != UNK_FMT)
        core->memory.write<uint16_t>(ARM11, colbufAddr + (tile << 7) + ((y & 0x7) << 4) + ((x & 0x7) << 1), -1);
}

float *GpuRenderSoft::getSrc(uint8_t src, uint32_t desc, uint8_t idx) {
    // Check if relative addressing should be used
    if (idx && src >= 0x20)
        LOG_CRIT("Unhandled GPU shader source read with address offset\n");

    // Get a source register with swizzling and negation based on OPDESC bits
    float *value = getRegs[getIdx++ & 0x3];
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

void GpuRenderSoft::setDst(uint8_t dst, uint32_t desc, float *value) {
    // Set a destination register with masking based on OPDESC bits
    if (desc & BIT(3)) dstRegs[dst][0] = value[0];
    if (desc & BIT(2)) dstRegs[dst][1] = value[1];
    if (desc & BIT(1)) dstRegs[dst][2] = value[2];
    if (desc & BIT(0)) dstRegs[dst][3] = value[3];
}

void GpuRenderSoft::shdAdd(uint32_t opcode) {
    // Add each component of two source registers with each other
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    float res[4];
    for (int i = 0; i < 4; i++)
        res[i] = src1[i] + src2[i];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdDp3(uint32_t opcode) {
    // Calculate the dot product of two 3-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    float res[4];
    res[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
    res[3] = res[2] = res[1] = res[0];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdDp4(uint32_t opcode) {
    // Calculate the dot product of two 4-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    float res[4];
    res[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + src1[3] * src2[3];
    res[3] = res[2] = res[1] = res[0];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdMul(uint32_t opcode) {
    // Multiply each component of two source registers with each other
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    float res[4];
    for (int i = 0; i < 4; i++)
        res[i] = src1[i] * src2[i];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdRcp(uint32_t opcode) {
    // Calculate the reciprocal of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float res[4];
    res[0] = 1.0f / src1[0];
    res[3] = res[2] = res[1] = res[0];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdRsq(uint32_t opcode) {
    // Calculate the reverse square root of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float res[4];
    res[0] = 1.0f / sqrtf(src1[0]);
    res[3] = res[2] = res[1] = res[0];
    setDst((opcode >> 21) & 0x1F, desc, res);
}

void GpuRenderSoft::shdMov(uint32_t opcode) {
    // Move a value from a source register to a destination register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdNop(uint32_t opcode) {
    // Do nothing
}

void GpuRenderSoft::shdEnd(uint32_t opcode) {
    // Finish shader execution
    running = false;
}

void GpuRenderSoft::shdCall(uint32_t opcode) {
    // Jump to an address and run a set number of opcodes
    uint16_t dst = (opcode >> 10) & 0xFFF;
    callStack.push_front((shdPc << 16) | (dst + (opcode & 0xFF)));
    if (callStack.size() > 4) callStack.pop_back(); // 4-deep
    shdPc = dst;
}

void GpuRenderSoft::shdCallc(uint32_t opcode) {
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

void GpuRenderSoft::shdCallu(uint32_t opcode) {
    // If a uniform bool is true, jump to an address and run a set number of opcodes
    if (!shdBools[(opcode >> 22) & 0xF]) return;
    uint16_t dst = (opcode >> 10) & 0xFFF;
    callStack.push_front((shdPc << 16) | (dst + (opcode & 0xFF)));
    if (callStack.size() > 4) callStack.pop_back(); // 4-deep
    shdPc = dst;
}

void GpuRenderSoft::shdIfu(uint32_t opcode) {
    // If a uniform bool is true, run until an address then jump past it; otherwise jump to the address
    uint16_t dst = (opcode >> 10) & 0xFFF;
    if (shdBools[(opcode >> 22) & 0xF]) {
        ifStack.push_front(((dst + (opcode & 0xFF)) << 16) | dst);
        if (ifStack.size() > 8) ifStack.pop_back(); // 8-deep
        return;
    }
    shdPc = dst;
}

void GpuRenderSoft::shdIfc(uint32_t opcode) {
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
        return;
    }
    shdPc = dst;
}

void GpuRenderSoft::shdCmp(uint32_t opcode) {
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

void GpuRenderSoft::vshUnk(uint32_t opcode) {
    // Handle an unknown vertex shader opcode
    LOG_CRIT("Unknown GPU vertex shader opcode: 0x%X\n", opcode);
}

void GpuRenderSoft::writeVshCode(uint16_t address, uint32_t value) {
    // Write a value to the vertex shader program buffer
    vshCode[address] = value;
}

void GpuRenderSoft::writeVshDesc(uint16_t address, uint32_t value) {
    // Write a value to the vertex operand descriptor buffer
    vshDesc[address] = value;
}

void GpuRenderSoft::setVshBool(int i, bool value) {
    // Set one of the vertex uniform boolean values
    vshBools[i] = value;
}

void GpuRenderSoft::setVshInt(int i, int j, uint8_t value) {
    // Set one of the vertex uniform integer values
    vshInts[i][j] = value;
}

void GpuRenderSoft::setVshFloat(int i, int j, float value) {
    // Set one of the vertex uniform float values
    vshFloats[i][j] = value;
}

void GpuRenderSoft::setOutMap(uint8_t (*map)[2]) {
    // Set the map of shader outputs to fixed semantics
    memcpy(outMap, map, sizeof(outMap));
}

void GpuRenderSoft::setViewScaleH(float scale) {
    // Set the viewport horizontal scale
    viewScaleH = scale;
}

void GpuRenderSoft::setViewScaleV(float scale) {
    // Set the viewport vertical scale
    viewScaleV = scale;
}

void GpuRenderSoft::setColbufAddr(uint32_t address) {
    // Set the color buffer address;
    colbufAddr = address;
}

void GpuRenderSoft::setColbufFmt(ColbufFmt format) {
    // Set the color buffer format
    colbufFmt = format;
}

void GpuRenderSoft::setBufferSize(uint16_t width, uint16_t height) {
    // Set the render buffer width and height
    bufWidth = width;
    bufHeight = height;
}
