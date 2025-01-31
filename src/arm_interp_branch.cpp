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

int ArmInterp::bx(uint32_t opcode) { // BX Rn
    // Branch to address and switch to THUMB if bit 0 is set
    uint32_t op0 = *registers[opcode & 0xF];
    cpsr |= (op0 & BIT(0)) << 5;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blxReg(uint32_t opcode) { // BLX Rn
    // Branch to address with link and switch to THUMB if bit 0 is set
    uint32_t op0 = *registers[opcode & 0xF];
    cpsr |= (op0 & BIT(0)) << 5;
    *registers[14] = *registers[15] - 4;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::b(uint32_t opcode) { // B label
    // Branch to offset
    int32_t op0 = (int32_t)(opcode << 8) >> 6;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bl(uint32_t opcode) { // BL label
    // Branch to offset with link
    int32_t op0 = (int32_t)(opcode << 8) >> 6;
    *registers[14] = *registers[15] - 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blx(uint32_t opcode) { // BLX label
    // Branch to offset with link and switch to THUMB
    int32_t op0 = ((int32_t)(opcode << 8) >> 6) | ((opcode & BIT(24)) >> 23);
    cpsr |= BIT(5);
    *registers[14] = *registers[15] - 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::swi(uint32_t opcode) { // SWI #i
    // Software interrupt
    *registers[15] -= 4;
    return exception(0x08);
}

int ArmInterp::cps(uint32_t opcode) { // CPS[IE/ID] AIF,#mode
    // Optionally enable or disable interrupt flags
    if (opcode & BIT(19)) {
        if (opcode & BIT(18)) // ID
            cpsr |= (opcode & 0xE0);
        else // IE
            cpsr &= ~(opcode & 0xE0);
    }

    // Optionally change the CPU mode
    if (opcode & BIT(17))
        setCpsr((cpsr & ~0x1F) | (opcode & 0x1F));
    return 1;
}

int ArmInterp::srs(uint32_t opcode) { // SRS[DA/IA/DB/IB] sp!,#mode
    // Get the stack pointer for the specified CPU mode
    uint32_t *op0;
    switch (opcode & 0x1F) {
        case 0x11: op0 = &registersFiq[5]; break; // FIQ
        case 0x12: op0 = &registersIrq[0]; break; // IRQ
        case 0x13: op0 = &registersSvc[0]; break; // Supervisor
        case 0x17: op0 = &registersAbt[0]; break; // Abort
        case 0x1B: op0 = &registersUnd[0]; break; // Undefined
        default: op0 = &registersUsr[13]; break; // User/System
    }

    // Store return state based on the addressing mode
    switch ((opcode >> 23) & 0x3) {
    case 0x0: // DA
        core->memory.write<uint32_t>(id, *op0 - 4, *registers[14]);
        core->memory.write<uint32_t>(id, *op0 - 0, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        return 1;

    case 0x1: // IA
        core->memory.write<uint32_t>(id, *op0 + 0, *registers[14]);
        core->memory.write<uint32_t>(id, *op0 + 4, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        return 1;

    case 0x2: // DB
        core->memory.write<uint32_t>(id, *op0 - 8, *registers[14]);
        core->memory.write<uint32_t>(id, *op0 - 4, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        return 1;

    case 0x3: // IB
        core->memory.write<uint32_t>(id, *op0 + 4, *registers[14]);
        core->memory.write<uint32_t>(id, *op0 + 8, spsr ? *spsr : 0);
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        return 1;
    }
}

int ArmInterp::rfe(uint32_t opcode) { // RFE[DA/IA/DB/IB] Rn!
    // Return from exception based on the addressing mode
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    switch ((opcode >> 23) & 0x3) {
    case 0x0: // DA
        *registers[15] = core->memory.read<uint32_t>(id, *op0 - 4);
        setCpsr(core->memory.read<uint32_t>(id, *op0 - 0));
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        break;

    case 0x1: // IA
        *registers[15] = core->memory.read<uint32_t>(id, *op0 + 0);
        setCpsr(core->memory.read<uint32_t>(id, *op0 + 4));
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        break;

    case 0x2: // DB
        *registers[15] = core->memory.read<uint32_t>(id, *op0 - 8);
        setCpsr(core->memory.read<uint32_t>(id, *op0 - 4));
        if (opcode & BIT(21)) *op0 -= 8; // Writeback
        break;

    case 0x3: // IB
        *registers[15] = core->memory.read<uint32_t>(id, *op0 + 4);
        setCpsr(core->memory.read<uint32_t>(id, *op0 + 8));
        if (opcode & BIT(21)) *op0 += 8; // Writeback
        break;
    }

    // Handle pipelining
    flushPipeline();
    return 1;
}

int ArmInterp::bxRegT(uint16_t opcode) { // BX Rs
    // Branch to address and switch to ARM mode if bit 0 is cleared (THUMB)
    uint32_t op0 = *registers[(opcode >> 3) & 0xF];
    cpsr &= ~((~op0 & BIT(0)) << 5);
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blxRegT(uint16_t opcode) { // BLX Rs
    // Branch to address with link and switch to ARM mode if bit 0 is cleared (THUMB)
    uint32_t op0 = *registers[(opcode >> 3) & 0xF];
    cpsr &= ~((~op0 & BIT(0)) << 5);
    *registers[14] = *registers[15] - 1;
    *registers[15] = op0;
    flushPipeline();
    return 3;
}

int ArmInterp::beqT(uint16_t opcode) { // BEQ label
    // Branch to offset if equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(30)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bneT(uint16_t opcode) { // BNE label
    // Branch to offset if not equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(30)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bcsT(uint16_t opcode) { // BCS label
    // Branch to offset if carry set (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(29)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bccT(uint16_t opcode) { // BCC label
    // Branch to offset if carry clear (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(29)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bmiT(uint16_t opcode) { // BMI label
    // Branch to offset if negative (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bplT(uint16_t opcode) { // BPL label
    // Branch to offset if positive (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bvsT(uint16_t opcode) { // BVS label
    // Branch to offset if overflow set (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~cpsr & BIT(28)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bvcT(uint16_t opcode) { // BVC label
    // Branch to offset if overflow clear (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (cpsr & BIT(28)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bhiT(uint16_t opcode) { // BHI label
    // Branch to offset if higher (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr & 0x60000000) != 0x20000000) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blsT(uint16_t opcode) { // BLS label
    // Branch to offset if lower or same (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr & 0x60000000) == 0x20000000) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bgeT(uint16_t opcode) { // BGE label
    // Branch to offset if signed greater or equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if ((cpsr ^ (cpsr << 3)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bltT(uint16_t opcode) { // BLT label
    // Branch to offset if signed less than (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~(cpsr ^ (cpsr << 3)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bgtT(uint16_t opcode) { // BGT label
    // Branch to offset if signed greater than (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (((cpsr ^ (cpsr << 3)) | (cpsr << 1)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bleT(uint16_t opcode) { // BLE label
    // Branch to offset if signed less or equal (THUMB)
    int32_t op0 = (int8_t)opcode << 1;
    if (~((cpsr ^ (cpsr << 3)) | (cpsr << 1)) & BIT(31)) return 1;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::bT(uint16_t opcode) { // B label
    // Branch to offset (THUMB)
    int32_t op0 = (int16_t)(opcode << 5) >> 4;
    *registers[15] += op0;
    flushPipeline();
    return 3;
}

int ArmInterp::blSetupT(uint16_t opcode) { // BL/BLX label
    // Set the upper 11 bits of the target address for a long BL/BLX (THUMB)
    int32_t op0 = (int16_t)(opcode << 5) >> 4;
    *registers[14] = *registers[15] + (op0 << 11);
    return 1;
}

int ArmInterp::blOffT(uint16_t opcode) { // BL label
    // Long branch to offset with link (THUMB)
    uint32_t op0 = (opcode & 0x7FF) << 1;
    uint32_t ret = *registers[15] - 1;
    *registers[15] = *registers[14] + op0;
    *registers[14] = ret;
    flushPipeline();
    return 3;
}

int ArmInterp::blxOffT(uint16_t opcode) { // BLX label
    // Long branch to offset with link and switch to ARM mode (THUMB)
    uint32_t op0 = (opcode & 0x7FF) << 1;
    cpsr &= ~BIT(5);
    uint32_t ret = *registers[15] - 1;
    *registers[15] = *registers[14] + op0;
    *registers[14] = ret;
    flushPipeline();
    return 3;
}

int ArmInterp::swiT(uint16_t opcode) { // SWI #i
    // Software interrupt (THUMB)
    *registers[15] -= 4;
    return exception(0x08);
}
