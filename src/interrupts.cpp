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

void Interrupts::sendInterrupt(bool arm9, int type) {
    // Send an interrupt to the ARM9
    if (arm9) {
        // Set the interrupt's request bit
        if (type != -1)
            irqIf |= BIT(type);

        // Schedule an interrupt if any requested interrupts are enabled
        if (scheduled[ARM9] || !(irqIe & irqIf)) return;
        core->schedule(ARM9_INTERRUPT, 2);
        scheduled[ARM9] = true;
        return;
    }

    // Send an interrupt to the ARM11 cores if enabled
    if (!mpIge) return;
    for (CpuId id = ARM11A; id < ARM9; id = CpuId(id + 1)) {
        // Set the interrupt's pending bit on targeted cores
        if (!mpIle[id]) continue;
        if (type != -1 && (readMpTarget(id, type) & BIT(id)))
            mpIp[id][type >> 5] |= BIT(type & 0x1F);

        // Schedule an interrupt if any pending interrupts are enabled
        if (scheduled[id]) continue;
        for (int i = 0; i < 4; i++) {
            if (!(mpIe[i] & mpIp[id][i])) continue;
            core->schedule(Task(ARM11A_INTERRUPT + id), 1);
            scheduled[id] = true;
            break;
        }
    }
}

void Interrupts::interrupt(CpuId id) {
    // Set the unhalted bit for extra ARM11 cores if started, or ignore the interrupt
    scheduled[id] = false;
    if (id == ARM11C || id == ARM11D) {
        if (cfg11MpBootcnt[id - 2] & BIT(4))
            cfg11MpBootcnt[id - 2] |= BIT(5);
        else return;
    }

    // Trigger an interrupt on a CPU if enabled and unhalt it
    if (~core->arms[id].cpsr & BIT(7))
        core->arms[id].exception(0x18);
    core->arms[id].halted &= ~(BIT(0) | BIT(1));
}

void Interrupts::halt(CpuId id, uint8_t type) {
    // Halt a CPU and check if all ARM11 cores have been halted
    core->arms[id].halted |= BIT(type);
    if (id != ARM9 && core->arms[ARM11A].halted && core->arms[ARM11B].halted &&
            core->arms[ARM11C].halted && core->arms[ARM11D].halted) {
        // Update the current clock/FCRAM mode and trigger an interrupt if changed
        uint8_t mode = (cfg11MpClkcnt & 0x1) ? (cfg11MpClkcnt & 0x7) : 0;
        if (mode != ((cfg11MpClkcnt >> 16) & 0x7)) {
            cfg11MpClkcnt = (cfg11MpClkcnt & ~0x70000) | (mode << 16) | BIT(15);
            core->memory.updateMap(false, 0x28000000, 0x2FFFFFFF);
            core->memory.updateMap(true, 0x28000000, 0x2FFFFFFF);
            sendInterrupt(false, 0x58);
            LOG_INFO("Changing to clock/FCRAM mode %d\n", mode);
        }
    }

    // Clear the unhalted bit for extra ARM11 cores
    if (id != ARM11C && id != ARM11D) return;
    cfg11MpBootcnt[id - 2] &= ~BIT(5);

    // Disable an extra ARM11 core if newly stopped
    if ((cfg11MpBootcnt[id - 2] & (BIT(0) | BIT(4))) != BIT(4)) return;
    cfg11MpBootcnt[id - 2] &= ~BIT(4);
    LOG_INFO("Disabling ARM11 core %d\n", id);

    // Switch to 2-core execution if both extra cores are now stopped
    if (~cfg11MpBootcnt[(id - 2) ^ 1] & BIT(4))
        core->schedule(TOGGLE_RUN_FUNC, 1);
}

uint16_t Interrupts::readCfg11Socinfo() {
    // Read a value indicating the console type
    return core->n3dsMode ? 0x7 : 0x1;
}

uint8_t Interrupts::readCfg11MpBootcnt(int i) {
    // Read from a CFG11_MP_BOOTCNT register or a dummy value depending on ID
    if (!core->n3dsMode) return 0; // N3DS-exclusive
    return (i >= 2) ? cfg11MpBootcnt[i - 2] : 0x30;
}

uint32_t Interrupts::readMpScuConfig() {
    // Read a value indicating CPU features based on console type
    return core->n3dsMode ? 0x5013 : 0x11;
}

uint32_t Interrupts::readMpAck(CpuId id) {
    // Get the ID and source of the next pending interrupt and switch it to active
    for (int i = 0; i < 4; i++) {
        for (int bit = 0; bit < 32; bit++) {
            if (!(mpIe[i] & mpIp[id][i] & BIT(bit))) continue;
            mpIp[id][i] &= ~BIT(bit);
            mpIa[id][i] |= BIT(bit);
            uint32_t type = (i << 5) + bit;
            return type | ((type < 0x10) ? (sources[id][type] << 10) : 0);
        }
    }
    return 0x3FF; // None
}

uint32_t Interrupts::readMpPending(CpuId id) {
    // Get the ID and source of the next pending interrupt but don't do anything
    for (int i = 0; i < 4; i++) {
        for (int bit = 0; bit < 32; bit++) {
            if (!(mpIe[i] & mpIp[id][i] & BIT(bit))) continue;
            uint32_t type = (i << 5) + bit;
            return type | ((type < 0x10) ? (sources[id][type] << 10) : 0);
        }
    }
    return 0x3FF; // None
}

uint32_t Interrupts::readMpCtrlType() {
    // Read a value indicating IRQ features based on console type
    return core->n3dsMode ? 0x63 : 0x23;
}

uint8_t Interrupts::readMpTarget(CpuId id, int i) {
    // Read from one of the MP_TARGET registers, or special-case private sources
    return (i >= 0x1D && i <= 0x1F) ? BIT(id) : mpTarget[i];
}

void Interrupts::writeCfg11MpClkcnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_MP_CLKCNT register
    // TODO: actually support changing clock speeds
    if (!core->n3dsMode) return; // N3DS-exclusive
    uint32_t mask2 = (mask & 0x7);
    cfg11MpClkcnt = (cfg11MpClkcnt & ~mask2) | (value & mask2);

    // Acknowledge the interrupt bit if it's set
    if (value & mask & BIT(15))
        cfg11MpClkcnt &= ~BIT(15);
}

void Interrupts::writeCfg11MpBootcnt(int i, uint8_t value) {
    // Write to one of the CFG11_MP_BOOTCNT registers
    if (!core->n3dsMode) return; // N3DS-exclusive
    cfg11MpBootcnt[i - 2] = (cfg11MpBootcnt[i - 2] & ~0x3) | (value & 0x3);

    // Enable an extra ARM11 core if newly started
    if ((cfg11MpBootcnt[i - 2] & (BIT(0) | BIT(4))) != BIT(0)) return;
    core->arms[i].halted &= ~(BIT(0) | BIT(1));
    cfg11MpBootcnt[i - 2] |= (BIT(4) | BIT(5));
    LOG_INFO("Enabling ARM11 core %d\n", i);

    // Switch to 4-core execution if both extra cores were stopped
    if (~cfg11MpBootcnt[(i - 2) ^ 1] & BIT(4))
        core->schedule(TOGGLE_RUN_FUNC, 1);
}

void Interrupts::writeMpIle(CpuId id, uint32_t mask, uint32_t value) {
    // Write to a core's local interrupt enable bit
    mask &= 0x1;
    mpIle[id] = (mpIle[id] & ~mask) | (value & mask);
}

void Interrupts::writeMpEoi(CpuId id, uint32_t mask, uint32_t value) {
    // Clear the active bit for a given interrupt type
    uint8_t type = (value & mask & 0x7F);
    mpIa[id][type >> 5] &= ~BIT(type & 0x1F);
}

void Interrupts::writeMpIge(uint32_t mask, uint32_t value) {
    // Write to the global interrupt enable bit
    mask &= 0x1;
    mpIge = (mpIge & ~mask) | (value & mask);
}

void Interrupts::writeMpIeSet(int i, uint32_t mask, uint32_t value) {
    // Set interrupt enable bits and check if any should trigger
    mpIe[i] |= (value & mask);
    checkInterrupt(false);
}

void Interrupts::writeMpIeClear(int i, uint32_t mask, uint32_t value) {
    // Clear interrupt enable bits
    if (!i) mask &= ~0xFFFF;
    mpIe[i] &= ~(value & mask);
}

void Interrupts::writeMpIpSet(int i, uint32_t mask, uint32_t value) {
    // Set interrupt pending bits on targeted ARM11 cores and check if any should trigger
    for (int j = 0; (value & mask) >> j; j++)
        if (value & mask & BIT(j))
            for (CpuId id = ARM11A; id < ARM9; id = CpuId(id + 1))
                if (readMpTarget(id, (i << 5) + j) & BIT(id))
                    mpIp[id][i] |= BIT(j);
    checkInterrupt(false);
}

void Interrupts::writeMpIpClear(int i, uint32_t mask, uint32_t value) {
    // Clear interrupt pending bits on all ARM11 cores
    if (!i) mask &= ~0xFFFF;
    for (int id = 0; id < MAX_CPUS - 1; id++)
        mpIp[id][i] &= ~(value & mask);
}

void Interrupts::writeMpTarget(int i, uint8_t value) {
    // Write to one of the MP_TARGET registers if it's writable
    if (i >= 0x20)
        mpTarget[i] = (value & 0xF);
}

void Interrupts::writeMpSoftIrq(CpuId id, uint32_t mask, uint32_t value) {
    // Verify the software interrupt type
    uint16_t type = (value & mask & 0x1FF);
    if (type >= 0x10) {
        LOG_CRIT("ARM11 core %d triggered software interrupt with unhandled type: 0x%X\n", id, type);
        return;
    }

    // Get a target mask of cores to send the interrupt to
    uint8_t cores;
    switch (((value & mask) >> 24) & 0x3) {
        case 0: cores = ((value & mask) >> 16) & 0xF; break;
        case 1: cores = ~BIT(id) & 0xF; break; // Other CPUs
        case 2: cores = BIT(id); break; // Local CPU

    default:
        LOG_CRIT("ARM11 core %d triggered software interrupt with unhandled target mode\n", id);
        return;
    }

    // Send the interrupt to selected ARM11 cores if enabled
    if (!mpIge) return;
    for (int i = 0; cores >> i; i++) {
        // Set the interrupt's pending bit and source ID
        if (!(cores & BIT(i)) || !mpIle[i]) continue;
        mpIp[i][type >> 5] |= BIT(type & 0x1F);
        sources[i][type] = id;

        // Schedule an interrupt if one hasn't been already
        if (scheduled[i]) continue;
        core->schedule(Task(ARM11A_INTERRUPT + i), 1);
        scheduled[i] = true;
    }
}

void Interrupts::writeIrqIe(uint32_t mask, uint32_t value) {
    // Write to the IRQ_IE register and check if an interrupt should occur
    mask &= 0x3FFFFFFF;
    irqIe = (irqIe & ~mask) | (value & mask);
    checkInterrupt(true);
}

void Interrupts::writeIrqIf(uint32_t mask, uint32_t value) {
    // Clear bits in the IRQ_IF register
    irqIf &= ~(value & mask);
}
