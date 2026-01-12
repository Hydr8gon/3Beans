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

#include "../core.h"

Cdma::Cdma(Core &core, CdmaId id): core(core), id(id) {
    // Set the CPU for memory accesses
    cpu = (id == XDMA) ? ARM9 : ARM11;
}

void Cdma::setDrq(uint8_t type) {
    // Set a DRQ type's active bit
    drqMask |= BIT(type);

    // Update if a channel should be unhalted
    for (int i = 0; i < 8; i++)
        if ((csrs[i] & 0x1FF) == ((type << 4) | 0x7))
            return triggerUpdate();
}

void Cdma::clearDrq(uint8_t type) {
    // Clear a DRQ type's active bit
    drqMask &= ~BIT(type);
}

void Cdma::triggerUpdate() {
    // Schedule a CDMA update if one hasn't been already
    if (scheduled) return;
    core.schedule(Task(CDMA0_UPDATE + id), 1);
    scheduled = true;
}

void Cdma::update() {
    // Unhalt any channels that were waiting for an active DRQ
    for (int i = 0; i < 8; i++)
        if ((csrs[i] & 0xF) == 0x7 && (drqMask & BIT((csrs[i] >> 4) & 0x1F)))
            csrs[i] = (csrs[i] & ~0xC1FF) | 0x1; // Executing

    // Run any channels that are now executing
    scheduled = false;
    for (int i = 0; i < 9; i++)
        if ((csrs[i] & 0xF) == 0x1) runOpcodes(i);
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
        switch (uint8_t op = core.memory.read<uint8_t>(cpu, cpcs[i]++)) {
            case 0x00: dmaEnd(i); break; // DMAEND
            case 0x01: dmaEnd(i); break; // DMAKILL (stub)
            case 0x04: dmaLd(i); break; // DMALD
            case 0x05: dmaLds(i); break; // DMALDS
            case 0x07: dmaLdb(i); break; // DMALDB
            case 0x08: dmaSt(i); break; // DMAST
            case 0x09: dmaSts(i); break; // DMASTS
            case 0x0B: dmaStb(i); break; // DMASTB
            case 0x12: dmaStubC(i, 0); break; // DMARMB (stub)
            case 0x13: dmaStubC(i, 0); break; // DMAWMB (stub)
            case 0x18: break; // DMANOP
            case 0x20: dmaLp0(i); break; // DMALP lpc0,len
            case 0x22: dmaLp1(i); break; // DMALP lpc1,len
            case 0x25: dmaLdps(i); break; // DMALDPS periph
            case 0x27: dmaLdpb(i); break; // DMALDPB periph
            case 0x29: dmaStps(i); break; // DMASTPS periph
            case 0x2B: dmaStpb(i); break; // DMASTPB periph
            case 0x30: dmaWfp(i, 0); break; // DMAWFP periph,single
            case 0x31: dmaWfp(i, BIT(15)); break; // DMAWFP periph,periph
            case 0x32: dmaWfp(i, BIT(14)); break; // DMAWFP periph,burst
            case 0x34: dmaSev(i); break; // DMASEV event
            case 0x35: dmaStubC(i, 1); break; // DMAFLUSHP (stub)
            case 0x38: dmaLpend0(i); break; // DMALPEND lpc0
            case 0x3C: dmaLpend1(i); break; // DMALPEND lpc1
            case 0x54: dmaAddhS(i); break; // DMAADDH SAR,imm16
            case 0x56: dmaAddhD(i); break; // DMAADDH DAR,imm16
            case 0xA0: dmaGoNs(i); break; // DMAGO chan,imm32 (stub)
            case 0xA2: dmaGoNs(i); break; // DMAGO chan,imm32,ns
            case 0xBC: dmaMov(i); break; // DMAMOV reg,imm32

        default:
            // Catch unknown CDMA opcodes
            if (id == XDMA)
                LOG_CRIT("Unknown XDMA channel %d opcode: 0x%X\n", i, op);
            else
                LOG_CRIT("Unknown CDMA%d channel %d opcode: 0x%X\n", id, i, op);
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

    // Send a fault interrupt to the CPU
    core.interrupts.sendInterrupt(cpu, (id == XDMA) ? 29 : ((id == CDMA0) ? 0x39 : 0x3B));
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

void Cdma::dmaLd(int i) { // DMALD
    // Get the configured load size and length
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t size = 1 << ((ccrs[i] >> 1) & 0x7);
    uint8_t len = burstReq ? (((ccrs[i] >> 4) & 0xF) + 1) : 1;

    // Load values depending on size and increment the address if enabled
    // TODO: handle the access type bits
    switch (size) {
    case 1: // 8-bit
        for (int j = 0; j < len; j++) {
            fifos[i].push(core.memory.read<uint8_t>(cpu, sars[i]));
            if (ccrs[i] & BIT(0)) sars[i]++;
        }
        return;

    case 2: // 16-bit
        for (int j = 0; j < len; j++) {
            uint16_t value = core.memory.read<uint16_t>(cpu, sars[i]);
            for (int k = 0; k < 16; k += 8)
                fifos[i].push(value >> k);
            if (ccrs[i] & BIT(0)) sars[i] += 2;
        }
        return;

    case 4: // 32-bit
        for (int j = 0; j < len; j++) {
            uint32_t value = core.memory.read<uint32_t>(cpu, sars[i]);
            for (int k = 0; k < 32; k += 8)
                fifos[i].push(value >> k);
            if (ccrs[i] & BIT(0)) sars[i] += 4;
        }
        return;

    case 8: // 64-bit
        for (int j = 0; j < len; j++) {
            uint64_t value = core.memory.read<uint32_t>(cpu, sars[i]);
            value |= uint64_t(core.memory.read<uint32_t>(cpu, sars[i] + 4)) << 32;
            for (int k = 0; k < 64; k += 8)
                fifos[i].push(value >> k);
            if (ccrs[i] & BIT(0)) sars[i] += 8;
        }
        return;

    default:
        if (id == XDMA)
            LOG_CRIT("XDMA channel %d loading with invalid size: %d\n", i, size);
        else
            LOG_CRIT("CDMA%d channel %d loading with invalid size: %d\n", id, i, size);
        return;
    }
}

void Cdma::dmaLds(int i) { // DMALDS
    // Perform a single load if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    if (!burstReq) dmaLd(i);
}

void Cdma::dmaLdb(int i) { // DMALDB
    // Perform a burst load if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    if (burstReq) dmaLd(i);
}

void Cdma::dmaSt(int i) { // DMAST
    // Get the configured store size and length
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t size = 1 << ((ccrs[i] >> 15) & 0x7);
    uint8_t len = burstReq ? (((ccrs[i] >> 18) & 0xF) + 1) : 1;

    // Store values depending on size and increment the address if enabled
    // TODO: handle the access type bits
    switch (size) {
    case 1: // 8-bit
        for (int j = 0; j < len; j++) {
            core.memory.write<uint8_t>(cpu, dars[i], fifos[i].front());
            fifos[i].pop();
            if (ccrs[i] & BIT(14)) dars[i]++;
        }
        return;

    case 2: // 16-bit
        for (int j = 0; j < len; j++) {
            uint16_t value = 0;
            for (int k = 0; k < 16; k += 8) {
                value |= fifos[i].front() << k;
                fifos[i].pop();
            }
            core.memory.write<uint16_t>(cpu, dars[i], value);
            if (ccrs[i] & BIT(14)) dars[i] += 2;
        }
        return;

    case 4: // 32-bit
        for (int j = 0; j < len; j++) {
            uint32_t value = 0;
            for (int k = 0; k < 32; k += 8) {
                value |= fifos[i].front() << k;
                fifos[i].pop();
            }
            core.memory.write<uint32_t>(cpu, dars[i], value);
            if (ccrs[i] & BIT(14)) dars[i] += 4;
        }
        return;

    case 8: // 64-bit
        for (int j = 0; j < len; j++) {
            uint64_t value = 0;
            for (int k = 0; k < 64; k += 8) {
                value |= uint64_t(fifos[i].front()) << k;
                fifos[i].pop();
            }
            core.memory.write<uint32_t>(cpu, dars[i], value);
            core.memory.write<uint32_t>(cpu, dars[i] + 4, value >> 32);
            if (ccrs[i] & BIT(14)) dars[i] += 8;
        }
        return;

    default:
        if (id == XDMA)
            LOG_CRIT("XDMA channel %d storing with invalid size: %d\n", i, size);
        else
            LOG_CRIT("CDMA%d channel %d storing with invalid size: %d\n", id, i, size);
        return;
    }
}

void Cdma::dmaSts(int i) { // DMASTS
    // Perform a single store if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    if (!burstReq) dmaSt(i);
}

void Cdma::dmaStb(int i) { // DMASTB
    // Perform a burst store if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    if (burstReq) dmaSt(i);
}

void Cdma::dmaLp0(int i) { // DMALP lpc0,len
    // Set a channel's loop count 0 register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    lc0s[i] = core.memory.read<uint8_t>(cpu, cpcs[i]++);
}

void Cdma::dmaLp1(int i) { // DMALP lpc1,len
    // Set a channel's loop count 1 register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    lc1s[i] = core.memory.read<uint8_t>(cpu, cpcs[i]++);
}

void Cdma::dmaLdps(int i) { // DMALDPS periph
    // Stub notifying a peripheral and perform a single load if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && periph >= 8) || (id == CDMA0 && periph >= 18)) return fault(i, 1);
    if (!burstReq) dmaLd(i);
}

void Cdma::dmaLdpb(int i) { // DMALDPB periph
    // Stub notifying a peripheral and perform a burst load if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && periph >= 8) || (id == CDMA0 && periph >= 18)) return fault(i, 1);
    if (burstReq) dmaLd(i);
}

void Cdma::dmaStps(int i) { // DMASTPS periph
    // Stub notifying a peripheral and perform a single store if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && periph >= 8) || (id == CDMA0 && periph >= 18)) return fault(i, 1);
    if (!burstReq) dmaSt(i);
}

void Cdma::dmaStpb(int i) { // DMASTPB periph
    // Stub notifying a peripheral and perform a burst store if requested
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && periph >= 8) || (id == CDMA0 && periph >= 18)) return fault(i, 1);
    if (burstReq) dmaSt(i);
}

void Cdma::dmaWfp(int i, uint16_t burst) { // DMAWFP periph,type
    // Put a channel in peripheral wait mode and set its burst state
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t periph = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && periph >= 8) || (id == CDMA0 && periph >= 18)) return fault(i, 1);
    csrs[i] = (csrs[i] & ~0xC1FF) | burst | (periph << 4) | 0x7;
    burstReq = burst; // Assume burst for peripheral

    // Update right away if the DRQ is already active
    if (drqMask & BIT(periph))
        triggerUpdate();

    // Log unimplemented peripheral DRQs
    if (id == XDMA) {
        if (periph >= 0x6) return;
        LOG_CRIT("XDMA channel %d waiting for unimplemented DRQ type: 0x%X\n", i, periph);
    }
    else {
        if (periph == 0x4 || (periph >= 0x6 && periph <= 0x8) || periph == 0xA) return;
        LOG_CRIT("CDMA%d channel %d waiting for unimplemented DRQ type: 0x%X\n", id, i, periph);
    }
}

void Cdma::dmaSev(int i) { // DMASEV event
    // Send an event and check if it's in interrupt mode
    uint8_t event = core.memory.read<uint8_t>(cpu, cpcs[i]++) >> 3;
    if ((id == XDMA && event >= 12) || (id == CDMA0 && event >= 16)) return fault(i, 1);
    intEventRis |= BIT(event);
    if (~inten & BIT(event)) return;

    // Send an interrupt to the CPU depending on event type
    intmis |= BIT(event);
    if (id == XDMA && event < 5)
        core.interrupts.sendInterrupt(ARM9, 28);
    else if (id == CDMA0 && event < 9)
        core.interrupts.sendInterrupt(ARM11, 0x30 + event);
    else if (id == CDMA1)
        core.interrupts.sendInterrupt(ARM11, 0x3A);
}

void Cdma::dmaLpend0(int i) { // DMALPEND lpc0
    // Decrement a channel's loop count 0 register and jump backwards if non-zero
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t jump = core.memory.read<uint8_t>(cpu, cpcs[i]++);
    if (lc0s[i]-- != 0) cpcs[i] -= jump + 2;
}

void Cdma::dmaLpend1(int i) { // DMALPEND lpc1
    // Decrement a channel's loop count 1 register and jump backwards if non-zero
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t jump = core.memory.read<uint8_t>(cpu, cpcs[i]++);
    if (lc1s[i]-- != 0) cpcs[i] -= jump + 2;
}

void Cdma::dmaAddhS(int i) { // DMAADDH SAR,imm16
    // Add a positive 16-bit immediate to a channel's source address register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    sars[i] += core.memory.read<uint16_t>(cpu, cpcs[i]);
    cpcs[i] += 2;
}

void Cdma::dmaAddhD(int i) { // DMAADDH DAR,imm16
    // Add a positive 16-bit immediate to a channel's destination address register
    if (i == 8) return fault(i, 0); // Channel-exclusive
    dars[i] += core.memory.read<uint16_t>(cpu, cpcs[i]);
    cpcs[i] += 2;
}

void Cdma::dmaGoNs(int i) { // DMAGO chan,imm32,ns
    // Set a channel's program counter and start non-secure execution
    if (i != 8) return fault(i, 0); // Manager-exclusive
    uint8_t chan = core.memory.read<uint8_t>(cpu, cpcs[i]++);
    if (chan >= (8 >> (id == XDMA))) return fault(i, 1);
    cpcs[chan] = core.memory.read<uint32_t>(cpu, (cpcs[i] += 4) - 4);
    csrs[chan] = (csrs[chan] & ~0xC1FF) | 0x1; // Executing
    triggerUpdate();

    // Log the start of a channel's execution
    if (id == XDMA)
        LOG_INFO("Starting XDMA channel %d execution at 0x%X\n", chan, cpcs[chan]);
    else
        LOG_INFO("Starting CDMA%d channel %d execution at 0x%X\n", id, chan, cpcs[chan]);
}

void Cdma::dmaMov(int i) { // DMAMOV reg,imm32
    // Set one of a channel's registers
    if (i == 8) return fault(i, 0); // Channel-exclusive
    uint8_t reg = core.memory.read<uint8_t>(cpu, cpcs[i]++);
    if (reg >= 3) return fault(i, 1);
    uint32_t *regs[] = { sars, ccrs, dars };
    regs[reg][i] = core.memory.read<uint32_t>(cpu, (cpcs[i] += 4) - 4);

    // Catch unimplemented endian bits being set
    if (reg != 1 || !(ccrs[i] & 0x70000000)) return;
    if (id == XDMA)
        LOG_CRIT("XDMA channel %d using unimplemented endian settings\n", i);
    else
        LOG_CRIT("CDMA%d channel %d using unimplemented endian settings\n", id, i);
}

void Cdma::writeInten(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core.interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to the CDMA INTEN register
    inten = (inten & ~mask) | (value & mask);
}

void Cdma::writeIntclr(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core.interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Clear enabled interrupt bits in the CDMA registers
    mask &= inten;
    intEventRis &= ~(value & mask);
    intmis &= ~(value & mask);
}

void Cdma::writeDbgcmd(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core.interrupts.readCfg11MpClkcnt() & 0x70000))
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
    if (id == CDMA1 && !(core.interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to one of the CDMA DBGINST0 registers
    mask &= 0xFFFF0701;
    dbginst0 = (dbginst0 & ~mask) | (value & mask);
}

void Cdma::writeDbginst1(uint32_t mask, uint32_t value) {
    // Ensure the new CDMA engine is enabled if accessed
    if (id == CDMA1 && !(core.interrupts.readCfg11MpClkcnt() & 0x70000))
        return;

    // Write to one of the CDMA DBGINST1 registers
    dbginst1 = (dbginst1 & ~mask) | (value & mask);
}
