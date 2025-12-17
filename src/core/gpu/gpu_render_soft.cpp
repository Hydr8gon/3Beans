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
#include "gpu_render_soft.h"

const int16_t GpuRenderSoft::etc1Tables[][4] {
    { 2, 8, -2, -8 },
    { 5, 17, -5, -17 },
    { 9, 29, -9, -29 },
    { 13, 42, -13, -42 },
    { 18, 60, -18, -60 },
    { 24, 80, -24, -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 }
};

const uint8_t GpuRenderSoft::paramCounts[] = { 1, 2, 2, 2, 3, 2, 2, 2, 3, 3 };
SoftColor GpuRenderSoft::zeroColor = { 0.0f, 0.0f, 0.0f, 0.0f };
SoftColor GpuRenderSoft::oneColor = { 1.0f, 1.0f, 1.0f, 1.0f };
SoftColor GpuRenderSoft::stubColor = { 0.5f, 0.5f, 0.5f, 1.0f };

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

void GpuRenderSoft::updateTexel(int i, float s, float t) {
    // Catch silly invalid textures like in Pokemon X/Y
    if (!texWidths[i] || !texHeights[i]) {
        texColors[i] = zeroColor;
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
            texColors[i] = texBorders[i];
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
            texColors[i] = texBorders[i];
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
    if (lastU[i] == u && lastV[i] == v)
        return;

    // Convert the texture coordinates to a swizzled memory offset
    uint32_t value, ofs = (u & 0x1) | ((u << 1) & 0x4) | ((u << 2) & 0x10);
    ofs |= ((v << 1) & 0x2) | ((v << 2) & 0x8) | ((v << 3) & 0x20);
    ofs += ((v & ~0x7) * texWidths[i]) + ((u & ~0x7) << 3);

    // Read a texel and convert it to floats based on format
    switch (texFmts[i]) {
    case TEX_RGBA8:
        value = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs * 4);
        texColors[i].r = float((value >> 24) & 0xFF) / 0xFF;
        texColors[i].g = float((value >> 16) & 0xFF) / 0xFF;
        texColors[i].b = float((value >> 8) & 0xFF) / 0xFF;
        texColors[i].a = float((value >> 0) & 0xFF) / 0xFF;
        break;
    case TEX_RGB8:
        texColors[i].r = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 2)) / 0xFF;
        texColors[i].g = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 1)) / 0xFF;
        texColors[i].b = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs * 3 + 0)) / 0xFF;
        texColors[i].a = 1.0f;
        break;
    case TEX_RGB5A1:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        texColors[i].r = float((value >> 11) & 0x1F) / 0x1F;
        texColors[i].g = float((value >> 6) & 0x1F) / 0x1F;
        texColors[i].b = float((value >> 1) & 0x1F) / 0x1F;
        texColors[i].a = (value & BIT(0)) ? 1.0f : 0.0f;
        break;
    case TEX_RGB565:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        texColors[i].r = float((value >> 11) & 0x1F) / 0x1F;
        texColors[i].g = float((value >> 5) & 0x3F) / 0x3F;
        texColors[i].b = float((value >> 0) & 0x1F) / 0x1F;
        texColors[i].a = 1.0f;
        break;
    case TEX_RGBA4:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        texColors[i].r = float((value >> 12) & 0xF) / 0xF;
        texColors[i].g = float((value >> 8) & 0xF) / 0xF;
        texColors[i].b = float((value >> 4) & 0xF) / 0xF;
        texColors[i].a = float((value >> 0) & 0xF) / 0xF;
        break;
    case TEX_LA8:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        texColors[i].r = texColors[i].g = texColors[i].b = float((value >> 8) & 0xFF) / 0xFF;
        texColors[i].a = float((value >> 0) & 0xFF) / 0xFF;
        break;
    case TEX_RG8:
        value = core->memory.read<uint16_t>(ARM11, texAddrs[i] + ofs * 2);
        texColors[i].r = float((value >> 8) & 0xFF) / 0xFF;
        texColors[i].g = float((value >> 0) & 0xFF) / 0xFF;
        texColors[i].b = 0.0f, texColors[i].a = 1.0f;
        break;
    case TEX_L8:
        texColors[i].r = texColors[i].g = texColors[i].b =
            float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs)) / 0xFF;
        texColors[i].a = 1.0f;
        break;
    case TEX_A8:
        texColors[i].r = texColors[i].g = texColors[i].b = 0.0f;
        texColors[i].a = float(core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs)) / 0xFF;
        break;
    case TEX_LA4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs);
        texColors[i].r = texColors[i].g = texColors[i].b = float((value >> 4) & 0xF) / 0xF;
        texColors[i].a = float((value >> 0) & 0xF) / 0xF;
        break;
    case TEX_L4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs / 2);
        texColors[i].r = texColors[i].g = texColors[i].b = float((value >> ((ofs & 0x1) * 4)) & 0xF) / 0xF;
        texColors[i].a = 1.0f;
        break;
    case TEX_A4:
        value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs / 2);
        texColors[i].r = texColors[i].g = texColors[i].b = 0.0f;
        texColors[i].a = float((value >> ((ofs & 0x1) * 4)) & 0xF) / 0xF;
        break;
    case TEX_UNK:
        texColors[i] = oneColor;
        break;

    case TEX_ETC1: case TEX_ETC1A4:
        // Adjust the offset for 4x4 ETC1 tiles and read alpha if provided
        uint8_t idx = (u & 0x3) * 4 + (v & 0x3);
        if (texFmts[i] == TEX_ETC1A4) {
            ofs = (ofs & ~0xF) + 8;
            value = core->memory.read<uint8_t>(ARM11, texAddrs[i] + ofs - 8 + idx / 2);
            texColors[i].a = float((value >> ((idx & 0x1) * 4)) & 0xF) / 0xF;
        }
        else {
            ofs = (ofs & ~0xF) >> 1;
            texColors[i].a = 1.0f;
        }

        // Decode an ETC1 texel based on the block it falls in and the base color mode
        int32_t val1 = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 0);
        int32_t val2 = core->memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 4);
        if ((((val2 & BIT(0)) ? v : u) & 0x3) < 2) { // Block 1
            int16_t tbl = etc1Tables[(val2 >> 5) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
            if (val2 & BIT(1)) { // Differential
                texColors[i].r = float(((val2 >> 27) & 0x1F) * 0x21 / 4 + tbl);
                texColors[i].g = float(((val2 >> 19) & 0x1F) * 0x21 / 4 + tbl);
                texColors[i].b = float(((val2 >> 11) & 0x1F) * 0x21 / 4 + tbl);
            }
            else { // Individual
                texColors[i].r = float(((val2 >> 28) & 0xF) * 0x11 + tbl);
                texColors[i].g = float(((val2 >> 20) & 0xF) * 0x11 + tbl);
                texColors[i].b = float(((val2 >> 12) & 0xF) * 0x11 + tbl);
            }
        }
        else { // Block 2
            int16_t tbl = etc1Tables[(val2 >> 2) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
            if (val2 & BIT(1)) { // Differential
                texColors[i].r = float((((val2 >> 27) & 0x1F) + (int8_t(val2 >> 19) >> 5)) * 0x21 / 4 + tbl);
                texColors[i].g = float((((val2 >> 19) & 0x1F) + (int8_t(val2 >> 11) >> 5)) * 0x21 / 4 + tbl);
                texColors[i].b = float((((val2 >> 11) & 0x1F) + (int8_t(val2 >> 3) >> 5)) * 0x21 / 4 + tbl);
            }
            else { // Individual
                texColors[i].r = float(((val2 >> 24) & 0xF) * 0x11 + tbl);
                texColors[i].g = float(((val2 >> 16) & 0xF) * 0x11 + tbl);
                texColors[i].b = float(((val2 >> 8) & 0xF) * 0x11 + tbl);
            }
        }

        // Normalize and clamp the final color values
        texColors[i].r = std::min(1.0f, std::max(0.0f, texColors[i].r / 255));
        texColors[i].g = std::min(1.0f, std::max(0.0f, texColors[i].g / 255));
        texColors[i].b = std::min(1.0f, std::max(0.0f, texColors[i].b / 255));
        break;
    }

    // Cache texel coordinates to avoid decoding multiple times
    lastU[i] = u, lastV[i] = v;
}

void GpuRenderSoft::updateCombine(SoftVertex &v) {
    // Update the per-pixel color sources that are used
    if (paramMask & BIT(COMB_PRIM)) primColor = { v.r / v.w, v.g / v.w, v.b / v.w, v.a / v.w };
    if (paramMask & BIT(COMB_TEX0)) updateTexel(0, v.s0 / v.w, v.t0 / v.w);
    if (paramMask & BIT(COMB_TEX1)) updateTexel(1, v.s1 / v.w, v.t1 / v.w);
    if (paramMask & BIT(COMB_TEX2)) updateTexel(2, v.s2 / v.w, v.t2 / v.w);
    SoftColor c[3];

    // Process the texture combiner opcode cache
    for (int i = 0; i < combCache.size(); i++) {
        CombOpcode &op = combCache[i];
        if (op.id < 6) { // RGB
            // Load parameter colors and apply RGB operand adjustments
            for (int j = 0; j < paramCounts[op.mode]; j++) {
                SoftColor &in = *op.params[j].color, &out = c[j];
                switch (op.params[j].oper) {
                    case OPER_SRC: out.r = in.r, out.g = in.g, out.b = in.b; continue;
                    case OPER_1MSRC: out.r = 1.0f - in.r, out.g = 1.0f - in.g, out.b = 1.0f - in.b; continue;
                    case OPER_SRCA: out.r = out.g = out.b = in.a; continue;
                    case OPER_1MSRCA: out.r = out.g = out.b = 1.0f - in.a; continue;
                    case OPER_SRCR: out.r = out.g = out.b = in.r; continue;
                    case OPER_1MSRCR: out.r = out.g = out.b = 1.0f - in.r; continue;
                    case OPER_SRCG: out.r = out.g = out.b = in.g; continue;
                    case OPER_1MSRCG: out.r = out.g = out.b = 1.0f - in.g; continue;
                    case OPER_SRCB: out.r = out.g = out.b = in.b; continue;
                    case OPER_1MSRCB: out.r = out.g = out.b = 1.0f - in.b; continue;
                }
            }

            // Calculate RGB values for a combiner using cached information
            SoftColor &out = combBuffer[op.id];
            switch (op.mode) {
            case MODE_REPLACE:
                out.r = c[0].r;
                out.g = c[0].g;
                out.b = c[0].b;
                break;
            case MODE_MOD:
                out.r = c[0].r * c[1].r;
                out.g = c[0].g * c[1].g;
                out.b = c[0].b * c[1].b;
                break;
            case MODE_ADD:
                out.r = c[0].r + c[1].r;
                out.g = c[0].g + c[1].g;
                out.b = c[0].b + c[1].b;
                break;
            case MODE_ADDS:
                out.r = c[0].r + c[1].r - 0.5f;
                out.g = c[0].g + c[1].g - 0.5f;
                out.b = c[0].b + c[1].b - 0.5f;
                break;
            case MODE_INTERP:
                out.r = c[0].r * c[2].r + c[1].r * (1.0f - c[2].r);
                out.g = c[0].g * c[2].g + c[1].g * (1.0f - c[2].g);
                out.b = c[0].b * c[2].b + c[1].b * (1.0f - c[2].b);
                break;
            case MODE_SUB:
                out.r = c[0].r - c[1].r;
                out.g = c[0].g - c[1].g;
                out.b = c[0].b - c[1].b;
                break;
            case MODE_DOT3:
            case MODE_DOT3A:
                out.r = out.g = out.b = 4.0f * c[0].r - 0.5f * c[1].r - 0.5f +
                    c[0].g - 0.5f * c[1].g - 0.5f + c[0].b - 0.5f * c[1].b - 0.5f;
                break;
            case MODE_MULADD:
                out.r = (c[0].r * c[1].r) + c[2].r;
                out.g = (c[0].g * c[1].g) + c[2].g;
                out.b = (c[0].b * c[1].b) + c[2].b;
                break;
            case MODE_ADDMUL:
                out.r = (c[0].r + c[1].r) * c[2].r;
                out.g = (c[0].g + c[1].g) * c[2].g;
                out.b = (c[0].b + c[1].b) * c[2].b;
                break;
            case MODE_UNK:
                out.r = out.g = out.b = 1.0f;
                break;
            }

            // Ensure final values are within range
            out.r = std::min(1.0f, std::max(0.0f, out.r));
            out.g = std::min(1.0f, std::max(0.0f, out.g));
            out.b = std::min(1.0f, std::max(0.0f, out.b));
        }
        else { // Alpha
            // Load parameter colors and apply alpha operand adjustments
            for (int j = 0; j < paramCounts[op.mode]; j++) {
                SoftColor &in = *op.params[j].color, &out = c[j];
                switch (op.params[j].oper) {
                    case OPER_SRCA: out.a = in.a; continue;
                    case OPER_1MSRCA: out.a = 1.0f - in.a; continue;
                    case OPER_SRCR: out.a = in.r; continue;
                    case OPER_1MSRCR: out.a = 1.0f - in.r; continue;
                    case OPER_SRCG: out.a = in.g; continue;
                    case OPER_1MSRCG: out.a = 1.0f - in.g; continue;
                    case OPER_SRCB: out.a = in.b; continue;
                    case OPER_1MSRCB: out.a = 1.0f - in.b; continue;
                }
            }

            // Calculate alpha values for a combiner using cached information
            SoftColor &out = combBuffer[op.id - 6];
            switch (op.mode) {
            case MODE_REPLACE:
                out.a = c[0].a;
                break;
            case MODE_MOD:
                out.a = c[0].a * c[1].a;
                break;
            case MODE_ADD:
                out.a = c[0].a + c[1].a;
                break;
            case MODE_ADDS:
                out.a = c[0].a + c[1].a - 0.5f;
                break;
            case MODE_INTERP:
                out.a = c[0].a * c[2].a + c[1].a * (1.0f - c[2].a);
                break;
            case MODE_SUB:
                out.a = c[0].a - c[1].a;
                break;
            case MODE_DOT3A:
                out.a = 4.0f * c[0].a - 0.5f * c[1].a - 0.5f + c[0].a -
                    0.5f * c[1].a - 0.5f + c[0].a - 0.5f * c[1].a - 0.5f;
                break;
            case MODE_MULADD:
                out.a = (c[0].a * c[1].a) + c[2].a;
                break;
            case MODE_ADDMUL:
                out.a = (c[0].a + c[1].a) * c[2].a;
                break;
            case MODE_DOT3:
            case MODE_UNK:
                out.a = 1.0f;
                break;
            }

            // Ensure final values are within range
            out.a = std::min(1.0f, std::max(0.0f, out.a));
        }
    }
}

CombParam GpuRenderSoft::cacheParam(int i, int j) {
    // Cache a combiner parameter's operation and mark the source as used
    CombParam param;
    param.oper = combOpers[i][j];
    paramMask |= BIT(combSrcs[i][j]);

    // Cache a combiner parameter's color source
    switch (combSrcs[i][j]) {
        case COMB_PRIM: param.color = &primColor; return param;
        case COMB_TEX0: param.color = &texColors[0]; return param;
        case COMB_TEX1: param.color = &texColors[1]; return param;
        case COMB_TEX2: param.color = &texColors[2]; return param;
        case COMB_CONST: param.color = &combColors[i]; return param;
        case COMB_FRAG0: param.color = &stubColor; return param;
        case COMB_FRAG1: param.color = &stubColor; return param;
        case COMB_TEX3: param.color = &oneColor; return param;
        case COMB_UNK: param.color = &zeroColor; return param;

    case COMB_PREV:
        // Read zeros if there's no previous combiner
        if (i == 0) {
            param.color = &zeroColor;
            return param;
        }

        // Cache the previous RGB or alpha combiner and have it computed first
        ((combOpers[i][j] & ~0x1) != OPER_SRCA) ? cacheCombRgb(i - 1) : cacheCombA(i - 1);
        param.color = &combBuffer[i - 1];
        return param;

    case COMB_PRVBUF:
        if ((combOpers[i][j] & ~0x1) != OPER_SRCA) { // RGB
            // Check which RGB combiner should be buffered at this stage
            int idx = std::min(3, i - 1);
            while (idx >= 0 && !(combBufMask & BIT(idx))) idx--;

            // Read the buffer color if nothing replaced it
            if (idx < 0) {
                param.color = &combBufColor;
                return param;
            }

            // Cache the buffered combiner and have it computed first
            cacheCombRgb(idx);
            param.color = &combBuffer[idx];
        }
        else { // Alpha
            // Check which alpha combiner should be buffered at this stage
            int idx = std::min(3, i - 1);
            while (idx >= 0 && !(combBufMask & BIT(idx + 4))) idx--;

            // Read the buffer color if nothing replaced it
            if (idx < 0) {
                param.color = &combBufColor;
                return param;
            }

            // Cache the buffered combiner and have it computed first
            cacheCombA(idx);
            param.color = &combBuffer[idx];
        }
        return param;
    }
}

void GpuRenderSoft::cacheCombRgb(int i) {
    // Cache an RGB combiner's parameters if not already done
    if (combMask & BIT(i)) return;
    CombOpcode opcode;
    for (int j = 0; j < paramCounts[combModes[i][0]]; j++)
        opcode.params[j] = cacheParam(i, j);

    // Add the combiner to the opcode list and mark it as done
    opcode.mode = combModes[i][0];
    opcode.id = i;
    combCache.push_back(opcode);
    combMask |= BIT(i);
}

void GpuRenderSoft::cacheCombA(int i) {
    // Cache an alpha combiner's parameters if not already done
    if (combMask & BIT(i + 8)) return;
    CombOpcode opcode;
    for (int j = 0; j < paramCounts[combModes[i][1]]; j++)
        opcode.params[j] = cacheParam(i, j + 3);

    // Add the combiner to the opcode list and mark it as done
    opcode.mode = combModes[i][1];
    opcode.id = (i + 6);
    combCache.push_back(opcode);
    combMask |= BIT(i + 8);
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

    // Read the current depth and scale the incoming value based on buffer format
    uint32_t depth = 0;
    switch (depbufFmt) {
    case DEP_16:
        depth = core->memory.read<uint16_t>(ARM11, depbufAddr + ofs * 2);
        val = std::max<int>(0, p.z * -0xFFFF);
        break;
    case DEP_24:
        depth = core->memory.read<uint32_t>(ARM11, depbufAddr + ofs * 3) & 0xFFFFFF;
        val = std::max<int>(0, p.z * -0xFFFFFF);
        break;
    case DEP_24S8:
        depth = core->memory.read<uint32_t>(ARM11, depbufAddr + ofs * 4) & 0xFFFFFF;
        val = std::max<int>(0, p.z * -0xFFFFFF);
        break;
    }

    // Compare the incoming depth value with the current one
    bool pass;
    switch (depthFunc) {
        case TEST_NV: pass = false; break;
        case TEST_AL: pass = true; break;
        case TEST_EQ: pass = (val == depth); break;
        case TEST_NE: pass = (val != depth); break;
        case TEST_LT: pass = (val < depth); break;
        case TEST_LE: pass = (val <= depth); break;
        case TEST_GT: pass = (val > depth); break;
        case TEST_GE: pass = (val >= depth); break;
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
    updateCombine(p);
    SoftColor s0 = combBuffer[combEnd], d0;

    // Compare the source alpha value with the provided one
    switch (alphaFunc) {
        case TEST_NV: return;
        case TEST_AL: break;
        case TEST_EQ: if (s0.a == alphaValue) break; return;
        case TEST_NE: if (s0.a != alphaValue) break; return;
        case TEST_LT: if (s0.a < alphaValue) break; return;
        case TEST_LE: if (s0.a <= alphaValue) break; return;
        case TEST_GT: if (s0.a > alphaValue) break; return;
        case TEST_GE: if (s0.a >= alphaValue) break; return;
    }

    // Store the incoming depth value based on buffer format if enabled
    if (depbufMask & BIT(1)) {
        switch (depbufFmt) {
        case DEP_16:
            core->memory.write<uint16_t>(ARM11, depbufAddr + ofs * 2, val);
            break;
        case DEP_24:
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 3 + 3) << 24;
            core->memory.write<uint32_t>(ARM11, depbufAddr + ofs * 3, val);
            break;
        case DEP_24S8:
            val |= core->memory.read<uint8_t>(ARM11, depbufAddr + ofs * 4 + 3) << 24;
            core->memory.write<uint32_t>(ARM11, depbufAddr + ofs * 4, val);
            break;
        }
    }

    // Read color values to blend with based on buffer format
    switch (colbufFmt) {
    case COL_RGBA8:
        val = core->memory.read<uint32_t>(ARM11, colbufAddr + ofs * 4);
        d0.r = float((val >> 24) & 0xFF) / 0xFF;
        d0.g = float((val >> 16) & 0xFF) / 0xFF;
        d0.b = float((val >> 8) & 0xFF) / 0xFF;
        d0.a = float((val >> 0) & 0xFF) / 0xFF;
        break;
    case COL_RGB8:
        d0.r = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 2)) / 0xFF;
        d0.g = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 1)) / 0xFF;
        d0.b = float(core->memory.read<uint8_t>(ARM11, colbufAddr + ofs * 3 + 0)) / 0xFF;
        d0.a = 1.0f;
        break;
    case COL_RGB565:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        d0.r = float((val >> 11) & 0x1F) / 0x1F;
        d0.g = float((val >> 5) & 0x3F) / 0x3F;
        d0.b = float((val >> 0) & 0x1F) / 0x1F;
        d0.a = 1.0f;
        break;
    case COL_RGB5A1:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        d0.r = float((val >> 11) & 0x1F) / 0x1F;
        d0.g = float((val >> 6) & 0x1F) / 0x1F;
        d0.b = float((val >> 1) & 0x1F) / 0x1F;
        d0.a = (val & BIT(0)) ? 1.0f : 0.0f;
        break;
    case COL_RGBA4:
        val = core->memory.read<uint16_t>(ARM11, colbufAddr + ofs * 2);
        d0.r = float((val >> 12) & 0xF) / 0xF;
        d0.g = float((val >> 8) & 0xF) / 0xF;
        d0.b = float((val >> 4) & 0xF) / 0xF;
        d0.a = float((val >> 0) & 0xF) / 0xF;
        break;
    case COL_UNK:
        return;
    }

    // Multiply source RGB values with the selected operand
    SoftColor s1 = s0;
    switch (blendOpers[0]) {
        case BLND_ZERO: s1.r = s1.g = s1.b = 0.0f; break;
        case BLND_ONE: break;
        case BLND_SRC: s1.r *= s0.r, s1.g *= s0.g, s1.b *= s0.b; break;
        case BLND_1MSRC: s1.r *= 1.0f - s0.r, s1.g *= 1.0f - s0.g, s1.b *= 1.0f - s0.b; break;
        case BLND_DST: s1.r *= d0.r, s1.g *= d0.g, s1.b *= d0.b; break;
        case BLND_1MDST: s1.r *= 1.0f - d0.r, s1.g *= 1.0f - d0.g, s1.b *= 1.0f - d0.b; break;
        case BLND_SRCA: s1.r *= s0.a, s1.g *= s0.a, s1.b *= s0.a; break;
        case BLND_1MSRCA: s1.r *= 1.0f - s0.a, s1.g *= 1.0f - s0.a, s1.b *= 1.0f - s0.a; break;
        case BLND_DSTA: s1.r *= d0.a, s1.g *= d0.a, s1.b *= d0.a; break;
        case BLND_1MDSTA: s1.r *= 1.0f - d0.a, s1.g *= 1.0f - d0.a, s1.b *= 1.0f - d0.a; break;
        case BLND_CONST: s1.r *= blendColor.r, s1.g *= blendColor.g, s1.b *= blendColor.b; break;
        case BLND_1MCON: s1.r *= 1.0f - blendColor.r, s1.g *= 1.0f - blendColor.g, s1.b *= 1.0f - blendColor.b; break;
        case BLND_CONSTA: s1.r *= blendColor.a, s1.g *= blendColor.a, s1.b *= blendColor.a; break;
        case BLND_1MCONA: s1.r *= 1.0f - blendColor.a, s1.g *= 1.0f - blendColor.a, s1.b *= 1.0f - blendColor.a; break;
    }

    // Multiply source alpha values with the selected operand
    switch (blendOpers[2]) {
        case BLND_ZERO: s1.a = 0.0f; break;
        case BLND_ONE: break;
        case BLND_SRC: case BLND_SRCA: s1.a *= s0.a; break;
        case BLND_1MSRC: case BLND_1MSRCA: s1.a *= 1.0f - s0.a; break;
        case BLND_DST: case BLND_DSTA: s1.a *= d0.a; break;
        case BLND_1MDST: case BLND_1MDSTA: s1.a *= 1.0f - d0.a; break;
        case BLND_CONST: case BLND_CONSTA: s1.a *= blendColor.a; break;
        case BLND_1MCON: case BLND_1MCONA: s1.a *= 1.0f - blendColor.a; break;
    }

    // Multiply destination RGB values with the selected operand
    SoftColor d1 = d0;
    switch (blendOpers[1]) {
        case BLND_ZERO: d1.r = d1.g = d1.b = 0.0f; break;
        case BLND_ONE: break;
        case BLND_SRC: d1.r *= s0.r, d1.g *= s0.g, d1.b *= s0.b; break;
        case BLND_1MSRC: d1.r *= 1.0f - s0.r, d1.g *= 1.0f - s0.g, d1.b *= 1.0f - s0.b; break;
        case BLND_DST: d1.r *= d0.r, d1.g *= d0.g, d1.b *= d0.b; break;
        case BLND_1MDST: d1.r *= 1.0f - d0.r, d1.g *= 1.0f - d0.g, d1.b *= 1.0f - d0.b; break;
        case BLND_SRCA: d1.r *= s0.a, d1.g *= s0.a, d1.b *= s0.a; break;
        case BLND_1MSRCA: d1.r *= 1.0f - s0.a, d1.g *= 1.0f - s0.a, d1.b *= 1.0f - s0.a; break;
        case BLND_DSTA: d1.r *= d0.a, d1.g *= d0.a, d1.b *= d0.a; break;
        case BLND_1MDSTA: d1.r *= 1.0f - d0.a, d1.g *= 1.0f - d0.a, d1.b *= 1.0f - d0.a; break;
        case BLND_CONST: d1.r *= blendColor.r, d1.g *= blendColor.g, d1.b *= blendColor.b; break;
        case BLND_1MCON: d1.r *= 1.0f - blendColor.r, d1.g *= 1.0f - blendColor.g, d1.b *= 1.0f - blendColor.b; break;
        case BLND_CONSTA: d1.r *= blendColor.a, d1.g *= blendColor.a, d1.b *= blendColor.a; break;
        case BLND_1MCONA: d1.r *= 1.0f - blendColor.a, d1.g *= 1.0f - blendColor.a, d1.b *= 1.0f - blendColor.a; break;
    }

    // Multiply destination alpha values with the selected operand
    switch (blendOpers[3]) {
        case BLND_ZERO: d1.a = 0.0f; break;
        case BLND_ONE: break;
        case BLND_SRC: case BLND_SRCA: d1.a *= s0.a; break;
        case BLND_1MSRC: case BLND_1MSRCA: d1.a *= 1.0f - s0.a; break;
        case BLND_DST: case BLND_DSTA: d1.a *= d0.a; break;
        case BLND_1MDST: case BLND_1MDSTA: d1.a *= 1.0f - d0.a; break;
        case BLND_CONST: case BLND_CONSTA: d1.a *= blendColor.a; break;
        case BLND_1MCON: case BLND_1MCONA: d1.a *= 1.0f - blendColor.a; break;
    }

    // Blend the source and destination RGB values based on mode
    float r, g, b, a;
    switch (blendModes[0]) {
        default: r = s1.r + d1.r, g = s1.g + d1.g, b = s1.b + d1.b; break;
        case MODE_SUB: r = s1.r - d1.r, g = s1.g - d1.g, b = s1.b - d1.b; break;
        case MODE_RSUB: r = d1.r - s1.r, g = d1.g - s1.g, b = d1.b - s1.b; break;
        case MODE_MIN: r = std::min(s1.r, d1.r), g = std::min(s1.g, d1.g), b = std::min(s1.b, d1.b); break;
        case MODE_MAX: r = std::max(s1.r, d1.r), g = std::max(s1.g, d1.g), b = std::max(s1.b, d1.b); break;
    }

    // Blend the source and destination alpha values based on mode
    switch (blendModes[1]) {
        default: a = s1.a + d1.a; break;
        case MODE_SUB: a = s1.a - d1.a; break;
        case MODE_RSUB: a = d1.a - s1.a; break;
        case MODE_MIN: a = std::min(s1.a, d1.a); break;
        case MODE_MAX: a = std::max(s1.a, d1.a); break;
    }

    // Clamp the final color values
    r = std::min(1.0f, std::max(0.0f, r));
    g = std::min(1.0f, std::max(0.0f, g));
    b = std::min(1.0f, std::max(0.0f, b));
    a = std::min(1.0f, std::max(0.0f, a));

    // Preserve the original value of some channels if any are unmasked
    if (colbufMask != 0xF) {
        if (!colbufMask) return;
        if (~colbufMask & BIT(0)) r = d0.r;
        if (~colbufMask & BIT(1)) g = d0.g;
        if (~colbufMask & BIT(2)) b = d0.b;
        if (~colbufMask & BIT(3)) a = d0.a;
    }

    // Store the final color values based on buffer format
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
    if (!std::isfinite(cross) || (cullMode == CULL_FRONT && (cross < 0)) || (cullMode == CULL_BACK && (cross > 0)))
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

    // Check if the texture combiner cache is dirty
    if (combEnd > 5) {
        // Skip over combiners set to simply output the previous color
        combEnd = 5;
        uint8_t &i = combEnd;
        while (i > 0 && combModes[i][0] == MODE_REPLACE && combModes[i][1] == MODE_REPLACE && combSrcs[i][0] ==
            COMB_PREV && combSrcs[i][3] == COMB_PREV && combOpers[i][0] == OPER_SRC && combOpers[i][3] == OPER_SRCA)
            combEnd--;

        // Reset and regenerate the cache
        combCache = {};
        paramMask = combMask = 0;
        cacheCombRgb(combEnd);
        cacheCombA(combEnd);
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

void GpuRenderSoft::submitVertex(SoftVertex &vertex) {
    // Build a triangle based on the current primitive mode
    switch (primMode) {
    case TRIANGLES:
    case GEO_PRIM:
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

void GpuRenderSoft::setPrimMode(PrimMode mode) {
    // Change primitive mode and reset the vertex count
    primMode = mode;
    vtxCount = 0;
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
    texBorders[i].r = r;
    texBorders[i].g = g;
    texBorders[i].b = b;
    texBorders[i].a = a;
}

void GpuRenderSoft::setTexFmt(int i, TexFmt format) {
    // Set one of the texture formats and invalidate its cache
    texFmts[i] = format;
    lastU[i] = lastV[i] = -1;
}

void GpuRenderSoft::setCombSrc(int i, int j, CombSrc src) {
    // Set a texture combiner source and invalidate the cache
    combSrcs[i][j] = src;
    combEnd = -1;
}

void GpuRenderSoft::setCombOper(int i, int j, CombOper oper) {
    // Set a texture combiner operand and invalidate the cache
    combOpers[i][j] = oper;
    combEnd = -1;
}

void GpuRenderSoft::setCombMode(int i, int j, CalcMode mode) {
    // Set a texture combiner mode and invalidate the cache
    combModes[i][j] = mode;
    combEnd = -1;
}

void GpuRenderSoft::setCombColor(int i, float r, float g, float b, float a) {
    // Set one of the texture combiner constant colors
    combColors[i].r = r;
    combColors[i].g = g;
    combColors[i].b = b;
    combColors[i].a = a;
}

void GpuRenderSoft::setCombBufColor(float r, float g, float b, float a) {
    // Set the texture combiner initial buffer color
    combBufColor.r = r;
    combBufColor.g = g;
    combBufColor.b = b;
    combBufColor.a = a;
}

void GpuRenderSoft::setCombBufMask(uint8_t mask) {
    // Set a texture combiner buffer mask and invalidate the cache
    combBufMask = mask;
    combEnd = -1;
}

void GpuRenderSoft::setBlendColor(float r, float g, float b, float a) {
    // Set the blender constant color
    blendColor.r = r;
    blendColor.g = g;
    blendColor.b = b;
    blendColor.a = a;
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
