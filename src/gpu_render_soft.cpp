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
void (GpuRenderSoft::*GpuRenderSoft::vshInstrs[])(uint32_t) {
    &GpuRenderSoft::shdAdd, &GpuRenderSoft::shdDp3, &GpuRenderSoft::shdDp4, &GpuRenderSoft::shdDph, // 0x00-0x03
    &GpuRenderSoft::vshUnk, &GpuRenderSoft::shdEx2, &GpuRenderSoft::shdLg2, &GpuRenderSoft::vshUnk, // 0x04-0x07
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

int16_t GpuRenderSoft::etc1Tables[][4] {
    { 2, 8, -2, -8 },
    { 5, 17, -5, -17 },
    { 9, 29, -9, -29 },
    { 13, 42, -13, -42 },
    { 18, 60, -18, -60 },
    { 24, 80, -24, -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 }
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

template <bool doX> SoftVertex GpuRenderSoft::interpolate(SoftVertex &v1, SoftVertex &v2, float x1, float x, float x2) {
    // Check bounds and calculate an interpolation factor
    if (x <= x1) return v1;
    if (x >= x2) return v2;
    float factor = (x - x1) / (x2 - x1);

    // Interpolate triangle vertex attributes using the factor
    SoftVertex v;
    if (doX) v.x = v1.x + (v2.x - v1.x) * factor;
    v.z = v1.z + (v2.z - v1.z) * factor;
    v.w = v1.w + (v2.w - v1.w) * factor;
    v.r = v1.r + (v2.r - v1.r) * factor;
    v.g = v1.g + (v2.g - v1.g) * factor;
    v.b = v1.b + (v2.b - v1.b) * factor;
    v.a = v1.a + (v2.a - v1.a) * factor;
    v.s0 = v1.s0 + (v2.s0 - v1.s0) * factor;
    v.s1 = v1.s1 + (v2.s1 - v1.s1) * factor;
    v.s2 = v1.s2 + (v2.s2 - v1.s2) * factor;
    v.t0 = v1.t0 + (v2.t0 - v1.t0) * factor;
    v.t1 = v1.t1 + (v2.t1 - v1.t1) * factor;
    v.t2 = v1.t2 + (v2.t2 - v1.t2) * factor;
    return v;
}

SoftVertex GpuRenderSoft::intersect(SoftVertex &v1, SoftVertex &v2, float x1, float x2) {
    // Calculate the intersection of two vertices at a clipping bound
    SoftVertex v;
    float c1 = x1 + v1.w, c2 = x2 + v2.w;
    v.x = ((v1.x * c2) - (v2.x * c1)) / (c2 - c1);
    v.y = ((v1.y * c2) - (v2.y * c1)) / (c2 - c1);
    v.z = ((v1.z * c2) - (v2.z * c1)) / (c2 - c1);
    v.w = ((v1.w * c2) - (v2.w * c1)) / (c2 - c1);
    v.r = ((v1.r * c2) - (v2.r * c1)) / (c2 - c1);
    v.g = ((v1.g * c2) - (v2.g * c1)) / (c2 - c1);
    v.b = ((v1.b * c2) - (v2.b * c1)) / (c2 - c1);
    v.a = ((v1.a * c2) - (v2.a * c1)) / (c2 - c1);
    v.s0 = ((v1.s0 * c2) - (v2.s0 * c1)) / (c2 - c1);
    v.s1 = ((v1.s1 * c2) - (v2.s1 * c1)) / (c2 - c1);
    v.s2 = ((v1.s2 * c2) - (v2.s2 * c1)) / (c2 - c1);
    v.t0 = ((v1.t0 * c2) - (v2.t0 * c1)) / (c2 - c1);
    v.t1 = ((v1.t1 * c2) - (v2.t1 * c1)) / (c2 - c1);
    v.t2 = ((v1.t2 * c2) - (v2.t2 * c1)) / (c2 - c1);
    return v;
}

uint8_t GpuRenderSoft::stencilOp(uint8_t value, StenOper oper) {
    // Adjust a stencil value based on the given operation
    switch (oper) {
        case STEN_KEEP: return value;
        case STEN_ZERO: return 0;
        case STEN_REPLACE: return stencilValue & stencilMasks[0];
        case STEN_INCR: return std::min(0xFF, int(value) + 1) & stencilMasks[0];
        case STEN_DECR: return std::max(0x00, int(value) - 1) & stencilMasks[0];
        case STEN_INVERT: return ~value & stencilMasks[0];
        case STEN_INCWR: return (value + 1) & stencilMasks[0];
        case STEN_DECWR: return (value - 1) & stencilMasks[0];
    }
}

void GpuRenderSoft::getTexel(float &r, float &g, float &b, float &a, float s, float t, int i) {
    // Catch silly invalid textures like in Pokemon X/Y
    if (!texWidths[i] || !texHeights[i]) {
        r = g = b = a = 0.0f;
        return;
    }

    // Scale the S-coordinate to texels and handle wrapping based on mode
    uint32_t u = uint32_t(s * texWidths[i]);
    if (u >= texWidths[i]) {
        switch (texWrapS[i]) {
        case WRAP_CLAMP:
            u = (int32_t(u) < 0) ? 0 : (texWidths[i] - 1);
            break;
        case WRAP_BORDER:
            r = texBorders[i][0], g = texBorders[i][1], b = texBorders[i][2], a = texBorders[i][3];
            return;
        case WRAP_REPEAT:
            u %= texWidths[i];
            break;
        case WRAP_MIRROR:
            if ((u %= texWidths[i] * 2) >= texWidths[i])
                u = (texWidths[i] * 2) - u - 1;
            break;
        }
    }

    // Scale the T-coordinate to texels and handle wrapping based on mode
    uint32_t v = texHeights[i] - uint32_t(t * texHeights[i]) - 1;
    if (v >= texHeights[i]) {
        switch (texWrapT[i]) {
        case WRAP_CLAMP:
            v = (int32_t(v) < 0) ? 0 : (texHeights[i] - 1);
            break;
        case WRAP_BORDER:
            r = texBorders[i][0], g = texBorders[i][1], b = texBorders[i][2], a = texBorders[i][3];
            return;
        case WRAP_REPEAT:
            v %= texHeights[i];
            break;
        case WRAP_MIRROR:
            if ((v %= texHeights[i] * 2) >= texHeights[i])
                v = (texHeights[i] * 2) - v - 1;
            break;
        }
    }

    // Use the cached texel if coordinates are unchanged
    if (lastU[i] == u && lastV[i] == v) {
        r = rt[i], g = gt[i], b = bt[i], a = at[i];
        return;
    }

    // Convert the texture coordinates to a swizzled memory offset
    uint32_t value, ofs = (u & 0x1) | ((u << 1) & 0x4) | ((u << 2) & 0x10);
    ofs |= ((v << 1) & 0x2) | ((v << 2) & 0x8) | ((v << 3) & 0x20);
    ofs += ((v & ~0x7) * texWidths[i]) + ((u & ~0x7) << 3);

    // Read a texel and convert it to floats based on format
    switch (texFmts[i]) {
    case TEX_RGBA8:
        value = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs * 4);
        r = float((value >> 24) & 0xFF) / 0xFF;
        g = float((value >> 16) & 0xFF) / 0xFF;
        b = float((value >> 8) & 0xFF) / 0xFF;
        a = float((value >> 0) & 0xFF) / 0xFF;
        break;
    case TEX_RGB8:
        r = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 2)) / 0xFF;
        g = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 1)) / 0xFF;
        b = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 0)) / 0xFF;
        a = 1.0f;
        break;
    case TEX_RGB5A1:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        r = float((value >> 11) & 0x1F) / 0x1F;
        g = float((value >> 6) & 0x1F) / 0x1F;
        b = float((value >> 1) & 0x1F) / 0x1F;
        a = (value & BIT(0)) ? 1.0f : 0.0f;
        break;
    case TEX_RGB565:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        r = float((value >> 11) & 0x1F) / 0x1F;
        g = float((value >> 5) & 0x3F) / 0x3F;
        b = float((value >> 0) & 0x1F) / 0x1F;
        a = 1.0f;
        break;
    case TEX_RGBA4:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        r = float((value >> 12) & 0xF) / 0xF;
        g = float((value >> 8) & 0xF) / 0xF;
        b = float((value >> 4) & 0xF) / 0xF;
        a = float((value >> 0) & 0xF) / 0xF;
        break;
    case TEX_LA8:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        r = g = b = float((value >> 8) & 0xFF) / 0xFF;
        a = float((value >> 0) & 0xFF) / 0xFF;
        break;
    case TEX_RG8:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        r = float((value >> 8) & 0xFF) / 0xFF;
        g = float((value >> 0) & 0xFF) / 0xFF;
        b = 0.0f, a = 1.0f;
        break;
    case TEX_L8:
        r = g = b = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs)) / 0xFF;
        a = 1.0f;
        break;
    case TEX_A8:
        r = g = b = 0.0f;
        a = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs)) / 0xFF;
        break;
    case TEX_LA4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs);
        r = g = b = float((value >> 4) & 0xF) / 0xF;
        a = float((value >> 0) & 0xF) / 0xF;
        break;
    case TEX_L4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs / 2);
        r = g = b = float((value >> ((ofs & 0x1) * 4)) & 0xF) / 0xF;
        a = 1.0f;
        break;
    case TEX_A4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs / 2);
        r = g = b = 0.0f;
        a = float((value >> ((ofs & 0x1) * 4)) & 0xF) / 0xF;
        break;
    case TEX_UNK:
        r = g = b = a = 1.0f;
        break;

    case TEX_ETC1: case TEX_ETC1A4:
        // Adjust the offset for 4x4 ETC1 tiles and read alpha if provided
        uint8_t idx = (u & 0x3) * 4 + (v & 0x3);
        if (texFmts[i] == TEX_ETC1A4) {
            ofs = (ofs & ~0xF) + 8;
            value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs - 8 + idx / 2);
            a = float((value >> ((idx & 0x1) * 4)) & 0xF) / 0xF;
        }
        else {
            ofs = (ofs & ~0xF) >> 1;
            a = 1.0f;
        }

        // Decode an ETC1 texel based on the block it falls in and the base color mode
        int32_t val1 = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 0);
        int32_t val2 = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 4);
        if ((((val2 & BIT(0)) ? v : u) & 0x3) < 2) { // Block 1
            int16_t tbl = etc1Tables[(val2 >> 5) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
            if (val2 & BIT(1)) { // Differential
                r = float(((val2 >> 27) & 0x1F) * 0x21 / 4 + tbl);
                g = float(((val2 >> 19) & 0x1F) * 0x21 / 4 + tbl);
                b = float(((val2 >> 11) & 0x1F) * 0x21 / 4 + tbl);
            }
            else { // Individual
                r = float(((val2 >> 28) & 0xF) * 0x11 + tbl);
                g = float(((val2 >> 20) & 0xF) * 0x11 + tbl);
                b = float(((val2 >> 12) & 0xF) * 0x11 + tbl);
            }
        }
        else { // Block 2
            int16_t tbl = etc1Tables[(val2 >> 2) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
            if (val2 & BIT(1)) { // Differential
                r = float((((val2 >> 27) & 0x1F) + (int8_t(val2 >> 19) >> 5)) * 0x21 / 4 + tbl);
                g = float((((val2 >> 19) & 0x1F) + (int8_t(val2 >> 11) >> 5)) * 0x21 / 4 + tbl);
                b = float((((val2 >> 11) & 0x1F) + (int8_t(val2 >> 3) >> 5)) * 0x21 / 4 + tbl);
            }
            else { // Individual
                r = float(((val2 >> 24) & 0xF) * 0x11 + tbl);
                g = float(((val2 >> 16) & 0xF) * 0x11 + tbl);
                b = float(((val2 >> 8) & 0xF) * 0x11 + tbl);
            }
        }

        // Normalize and clamp the final color values
        r = std::min(1.0f, std::max(0.0f, r / 255));
        g = std::min(1.0f, std::max(0.0f, g / 255));
        b = std::min(1.0f, std::max(0.0f, b / 255));
        break;
    }

    // Cache the texel to avoid decoding multiple times
    rt[i] = r, gt[i] = g, bt[i] = b, at[i] = a;
    lastU[i] = u, lastV[i] = v;
}

void GpuRenderSoft::getSource(float &r, float &g, float &b, float &a, SoftVertex &v, int i, int j) {
    // Get the components of a texture combiner source color
    switch (combSrcs[i][j]) {
        case COMB_PRIM: r = v.r / v.w, g = v.g / v.w, b = v.b / v.w, a = v.a / v.w; break;
        case COMB_TEX0: getTexel(r, g, b, a, v.s0 / v.w, v.t0 / v.w, 0); break;
        case COMB_TEX1: getTexel(r, g, b, a, v.s1 / v.w, v.t1 / v.w, 1); break;
        case COMB_TEX2: getTexel(r, g, b, a, v.s2 / v.w, v.t2 / v.w, 2); break;
        case COMB_CONST: r = combColors[i][0], g = combColors[i][1], b = combColors[i][2], a = combColors[i][3]; break;
        case COMB_FRAG0: case COMB_FRAG1: r = g = b = a = 0.5f; break; // Stub
        case COMB_TEX3: r = g = b = a = 1.0f; break; // Stub
        case COMB_UNK: r = g = b = a = 0.0f; break;

    case COMB_PREV:
        // Generate the previous combiner's output, or use the cache if already done
        if (i == 0)
            r = g = b = a = 0.0f;
        else if (combMask & BIT(i - 1))
            r = rc[i - 1], g = gc[i - 1], b = bc[i - 1], a = ac[i - 1];
        else
            getCombine(r, g, b, a, v, i - 1);
        break;

    case COMB_PRVBUF:
        // Generate the buffered previous output, using buffer color for the first update
        if (i == 0)
            r = g = b = a = 0.0f;
        else if (i == 1)
            r = combBufColor[0], g = combBufColor[1], b = combBufColor[2], a = combBufColor[3];
        else if (combMask & BIT(i - 2))
            r = rc[i - 2], g = gc[i - 2], b = bc[i - 2], a = ac[i - 2];
        else
            getCombine(r, g, b, a, v, i - 2);
        break;
    }

    // Modify the source color based on its operand type
    switch (combOpers[i][j]) {
        case OPER_SRC: return;
        case OPER_1MSRC: r = 1.0f - r, g = 1.0f - g, b = 1.0f - b, a = 1.0f - a; return;
        case OPER_SRCA: r = g = b = a; return;
        case OPER_1MSRCA: r = g = b = a = 1.0f - a; return;
    }
}

void GpuRenderSoft::getCombine(float &r, float &g, float &b, float &a, SoftVertex &v, int i) {
    // Define temporary values
    float r0, g0, b0, a0;
    float r1, g1, b1, a1;
    float r2, g2, b2, a2;

    // Combine source RGB values based on the selected mode
    switch (combModes[i][0]) {
    case MODE_REPLACE:
        getSource(r0, g0, b0, a0, v, i, 0);
        r = r0, g = g0, b = b0;
        break;
    case MODE_MOD:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        r = r0 * r1, g = g0 * g1, b = b0 * b1;
        break;
    case MODE_ADD:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        r = r0 + r1, g = g0 + g1, b = b0 + b1;
        break;
    case MODE_ADDS:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        r = r0 + r1 - 0.5f, g = g0 + g1 - 0.5f, b = b0 + b1 - 0.5f;
        break;
    case MODE_INTERP:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        getSource(r2, g2, b2, a2, v, i, 2);
        r = r0 * r2 + r1 * (1.0f - r2), g = g0 * g2 + g1 * (1.0f - g2), b = b0 * b2 + b1 * (1.0f - b2);
        break;
    case MODE_SUB:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        r = r0 - r1, g = g0 - g1, b = b0 - b1;
        break;
    case MODE_DOT3:
    case MODE_DOT3A:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        r = g = b = 4.0f * r0 - 0.5f * r1 - 0.5f + g0 - 0.5f * g1 - 0.5f + b0 - 0.5f * b1 - 0.5f;
        break;
    case MODE_MULADD:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        getSource(r2, g2, b2, a2, v, i, 2);
        r = (r0 * r1) + r2, g = (g0 * g1) + g2, b = (b0 * b1) + b2;
        break;
    case MODE_ADDMUL:
        getSource(r0, g0, b0, a0, v, i, 0);
        getSource(r1, g1, b1, a1, v, i, 1);
        getSource(r2, g2, b2, a2, v, i, 2);
        r = (r0 + r1) * r2, g = (g0 + g1) * g2, b = (b0 + b1) * b2;
        break;
    case MODE_UNK:
        r = g = b = 1.0f;
        break;
    }

    // Combine source alpha values based on the selected mode
    switch (combModes[i][1]) {
    case MODE_REPLACE:
        getSource(r0, g0, b0, a0, v, i, 3);
        a = a0;
        break;
    case MODE_MOD:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        a = a0 * a1;
        break;
    case MODE_ADD:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        a = a0 + a1;
        break;
    case MODE_ADDS:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        a = a0 + a1 - 0.5f;
        break;
    case MODE_INTERP:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        getSource(r2, g2, b2, a2, v, i, 5);
        a = a0 * a2 + a1 * (1.0f - a2);
        break;
    case MODE_SUB:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        a = a0 - a1;
        break;
    case MODE_DOT3A:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        a = 4.0f * r0 - 0.5f * r1 - 0.5f + g0 - 0.5f * g1 - 0.5f + b0 - 0.5f * b1 - 0.5f;
        break;
    case MODE_MULADD:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        getSource(r2, g2, b2, a2, v, i, 5);
        a = (a0 * a1) + a2;
        break;
    case MODE_ADDMUL:
        getSource(r0, g0, b0, a0, v, i, 3);
        getSource(r1, g1, b1, a1, v, i, 4);
        getSource(r2, g2, b2, a2, v, i, 5);
        a = (a0 + a1) * a2;
        break;
    case MODE_DOT3:
    case MODE_UNK:
        a = 1.0f;
        break;
    }

    // Ensure final values are within range and cache them
    rc[i] = r = std::min(1.0f, std::max(0.0f, r));
    gc[i] = g = std::min(1.0f, std::max(0.0f, g));
    bc[i] = b = std::min(1.0f, std::max(0.0f, b));
    ac[i] = a = std::min(1.0f, std::max(0.0f, a));
    combMask |= BIT(i);
}

void GpuRenderSoft::drawPixel(SoftVertex &p) {
    // Check bounds and convert coordinates to an 8x8 tile offset
    int x = int(p.x), y = flipY ? (bufHeight - int(p.y) - 1) : int(p.y);
    if (x < 0 || x >= bufWidth || y < 0 || y >= bufHeight) return;
    uint32_t val, ofs = (((y >> 3) * (bufWidth >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    ofs |= ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);

    // Perform stencil testing on the pixel if enabled
    uint8_t stencil = 0;
    if (stencilEnable) {
        // Read and mask the buffer and reference values
        if (depbufFmt == DEP_24S8)
            stencil = core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3) & stencilMasks[0];
        uint8_t ref = (stencilValue & stencilMasks[1]);

        // Compare the incoming stencil value with the reference
        bool pass;
        switch (stencilFunc) {
            case TEST_NV: pass = false; break;
            case TEST_AL: pass = true; break;
            case TEST_EQ: pass = (stencil == ref); break;
            case TEST_NE: pass = (stencil != ref); break;
            case TEST_LT: pass = (stencil < ref); break;
            case TEST_LE: pass = (stencil <= ref); break;
            case TEST_GT: pass = (stencil > ref); break;
            case TEST_GE: pass = (stencil >= ref); break;
        }

        // If failed, perform the fail operation on the buffer and don't draw
        if (!pass) {
            if (depbufFmt == DEP_24S8)
                core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3, stencilOp(stencil, stencilFail));
            return;
        }
    }

    // Read a depth value to compare with based on buffer format
    float depth = 0.0f;
    if (depthFunc >= TEST_EQ) {
        switch (depbufFmt) {
        case DEP_16:
            val = core->memory.read<uint16_t>(ARM11, depbufAddr + ofs * 2);
            depth = -float(val) / 0xFFFF;
            break;
        case DEP_24:
            val = core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 0) << 0;
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 1) << 8;
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 2) << 16;
            depth = -float(val) / 0xFFFFFF;
            break;
        case DEP_24S8:
            val = core->memory.read<uint32_t>(ARM11, depbufAddr + ofs * 4) & 0xFFFFFF;
            depth = -float(val) / 0xFFFFFF;
            break;
        }
    }

    // Compare the incoming depth value with the existing one
    bool pass;
    switch (depthFunc) {
        case TEST_NV: pass = false; break;
        case TEST_AL: pass = true; break;
        case TEST_EQ: pass = (depth == p.z); break;
        case TEST_NE: pass = (depth != p.z); break;
        case TEST_LT: pass = (depth < p.z); break;
        case TEST_LE: pass = (depth <= p.z); break;
        case TEST_GT: pass = (depth > p.z); break;
        case TEST_GE: pass = (depth >= p.z); break;
    }

    // Perform the stencil depth pass/fail operation if enabled, and don't draw if failed
    if (!pass) {
        if (stencilEnable && depbufFmt == DEP_24S8)
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3, stencilOp(stencil, stenDepFail));
        return;
    }
    else if (stencilEnable && depbufFmt == DEP_24S8) {
        core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3, stencilOp(stencil, stenDepPass));
    }

    // Get source color values from the texture combiner
    float srcR, srcG, srcB, srcA;
    combMask = 0; // Reset cache
    getCombine(srcR, srcG, srcB, srcA, p, combStart);

    // Compare the source alpha value with the provided one
    switch (alphaFunc) {
        case TEST_NV: return;
        case TEST_AL: break;
        case TEST_EQ: if (srcA == alphaValue) break; return;
        case TEST_NE: if (srcA != alphaValue) break; return;
        case TEST_LT: if (srcA < alphaValue) break; return;
        case TEST_LE: if (srcA <= alphaValue) break; return;
        case TEST_GT: if (srcA > alphaValue) break; return;
        case TEST_GE: if (srcA >= alphaValue) break; return;
    }

    // Store the incoming depth value based on buffer format if enabled
    if (depbufMask & BIT(1)) {
        switch (depbufFmt) {
        case DEP_16:
            val = std::min<int>(0xFFFF, std::max<int>(0, -p.z * 0xFFFF));
            core->memory.write<uint16_t>(ARM11, depbufAddr + ofs * 2, val);
            break;
        case DEP_24:
            val = std::min<int>(0xFFFFFF, std::max<int>(0, -p.z * 0xFFFFFF));
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 0, val >> 0);
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 1, val >> 8);
            core->memory.write<uint8_t>(ARM11, depbufAddr + ofs * 3 + 2, val >> 16);
            break;
        case DEP_24S8:
            val = std::min<int>(0xFFFFFF, std::max<int>(0, -p.z * 0xFFFFFF));
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3) << 24;
            core->memory.write<uint32_t>(ARM11, depbufAddr + ofs * 4, val);
            break;
        }
    }

    // Read color values to blend with based on buffer format
    float dstR, dstG, dstB, dstA;
    switch (colbufFmt) {
    case COL_RGBA8:
        val = core->memory.read<uint32_t>(ARM11, colbufAddr + ofs * 4);
        dstR = float((val >> 24) & 0xFF) / 0xFF;
        dstG = float((val >> 16) & 0xFF) / 0xFF;
        dstB = float((val >> 8) & 0xFF) / 0xFF;
        dstA = float((val >> 0) & 0xFF) / 0xFF;
        break;
    case COL_RGB8:
        dstR = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 2)) / 0xFF;
        dstG = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 1)) / 0xFF;
        dstB = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 0)) / 0xFF;
        dstA = 1.0f;
        break;
    case COL_RGB565:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        dstR = float((val >> 11) & 0x1F) / 0x1F;
        dstG = float((val >> 5) & 0x3F) / 0x3F;
        dstB = float((val >> 0) & 0x1F) / 0x1F;
        dstA = 1.0f;
        break;
    case COL_RGB5A1:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        dstR = float((val >> 11) & 0x1F) / 0x1F;
        dstG = float((val >> 6) & 0x1F) / 0x1F;
        dstB = float((val >> 1) & 0x1F) / 0x1F;
        dstA = (val & BIT(0)) ? 1.0f : 0.0f;
        break;
    case COL_RGBA4:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        dstR = float((val >> 12) & 0xF) / 0xF;
        dstG = float((val >> 8) & 0xF) / 0xF;
        dstB = float((val >> 4) & 0xF) / 0xF;
        dstA = float((val >> 0) & 0xF) / 0xF;
        break;
    case COL_UNK:
        return;
    }

    // Multiply source RGB values with the selected operand
    float r1 = srcR, g1 = srcG, b1 = srcB;
    switch (blendOpers[0]) {
        case OPER_ZERO: r1 = g1 = b1 = 0.0f; break;
        case OPER_ONE: break;
        case OPER_SRC: r1 *= srcR, g1 *= srcG, b1 *= srcB; break;
        case OPER_1MSRC: r1 *= 1.0f - srcR, g1 *= 1.0f - srcG, b1 *= 1.0f - srcB; break;
        case OPER_DST: r1 *= dstR, g1 *= dstG, b1 *= dstB; break;
        case OPER_1MDST: r1 *= 1.0f - dstR, g1 *= 1.0f - dstG, b1 *= 1.0f - dstB; break;
        case OPER_SRCA: r1 *= srcA, g1 *= srcA, b1 *= srcA; break;
        case OPER_1MSRCA: r1 *= 1.0f - srcA, g1 *= 1.0f - srcA, b1 *= 1.0f - srcA; break;
        case OPER_DSTA: r1 *= dstA, g1 *= dstA, b1 *= dstA; break;
        case OPER_1MDSTA: r1 *= 1.0f - dstA, g1 *= 1.0f - dstA, b1 *= 1.0f - dstA; break;
        case OPER_CONST: r1 *= blendR, g1 *= blendG, b1 *= blendB; break;
        case OPER_1MCON: r1 *= 1.0f - blendR, g1 *= 1.0f - blendG, b1 *= 1.0f - blendB; break;
        case OPER_CONSTA: r1 *= blendA, g1 *= blendA, b1 *= blendA; break;
        case OPER_1MCONA: r1 *= 1.0f - blendA, g1 *= 1.0f - blendA, b1 *= 1.0f - blendA; break;
    }

    // Multiply destination RGB values with the selected operand
    float r2 = dstR, g2 = dstG, b2 = dstB;
    switch (blendOpers[1]) {
        case OPER_ZERO: r2 = g2 = b2 = 0.0f; break;
        case OPER_ONE: break;
        case OPER_SRC: r2 *= srcR, g2 *= srcG, b2 *= srcB; break;
        case OPER_1MSRC: r2 *= 1.0f - srcR, g2 *= 1.0f - srcG, b2 *= 1.0f - srcB; break;
        case OPER_DST: r2 *= dstR, g2 *= dstG, b2 *= dstB; break;
        case OPER_1MDST: r2 *= 1.0f - dstR, g2 *= 1.0f - dstG, b2 *= 1.0f - dstB; break;
        case OPER_SRCA: r2 *= srcA, g2 *= srcA, b2 *= srcA; break;
        case OPER_1MSRCA: r2 *= 1.0f - srcA, g2 *= 1.0f - srcA, b2 *= 1.0f - srcA; break;
        case OPER_DSTA: r2 *= dstA, g2 *= dstA, b2 *= dstA; break;
        case OPER_1MDSTA: r2 *= 1.0f - dstA, g2 *= 1.0f - dstA, b2 *= 1.0f - dstA; break;
        case OPER_CONST: r2 *= blendR, g2 *= blendG, b2 *= blendB; break;
        case OPER_1MCON: r2 *= 1.0f - blendR, g2 *= 1.0f - blendG, b2 *= 1.0f - blendB; break;
        case OPER_CONSTA: r2 *= blendA, g2 *= blendA, b2 *= blendA; break;
        case OPER_1MCONA: r2 *= 1.0f - blendA, g2 *= 1.0f - blendA, b2 *= 1.0f - blendA; break;
    }

    // Multiply source alpha values with the selected operand
    float a1 = srcA;
    switch (blendOpers[2]) {
        case OPER_ZERO: a1 = 0.0f; break;
        case OPER_ONE: break;
        case OPER_SRC: case OPER_SRCA: a1 *= srcA; break;
        case OPER_1MSRC: case OPER_1MSRCA: a1 *= 1.0f - srcA; break;
        case OPER_DST: case OPER_DSTA: a1 *= dstA; break;
        case OPER_1MDST: case OPER_1MDSTA: a1 *= 1.0f - dstA; break;
        case OPER_CONST: case OPER_CONSTA: a1 *= blendA; break;
        case OPER_1MCON: case OPER_1MCONA: a1 *= 1.0f - blendA; break;
    }

    // Multiply destination alpha values with the selected operand
    float a2 = dstA;
    switch (blendOpers[3]) {
        case OPER_ZERO: a2 = 0.0f; break;
        case OPER_ONE: break;
        case OPER_SRC: case OPER_SRCA: a2 *= srcA; break;
        case OPER_1MSRC: case OPER_1MSRCA: a2 *= 1.0f - srcA; break;
        case OPER_DST: case OPER_DSTA: a2 *= dstA; break;
        case OPER_1MDST: case OPER_1MDSTA: a2 *= 1.0f - dstA; break;
        case OPER_CONST: case OPER_CONSTA: a2 *= blendA; break;
        case OPER_1MCON: case OPER_1MCONA: a2 *= 1.0f - blendA; break;
    }

    // Blend the source and destination RGB values based on mode
    float r, g, b;
    switch (blendModes[0]) {
        default: r = r1 + r2, g = g1 + g2, b = b1 + b2; break;
        case MODE_SUB: r = r1 - r2, g = g1 - g2, b = b1 - b2; break;
        case MODE_RSUB: r = r2 - r1, g = g2 - g1, b = b2 - b1; break;
        case MODE_MIN: r = std::min(r1, r2), g = std::min(g1, g2), b = std::min(b1, b2); break;
        case MODE_MAX: r = std::max(r1, r2), g = std::max(g1, g2), b = std::max(b1, b2); break;
    }

    // Blend the source and destination alpha values based on mode
    float a;
    switch (blendModes[1]) {
        default: a = a1 + a2; break;
        case MODE_SUB: a = a1 - a2; break;
        case MODE_RSUB: a = a2 - a1; break;
        case MODE_MIN: a = std::min(a1, a2); break;
        case MODE_MAX: a = std::max(a1, a2); break;
    }

    // Clamp the final color values
    r = std::min(1.0f, std::max(0.0f, r));
    g = std::min(1.0f, std::max(0.0f, g));
    b = std::min(1.0f, std::max(0.0f, b));
    a = std::min(1.0f, std::max(0.0f, a));

    // Store the final color values based on buffer format if enabled
    if (!colbufMask) return; // TODO: use individual components
    switch (colbufFmt) {
    case COL_RGBA8:
        val = (int(r * 255) << 24) | (int(g * 255) << 16) | (int(b * 255) << 8) | int(a * 255);
        return core->memory.write<uint32_t>(ARM11, colbufAddr + ofs * 4, val);
    case COL_RGB8:
        core->memory.write<uint8_t>(ARM11, colbufAddr + ofs * 3 + 2, r * 255);
        core->memory.write<uint8_t>(ARM11, colbufAddr + ofs * 3 + 1, g * 255);
        return core->memory.write<uint8_t>(ARM11, colbufAddr + ofs * 3 + 0, b * 255);
    case COL_RGB565:
        val = (int(r * 31) << 11) | (int(g * 63) << 5) | int(b * 31);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    case COL_RGB5A1:
        val = (int(r * 31) << 11) | (int(g * 31) << 6) | (int(b * 31) << 1) | (a > 0);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    case COL_RGBA4:
        val = (int(r * 15) << 12) | (int(g * 15) << 8) | (int(b * 15) << 4) | int(a * 15);
        return core->memory.write<uint16_t>(ARM11, colbufAddr + ofs * 2, val);
    }
}

void GpuRenderSoft::drawTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c) {
    // Cull triangles by determining their orientation with a cross product
    float cross = ((b.y - a.y) * (c.x - b.x)) - ((b.x - a.x) * (c.y - b.y));
    if ((cullMode == CULL_FRONT && (cross < 0)) || (cullMode == CULL_BACK && (cross > 0)))
        return;

    // Scale the coordinate steps to screen space
    if (viewStepH <= 0 || viewStepV <= 0) return;
    float ys = viewStepV * viewScaleV;
    float xs = viewStepH * viewScaleH;

    // Sort the vertices by increasing Y-coordinates
    SoftVertex *v[3] = { &a, &b, &c };
    if (v[0]->y > v[1]->y) std::swap(v[0], v[1]);
    if (v[0]->y > v[2]->y) std::swap(v[0], v[2]);
    if (v[1]->y > v[2]->y) std::swap(v[1], v[2]);

    // Check for combiners set to simply output the previous color, so they can be skipped
    if (combStart > 5) {
        combStart = 5;
        uint8_t &i = combStart;
        while (i > 0 && combModes[i][0] == MODE_REPLACE && combModes[i][1] == MODE_REPLACE && combSrcs[i][0] ==
            COMB_PREV && combSrcs[i][3] == COMB_PREV && combOpers[i][0] == OPER_SRC && combOpers[i][3] == OPER_SRC)
            combStart--;
    }

    // Draw the pixels of a triangle by interpolating between X and Y bounds
    for (float y = roundf(v[0]->y) + ys / 2; y < roundf(v[2]->y); y += ys) {
        int r = (y >= v[1]->y) ? 1 : 0;
        SoftVertex vl = interpolate<true>(*v[0], *v[2], v[0]->y, y, v[2]->y);
        SoftVertex vr = interpolate<true>(*v[r], *v[r + 1], v[r]->y, y, v[r + 1]->y);
        if (vl.x > vr.x) std::swap(vl, vr);
        for (float x = roundf(vl.x) + xs / 2; x < roundf(vr.x); x += xs) {
            SoftVertex vm = interpolate<false>(vl, vr, vl.x, x, vr.x);
            vm.x = x, vm.y = y;
            drawPixel(vm);
        }
    }
}

void GpuRenderSoft::clipTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c) {
    // Copy the vertices to an initial working buffer
    SoftVertex vert[10], clip[10];
    vert[0] = a, vert[1] = b, vert[2] = c;
    uint8_t size = 3;

    // Clip a triangle on 6 sides using the Sutherland-Hodgman algorithm
    for (int i = 0; i < 6; i++) {
        // Build a list of clipped vertices from the working ones
        uint8_t idx = 0;
        for (int j = 0; j < size; j++) {
            // Get the current and previous working vertices
            SoftVertex &v1 = vert[j];
            SoftVertex &v2 = vert[(j - 1 + size) % size];

            // Get coordinates to check based on the side being clipped against
            float x1, x2;
            switch (i) {
                case 0: x1 = v1.x, x2 = v2.x; break;
                case 1: x1 = -v1.x, x2 = -v2.x; break;
                case 2: x1 = v1.y, x2 = v2.y; break;
                case 3: x1 = -v1.y, x2 = -v2.y; break;
                case 4: x1 = v1.z, x2 = v2.z; break;
                default: x1 = -v1.z, x2 = -v2.z; break;
            }

            // Add vertices to the list, clipping them if out of bounds
            if (x1 >= -v1.w) { // Current in bounds
                if (x2 < -v2.w) // Previous not in bounds
                    clip[idx++] = intersect(v1, v2, x1, x2);
                clip[idx++] = v1;
            }
            else if (x2 >= -v2.w) { // Previous in bounds
                clip[idx++] = intersect(v1, v2, x1, x2);
            }
        }

        // Update the working vertices
        size = idx;
        memcpy(vert, clip, size * sizeof(SoftVertex));
    }

    // Apply perspective division, scale coordinates, and draw clipped triangles in a fan
    if (size < 3) return;
    for (int i = 0; i < size; i++) {
        vert[i].x = (vert[i].x / vert[i].w) * viewScaleH + viewScaleH;
        vert[i].y = (vert[i].y / vert[i].w) * viewScaleV + viewScaleV;
        vert[i].z /= vert[i].w, vert[i].a /= vert[i].w;
        vert[i].r /= vert[i].w, vert[i].g /= vert[i].w, vert[i].b /= vert[i].w;
        vert[i].s0 /= vert[i].w, vert[i].s1 /= vert[i].w, vert[i].s2 /= vert[i].w;
        vert[i].t0 /= vert[i].w, vert[i].t1 /= vert[i].w, vert[i].t2 /= vert[i].w;
        vert[i].w = 1.0f / vert[i].w;
        if (i >= 2) drawTriangle(vert[0], vert[i - 1], vert[i]);
    }
}

void GpuRenderSoft::runShader(float (*input)[4], PrimMode mode, uint32_t idx) {
    // Get an entry in the vertex cache, up to the first 256 in a list
    idx = std::min<uint32_t>(0x100, idx);
    VertexCache &cache = vtxCache[idx];
    SoftVertex &vertex = cache.vtx;

    // Process a vertex if it can't be cached or is outdated
    if (idx >= 0x100 || cache.tag != vtxTag) {
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
                shdPc = (loopStack.front() >> 12) & 0xFFF;
                if (loopStack.front() >> 24)
                    loopStack[loopStack.size() - 1] -= BIT(24);
                else
                    loopStack.pop_front();
            }
        }

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
        cache.tag = vtxTag;
    }

    // Reset the vertex count if the primitive mode changed
    if (mode != SAME_PRIM) {
        vtxCount = 0;
        primMode = mode;
    }

    // Build a triangle based on the current primitive mode
    switch (primMode) {
    case TRIANGLES:
    case GEO_PRIM: // TODO?
        // Draw separate triangles every 3 vertices
        vertices[vtxCount] = vertex;
        if (++vtxCount < 3) return;
        vtxCount = 0;
        return clipTriangle(vertices[0], vertices[1], vertices[2]);

    case TRI_STRIPS:
        // Draw triangles in strips that reuse the last 2 vertices
        switch (vtxCount++) {
        case 0: case 1:
            vertices[vtxCount - 1] = vertex;
            return;
        case 2: // v0, v1, v2
            vertices[2] = vertex;
            return clipTriangle(vertices[0], vertices[1], vertices[2]);
        case 3: // v2, v1, v3
            vertices[0] = vertex;
            return clipTriangle(vertices[2], vertices[1], vertices[0]);
        case 4: // v2, v3, v4
            vertices[1] = vertex;
            return clipTriangle(vertices[2], vertices[0], vertices[1]);
        case 5: // v4, v3, v5
            vertices[2] = vertex;
            return clipTriangle(vertices[1], vertices[0], vertices[2]);
        case 6: // v4, v5, v6
            vertices[0] = vertex;
            return clipTriangle(vertices[1], vertices[2], vertices[0]);
        default: // v6, v5, v7
            vertices[1] = vertex;
            vtxCount = 2;
            return clipTriangle(vertices[0], vertices[2], vertices[1]);
        }

    case TRI_FANS:
        // Draw triangles in a fan that reuses the first vertex
        switch (vtxCount++) {
        case 0: case 1:
            vertices[vtxCount - 1] = vertex;
            return;
        case 2: // v0, v1, v2
            vertices[2] = vertex;
            return clipTriangle(vertices[0], vertices[1], vertices[2]);
        default: // v0, v2, v3
            vertices[1] = vertex;
            vtxCount = 2;
            return clipTriangle(vertices[0], vertices[2], vertices[1]);
        }
    }
}

void GpuRenderSoft::startList() {
    // Increment the vertex tag to invalidate cache and reset on overflow to avoid false positives
    if (++vtxTag) return;
    memset(vtxCache, 0, sizeof(vtxCache));
    vtxTag = 1;
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

void GpuRenderSoft::shdEx2(uint32_t opcode) {
    // Calculate the base-2 exponent of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[3] = src1[2] = src1[1] = src1[0] = exp2f(src1[0]);
    setDst((opcode >> 21) & 0x1F, desc, src1);
}

void GpuRenderSoft::shdLg2(uint32_t opcode) {
    // Calculate the base-2 logarithm of a source register's first component
    uint32_t desc = shdDesc[opcode & 0x7F];
    float *src1 = getSrc((opcode >> 12) & 0x7F, desc, (opcode >> 19) & 0x3);
    src1[3] = src1[2] = src1[1] = src1[0] = log2f(src1[0]);
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

void GpuRenderSoft::setVshEntry(uint16_t entry, uint16_t end) {
    // Set the vertex shader entry and end points
    vshEntry = entry;
    vshEnd = end;
}

void GpuRenderSoft::setOutMap(uint8_t (*map)[2]) {
    // Set the map of shader outputs to fixed semantics
    memcpy(outMap, map, sizeof(outMap));
}

void GpuRenderSoft::setTexAddr(int i, uint32_t address) {
    // Set one of the texture addresses and invalidate its cache
    texAddrs[i] = address;
    lastU[i] = lastV[i] = -1;
}

void GpuRenderSoft::setTexDims(int i, uint16_t width, uint16_t height) {
    // Set one of the texture unit widths/heights and invalidate its cache
    texWidths[i] = width;
    texHeights[i] = height;
    lastU[i] = lastV[i] = -1;
}

void GpuRenderSoft::setTexBorder(int i, float r, float g, float b, float a) {
    // Set one of the texture unit border colors
    texBorders[i][0] = r;
    texBorders[i][1] = g;
    texBorders[i][2] = b;
    texBorders[i][3] = a;
}

void GpuRenderSoft::setTexFmt(int i, TexFmt format) {
    // Set one of the texture formats and invalidate its cache
    texFmts[i] = format;
    lastU[i] = lastV[i] = -1;
}

void GpuRenderSoft::setCombSrc(int i, int j, CombSrc src) {
    // Set a texture combiner source and invalidate the start index
    combSrcs[i][j] = src;
    combStart = -1;
}

void GpuRenderSoft::setCombOper(int i, int j, OperFunc oper) {
    // Set a texture combiner operand and invalidate the start index
    combOpers[i][j] = oper;
    combStart = -1;
}

void GpuRenderSoft::setCombMode(int i, int j, CalcMode mode) {
    // Set a texture combiner mode and invalidate the start index
    combModes[i][j] = mode;
    combStart = -1;
}

void GpuRenderSoft::setCombColor(int i, float r, float g, float b, float a) {
    // Set one of the texture combiner constant colors
    combColors[i][0] = r;
    combColors[i][1] = g;
    combColors[i][2] = b;
    combColors[i][3] = a;
}

void GpuRenderSoft::setCombBufColor(float r, float g, float b, float a) {
    // Set the texture combiner initial buffer color
    combBufColor[0] = r;
    combBufColor[1] = g;
    combBufColor[2] = b;
    combBufColor[3] = a;
}

void GpuRenderSoft::setBlendColor(float r, float g, float b, float a) {
    // Set the blender constant color
    blendR = r;
    blendG = g;
    blendB = b;
    blendA = a;
}

void GpuRenderSoft::setStencilTest(TestFunc func, bool enable) {
    // Set the stencil test function and toggle
    stencilFunc = func;
    stencilEnable = enable;
}

void GpuRenderSoft::setStencilOps(StenOper fail, StenOper depFail, StenOper depPass) {
    // Set the stencil test result operations
    stencilFail = fail;
    stenDepFail = depFail;
    stenDepPass = depPass;
}

void GpuRenderSoft::setStencilMasks(uint8_t bufMask, uint8_t refMask) {
    // Set the stencil buffer and reference value masks
    stencilMasks[0] = bufMask;
    stencilMasks[1] = refMask;
}

void GpuRenderSoft::setBufferDims(uint16_t width, uint16_t height, bool flip) {
    // Set the render buffer width, height, and Y-flip
    bufWidth = width;
    bufHeight = height;
    flipY = flip;
}
