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

// Move a value generically
#define MOV_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    op1(op0); \
    return cyc; \
}

MOV_FUNC(movI16arap, readParam(), (this->*writeArArp[opcode & 0x7]), 2) // MOV Imm16, ArArp
MOV_FUNC(movI16reg, readParam(), (this->*writeRegM[opcode & 0x1F]), 2) // MOV Imm16, Register
MOV_FUNC(movI16r6, readParam(), regR[6]=, 2) // MOV Imm16, R6
MOV_FUNC(movI16sm, readParam(), (this->*writeSttMod[opcode & 0x7]), 2) // MOV Imm16, SttMod
MOV_FUNC(movI16stp, readParam(), regStep0[(opcode >> 3) & 0x1]=, 2) // MOV Imm16, Step0
MOV_FUNC(movI8al, (opcode & 0xFF), (this->*writeAx16M[(opcode >> 12) & 0x1]), 1) // MOV Imm8u, Axl
MOV_FUNC(movI8ry, int8_t(opcode), (this->*writeRegM[(opcode >> 10) & 0x7]), 1) // MOV Imm8s, R0123457y0
MOV_FUNC(movI8sv, int8_t(opcode), regSv=, 1) // MOV Imm8s, SV
MOV_FUNC(movRegr6, (this->*readRegS[opcode & 0x1F])(), regR[6]=, 1) // MOV Register, R6
MOV_FUNC(movSmabl, *readSttMod[opcode & 0x7], (this->*writeAblM[(opcode >> 10) & 0x3]), 1) // MOV SttMod, Abl

// Move a value from a read handler to a write handler
#define MOVHH_FUNC(name, op0, op1) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)((this->*op0)()); \
    return 1; \
}

MOVHH_FUNC(movAblsm, readAbS[(opcode >> 3) & 0x3], writeSttMod[opcode & 0x7]) // MOV Abl, SttMod
MOVHH_FUNC(movRegb, readRegP0S[opcode & 0x1F], writeBx16M[(opcode >> 5) & 0x1]) // MOV RegisterP0, Bx
MOVHH_FUNC(movRegreg, readRegP0S[opcode & 0x1F], writeRegM[(opcode >> 5) & 0x1F]) // MOV RegisterP0, Register

// Move a value from data memory to a write handler
#define MOVMH_FUNC(name, op0a, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)(core->dsp.readData(op0a)); \
    return cyc; \
}

MOVMH_FUNC(movMi16a, readParam(), writeAx16M[(opcode >> 8) & 0x1], 2) // MOV MemImm16, Ax
MOVMH_FUNC(movM7i16a, regR[7] + readParam(), writeAx16M[(opcode >> 8) & 0x1], 2) // MOV MemR7Imm16, Ax
MOVMH_FUNC(movM7i7a, regR[7] + (int8_t(opcode << 1) >> 1), writeAx16M[(opcode >> 8) & 0x1], 1) // MOV MemR7Imm7s, Ax
MOVMH_FUNC(movMrnreg, getRnStepZids(opcode), writeRegM[(opcode >> 5) & 0x1F], 1) // MOV MemRnStepZids, Register

// Move a value from a read handler to data memory
#define MOVHM_FUNC(name, op0, op1a, cyc) int TeakInterp::name(uint16_t opcode) { \
    core->dsp.writeData(op1a, (this->*op0)()); \
    return cyc; \
}

MOVHM_FUNC(movAblhmi8, readAblhS[(opcode >> 9) & 0x7], (regMod[1] << 8) | (opcode & 0xFF), 1) // MOV Ablh, MemImm8
MOVHM_FUNC(movAlmi16, readAxS[(opcode >> 8) & 0x1], readParam(), 2) // MOV Axl, MemImm16
MOVHM_FUNC(movAlm7i16, readAxS[(opcode >> 8) & 0x1], regR[7] + readParam(), 2) // MOV Axl, MemR7Imm16
MOVHM_FUNC(movAlm7i7, readAxS[(opcode >> 8) & 0x1], regR[7] + (int8_t(opcode << 1) >> 1), 1) // MOV Axl, MemR7Imm7s
MOVHM_FUNC(movRegmrn, readRegS[(opcode >> 5) & 0x1F], getRnStepZids(opcode), 1) // MOV Register, MemRnStepZids

int TeakInterp::movaAbrar(uint16_t opcode) { // MOVA Ab, MemRarOffsStep
    // Move the upper and lower parts of an accumulator to data memory
    int64_t value = (this->*readAbS[(opcode >> 4) & 0x3])();
    core->dsp.writeData(getRarOffsAr(opcode), value >> 0);
    core->dsp.writeData(getRarStepAr(opcode), value >> 16);
    return 1;
}

int TeakInterp::movpPmareg(uint16_t opcode) { // MOVP ProgMemAx, Register
    // Move program memory addressed by an A accumulator to a register
    uint32_t address = 0x1FF00000 + ((regA[(opcode >> 5) & 0x1].v & 0x3FFFF) << 1);
    (this->*writeRegM[opcode & 0x1F])(core->memory.read<uint16_t>(ARM11, address));
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
