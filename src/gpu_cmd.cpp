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

TEMPLATE4(void Gpu::writeIrqReq, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 8, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 12, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdbufSize, 0, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdbufAddr, 0, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdbufJump, 0, uint32_t, uint32_t)

void Gpu::runCommands() {
    // Execute GPU commands until the end is reached
    while (cmdAddr < cmdEnd) {
        // Decode the command header
        uint32_t header = core->memory.read<uint32_t>(ARM11, cmdAddr + 4);
        uint32_t mask = maskTable[(header >> 16) & 0xF];
        uint8_t count = (header >> 20) & 0xFF;
        curCmd = (header & 0x3FF);

        // Adjust the address and write the first parameter
        uint32_t address = (cmdAddr + 4);
        cmdAddr += ((count + 3) << 2) & ~0x7;
        (this->*cmdWrites[curCmd])(mask, core->memory.read<uint32_t>(ARM11, address - 4));

        // Write the remaining parameters if any, with optionally increasing command ID
        if (header & BIT(31)) // Increasing
            for (int i = 0; i < count; i++)
                (this->*cmdWrites[++curCmd & 0x3FF])(mask, core->memory.read<uint32_t>(ARM11, address += 4));
        else // Fixed
            for (int i = 0; i < count; i++)
                (this->*cmdWrites[curCmd])(mask, core->memory.read<uint32_t>(ARM11, address += 4));
    }

    // Reset the command address to indicate being stopped
    LOG_INFO("Finished executing GPU command list\n");
    cmdAddr = -1;
}

template <int i> void Gpu::writeIrqReq(uint32_t mask, uint32_t value) {
    // Write IRQ request bytes, check if they match, and stop execution on interrupt if enabled
    gpuIrqReq[i] = (gpuIrqReq[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if ((mask & (0xFF << (j * 8))) && checkInterrupt((i << 2) + j) && gpuIrqAutostop)
            cmdAddr = -1;
}

template <int i> void Gpu::writeCmdbufSize(uint32_t mask, uint32_t value) {
    // Write to one of the command buffer sizes
    mask &= 0x1FFFFE;
    gpuCmdbufSize[i] = (gpuCmdbufSize[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeCmdbufAddr(uint32_t mask, uint32_t value) {
    // Write to one of the command buffer addresses
    mask &= 0x1FFFFFFE;
    gpuCmdbufAddr[i] = (gpuCmdbufAddr[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeCmdbufJump(uint32_t mask, uint32_t value) {
    // Jump to a new command buffer and start executing if stopped
    bool stopped = (cmdAddr == -1);
    cmdAddr = (gpuCmdbufAddr[i] << 3);
    cmdEnd = (gpuCmdbufAddr[i] + gpuCmdbufSize[i]) << 3;
    LOG_INFO("Jumping to GPU command list at 0x%X with size 0x%X\n", cmdAddr, cmdEnd - cmdAddr);
    if (stopped) runCommands();
}

void Gpu::writeUnkCmd(uint32_t mask, uint32_t value) {
    // Catch unknown GPU command IDs
    LOG_WARN("Unknown GPU command ID: 0x%X\n", curCmd & 0x3FF);
}
