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

int TeakInterp::loadMod(uint16_t opcode) { // LOAD Imm9u, Mod
    // Load one of the mod values with a 9-bit immediate
    uint16_t &value = regCfg[(opcode >> 11) & 0x1];
    value = (value & ~0xFF80) | ((opcode << 7) & 0xFF80);
    return 1;
}

int TeakInterp::loadPage(uint16_t opcode) { // LOAD Imm8u, PAGE
    // Load the data page with an 8-bit immediate
    writeMod1((regMod[1] & 0xFF00) | (opcode & 0xFF));
    return 1;
}

int TeakInterp::loadStep(uint16_t opcode) { // LOAD Imm7s, Step
    // Load one of the step values with a 7-bit immediate
    uint16_t &value = regCfg[(opcode >> 10) & 0x1];
    value = (value & ~0x7F) | (opcode & 0x7F);
    return 1;
}

// Move a value to a register directly
#define MOV_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    op1 = op0; \
    return cyc; \
}

MOV_FUNC(movI16r6, readParam(), regR[6], 2) // MOV Imm16, R6
MOV_FUNC(movI16stp, readParam(), regStep[(opcode >> 3) & 0x1], 2) // MOV Imm16, Step0
MOV_FUNC(movRegr6, (this->*readRegS[opcode & 0x1F])(), regR[6], 1) // MOV Register, R6

// Move a value to a register using a write handler
#define MOVH_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)(op0); \
    return cyc; \
}

MOVH_FUNC(movI16arap, readParam(), writeArArp[opcode & 0x7], 2) // MOV Imm16, ArArp
MOVH_FUNC(movI16reg, readParam(), writeRegAe[opcode & 0x1F], 2) // MOV Imm16, Register
MOVH_FUNC(movI16sm, readParam(), writeSttMod[opcode & 0x7], 2) // MOV Imm16, SttMod
MOVH_FUNC(movMi16a, int16_t(core->dsp.readData(readParam())), writeAx[(opcode >> 8) & 0x1], 2) // MOV MemImm16, Ax
MOVH_FUNC(movRegreg, (this->*readRegP0S[opcode & 0x1F])(), writeRegAe[(opcode >> 5) & 0x1F], 1) // MOV RegisterP0, Register

// Move a value to an address in data memory
#define MOVM_FUNC(name, op0, op1a, cyc) int TeakInterp::name(uint16_t opcode) { \
    core->dsp.writeData(op1a, op0); \
    return cyc; \
}

MOVM_FUNC(movAblhmi8, (this->*readAblhS[(opcode >> 9) & 0x7])(), (regMod[1] << 8) | (opcode & 0xFF), 1) // MOV Ablh, MemImm8
MOVM_FUNC(movAlmi16, regA[(opcode >> 8) & 0x1].l, readParam(), 2) // MOV Axl, MemImm16
MOVM_FUNC(movRegmrn, (this->*readRegS[(opcode >> 5) & 0x1F])(), getRnStepZids(opcode), 1) // MOV Register, MemRnStepZids

int TeakInterp::movpPmareg(uint16_t opcode) { // MOVP ProgMemAx, Register
    // Move program memory addressed by an A accumulator to a register
    uint32_t address = 0x1FF00000 + ((regA[(opcode >> 5) & 0x1].v & 0x3FFFF) << 1);
    (this->*writeRegAe[opcode & 0x1F])(core->memory.read<uint16_t>(ARM11, address));
    return 1;
}

int TeakInterp::popReg(uint16_t opcode) { // POP Register
    // Pop a register from the stack
    (this->*writeReg[opcode & 0x1F])(core->dsp.readData(regSp++));
    return 1;
}

int TeakInterp::pushReg(uint16_t opcode) { // PUSH Register
    // Push a register to the stack
    core->dsp.writeData(--regSp, (this->*readRegS[opcode & 0x1F])());
    return 1;
}
