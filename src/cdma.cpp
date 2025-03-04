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

void Cdma::triggerUpdate() {
    // Schedule a CDMA update if one hasn't been already
    if (scheduled) return;
    core->schedule(Task(CDMA0_UPDATE + id), 1);
    scheduled = true;
}

void Cdma::update() {
    // Run CDMA channels that are currently executing
    for (int i = 0; i < 9; i++)
        if ((csrs[i] & 0xF) == 0x1)
            runOpcodes(i);
    scheduled = false;
}

void Cdma::runOpcodes(int i) {
    // Run opcodes on a CDMA thread until it's no longer in executing state
    dbgstatus &= ~BIT(0); // Debug idle
    while ((csrs[i] & 0xF) == 0x1) {
        switch (uint8_t op = core->memory.read<uint8_t>(id == XDMA ? ARM9 : ARM11, cpcs[i])) {
        default:
            // Catch unknown CDMA opcodes
            if (id == XDMA)
                LOG_CRIT("Unknown XDMA channel %d opcode: 0x%02X\n", i, op);
            else
                LOG_CRIT("Unknown CDMA%d channel %d opcode: 0x%02X\n", id, i, op);
            csrs[i] &= ~0xF; // Stopped
            return;
        }
    }
}

void Cdma::writeDbgcmd(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Start executing a DMA channel or manager thread from the DBGINST registers
    // TODO: handle this better (these registers shouldn't be readable)
    int i = (dbginst0 & BIT(0)) ? ((dbginst0 >> 8) & 0x7) : 8;
    const uint32_t starts[] = { 0x10200D0A, 0x10206D0A, 0x1000CD0A };
    cpcs[i] = starts[id];
    csrs[i] = (csrs[i] & ~0xF) | 0x1; // Executing
    dbgstatus |= BIT(0); // Debug busy
    triggerUpdate();
}

void Cdma::writeDbginst0(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to one of the CDMA DBGINST0 registers
    mask &= 0xFFFF0701;
    dbginst0 = (dbginst0 & ~mask) | (value & mask);
}

void Cdma::writeDbginst1(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to one of the CDMA DBGINST1 registers
    dbginst1 = (dbginst1 & ~mask) | (value & mask);
}
