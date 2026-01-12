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

#define IRQ_NONE 0x3FF

void Interrupts::sendInterrupt(CpuId id, int type) {
    // Send an interrupt to the ARM9
    if (id == ARM9) {
        // Set the interrupt's request bit
        if (type != -1)
            irqIf |= BIT(type);

        // Schedule an interrupt if any requested interrupts are enabled
        if (scheduled[ARM9] || !(irqIe & irqIf)) return;
        core.schedule(ARM9_INTERRUPT, 2);
        scheduled[ARM9] = true;
        return;
    }

    // Send an interrupt to the ARM11 cores if enabled
    if (!mpIge) return;
    for (int i = 0; i < MAX_CPUS - 1; i++) {
        // Set the interrupt's pending bit on targeted cores
        if (!mpIle[i]) continue;
        if (type != -1 && (readMpTarget(id, type) & BIT(i)))
            mpIp[i][type >> 5] |= BIT(type & 0x1F);

        // Schedule an interrupt if any pending interrupts are enabled
        if (scheduled[i] || readMpPending(CpuId(i)) == IRQ_NONE) continue;
        core.schedule(Task(ARM11A_INTERRUPT + i), 1);
        scheduled[i] = true;
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
    if (~core.arms[id].cpsr & BIT(7))
        core.arms[id].exception(0x18);
    core.arms[id].unhalt(BIT(0) | BIT(1));
}

void Interrupts::halt(CpuId id, uint8_t type) {
    // Halt a CPU and check if all ARM11 cores have been halted
    core.arms[id].halt(BIT(type));
    if (id != ARM9 && core.arms[ARM11A].halted && core.arms[ARM11B].halted &&
            core.arms[ARM11C].halted && core.arms[ARM11D].halted) {
        // Check if the current clock/FCRAM mode changed
        uint8_t mode = (cfg11MpClkcnt & 0x1) ? (cfg11MpClkcnt & 0x7) : 0;
        if (mode != ((cfg11MpClkcnt >> 16) & 0x7)) {
            // Update FCRAM mappings and trigger an interrupt
            LOG_INFO("Changing to clock/FCRAM mode %d\n", mode);
            cfg11MpClkcnt = (cfg11MpClkcnt & ~0x70000) | (mode << 16) | BIT(15);
            core.memory.updateMap(false, 0x28000000, 0x2FFFFFFF);
            core.memory.updateMap(true, 0x28000000, 0x2FFFFFFF);
            sendInterrupt(ARM11, 0x58);

            // Update the ARM11 timer scale based on clock speed
            // TODO: actually change CPU execution speed
            if (mode >= 5) core.timers.setMpScale(3);
            else if (mode == 3) core.timers.setMpScale(2);
            else core.timers.setMpScale(1);
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
        core.schedule(TOGGLE_RUN_FUNC, 1);
}

uint16_t Interrupts::readCfg11Socinfo() {
    // Read a value indicating the console type
    return core.n3dsMode ? 0x7 : 0x1;
}

uint8_t Interrupts::readCfg11MpBootcnt(int i) {
    // Read from a CFG11_MP_BOOTCNT register or a dummy value depending on ID
    if (!core.n3dsMode) return 0; // N3DS-exclusive
    return (i >= 2) ? cfg11MpBootcnt[i - 2] : 0x30;
}

uint32_t Interrupts::readMpScuConfig() {
    // Read a value indicating CPU features based on console type
    return core.n3dsMode ? 0x5013 : 0x11;
}

uint32_t Interrupts::readMpAck(CpuId id) {
    // Read the next pending interrupt and switch it to active state
    uint32_t value = readMpPending(id);
    if (value == IRQ_NONE) return value;
    int i = (value >> 5) & 0x3;
    uint32_t mask = BIT(value & 0x1F);
    mpIa[id][i] |= mask;

    // Clear the pending bit if non-software or all sources are handled
    if ((value & 0x3F0) || !(sources[id][value & 0xF] &= ~BIT(value >> 10)))
        mpIp[id][i] &= ~mask;
    return value;
}

uint32_t Interrupts::readMpPending(CpuId id) {
    // Find the lowest enabled and pending interrupt type with the highest priority
    uint32_t type = IRQ_NONE, prio = 0xF0, mask;
    for (int i = 0; i < MAX_CPUS - 1; i++) {
        if (!(mask = mpIe[i] & mpIp[id][i])) continue;
        for (int bit = 0; mask >> bit; bit++) {
            if (~mask & BIT(bit)) continue;
            uint32_t t = (i << 5) + bit;
            uint32_t p = (t < 0x20) ? mpPriorityL[id][t] : mpPriorityG[t - 0x20];
            if (p < prio) type = t, prio = p;
        }
    }

    // Return the interrupt type if its priority isn't masked
    if (prio >= mpPrioMask[id]) return IRQ_NONE;
    if (type >= 0x10) return type;

    // Append the lowest source core ID for software interrupts
    for (int i = 0; i < MAX_CPUS - 1; i++)
        if (sources[id][type] & BIT(i)) return (i << 10) | type;
    return IRQ_NONE;
}

uint32_t Interrupts::readMpCtrlType() {
    // Read a value indicating IRQ features based on console type
    return core.n3dsMode ? 0x63 : 0x23;
}

uint8_t Interrupts::readMpTarget(CpuId id, int i) {
    // Read from one of the MP_TARGET registers, or special-case private sources
    return (i >= 0x1D && i <= 0x1F) ? BIT(id) : mpTarget[i];
}

void Interrupts::writeCfg11MpClkcnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_MP_CLKCNT register
    if (!core.n3dsMode) return; // N3DS-exclusive
    uint32_t mask2 = (mask & 0x7);
    cfg11MpClkcnt = (cfg11MpClkcnt & ~mask2) | (value & mask2);

    // Acknowledge the interrupt bit if it's set
    if (value & mask & BIT(15))
        cfg11MpClkcnt &= ~BIT(15);
}

void Interrupts::writeCfg11MpBootcnt(int i, uint8_t value) {
    // Write to one of the CFG11_MP_BOOTCNT registers
    if (!core.n3dsMode) return; // N3DS-exclusive
    cfg11MpBootcnt[i - 2] = (cfg11MpBootcnt[i - 2] & ~0x3) | (value & 0x3);

    // Enable an extra ARM11 core if newly started
    if ((cfg11MpBootcnt[i - 2] & (BIT(0) | BIT(4))) != BIT(0)) return;
    core.arms[i].unhalt(BIT(0) | BIT(1));
    cfg11MpBootcnt[i - 2] |= (BIT(4) | BIT(5));
    LOG_INFO("Enabling ARM11 core %d\n", i);

    // Switch to 4-core execution if both extra cores were stopped
    if (~cfg11MpBootcnt[(i - 2) ^ 1] & BIT(4))
        core.schedule(TOGGLE_RUN_FUNC, 1);
}

void Interrupts::writeMpIle(CpuId id, uint32_t mask, uint32_t value) {
    // Write to a core's local interrupt enable bit
    mask &= 0x1;
    mpIle[id] = (mpIle[id] & ~mask) | (value & mask);
}

void Interrupts::writeMpPrioMask(CpuId id, uint32_t mask, uint32_t value) {
    // Write to a core's local priority mask register
    mask &= 0xF0;
    mpPrioMask[id] = (mpPrioMask[id] & ~mask) | (value & mask);
    checkInterrupt(ARM11);
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
    checkInterrupt(ARM11);
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
                if (mpTarget[(i << 5) + j] & BIT(id))
                    mpIp[id][i] |= BIT(j);
    checkInterrupt(ARM11);
}

void Interrupts::writeMpIpClear(int i, uint32_t mask, uint32_t value) {
    // Clear interrupt pending bits on all ARM11 cores
    if (!i) mask &= ~0xFFFF;
    for (int id = 0; id < MAX_CPUS - 1; id++)
        mpIp[id][i] &= ~(value & mask);
}

void Interrupts::writeMpPriorityL(CpuId id, int i, uint8_t value) {
    // Write to one of the local MP_PRIORITY registers if it's writable
    if (i >= 0x10 && i <= 0x1C) return;
    mpPriorityL[id][i] = (value & 0xF0);
    checkInterrupt(ARM11);
}

void Interrupts::writeMpPriorityG(int i, uint8_t value) {
    // Write to one of the global MP_PRIORITY registers
    mpPriorityG[i - 0x20] = (value & 0xF0);
    checkInterrupt(ARM11);
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

    // Set the interrupt's source and pending bits on enabled cores
    if (!mpIge) return;
    for (int i = 0; cores >> i; i++) {
        if (!(cores & BIT(i)) || !mpIle[i]) continue;
        mpIp[i][type >> 5] |= BIT(type & 0x1F);
        sources[i][type] |= BIT(id);
    }
    checkInterrupt(ARM11);
}

void Interrupts::writeIrqIe(uint32_t mask, uint32_t value) {
    // Write to the IRQ_IE register and check if an interrupt should occur
    mask &= 0x3FFFFFFF;
    irqIe = (irqIe & ~mask) | (value & mask);
    checkInterrupt(ARM9);
}

void Interrupts::writeIrqIf(uint32_t mask, uint32_t value) {
    // Clear bits in the IRQ_IF register
    irqIf &= ~(value & mask);
}
