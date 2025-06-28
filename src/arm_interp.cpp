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

#include "arm_interp.h"
#include "core.h"

template void ArmInterp::runFrame<false>(Core*);
template void ArmInterp::runFrame<true>(Core*);

ArmInterp::ArmInterp(Core *core, CpuId id): core(core), id(id) {
    // Initialize the registers for user mode
    for (int i = 0; i < 32; i++)
        registers[i] = &registersUsr[i & 0xF];

    // Don't start extra ARM11 cores right away
    if (id == ARM11C || id == ARM11D)
        halt(BIT(0));
}

void ArmInterp::init() {
    // Prepare to execute the boot ROM
    setCpsr(0xD3); // Supervisor, interrupts off
    registersUsr[15] = (id == ARM9) ? 0xFFFF0000 : 0x10000;
    flushPipeline();
}

void ArmInterp::resetCycles() {
    // Adjust CPU cycles for a global cycle reset
    if (cycles != -1)
        cycles -= std::min(core->globalCycles, cycles);
}

void ArmInterp::stopCycles(Core *core) {
    // Set the next cycle of halted CPUs to an unreachable value
    for (int i = 0; i < MAX_CPUS; i++)
        if (core->arms[i].halted)
            core->arms[i].cycles = -1;
}

template <bool extra> void ArmInterp::runFrame(Core *core) {
    // Run a frame of CPU instructions and events
    while (core->running.exchange(true)) {
        // Run the CPUs until the next scheduled task
        while (core->events[0].cycles > core->globalCycles) {
            // Run 2 or 4 ARM11 cores depending on execution mode
            for (int i = 0; i < (extra ? 4 : 2); i++)
                if (core->globalCycles >= core->arms[i].cycles)
                    core->arms[i].cycles = core->globalCycles + core->arms[i].runOpcode();

            // Run the ARM9 and DSP at half the speed of the ARM11
            if (core->globalCycles >= core->arms[ARM9].cycles)
                core->arms[ARM9].cycles = core->globalCycles + (core->arms[ARM9].runOpcode() << 1);
            if (core->globalCycles >= core->teak.cycles)
                core->teak.cycles = core->globalCycles + (core->teak.runOpcode() << 1);

            // Count cycles up to the next soonest CPU event
            core->globalCycles = std::min(core->arms[ARM9].cycles, core->teak.cycles);
            for (int i = 0; i < (extra ? 4 : 2); i++)
                core->globalCycles = std::min(core->globalCycles, core->arms[i].cycles);
        }

        // Jump to the next task and run all that are scheduled now
        core->globalCycles = core->events[0].cycles;
        while (core->events[0].cycles <= core->globalCycles) {
            (*core->events[0].task)();
            core->events.erase(core->events.begin());
        }
    }
}

FORCE_INLINE int ArmInterp::runOpcode() {
    // Push the next opcode through the pipeline
    uint32_t opcode = pipeline[0];
    pipeline[0] = pipeline[1];

    // Execute an instruction
    if (cpsr & BIT(5)) { // THUMB mode
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->cp15.read<uint16_t>(id, *registers[15] += 2);

        // Execute a THUMB instruction
        return (this->*thumbInstrs[(opcode >> 6) & 0x3FF])(opcode);
    }
    else { // ARM mode
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->cp15.read<uint32_t>(id, *registers[15] += 4);

        // Execute an ARM instruction based on its condition
        switch (condition[((opcode >> 24) & 0xF0) | (cpsr >> 28)]) {
            case 0: return 1; // False
            case 2: return handleReserved(opcode); // Reserved
            default: return (this->*armInstrs[((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0xF)])(opcode);
        }
    }
}

void ArmInterp::halt(uint8_t mask) {
    // Set a halt bit and disable the CPU if newly halted
    bool before = halted;
    halted |= mask;
    if (!before && halted)
        core->schedule(ARM_STOP_CYCLES, 0);
}

void ArmInterp::unhalt(uint8_t mask) {
    // Clear a halt bit and enable the CPU if newly unhalted
    bool before = halted;
    halted &= ~mask;
    if (before && !halted)
        cycles = 0;
}

int ArmInterp::exception(uint8_t vector) {
    // Switch the CPU mode, save the return address, and jump to the exception vector
    static const uint8_t modes[] = { 0x13, 0x1B, 0x13, 0x17, 0x17, 0x13, 0x12, 0x11 };
    setCpsr((cpsr & ~0x3F) | BIT(7) | modes[vector >> 2], true); // ARM, interrupts off, new mode
    *registers[14] = *registers[15] + ((*spsr & BIT(5)) >> 4);
    *registers[15] = core->cp15.exceptAddrs[id] + vector;
    flushPipeline();
    return 3;
}

void ArmInterp::flushPipeline() {
    // Adjust the program counter and refill the pipeline after a jump
    if (cpsr & BIT(5)) { // THUMB mode
        pipeline[0] = core->cp15.read<uint16_t>(id, *registers[15] &= ~1);
        pipeline[1] = core->cp15.read<uint16_t>(id, *registers[15] += 2);
    }
    else { // ARM mode
        pipeline[0] = core->cp15.read<uint32_t>(id, *registers[15] &= ~3);
        pipeline[1] = core->cp15.read<uint32_t>(id, *registers[15] += 4);
    }
}

void ArmInterp::setCpsr(uint32_t value, bool save) {
    // Swap banked registers if the CPU mode changed
    if ((value & 0x1F) != (cpsr & 0x1F)) {
        switch (value & 0x1F) {
        case 0x10: // User
        case 0x1F: // System
            registers[8] = &registersUsr[8];
            registers[9] = &registersUsr[9];
            registers[10] = &registersUsr[10];
            registers[11] = &registersUsr[11];
            registers[12] = &registersUsr[12];
            registers[13] = &registersUsr[13];
            registers[14] = &registersUsr[14];
            spsr = nullptr;
            break;

        case 0x11: // FIQ
            registers[8] = &registersFiq[0];
            registers[9] = &registersFiq[1];
            registers[10] = &registersFiq[2];
            registers[11] = &registersFiq[3];
            registers[12] = &registersFiq[4];
            registers[13] = &registersFiq[5];
            registers[14] = &registersFiq[6];
            spsr = &spsrFiq;
            break;

        case 0x12: // IRQ
            registers[8] = &registersUsr[8];
            registers[9] = &registersUsr[9];
            registers[10] = &registersUsr[10];
            registers[11] = &registersUsr[11];
            registers[12] = &registersUsr[12];
            registers[13] = &registersIrq[0];
            registers[14] = &registersIrq[1];
            spsr = &spsrIrq;
            break;

        case 0x13: // Supervisor
            registers[8] = &registersUsr[8];
            registers[9] = &registersUsr[9];
            registers[10] = &registersUsr[10];
            registers[11] = &registersUsr[11];
            registers[12] = &registersUsr[12];
            registers[13] = &registersSvc[0];
            registers[14] = &registersSvc[1];
            spsr = &spsrSvc;
            break;

        case 0x17: // Abort
            registers[8] = &registersUsr[8];
            registers[9] = &registersUsr[9];
            registers[10] = &registersUsr[10];
            registers[11] = &registersUsr[11];
            registers[12] = &registersUsr[12];
            registers[13] = &registersAbt[0];
            registers[14] = &registersAbt[1];
            spsr = &spsrAbt;
            break;

        case 0x1B: // Undefined
            registers[8] = &registersUsr[8];
            registers[9] = &registersUsr[9];
            registers[10] = &registersUsr[10];
            registers[11] = &registersUsr[11];
            registers[12] = &registersUsr[12];
            registers[13] = &registersUnd[0];
            registers[14] = &registersUnd[1];
            spsr = &spsrUnd;
            break;

        default:
            if (id == ARM9)
                LOG_CRIT("Unknown ARM9 CPU mode: 0x%X\n", value & 0x1F);
            else
                LOG_CRIT("Unknown ARM11 core %d CPU mode: 0x%X\n", id, value & 0x1F);
            break;
        }
    }

    // Set the CPSR, save the old value, and check if an interrupt should occur
    if (save && spsr) *spsr = cpsr;
    cpsr = value;
    core->interrupts.checkInterrupt(id);
}

int ArmInterp::handleReserved(uint32_t opcode) {
    // Check for special opcodes that use the reserved condition code
    if ((opcode & 0xE000000) == 0xA000000)
        return blx(opcode); // BLX label
    else if ((opcode & 0xC100000) == 0x4100000)
        return pld(opcode); // PLD address
    else if ((opcode & 0xFF1FE00) == 0x1000000 && id != ARM9)
        return cps(opcode); // CPS[IE/ID] AIF,#mode
    else if ((opcode & 0xE5FFFE0) == 0x84D0500 && id != ARM9)
        return srs(opcode); // SRS[DA/IA/DB/IB] sp!,#mode
    else if ((opcode & 0xE50FFFF) == 0x8100A00 && id != ARM9)
        return rfe(opcode); // RFE[DA/IA/DB/IB] Rn!
    else if (opcode == 0xF57FF01F && id != ARM9)
        return clrex(opcode); // CLREX
    return unkArm(opcode);
}

int ArmInterp::unkArm(uint32_t opcode) {
    // Handle an unknown ARM opcode
    if (id == ARM9)
        LOG_CRIT("Unknown ARM9 ARM opcode: 0x%X\n", opcode);
    else
        LOG_CRIT("Unknown ARM11 core %d ARM opcode: 0x%X\n", id, opcode);
    return 1;
}

int ArmInterp::unkThumb(uint16_t opcode) {
    // Handle an unknown THUMB opcode
    if (id == ARM9)
        LOG_CRIT("Unknown ARM9 THUMB opcode: 0x%X\n", opcode);
    else
        LOG_CRIT("Unknown ARM11 core %d THUMB opcode: 0x%X\n", id, opcode);
    return 1;
}
