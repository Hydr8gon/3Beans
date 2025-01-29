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

    // Check the start bit and trigger an interrupt if enabled
    if (!(value & mask & BIT(0))) return;
    gpuMemfillCnt[i] |= BIT(1);
    core->interrupts.sendInterrupt(false, 0x28 + i);

    // Perform a memory fill with the selected data width
    uint32_t start = (gpuMemfillDstAddr[i] << 3), end = (gpuMemfillDstEnd[i] << 3);
    LOG_INFO("Performing GPU memory fill from 0x%08X to 0x%08X\n", start, end);
    switch ((gpuMemfillCnt[i] >> 8) & 0x3) {
    case 0: // 16-bit
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11A, addr, gpuMemfillData[i]);
        return;

    case 1: case 3: // 24-bit
        value = (gpuMemfillData[i] & 0xFFFFFF) | (gpuMemfillData[i] << 24);
        for (uint32_t addr = start; addr < end; addr += 2)
            core->memory.write<uint16_t>(ARM11A, addr, value >> (((addr - start) * 8) % 24));
        return;

    case 2: // 32-bit
        for (uint32_t addr = start; addr < end; addr += 4)
            core->memory.write<uint32_t>(ARM11A, addr, gpuMemfillData[i]);
        return;
    }
}
