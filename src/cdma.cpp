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

Cdma::Cdma(Core *core, CdmaId id): core(core), id(id) {
    // Set the CPU for memory accesses
    cpu = (id == XDMA) ? ARM9 : ARM11;
}

void Cdma::sendInterrupt(int type) {
    // Set interrupt bits in the CDMA registers if enabled
    if (inten & BIT(type)) {
        intEventRis |= BIT(type);
        intmis |= BIT(type);
    }

    // Unhalt any CDMA channels that are waiting for this interrupt
    for (int i = 0; i < 8; i++) {
        if ((csrs[i] & 0x1FF) != ((type << 4) | 0x7)) continue;
        csrs[i] = (csrs[i] & ~0xC1FF) | 0x1; // Executing
        triggerUpdate();
    }
}

void Cdma::triggerUpdate() {
    // Schedule a CDMA update if one hasn't been already
    if (scheduled) return;
    core->schedule(Task(CDMA0_UPDATE + id), 1);
    scheduled = true;
}

void Cdma::update() {
    // Run CDMA channels that are currently executing
    scheduled = false;
    for (int i = 0; i < 9; i++)
        if ((csrs[i] & 0xF) == 0x1)
            runOpcodes(i);
}

void Cdma::runOpcodes(int i) {
    // Run a CDMA thread until it's no longer in executing state
    while ((csrs[i] & 0xF) == 0x1) {
        // Only run one opcode if debugging
        if (dbgstatus && dbgId == i) {
            dbgstatus &= ~BIT(0); // Debug idle
            csrs[i] &= ~0xC1FF; // Stopped
        }

        // Decode and execute a CDMA opcode
        switch (uint8_t op = core->memory.read<uint8_t>(cpu, cpcs[i]++)) {
            case 0x00: dmaEnd(i); break; // DMAEND
            case 0x01: dmaEnd(i); break; // DMAKILL (stub)
            case 0x12: dmaStubC(i, 0); break; // DMARMB (stub)
            case 0x13: dmaStubC(i, 0); break; // DMAWMB (stub)
            case 0x18: break; // DMANOP
            case 0x20: dmaLp0(i); break; // DMALP lpc0,len
            case 0x22: dmaLp1(i); break; // DMALP lpc1,len
            case 0x30: dmaWfp(i, 0); break; // DMAWFP periph,single
            case 0x31: dmaWfp(i, BIT(15)); break; // DMAWFP periph,periph
            case 0x32: dmaWfp(i, BIT(14)); break; // DMAWFP periph,burst
            case 0x35: dmaStubC(i, 1); break; // DMAFLUSHP (stub)
            case 0xA0: dmaGoNs(i); break; // DMAGO chan,imm32 (stub)
            case 0xA2: dmaGoNs(i); break; // DMAGO chan,imm32,ns
            case 0xBC: dmaMov(i); break; // DMAMOV reg,imm32

        default:
            // Catch unknown CDMA opcodes
            if (id == XDMA)
                LOG_CRIT("Unknown XDMA channel %d opcode: 0x%02X\n", i, op);
            else
                LOG_CRIT("Unknown CDMA%d channel %d opcode: 0x%02X\n", id, i, op);
            csrs[i] &= ~0xC1FF; // Stopped
            return;
        }
    }
}

void Cdma::fault(int i, int type) {
    // Put a channel in faulting mode and set the type
    csrs[i] = (csrs[i] & ~0xC1FF) | 0xF;
    ftrs[i] = BIT(type);
    fsrc |= BIT(i);
}

void Cdma::dmaStubC(int i, int inc) {
    // Stub a CDMA channel command and skip any operands
    if (i == 8) return fault(i, 0); // Channel-exclusive
    cpcs[i] += inc;
}

void Cdma::dmaEnd(int i) { // DMAEND
    // Stop a thread and reset it
    if (i < 8) fifos[i] = {};
    csrs[i] &= ~0xC1FF; // Stopped
    ftrs[i] = 0;
    fsrc &= ~BIT(i);

    // Log the end of a channel's execution
    if (i == 8) return;
    if (id == XDMA)
        LOG_INFO("Stopping XDMA channel %d execution\n", i);
    else
        LOG_INFO("Stopping CDMA%d channel %d execution\n", id, i);
}

void Cdma::dmaLp0(int i) { // DMALP lpc0,len
    // Set a channel's loop count 0 register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    lc0s[i] = core->memory.read<uint8_t>(cpu, cpcs[i]++);
}

void Cdma::dmaLp1(int i) { // DMALP lpc1,len
    // Set a channel's loop count 1 register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    lc1s[i] = core->memory.read<uint8_t>(cpu, cpcs[i]++);
}

void Cdma::dmaWfp(int i, uint16_t burst) { // DMAWFP periph,type
    // Put a channel in peripheral wait mode and set its burst state
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core->memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if (id == XDMA && periph >= 8) return fault(i, 1);
    csrs[i] = (csrs[i] & ~0xC1FF) | burst | (periph << 4) | 0x7;

    // Log unimplemented peripheral interrupts
    if (id == XDMA) {
        if (periph >= 6) return;
        LOG_CRIT("XDMA channel %d waiting for unimplemented DRQ type: 0x%X\n", i, periph);
    }
    else {
        LOG_CRIT("Starting CDMA%d channel %d waiting for unimplemented DRQ type: 0x%X\n", id, i, periph);
    }
}

void Cdma::dmaGoNs(int i) { // DMAGO chan,imm32,ns
    // Set a channel's program counter and start non-secure execution
    if (i != 8) return fault(i, 0); // Manager-exclusive
    uint8_t chan = core->memory.read<uint8_t>(cpu, cpcs[i]++);
    if (chan >= (8 >> (id == XDMA))) return fault(i, 1);
    cpcs[chan] = core->memory.read<uint32_t>(cpu, (cpcs[i] += 4) - 4);
    csrs[chan] = (csrs[chan] & ~0xC1FF) | 0x1; // Executing
    triggerUpdate();

    // Log the start of a channel's execution
    if (id == XDMA)
        LOG_INFO("Starting XDMA channel %d execution at 0x%08X\n", chan, cpcs[chan]);
    else
        LOG_INFO("Starting CDMA%d channel %d execution at 0x%08X\n", id, chan, cpcs[chan]);
}

void Cdma::dmaMov(int i) { // DMAMOV reg,imm32
    // Set one of a channel's registers
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t reg = core->memory.read<uint8_t>(cpu, cpcs[i]++);
    if (reg >= 3) return fault(i, 1);
    uint32_t *regs[] = { sars, ccrs, dars };
    regs[reg][i] = core->memory.read<uint32_t>(cpu, (cpcs[i] += 4) - 4);
}

void Cdma::writeInten(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to the CDMA INTEN register
    inten = (inten & ~mask) | (value & mask);
}

void Cdma::writeIntclr(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Clear enabled interrupt bits in the CDMA registers
    mask &= inten;
    intEventRis &= ~(value & mask);
    intmis &= ~(value & mask);
}

void Cdma::writeDbgcmd(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core->interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Start executing a DMA channel or manager thread from the DBGINST registers
    // TODO: handle this better (these registers shouldn't be readable)
    dbgId = (dbginst0 & BIT(0)) ? ((dbginst0 >> 8) & 0x7) : 8;
    const uint32_t starts[] = { 0x10200D0A, 0x10206D0A, 0x1000CD0A };
    cpcs[dbgId] = starts[id];
    csrs[dbgId] = (csrs[dbgId] & ~0xC1FF) | 0x1; // Executing
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
