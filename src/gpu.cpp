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

void Gpu::writeCfg11GpuCnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_GPU_CNT register
    mask &= 0x1007F;
    cfg11GpuCnt = (cfg11GpuCnt & ~mask) | (value & mask);
}

void Gpu::writeMemfillDstAddr(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMFILL_DST_ADDR registers
    mask &= 0x1FFFFFFE;
    gpuMemfillDstAddr[i] = (gpuMemfillDstAddr[i] & ~mask) | (value & mask);
}

void Gpu::writeMemfillDstEnd(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMFILL_DST_END registers
    mask &= 0x1FFFFFFE;
    gpuMemfillDstEnd[i] = (gpuMemfillDstEnd[i] & ~mask) | (value & mask);
}

void Gpu::writeMemfillData(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMFILL_DATA registers
    gpuMemfillData[i] = (gpuMemfillData[i] & ~mask) | (value & mask);
}

void Gpu::writeMemfillCnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_MEMFILL_CNT registers
    uint32_t mask2 = (mask & 0x1F0300);
    gpuMemfillCnt[i] = (gpuMemfillCnt[i] & ~mask2) | (value & mask2);

    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(1)) && !(value & BIT(1)))
        gpuMemfillCnt[i] &= ~BIT(1);

    // Check the start and enable bits and trigger an interrupt if running
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(1))) return;
    gpuMemfillCnt[i] |= BIT(1);
    core->interrupts.sendInterrupt(ARM11, 0x28 + i);

    // Perform a memory fill with the selected data width
    uint32_t start = (gpuMemfillDstAddr[i] << 3), end = (gpuMemfillDstEnd[i] << 3);
    LOG_INFO("Performing GPU memory fill between 0x%X and 0x%X\n", start, end);
    switch ((gpuMemfillCnt[i] >> 8) & 0x3) {
    case 0: // 16-bit
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11, addr, gpuMemfillData[i]);
        return;

    case 1: case 3: // 24-bit
        value = (gpuMemfillData[i] & 0xFFFFFF) | (gpuMemfillData[i] << 24);
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11, addr, value >> (((addr - start) * 8) % 24));
        return;

    case 2: // 32-bit
        for (uint32_t addr = start; addr < end; addr += 4)
            core->memory.write<uint32_t>(ARM11, addr, gpuMemfillData[i]);
        return;
    }
}

void Gpu::writeMemcopySrcAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_SRC_ADDR register
    mask &= 0x1FFFFFFE;
    gpuMemcopySrcAddr = (gpuMemcopySrcAddr & ~mask) | (value & mask);
}

void Gpu::writeMemcopyDstAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_DST_ADDR register
    mask &= 0x1FFFFFFE;
    gpuMemcopyDstAddr = (gpuMemcopyDstAddr & ~mask) | (value & mask);
}

void Gpu::writeMemcopyFlags(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_FLAGS register
    mask &= 0x301772F;
    gpuMemcopyFlags = (gpuMemcopyFlags & ~mask) | (value & mask);
}

void Gpu::writeMemcopyCnt(uint32_t mask, uint32_t value) {
    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(8)) && !(value & BIT(8)))
        gpuMemcopyCnt &= ~BIT(8);

    // Check the start and enable bits and trigger an interrupt if running
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(4))) return;
    gpuMemcopyCnt |= BIT(8);
    core->interrupts.sendInterrupt(ARM11, 0x2C);

    // Check the copy mode and ignore display copies for now
    if (~gpuMemcopyFlags & BIT(3)) {
        LOG_CRIT("GPU memory copy started with unimplemented display mode\n");
        return;
    }

    // Get the source and destination parameters for a texture copy
    uint32_t srcAddr = (gpuMemcopySrcAddr << 3);
    uint32_t srcWidth = (gpuMemcopyTexSrcWidth << 4) & 0xFFFF0;
    uint32_t srcGap = (gpuMemcopyTexSrcWidth >> 12) & 0xFFFF0;
    uint32_t dstAddr = (gpuMemcopyDstAddr << 3);
    uint32_t dstWidth = (gpuMemcopyTexDstWidth << 4) & 0xFFFF0;
    uint32_t dstGap = (gpuMemcopyTexDstWidth >> 12) & 0xFFFF0;

    // Perform a texture copy, applying address gaps when widths are reached
    LOG_INFO("Performing GPU texture copy from 0x%X to 0x%X with size 0x%X\n", srcAddr, dstAddr, gpuMemcopyTexSize);
    for (uint32_t i = 0; i < gpuMemcopyTexSize; i += 4) {
        core->memory.write<uint32_t>(ARM11, dstAddr + i, core->memory.read<uint32_t>(ARM11, srcAddr + i));
        if (srcWidth && !((i + 4) % srcWidth)) srcAddr += srcGap;
        if (dstWidth && !((i + 4) % dstWidth)) dstAddr += dstGap;
    }
}

void Gpu::writeMemcopyTexSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_TEX_SIZE register
    mask &= 0xFFFFFFF0;
    gpuMemcopyTexSize = (gpuMemcopyTexSize & ~mask) | (value & mask);
}

void Gpu::writeMemcopyTexSrcWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_TEX_SRC_WIDTH register
    gpuMemcopyTexSrcWidth = (gpuMemcopyTexSrcWidth & ~mask) | (value & mask);
}

void Gpu::writeMemcopyTexDstWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_MEMCOPY_TEX_DST_WIDTH register
    gpuMemcopyTexDstWidth = (gpuMemcopyTexDstWidth & ~mask) | (value & mask);
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
