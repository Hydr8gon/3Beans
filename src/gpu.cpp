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

#include "core.h"

bool Gpu::checkInterrupt(int i) {
    // Trigger a GPU interrupt if enabled and its request/compare bytes match
    if ((gpuIrqMask & BITL(i)) || ((gpuIrqCmp[i >> 2] ^ gpuIrqReq[i >> 2]) & (0xFF << ((i & 0x3) * 8))))
        return false;
    gpuIrqStat |= BITL(i);
    core->interrupts.sendInterrupt(ARM11, 0x2D);
    return true;
}

uint32_t Gpu::getDispSrcOfs(uint32_t x, uint32_t y, uint32_t width) {
    // Get a linear source offset in linear-to-linear and linear-to-tiled display copy modes
    if (gpuMemcpyFlags & (BIT(5) | BIT(1)))
        return (y * width) + x;

    // Get a swizzled source offset in tiled-to-linear display copy mode
    uint32_t ofs = (((y >> 3) * (width >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    return ofs | ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
}

uint32_t Gpu::getDispDstOfs(uint32_t x, uint32_t y, uint32_t width) {
    // Get a linear destination offset in linear-to-linear and tiled-to-linear display copy modes
    if ((gpuMemcpyFlags & (BIT(5) | BIT(1))) != BIT(1))
        return (y * width) + x;

    // Get a swizzled destination offset in linear-to-tiled display copy mode
    uint32_t ofs = (((y >> 3) * (width >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    return ofs | ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
}

void Gpu::setReady(int i) {
    // Trigger a GPU set ready interrupt after some time
    gpuMemsetCnt[i] |= BIT(1);
    core->interrupts.sendInterrupt(ARM11, 0x28 + i);
}

void Gpu::cpyReady() {
    // Trigger a GPU copy ready interrupt after some time
    gpuMemcpyCnt |= BIT(8);
    core->interrupts.sendInterrupt(ARM11, 0x2C);
}

void Gpu::writeCfg11GpuCnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_GPU_CNT register
    mask &= 0x1007F;
    cfg11GpuCnt = (cfg11GpuCnt & ~mask) | (value & mask);
}

void Gpu::writeMemsetDstAddr(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMSET_DST_ADDR registers
    mask &= 0x1FFFFFFE;
    gpuMemsetDstAddr[i] = (gpuMemsetDstAddr[i] & ~mask) | (value & mask);
}

void Gpu::writeMemsetDstEnd(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMSET_DST_END registers
    mask &= 0x1FFFFFFE;
    gpuMemsetDstEnd[i] = (gpuMemsetDstEnd[i] & ~mask) | (value & mask);
}

void Gpu::writeMemsetData(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMSET_DATA registers
    gpuMemsetData[i] = (gpuMemsetData[i] & ~mask) | (value & mask);
}

void Gpu::writeMemsetCnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMSET_CNT registers
    uint32_t mask2 = (mask & 0x1F0300);
    gpuMemsetCnt[i] = (gpuMemsetCnt[i] & ~mask2) | (value & mask2);

    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(1)) && !(value & BIT(1)))
        gpuMemsetCnt[i] &= ~BIT(1);

    // Check if a set should start and schedule the ready signal based on size
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(1))) return;
    uint32_t start = (gpuMemsetDstAddr[i] << 3), end = (gpuMemsetDstEnd[i] << 3);
    core->schedule(Task(GPU_SET0_READY + i), (end - start) / 8);
    LOG_INFO("Performing GPU memory set at 0x%X with size 0x%X\n", start, end - start);

    // Perform a memory set with the selected data width
    switch ((gpuMemsetCnt[i] >> 8) & 0x3) {
    case 0: // 16-bit
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11, addr, gpuMemsetData[i]);
        return;

    case 1: case 3: // 24-bit
        value = (gpuMemsetData[i] & 0xFFFFFF) | (gpuMemsetData[i] << 24);
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11, addr, value >> (((addr - start) * 8) % 24));
        return;

    case 2: // 32-bit
        for (uint32_t addr = start; addr < end; addr += 4)
            core->memory.write<uint32_t>(ARM11, addr, gpuMemsetData[i]);
        return;
    }
}

void Gpu::writeMemcpySrcAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_SRC_ADDR register
    mask &= 0x1FFFFFFE;
    gpuMemcpySrcAddr = (gpuMemcpySrcAddr & ~mask) | (value & mask);
}

void Gpu::writeMemcpyDstAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_DST_ADDR register
    mask &= 0x1FFFFFFE;
    gpuMemcpyDstAddr = (gpuMemcpyDstAddr & ~mask) | (value & mask);
}

void Gpu::writeMemcpyDispDstSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_DISP_DST_SIZE register
    mask &= 0xFFF8FFF8;
    gpuMemcpyDispDstSize = (gpuMemcpyDispDstSize & ~mask) | (value & mask);
}

void Gpu::writeMemcpyDispSrcSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_DISP_SRC_SIZE register
    mask &= 0xFFF8FFF8;
    gpuMemcpyDispSrcSize = (gpuMemcpyDispSrcSize & ~mask) | (value & mask);
}

void Gpu::writeMemcpyFlags(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_FLAGS register
    mask &= 0x301772F;
    gpuMemcpyFlags = (gpuMemcpyFlags & ~mask) | (value & mask);
}

void Gpu::writeMemcpyCnt(uint32_t mask, uint32_t value) {
    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(8)) && !(value & BIT(8)))
        gpuMemcpyCnt &= ~BIT(8);

    // Check if a copy should start and get common parameters
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(4))) return;
    uint32_t srcAddr = (gpuMemcpySrcAddr << 3);
    uint32_t dstAddr = (gpuMemcpyDstAddr << 3);

    // Perform a texture copy if enabled, which ignores most settings
    if (gpuMemcpyFlags & BIT(3)) {
        // Get the source and destination parameters for a texture copy
        uint32_t srcWidth = (gpuMemcpyTexSrcWidth << 4) & 0xFFFF0;
        uint32_t srcGap = (gpuMemcpyTexSrcWidth >> 12) & 0xFFFF0;
        uint32_t dstWidth = (gpuMemcpyTexDstWidth << 4) & 0xFFFF0;
        uint32_t dstGap = (gpuMemcpyTexDstWidth >> 12) & 0xFFFF0;

        // Schedule the ready signal based on texture size
        core->schedule(GPU_CPY_READY, gpuMemcpyTexSize / 4);
        LOG_INFO("Performing GPU texture copy from 0x%X to 0x%X with size 0x%X\n", srcAddr, dstAddr, gpuMemcpyTexSize);

        // Perform a texture copy, applying address gaps when widths are reached
        for (uint32_t i = 0; i < gpuMemcpyTexSize; i += 4) {
            core->memory.write<uint32_t>(ARM11, dstAddr + i, core->memory.read<uint32_t>(ARM11, srcAddr + i));
            if (srcWidth && !((i + 4) % srcWidth)) srcAddr += srcGap;
            if (dstWidth && !((i + 4) % dstWidth)) dstAddr += dstGap;
        }
        return;
    }

    // Get the source and destination parameters for a display copy
    uint8_t srcFmt = (gpuMemcpyFlags >> 8) & 0x7;
    uint16_t srcWidth = (gpuMemcpyFlags & BIT(2)) ? gpuMemcpyDispSrcSize : gpuMemcpyDispDstSize;
    uint8_t dstFmt = (gpuMemcpyFlags >> 12) & 0x7;
    uint16_t dstWidth = (gpuMemcpyDispDstSize >> 0);
    uint16_t dstHeight = (gpuMemcpyDispDstSize >> 16);
    uint8_t scaleType = (gpuMemcpyFlags >> 24) & 0x3;

    // Scale the destination dimensions based on scale type
    dstWidth >>= (scaleType == 0x1 || scaleType == 0x2);
    dstHeight >>= (scaleType == 0x2);

    // Adjust the Y order based on the vertical flip bit
    int yStart, yInc;
    if (gpuMemcpyFlags & BIT(0)) // Flipped
        yStart = dstHeight - 1, yInc = -1;
    else // Normal
        yStart = 0, yInc = 1;

    // Schedule the ready signal based on display size
    core->schedule(GPU_CPY_READY, dstWidth * dstHeight);

    // Log the display copy and check for unimplemented flags
    LOG_INFO("Performing GPU display copy from 0x%X to 0x%X with width %d and height %d\n",
        srcAddr, dstAddr, dstWidth, dstHeight);
    if (gpuMemcpyFlags & BIT(16))
        LOG_CRIT("Unhandled GPU display copy tile size: 32x32\n");

    // Perform an 8x8 tiled to linear display copy based on format and scale settings
    uint8_t r, g, b, a;
    uint32_t ofs, c0, c1, c2, c3;
    switch (srcFmt) {
    case 0x0: // RGBA8
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = getDispSrcOfs(x, y0, srcWidth) * 4;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs);
                    r = ((c0 >> 24) & 0xFF);
                    g = ((c0 >> 16) & 0xFF);
                    b = ((c0 >> 8) & 0xFF);
                    a = ((c0 >> 0) & 0xFF);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = getDispSrcOfs(x << 1, y0, srcWidth) * 4;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x4);
                    r = (((c0 >> 24) & 0xFF) + ((c1 >> 24) & 0xFF)) / 2;
                    g = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF)) / 2;
                    b = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF)) / 2;
                    a = (((c0 >> 0) & 0xFF) + ((c1 >> 0) & 0xFF)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 4;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x4);
                    c2 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x8);
                    c3 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0xC);
                    r = (((c0 >> 24) & 0xFF) + ((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF) + ((c3 >> 24) & 0xFF)) / 4;
                    g = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF) + ((c3 >> 16) & 0xFF)) / 4;
                    b = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF) + ((c2 >> 8) & 0xFF) + ((c3 >> 8) & 0xFF)) / 4;
                    a = (((c0 >> 0) & 0xFF) + ((c1 >> 0) & 0xFF) + ((c2 >> 0) & 0xFF) + ((c3 >> 0) & 0xFF)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                    ofs = getDispDstOfs(x, y1, dstWidth) * 4;
                    c0 = (r << 24) | (g << 16) | (b << 8) | a;
                    core->memory.write<uint32_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                case 0x1: // RGB8
                    ofs = getDispDstOfs(x, y1, dstWidth) * 3;
                    core->memory.write<uint8_t>(ARM11, dstAddr + ofs + 2, r);
                    core->memory.write<uint8_t>(ARM11, dstAddr + ofs + 1, g);
                    core->memory.write<uint8_t>(ARM11, dstAddr + ofs + 0, b);
                    continue;
                case 0x2: // RGB565
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 255) << 11) | ((g * 63 / 255) << 5) | (b * 31 / 255);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 255) << 11) | ((g * 31 / 255) << 6) | ((b * 31 / 255) << 1) | bool(a);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 255) << 12) | ((g * 15 / 255) << 8) | ((b * 15 / 255) << 4) | (a * 15 / 255);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                }
            }
        }
        return;

    case 0x1: // RGB8
        if (dstFmt != 0x1) { // RGB8
            LOG_CRIT("Invalid destination format for RGB8 display copy: 0x%X\n", dstFmt);
            return;
        }

        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = getDispSrcOfs(x, y0, srcWidth) * 3;
                    r = core->memory.read<uint8_t>(ARM11, ofs + 2);
                    g = core->memory.read<uint8_t>(ARM11, ofs + 1);
                    b = core->memory.read<uint8_t>(ARM11, ofs + 0);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = getDispSrcOfs(x << 1, y0, srcWidth) * 3;
                    r = (core->memory.read<uint8_t>(ARM11, ofs + 2) + core->memory.read<uint8_t>(ARM11, ofs + 5)) / 2;
                    g = (core->memory.read<uint8_t>(ARM11, ofs + 1) + core->memory.read<uint8_t>(ARM11, ofs + 4)) / 2;
                    b = (core->memory.read<uint8_t>(ARM11, ofs + 0) + core->memory.read<uint8_t>(ARM11, ofs + 3)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 3;
                    r = (core->memory.read<uint8_t>(ARM11, ofs + 2) + core->memory.read<uint8_t>(ARM11, ofs + 5) +
                        core->memory.read<uint8_t>(ARM11, ofs + 8) + core->memory.read<uint8_t>(ARM11, ofs + 11)) / 4;
                    g = (core->memory.read<uint8_t>(ARM11, ofs + 1) + core->memory.read<uint8_t>(ARM11, ofs + 4) +
                        core->memory.read<uint8_t>(ARM11, ofs + 7) + core->memory.read<uint8_t>(ARM11, ofs + 10)) / 4;
                    b = (core->memory.read<uint8_t>(ARM11, ofs + 0) + core->memory.read<uint8_t>(ARM11, ofs + 3) +
                        core->memory.read<uint8_t>(ARM11, ofs + 6) + core->memory.read<uint8_t>(ARM11, ofs + 9)) / 4;
                    break;
                }

                ofs = getDispDstOfs(x, y1, dstWidth) * 3;
                core->memory.write<uint8_t>(ARM11, ofs + 2, r);
                core->memory.write<uint8_t>(ARM11, ofs + 1, g);
                core->memory.write<uint8_t>(ARM11, ofs + 0, b);
            }
        }
        return;

    case 0x2: // RGB565
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs);
                    r = ((c0 >> 11) & 0x1F);
                    g = ((c0 >> 5) & 0x3F);
                    b = ((c0 >> 0) & 0x1F);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x2);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F)) / 2;
                    g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F)) / 2;
                    b = (((c0 >> 0) & 0x1F) + ((c1 >> 0) & 0x1F)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x2);
                    c2 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x4);
                    c3 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x6);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + ((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 4;
                    g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + ((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) / 4;
                    b = (((c0 >> 0) & 0x1F) + ((c1 >> 0) & 0x1F) + ((c2 >> 0) & 0x1F) + ((c3 >> 0) & 0x1F)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGB565 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | (g << 5) | b;
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | ((g * 31 / 63) << 6) | (b << 1) | 0x1;
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 31) << 12) | ((g * 15 / 63) << 8) | ((b * 15 / 31) << 4) | 0xF;
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                }
            }
        }
        return;

    case 0x3: // RGB5A1
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs);
                    r = ((c0 >> 11) & 0x1F);
                    g = ((c0 >> 6) & 0x1F);
                    b = ((c0 >> 1) & 0x1F);
                    a = (c0 & BIT(0)) * 4;
                    break;
                case 0x1: // 2x1 downscale
                    ofs = getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x2);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F)) / 2;
                    g = (((c0 >> 6) & 0x1F) + ((c1 >> 6) & 0x1F)) / 2;
                    b = (((c0 >> 1) & 0x1F) + ((c1 >> 1) & 0x1F)) / 2;
                    a = ((c0 & BIT(0)) + (c1 & BIT(0))) * 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x2);
                    c2 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x4);
                    c3 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x6);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + ((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 4;
                    g = (((c0 >> 6) & 0x1F) + ((c1 >> 6) & 0x1F) + ((c2 >> 6) & 0x1F) + ((c3 >> 6) & 0x1F)) / 4;
                    b = (((c0 >> 1) & 0x1F) + ((c1 >> 1) & 0x1F) + ((c2 >> 1) & 0x1F) + ((c3 >> 1) & 0x1F)) / 4;
                    a = ((c0 & BIT(0)) + (c1 & BIT(0)) + (c2 & BIT(0)) + (c3 & BIT(0)));
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGB5A1 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | ((g * 63 / 31) << 5) | b;
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | (g << 6) | (b << 1) | bool(a);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 31) << 12) | ((g * 15 / 31) << 8) | ((b * 15 / 31) << 4) | (a * 15 / 4);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                }
            }
        }
        return;

    default: // RGBA4
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs);
                    r = ((c0 >> 12) & 0xF);
                    g = ((c0 >> 8) & 0xF);
                    b = ((c0 >> 4) & 0xF);
                    a = ((c0 >> 0) & 0xF);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint16_t>(ARM11, srcAddr + ofs + 0x2);
                    r = (((c0 >> 12) & 0xF) + ((c1 >> 12) & 0xF)) / 2;
                    g = (((c0 >> 8) & 0xF) + ((c1 >> 8) & 0xF)) / 2;
                    b = (((c0 >> 4) & 0xF) + ((c1 >> 4) & 0xF)) / 2;
                    a = (((c0 >> 0) & 0xF) + ((c1 >> 0) & 0xF)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x0);
                    c1 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x2);
                    c2 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x4);
                    c3 = core->memory.read<uint32_t>(ARM11, srcAddr + ofs + 0x6);
                    r = (((c0 >> 12) & 0xF) + ((c1 >> 12) & 0xF) + ((c2 >> 12) & 0xF) + ((c3 >> 12) & 0xF)) / 4;
                    g = (((c0 >> 8) & 0xF) + ((c1 >> 8) & 0xF) + ((c2 >> 8) & 0xF) + ((c3 >> 8) & 0xF)) / 4;
                    b = (((c0 >> 4) & 0xF) + ((c1 >> 4) & 0xF) + ((c2 >> 4) & 0xF) + ((c3 >> 4) & 0xF)) / 4;
                    a = (((c0 >> 0) & 0xF) + ((c1 >> 0) & 0xF) + ((c2 >> 0) & 0xF) + ((c3 >> 0) & 0xF)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGBA4 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 15) << 11) | ((g * 63 / 15) << 5) | (b * 31 / 15);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 15) << 11) | ((g * 31 / 15) << 6) | ((b * 31 / 15) << 1) | bool(a);
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 12) | (g << 8) | (b << 4) | a;
                    core->memory.write<uint16_t>(ARM11, dstAddr + ofs, c0);
                    continue;
                }
            }
        }
        return;
    }
}

void Gpu::writeMemcpyTexSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_TEX_SIZE register
    mask &= 0xFFFFFFF0;
    gpuMemcpyTexSize = (gpuMemcpyTexSize & ~mask) | (value & mask);
}

void Gpu::writeMemcpyTexSrcWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_TEX_SRC_WIDTH register
    gpuMemcpyTexSrcWidth = (gpuMemcpyTexSrcWidth & ~mask) | (value & mask);
}

void Gpu::writeMemcpyTexDstWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCPY_TEX_DST_WIDTH register
    gpuMemcpyTexDstWidth = (gpuMemcpyTexDstWidth & ~mask) | (value & mask);
}

void Gpu::writeIrqAck(int i, uint32_t mask, uint32_t value) {
    // Write to some of the GPU_IRQ_REQ registers and acknowledge if they don't match
    gpuIrqReq[i] = (gpuIrqReq[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if ((mask & (0xFF << (j * 8))) && !checkInterrupt((i << 2) + j))
            gpuIrqStat &= ~BITL((i << 2) + j);
}

void Gpu::writeIrqCmp(int i, uint32_t mask, uint32_t value) {
    // Write to some of the GPU_IRQ_CMP registers and check if they match
    gpuIrqCmp[i] = (gpuIrqCmp[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if (mask & (0xFF << (j * 8)))
            checkInterrupt((i << 2) + j);
}

void Gpu::writeIrqMaskL(uint32_t mask, uint32_t value) {
    // Write to the low part of the GPU_IRQ_MASK register
    gpuIrqMask = (gpuIrqMask & ~mask) | (value & mask);
}

void Gpu::writeIrqMaskH(uint32_t mask, uint32_t value) {
    // Write to the high part of the GPU_IRQ_MASK register
    gpuIrqMask = (gpuIrqMask & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
}

void Gpu::writeIrqAutostop(uint32_t mask, uint32_t value) {
    // Write to the GPU_IRQ_AUTOSTOP register
    mask &= 0x1;
    gpuIrqAutostop = (gpuIrqAutostop & ~mask) | (value & mask);
}
