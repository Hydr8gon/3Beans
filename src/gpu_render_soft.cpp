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
    &GpuRenderSoft::shdAdd, &GpuRenderSoft::shdDp3, &GpuRenderSoft::shdDp4, &GpuRenderSoft::shdDph, // 0x00-0x03
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x04-0x07
    &GpuRenderSoft::shdMul, &GpuRenderSoft::shdSge, &GpuRenderSoft::shdSlt, &GpuRenderSoft::shdFlr, // 0x08-0x0B
    &GpuRenderSoft::shdMax, &GpuRenderSoft::shdMin, &GpuRenderSoft::shdRcp, &GpuRenderSoft::shdRsq, // 0x0C-0x0F
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdMova, &GpuRenderSoft::shdMov, // 0x10-0x13
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x14-0x17
    &GpuRenderSoft::shdDphi, &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdSgei, &GpuRenderSoft::shdSlti, // 0x18-0x1B
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x1C-0x1F
    &GpuRenderSoft::shdBreak, &GpuRenderSoft::shdNop, &GpuRenderSoft::shdEnd, &GpuRenderSoft::shdBreakc, // 0x20-0x23
    &GpuRenderSoft::shdCall, &GpuRenderSoft::shdCallc, &GpuRenderSoft::shdCallu, &GpuRenderSoft::shdIfu, // 0x24-0x27
    &GpuRenderSoft::shdIfc, &GpuRenderSoft::shdLoop, &GpuRenderSoft::vshUnk, &GpuRenderSoft::vshUnk, // 0x28-0x2B
    &GpuRenderSoft::shdJmpc, &GpuRenderSoft::shdJmpu, &GpuRenderSoft::shdCmp, &GpuRenderSoft::shdCmp, // 0x2C-0x2F
    &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, // 0x30-0x33
    &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, &GpuRenderSoft::shdMadi, // 0x34-0x37
    &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad, // 0x38-0x3B
    &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad, &GpuRenderSoft::shdMad // 0x3C-0x3F
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
    shdInts = vshInts;
    shdBools = vshBools;
}

float GpuRenderSoft::interpolate(float v1, float v2, float x1, float x, float x2) {
    // Linearly interpolate a new value between the min and max values
    if (x <= x1) return v1;
    if (x >= x2) return v2;
    return v1 + (v2 - v1) * (x - x1) / (x2 - x1);
}

SoftVertex GpuRenderSoft::interpolate(SoftVertex &v1, SoftVertex &v2, float x1, float x, float x2) {
    // Interpolate all values in a vertex with perspective correction
    SoftVertex v;
    v.x = interpolate(v1.x, v2.x, x1, x, x2);
    v.y = interpolate(v1.y, v2.y, x1, x, x2);
    v.z = interpolate(v1.z, v2.z, x1, x, x2);
    v.w = interpolate(v1.w, v2.w, x1, x, x2);
    v.r = interpolate(v1.r * v1.w, v2.r * v2.w, x1, x, x2) / v.w;
    v.g = interpolate(v1.g * v1.w, v2.g * v2.w, x1, x, x2) / v.w;
    v.b = interpolate(v1.b * v1.w, v2.b * v2.w, x1, x, x2) / v.w;
    v.a = interpolate(v1.a * v1.w, v2.a * v2.w, x1, x, x2) / v.w;
    return v;
}

void GpuRenderSoft::drawPixel(SoftVertex &p) {
    // Convert coordinates to screen space and check bounds
    int x = p.x * viewScaleH + viewScaleH;
    int y = p.y * signY * viewScaleV + viewScaleV;
    if (x < 0 || x >= bufWidth || y < 0 || y >= bufHeight) return;
    uint32_t val, ofs = (((y >> 3) * (bufWidth >> 3) + (x >> 3)) << 6) + ((y & 0x7) << 3) + (x & 0x7); // 8x8 tiles

    // Read a depth value to compare with based on buffer format
    float depth = 0.0f;
    switch (depbufFmt) {
    case DEP16:
        val = core->memory.read<uint16_t>(ARM11, depbufAddr + ofs * 2);
        depth = float(val) / 0xFFFF;
        break;
    case DEP24:
        val = core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 0) << 0;
        val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 1) << 8;
        val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 2) << 16;
        depth = float(val) / 0xFFFFFF;
        break;
    case DEP24STN8:
        val = core->memory.read<uint32_t>(ARM11, depbufAddr + ofs * 4) & 0xFFFFFF;
        depth = float(val) / 0xFFFFFF;
        break;
    }

    // Compare the incoming depth value with the existing one
    switch (depthFunc) {
        case DEPTH_NV: return;
        case DEPTH_AL: break;
        case DEPTH_EQ: if (p.z == depth) break; return;
        case DEPTH_NE: if (p.z != depth) break; return;
        case DEPTH_LT: if (p.z < depth) break; return;
        case DEPTH_LE: if (p.z <= depth) break; return;
        case DEPTH_GT: if (p.z > depth) break; return;
        case DEPTH_GE: if (p.z >= depth) break; return;
    }

    // Store the incoming depth value based on buffer format if enabled
    if (depbufMask & BIT(1)) {
        switch (depbufFmt) {
        case DEP16:
            val = std::min<int>(0xFFFF, std::max<int>(0, p.z * 0xFFFF));
            core->memory.write<uint16_t>(ARM11, depbufAddr + ofs * 2, val);
            break;
        case DEP24:
            val = std::min<int>(0xFFFFFF, std::max<int>(0, p.z * 0xFFFFFF));
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 0, val >> 0);
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 1, val >> 8);
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 2, val >> 16);
            break;
        case DEP24STN8:
            val = std::min<int>(0xFFFFFF, std::max<int>(0, p.z * 0xFFFFFF));
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3) << 24;
            core->memory.write<uint32_t>(ARM11, depbufAddr + ofs * 4, val);
            break;
        }
    }

    // Store the incoming color values based on buffer format if enabled
    if (!colbufMask) return; // TODO: use individual components
    switch (colbufFmt) {
    case RGBA8:
        val = (int(p.r * 255) << 24) | (int(p.g * 255) << 16) | (int(p.b * 255) << 8) | int(p.a * 255);
        return core->memory.write<uint32_t>(ARM11, colbufAddr + ofs * 4, val);
    case RGB565:
        val = (int(p.r * 31) << 11) | (int(p.g * 63) << 5) | int(p.b * 31);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    case RGB5A1:
        val = (int(p.r * 31) << 11) | (int(p.g * 31) << 6) | (int(p.b * 31) << 1) | (p.a > 0);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    case RGBA4:
        val = (int(p.r * 15) << 12) | (int(p.g * 15) << 8) | (int(p.b * 15) << 4) | int(p.a * 15);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    }
}

void GpuRenderSoft::drawTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c) {
    // Sort the vertices by increasing Y-coordinates
    SoftVertex *v[3] = { &a, &b, &c };
    if (v[0]->y > v[1]->y) std::swap(v[0], v[1]);
    if (v[0]->y > v[2]->y) std::swap(v[0], v[2]);
    if (v[1]->y > v[2]->y) std::swap(v[1], v[2]);

    // Ignore triangles that have out-of-bounds vertices
    // TODO: actually implement clipping
    for (int i = 0; i < 3; i++)
        if (fabsf(v[i]->x) >= 1.0f || fabsf(v[i]->y) >= 1.0f) return;

    // Draw the pixels of a triangle by interpolating between X and Y bounds
    if (viewStepH <= 0 || viewStepV <= 0) return;
    float y1 = std::max(-1.0f, v[0]->y), y2 = std::min(1.0f, v[2]->y);
    for (float y = y1; y < y2; y += viewStepV) {
        int r = (y >= v[1]->y) ? 1 : 0;
        SoftVertex vl = interpolate(*v[0], *v[2], v[0]->y, y, v[2]->y);
        SoftVertex vr = interpolate(*v[r], *v[r + 1], v[r]->y, y, v[r + 1]->y);
        if (vl.x > vr.x) std::swap(vl, vr);
        float x1 = std::max(-1.0f, vl.x), x2 = std::min(1.0f, vr.x);
        for (float x = x1; x < x2; x += viewStepH) {
            SoftVertex vm = interpolate(vl, vr, vl.x, x, vr.x);
            drawPixel(vm);
        }
    }
}

void GpuRenderSoft::runShader(float (*input)[4], PrimMode mode) {
    // Update source registers and reset the shader state
    for (int i = 0x0; i < 0x10; i++)
        srcRegs[i] = input[i];
    memset(shdTmp, 0, sizeof(shdTmp));
    memset(shdOut, 0, sizeof(shdOut));
    memset(shdAddr, 0, sizeof(shdAddr));
    memset(shdCond, 0, sizeof(shdCond));
    ifStack = callStack = {};
    shdPc = vshEntry;

    // Execute the vertex shader until completion
    while (shdPc != vshEnd) {
        // Run an opcode and increment the program counter
        uint32_t opcode = vshCode[shdPc & 0x1FF];
        uint16_t cmpPc = ++shdPc;
        (this->*vshInstrs[opcode >> 26])(opcode);

        // Check the program counter against flow stacks and pop on match
        while (!callStack.empty() && !((cmpPc ^ callStack.front()) & 0x1FF))
            shdPc = (callStack.front() >> 16), callStack.pop_front(); // Multiple checks
        if (!ifStack.empty() && !((cmpPc ^ ifStack.front()) & 0x1FF))
            shdPc = (ifStack.front() >> 16), ifStack.pop_front(); // Single check

        // Adjust the loop counter and loop again or end on program counter match
        if (!loopStack.empty() && !((cmpPc ^ loopStack.front()) & 0x1FF)) {
            opcode = vshCode[((loopStack.front() >> 12) - 1) & 0x1FF];
            shdAddr[2] += shdInts[(opcode >> 22) & 0x3][2];
            if (loopStack.front() >> 24) {
                loopStack[loopStack.size() - 1] -= BIT(24);
                shdPc = (loopStack.front() >> 12) & 0xFFF;
            }
            else {
                loopStack.pop_front();
            }
        }
    }

    // Build a vertex using the shader output map
    SoftVertex vertex;
    vertex.w = shdOut[outMap[0x3][0]][outMap[0x3][1]];
    vertex.x = shdOut[outMap[0x0][0]][outMap[0x0][1]] / vertex.w;
    vertex.y = shdOut[outMap[0x1][0]][outMap[0x1][1]] / vertex.w;
    vertex.z = shdOut[outMap[0x2][0]][outMap[0x2][1]] / vertex.w;
    vertex.r = std::min(1.0f, std::max(0.0f, shdOut[outMap[0x8][0]][outMap[0x8][1]]));
    vertex.g = std::min(1.0f, std::max(0.0f, shdOut[outMap[0x9][0]][outMap[0x9][1]]));
    vertex.b = std::min(1.0f, std::max(0.0f, shdOut[outMap[0xA][0]][outMap[0xA][1]]));
    vertex.a = std::min(1.0f, std::max(0.0f, shdOut[outMap[0xB][0]][outMap[0xB][1]]));

    // Reset the vertex count if the primitive mode changed
    if (mode != SAME_PRIM) {
        vtxCount = 0;
        curMode = mode;
    }

    // Build a triangle based on the current primitive mode
    switch (curMode) {
    case TRIANGLES:
    case GEO_PRIM: // TODO?
        // Draw separate triangles every 3 vertices
        vertices[vtxCount] = vertex;
        if (++vtxCount < 3) return;
        vtxCount = 0;
        return drawTriangle(vertices[0], vertices[1], vertices[2]);

    case TRI_STRIPS:
        // Draw triangles in strips that reuse the last 2 vertices
        switch (vtxCount++) {
        case 0: case 1:
            vertices[vtxCount - 1] = vertex;
            return;
        case 2: // v0, v1, v2
            vertices[2] = vertex;
            return drawTriangle(vertices[0], vertices[1], vertices[2]);
        case 3: // v2, v1, v3
            vertices[0] = vertex;
            return drawTriangle(vertices[2], vertices[1], vertices[0]);
        case 4: // v2, v3, v4
            vertices[1] = vertex;
            return drawTriangle(vertices[2], vertices[0], vertices[1]);
        case 5: // v4, v3, v5
            vertices[2] = vertex;
            return drawTriangle(vertices[1], vertices[0], vertices[2]);
        case 6: // v4, v5, v6
            vertices[0] = vertex;
            return drawTriangle(vertices[1], vertices[2], vertices[0]);
        default: // v6, v5, v7
            vertices[1] = vertex;
            vtxCount = 2;
            return drawTriangle(vertices[0], vertices[2], vertices[1]);
        }

    case TRI_FANS:
        // Draw triangles in a fan that reuses the first vertex
        switch (vtxCount++) {
        case 0: case 1:
            vertices[vtxCount - 1] = vertex;
            return;
        case 2: // v0, v1, v2
            vertices[2] = vertex;
            return drawTriangle(vertices[0], vertices[1], vertices[2]);
        default: // v0, v2, v3
            vertices[1] = vertex;
            vtxCount = 2;
            return drawTriangle(vertices[0], vertices[2], vertices[1]);
        }

    case SAME_PRIM:
        // Catch vertices sent without setting primitive mode
        LOG_WARN("Vertex sent to GPU without setting primitive mode\n");
        return;
    }
}

float *GpuRenderSoft::getSrc(uint8_t src, uint32_t desc, uint8_t idx) {
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
    for (int i = 0; i < 4; i++)
        src1[i] = src1[i] + src2[i];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdDp3(uint32_t opcode) {
    // Calculate the dot product of two 3-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdDp4(uint32_t opcode) {
    // Calculate the dot product of two 4-component source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + src1[3] * src2[3];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdDph(uint32_t opcode) {
    // Calculate the dot product of a 3-component and a 4-component source register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    src1[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + src2[3];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdMul(uint32_t opcode) {
    // Multiply each component of two source registers with each other
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = src1[i] * src2[i];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdSge(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are greater or equal to source 2's
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] >= src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdSlt(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are less than source 2's
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] < src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdFlr(uint32_t opcode) {
    // Set the destination components to the floor of the source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = floorf(src1[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdMax(uint32_t opcode) {
    // Set the destination components to the maximum of two source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = std::max(src1[i], src2[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdMin(uint32_t opcode) {
    // Set the destination components to the minimum of two source components
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = std::min(src1[i], src2[i]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdRcp(uint32_t opcode) {
    // Calculate the reciprocal of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[0] = 1.0f / src1[0];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdRsq(uint32_t opcode) {
    // Calculate the reverse square root of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[0] = 1.0f / sqrtf(src1[0]);
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdMova(uint32_t opcode) {
    // Move a value from a source register to the X/Y address register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    if (desc & BIT(3)) shdAddr[0] = src1[0];
    if (desc & BIT(2)) shdAddr[1] = src1[1];
}

void GpuRenderSoft::shdMov(uint32_t opcode) {
    // Move a value from a source register to a destination register
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdDphi(uint32_t opcode) {
    // Calculate the dot product of a 3-component and a 4-component source register (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    src1[0] = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + src2[3];
    src1[3] = src1[2] = src1[1] = src1[0];
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdSgei(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are greater or equal to source 2's (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] >= src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdSlti(uint32_t opcode) {
    // Output 1 or 0 based on if source 1's components are less than source 2's (alternate)
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 14) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 7) & 0x7F, desc >> 9, (opcode >> 19) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = (src1[i] < src2[i]) ? 1.0f : 0.0f;
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdBreak(uint32_t opcode) {
    // Jump to the end of a loop and finish it
    if (loopStack.empty()) return;
    shdPc = (loopStack.front() & 0xFFF);
    loopStack.pop_front();
}

void GpuRenderSoft::shdNop(uint32_t opcode) {
    // Do nothing
}

void GpuRenderSoft::shdEnd(uint32_t opcode) {
    // Finish shader execution
    shdPc = vshEnd;
}

void GpuRenderSoft::shdBreakc(uint32_t opcode) {
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
    }
    else {
        shdPc = dst;
    }
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
    }
    else {
        shdPc = dst;
    }
}

void GpuRenderSoft::shdLoop(uint32_t opcode) {
    // Reload the loop counter and set up a loop between the next opcode and an address
    uint8_t *ints = shdInts[(opcode >> 22) & 0x3];
    shdAddr[2] = ints[1]; // Loop counter
    uint16_t dst = ((opcode >> 10) + 1) & 0xFFF;
    loopStack.push_front((ints[0] << 24) | ((shdPc & 0xFFF) << 12) | dst);
    if (loopStack.size() > 4) loopStack.pop_back(); // 4-deep
}

void GpuRenderSoft::shdJmpc(uint32_t opcode) {
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

void GpuRenderSoft::shdJmpu(uint32_t opcode) {
    // If a uniform bool is true, jump to an address
    if (!shdBools[(opcode >> 22) & 0xF]) return;
    shdPc = (opcode >> 10) & 0xFFF;
}

void GpuRenderSoft::shdCmp(uint32_t opcode) {
    // Set the X/Y condition values by comparing X/Y of two source registers
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    float *src2 = getSrc((opcode >> 7) & 0x1F, desc >> 9, 0);
    for (int i = 0; i < 2; i++) {
        if (~desc & BIT(3 - i)) continue;
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

void GpuRenderSoft::shdMadi(uint32_t opcode) {
    // Multiply each component of two source registers and add a third one (alternate)
    uint32_t desc = shdDesc[opcode & 0x1F];
    float *src1 = getSrc((opcode >> 17) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 12) & 0x1F, desc >> 9, 0);
    float *src3 = getSrc((opcode >> 5) & 0x7F, desc >> 18, (opcode >> 22) & 0x3);
    for (int i = 0; i < 4; i++)
        src1[i] = src1[i] * src2[i] + src3[i];
    setDst((opcode >> 24) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdMad(uint32_t opcode) {
    // Multiply each component of two source registers and add a third one
    uint32_t desc = shdDesc[opcode & 0x1F];
    float *src1 = getSrc((opcode >> 17) & 0x1F, desc, 0);
    float *src2 = getSrc((opcode >> 10) & 0x7F, desc >> 9, (opcode >> 22) & 0x3);
    float *src3 = getSrc((opcode >> 5) & 0x1F, desc >> 18, 0);
    for (int i = 0; i < 4; i++)
        src1[i] = src1[i] * src2[i] + src3[i];
    setDst((opcode >> 24) & 0x1F, desc, src1);
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

void GpuRenderSoft::setVshEntry(uint16_t entry, uint16_t end) {
    // Set the vertex shader entry and end points
    vshEntry = entry;
    vshEnd = end;
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

void GpuRenderSoft::setViewStepH(float step) {
    // Set the viewport horizontal step
    viewStepH = step;
}

void GpuRenderSoft::setViewScaleV(float scale) {
    // Set the viewport vertical scale
    viewScaleV = scale;
}

void GpuRenderSoft::setViewStepV(float step) {
    // Set the viewport vertical step
    viewStepV = step;
}

void GpuRenderSoft::setBufferDims(uint16_t width, uint16_t height, bool mirror) {
    // Set the render buffer width, height, and Y-mirror
    bufWidth = width;
    bufHeight = height;
    signY = (mirror ? -1.0f : 1.0f);
}

void GpuRenderSoft::setColbufAddr(uint32_t address) {
    // Set the color buffer address
    colbufAddr = address;
}

void GpuRenderSoft::setColbufFmt(ColbufFmt format) {
    // Set the color buffer format
    colbufFmt = format;
}

void GpuRenderSoft::setColbufMask(uint8_t mask) {
    // Set the color buffer write mask
    colbufMask = mask;
}

void GpuRenderSoft::setDepbufAddr(uint32_t address) {
    // Set the depth buffer address
    depbufAddr = address;
}

void GpuRenderSoft::setDepbufFmt(DepbufFmt format) {
    // Set the depth buffer format
    depbufFmt = format;
}

void GpuRenderSoft::setDepbufMask(uint8_t mask) {
    // Set the depth buffer write mask
    depbufMask = mask;
}

void GpuRenderSoft::setDepthFunc(DepthFunc func) {
    // Set the depth test comparison function
    depthFunc = func;
}
