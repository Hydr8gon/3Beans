/*
    Copyright 2023 Hydr8gon

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

#include "interpreter.h"
#include "core.h"

Interpreter::Interpreter(Core *core, CpuId id): core(core), id(id), cp15(core, id)
{
    // Initialize the registers for user mode
    for (int i = 0; i < 32; i++)
        registers[i] = &registersUsr[i & 0xF];
}

void Interpreter::init()
{
    // Prepare to execute the boot ROM
    setCpsr(0x000000D3); // Supervisor, interrupts off
    registersUsr[15] = (id == ARM9) ? 0xFFFF0000 : 0x10000;
    flushPipeline();
}

void Interpreter::resetCycles()
{
    // Adjust CPU cycles for a global cycle reset
    cycles -= std::min(core->globalCycles, cycles);
}

void Interpreter::runFrame(Core *core)
{
    Interpreter &arm11a = core->cpus[ARM11A];
    Interpreter &arm11b = core->cpus[ARM11B];
    Interpreter &arm9 = core->cpus[ARM9];

    // Run a frame of CPU instructions and events
    while (core->running.exchange(true))
    {
        // Run the CPUs until the next scheduled task
        while (core->events[0].cycles > core->globalCycles)
        {
            // Run the first ARM11 core
            if (!arm11a.halted && core->globalCycles >= arm11a.cycles)
                arm11a.cycles = core->globalCycles + arm11a.runOpcode();

            // Run the second ARM11 core
            if (!arm11b.halted && core->globalCycles >= arm11b.cycles)
                arm11b.cycles = core->globalCycles + arm11b.runOpcode();

            // Run the ARM9 at half the speed of the ARM11
            if (!arm9.halted && core->globalCycles >= arm9.cycles)
                arm9.cycles = core->globalCycles + (arm9.runOpcode() << 1);

            // Count cycles up to the next soonest event
            core->globalCycles = std::min<uint32_t>(arm9.halted ? -1 : arm9.cycles,
                std::min<uint32_t>(arm11a.halted ? -1 : arm11a.cycles, arm11b.halted ? -1 : arm11b.cycles));
        }

        // Jump to the next scheduled event
        core->globalCycles = core->events[0].cycles;

        // Run all events that are scheduled now
        while (core->events[0].cycles <= core->globalCycles)
        {
            (*core->events[0].task)();
            core->events.erase(core->events.begin());
        }
    }
}

FORCE_INLINE int Interpreter::runOpcode()
{
    // Push the next opcode through the pipeline
    uint32_t opcode = pipeline[0];
    pipeline[0] = pipeline[1];

    // Execute an instruction
    if (cpsr & BIT(5)) // THUMB mode
    {
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->memory.read<uint16_t>(id, *registers[15] += 2);

        // Execute a THUMB instruction
        return (this->*thumbInstrs[(opcode >> 6) & 0x3FF])(opcode);
    }
    else // ARM mode
    {
        // Fill the pipeline, incrementing the program counter
        pipeline[1] = core->memory.read<uint32_t>(id, *registers[15] += 4);

        // Execute an ARM instruction based on its condition
        switch (condition[((opcode >> 24) & 0xF0) | (cpsr >> 28)])
        {
            case 0:  return 1;                      // False
            case 2:  return handleReserved(opcode); // Reserved
            default: return (this->*armInstrs[((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0xF)])(opcode);
        }
    }
}

int Interpreter::exception(uint8_t vector)
{
    // Switch the CPU mode, save the return address, and jump to the exception vector
    static const uint8_t modes[] = { 0x13, 0x1B, 0x13, 0x17, 0x17, 0x13, 0x12, 0x11 };
    setCpsr((cpsr & ~0x3F) | BIT(7) | modes[vector >> 2], true); // ARM, interrupts off, new mode
    *registers[14] = *registers[15] + ((*spsr & BIT(5)) >> 4);
    *registers[15] = cp15.exceptAddr + vector;
    flushPipeline();
    return 3;
}

void Interpreter::flushPipeline()
{
    // Adjust the program counter and refill the pipeline after a jump
    if (cpsr & BIT(5)) // THUMB mode
    {
        *registers[15] = (*registers[15] & ~0x1) + 2;
        pipeline[0] = core->memory.read<uint16_t>(id, *registers[15] - 2);
        pipeline[1] = core->memory.read<uint16_t>(id, *registers[15]);
    }
    else // ARM mode
    {
        *registers[15] = (*registers[15] & ~0x3) + 4;
        pipeline[0] = core->memory.read<uint32_t>(id, *registers[15] - 4);
        pipeline[1] = core->memory.read<uint32_t>(id, *registers[15]);
    }
}

void Interpreter::setCpsr(uint32_t value, bool save)
{
    // Swap banked registers if the CPU mode changed
    if ((value & 0x1F) != (cpsr & 0x1F))
    {
        switch (value & 0x1F)
        {
            case 0x10: // User
            case 0x1F: // System
                registers[8]  = &registersUsr[8];
                registers[9]  = &registersUsr[9];
                registers[10] = &registersUsr[10];
                registers[11] = &registersUsr[11];
                registers[12] = &registersUsr[12];
                registers[13] = &registersUsr[13];
                registers[14] = &registersUsr[14];
                spsr = nullptr;
                break;

            case 0x11: // FIQ
                registers[8]  = &registersFiq[0];
                registers[9]  = &registersFiq[1];
                registers[10] = &registersFiq[2];
                registers[11] = &registersFiq[3];
                registers[12] = &registersFiq[4];
                registers[13] = &registersFiq[5];
                registers[14] = &registersFiq[6];
                spsr = &spsrFiq;
                break;

            case 0x12: // IRQ
                registers[8]  = &registersUsr[8];
                registers[9]  = &registersUsr[9];
                registers[10] = &registersUsr[10];
                registers[11] = &registersUsr[11];
                registers[12] = &registersUsr[12];
                registers[13] = &registersIrq[0];
                registers[14] = &registersIrq[1];
                spsr = &spsrIrq;
                break;

            case 0x13: // Supervisor
                registers[8]  = &registersUsr[8];
                registers[9]  = &registersUsr[9];
                registers[10] = &registersUsr[10];
                registers[11] = &registersUsr[11];
                registers[12] = &registersUsr[12];
                registers[13] = &registersSvc[0];
                registers[14] = &registersSvc[1];
                spsr = &spsrSvc;
                break;

            case 0x17: // Abort
                registers[8]  = &registersUsr[8];
                registers[9]  = &registersUsr[9];
                registers[10] = &registersUsr[10];
                registers[11] = &registersUsr[11];
                registers[12] = &registersUsr[12];
                registers[13] = &registersAbt[0];
                registers[14] = &registersAbt[1];
                spsr = &spsrAbt;
                break;

            case 0x1B: // Undefined
                registers[8]  = &registersUsr[8];
                registers[9]  = &registersUsr[9];
                registers[10] = &registersUsr[10];
                registers[11] = &registersUsr[11];
                registers[12] = &registersUsr[12];
                registers[13] = &registersUnd[0];
                registers[14] = &registersUnd[1];
                spsr = &spsrUnd;
                break;

            default:
                LOG_CRIT("Unknown ARM%d CPU mode: 0x%X\n", (id == ARM9) ? 9 : 11, value & 0x1F);
                break;
        }
    }

    // Set the CPSR, save the old value, and check if an interrupt should occur
    if (save && spsr) *spsr = cpsr;
    cpsr = value;
    core->interrupts.checkInterrupt(id == ARM9);
}

int Interpreter::handleReserved(uint32_t opcode)
{
    // The ARM9-exclusive BLX instruction uses the reserved condition code, so let it run
    if ((opcode & 0xE000000) == 0xA000000)
        return blx(opcode); // BLX label

    // Treat anything else as an unknown opcode
    return unkArm(opcode);
}

int Interpreter::unkArm(uint32_t opcode)
{
    // Handle an unknown ARM opcode
    LOG_CRIT("Unknown ARM%d ARM opcode: 0x%X\n", (id == ARM9) ? 9 : 11, opcode);
    return 1;
}

int Interpreter::unkThumb(uint16_t opcode)
{
    // Handle an unknown THUMB opcode
    LOG_CRIT("Unknown ARM%d THUMB opcode: 0x%X\n", (id == ARM9) ? 9 : 11, opcode);
    return 1;
}
