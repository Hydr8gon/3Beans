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

// Store an upper A accumulator to memory and load it with another value, without changing flags
#define EXCH_FUNC(name, i, j, ops, op0s, op1s, op2s) int TeakInterp::name(uint16_t opcode) { \
    bool l = (regStt[0] & BIT(0)); \
    uint16_t value = (this->*readAxS[(opcode >> op0s) & 0x1])() >> 16; \
    core.dsp.writeData(stepReg(arpR##i[(opcode >> ops) & 0x3], arpP##i[(opcode >> op1s) & 0x3]), value); \
    value = core.dsp.readData(stepReg(arpR##j[(opcode >> ops) & 0x3], arpP##j[(opcode >> op2s) & 0x3])); \
    (this->*writeAx40[(opcode >> op0s) & 0x1])(int32_t(value << 16)); \
    if (regStt[0] & !l) writeStt0(regStt[0] & ~BIT(0)); \
    return 1; \
}

EXCH_FUNC(exchIj, i, j, 4, 6, 0, 2) // EXCH Axh, MemRiStepArp, MemRjStepArp
EXCH_FUNC(exchJi, j, i, 8, 4, 2, 0) // EXCH Axh, MemRjStepArp, MemRiStepArp

int TeakInterp::loadMod(uint16_t opcode) { // LOAD Imm9u, Mod
    // Load one of the mod values with a 9-bit immediate
    uint16_t value = regCfg[(opcode >> 11) & 0x1];
    (this->*writeCfgx[(opcode >> 11) & 0x1])((value & ~0xFF80) | ((opcode << 7) & 0xFF80));
    return 1;
}

int TeakInterp::loadMpd(uint16_t opcode) { // LOAD Imm2u, MOVPD
    // Load the program bank with a 2-bit immediate
    writeStt2((regStt[2] & ~0xC0) | ((opcode << 5) & 0xC0));
    return 1;
}

int TeakInterp::loadPage(uint16_t opcode) { // LOAD Imm8u, PAGE
    // Load the data page with an 8-bit immediate
    writeMod1((regMod[1] & ~0xFF) | (opcode & 0xFF));
    return 1;
}

int TeakInterp::loadPs(uint16_t opcode) { // LOAD Imm2u, PS
    // Load product shifter 0 with a 2-bit immediate
    writeMod0((regMod[0] & ~0xC00) | ((opcode << 10) & 0xC00));
    return 1;
}

int TeakInterp::loadPs01(uint16_t opcode) { // LOAD Imm4u, PS01
    // Load the product shifters with a 4-bit immediate
    writeMod0((regMod[0] & ~0x6C00) | ((opcode << 10) & 0xC00) | ((opcode << 11) & 0x6000));
    return 1;
}

int TeakInterp::loadStep(uint16_t opcode) { // LOAD Imm7s, Step
    // Load one of the step values with a 7-bit immediate
    uint16_t value = regCfg[(opcode >> 10) & 0x1];
    (this->*writeCfgx[(opcode >> 10) & 0x1])((value & ~0x7F) | (opcode & 0x7F));
    return 1;
}

// Move a value generically
#define MOV_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    op1(op0); \
    return cyc; \
}

MOV_FUNC(movApc, regA[(opcode >> 8) & 0x1].v & 0x1FFFF, regPc=, 1) // MOV Ax, PC
MOV_FUNC(movA0hstp, readAhS<0>(), regStep0[(opcode >> 8) & 0x1]=, 1) // MOV A0H, Step0
MOV_FUNC(movAbab, *readAb[(opcode >> 10) & 0x3], (this->*writeAb40M[(opcode >> 5) & 0x3]), 1) // MOV Ab, Ab
MOV_FUNC(movAbp0, (this->*readAbS[opcode & 0x3])(), writeP33<0>, 1) // MOV Ab, P0
MOV_FUNC(movAblx1, (this->*readAbS[opcode & 0x3])(), regX[1]=, 1) // MOV Abl, X1
MOV_FUNC(movAbly1, (this->*readAbS[opcode & 0x3])(), regY[1]=, 1) // MOV Abl, Y1
MOV_FUNC(movArapabl, *readArArp[opcode & 0x7], (this->*writeAblM[(opcode >> 3) & 0x3]), 1) // MOV ArArp, Abl
MOV_FUNC(movI16arap, readParam(), (this->*writeArArp[opcode & 0x7]), 2) // MOV Imm16, ArArp
MOV_FUNC(movI16b, readParam(), (this->*writeBx16M[(opcode >> 8) & 0x1]), 2) // MOV Imm16, Bx
MOV_FUNC(movI16reg, readParam(), (this->*writeRegM[opcode & 0x1F]), 2) // MOV Imm16, Register
MOV_FUNC(movI16r6, readParam(), regR[6]=, 2) // MOV Imm16, R6
MOV_FUNC(movI16sm, readParam(), (this->*writeSttMod[opcode & 0x7]), 2) // MOV Imm16, SttMod
MOV_FUNC(movI16stp, readParam(), regStep0[(opcode >> 3) & 0x1]=, 2) // MOV Imm16, Step0
MOV_FUNC(movI8al, (opcode & 0xFF), (this->*writeAxlM[(opcode >> 12) & 0x1]), 1) // MOV Imm8u, Axl
MOV_FUNC(movI8ry, int8_t(opcode), (this->*writeRegM[(opcode >> 10) & 0x7]), 1) // MOV Imm8s, R0123457y0
MOV_FUNC(movI8sv, int8_t(opcode), regSv=, 1) // MOV Imm8s, SV
MOV_FUNC(movMi8sv, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), regSv=, 1) // MOV MemImm8, SV
MOV_FUNC(movMrnr6, core.dsp.readData(getRnStepZids(opcode)), regR[6]=, 1) // MOV MemRnStepZids, R6
MOV_FUNC(movMxpreg, regMixp, (this->*writeRegM[opcode & 0x1F]), 1) // MOV MIXP, Register
MOV_FUNC(movP0a, shiftP[0].v, (this->*writeAx40M[(opcode >> 5) & 0x1]), 1) // MOV P0, Ax
MOV_FUNC(movP1ab, shiftP[1].v, (this->*writeAb40M[opcode & 0x3]), 1) // MOV P1, Ab
MOV_FUNC(movRegb, readRegP0S(opcode & 0x1F), (this->*writeBx40M[(opcode >> 5) & 0x1]), 1) // MOV RegisterP0, Bx
MOV_FUNC(movRegmxp, (this->*readRegS[opcode & 0x1F])(), regMixp=, 1) // MOV Register, MIXP
MOV_FUNC(movRegreg, readRegP0S(opcode & 0x1F), (this->*writeRegM[(opcode >> 5) & 0x1F]), 1) // MOV RegisterP0, Register
MOV_FUNC(movRegr6, (this->*readRegS[opcode & 0x1F])(), regR[6]=, 1) // MOV Register, R6
MOV_FUNC(movR6reg, regR[6], (this->*writeRegM[opcode & 0x1F]), 1) // MOV R6, Register
MOV_FUNC(movSmabl, *readSttMod[opcode & 0x7], (this->*writeAblM[(opcode >> 10) & 0x3]), 1) // MOV SttMod, Abl
MOV_FUNC(movStpa0h, regStep0[(opcode >> 8) & 0x1], writeAhM<0>, 1) // MOV Step0, A0H

// Move a value directly to memory
#define MOVM_FUNC(name, op0, op1a) int TeakInterp::name(uint16_t opcode) { \
    core.dsp.writeData(op1a, op0); \
    return 1; \
}

MOVM_FUNC(movR6mrn, regR[6], getRnStepZids(opcode)) // MOV R6, MemRnStepZids
MOVM_FUNC(movSvmi8, regSv, (regMod[1] << 8) | (opcode & 0xFF)) // MOV SV, MemImm8

// Move a value from a read handler to a write handler
#define MOVHH_FUNC(name, op0, op1) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)((this->*op0)()); \
    return 1; \
}

MOVHH_FUNC(movAblarap, readAbS[(opcode >> 3) & 0x3], writeArArp[opcode & 0x7]) // MOV Abl, ArArp
MOVHH_FUNC(movAblsm, readAbS[(opcode >> 3) & 0x3], writeSttMod[opcode & 0x7]) // MOV Abl, SttMod

// Move a value from data memory to a write handler
#define MOVMH_FUNC(name, op0a, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)(core.dsp.readData(op0a)); \
    return cyc; \
}

MOVMH_FUNC(movMi16a, readParam(), writeAx16M[(opcode >> 8) & 0x1], 2) // MOV MemImm16, Ax
MOVMH_FUNC(movMi8ab, (regMod[1] << 8) | (opcode & 0xFF), writeAb16M[(opcode >> 11) & 0x3], 1) // MOV MemImm8, Ab
MOVMH_FUNC(movMi8ablh, (regMod[1] << 8) | (opcode & 0xFF), writeAblhM[(opcode >> 10) & 0x7], 1) // MOV MemImm8, Ablh
MOVMH_FUNC(movMi8ry, (regMod[1] << 8) | (opcode & 0xFF), writeRegM[(opcode >> 10) & 0x7], 1) // MOV MemImm8, R0123457y0
MOVMH_FUNC(movM7i16a, regR[7] + readParam(), writeAx16M[(opcode >> 8) & 0x1], 2) // MOV MemR7Imm16, Ax
MOVMH_FUNC(movM7i7a, regR[7] + (int8_t(opcode << 1) >> 1), writeAx16M[(opcode >> 8) & 0x1], 1) // MOV MemR7Imm7s, Ax
MOVMH_FUNC(movMrnb, getRnStepZids(opcode), writeBx16M[(opcode >> 8) & 0x1], 1) // MOV MemRnStepZids, Bx
MOVMH_FUNC(movMrnreg, getRnStepZids(opcode), writeRegM[(opcode >> 5) & 0x1F], 1) // MOV MemRnStepZids, Register

// Move a value from a read handler to data memory
#define MOVHM_FUNC(name, op0, op1a, cyc) int TeakInterp::name(uint16_t opcode) { \
    core.dsp.writeData(op1a, (this->*op0)()); \
    return cyc; \
}

MOVHM_FUNC(movAblhmi8, readAblhS[(opcode >> 9) & 0x7], (regMod[1] << 8) | (opcode & 0xFF), 1) // MOV Ablh, MemImm8
MOVHM_FUNC(movAlmi16, readAxS[(opcode >> 8) & 0x1], readParam(), 2) // MOV Axl, MemImm16
MOVHM_FUNC(movAlm7i16, readAxS[(opcode >> 8) & 0x1], regR[7] + readParam(), 2) // MOV Axl, MemR7Imm16
MOVHM_FUNC(movAlm7i7, readAxS[(opcode >> 8) & 0x1], regR[7] + (int8_t(opcode << 1) >> 1), 1) // MOV Axl, MemR7Imm7s
MOVHM_FUNC(movRegmrn, readRegS[(opcode >> 5) & 0x1F], getRnStepZids(opcode), 1) // MOV Register, MemRnStepZids
MOVHM_FUNC(movRymi8, readRegS[(opcode >> 9) & 0x7], (regMod[1] << 8) | (opcode & 0xFF), 1) // MOV R0123457y0, MemImm8

// Move the high and low parts of an accumulator to data memory
#define MOVAM_FUNC(name, op0, ars0, ars1) int TeakInterp::name(uint16_t opcode) { \
    int64_t value = op0; \
    uint8_t rar = ((opcode >> ars0) & 0xC) | ((opcode >> ars1) & 0x3); \
    core.dsp.writeData(getRarOffsAr(rar), value >> 0); \
    core.dsp.writeData(getRarStepAr(rar), value >> 16); \
    return 1; \
}

MOVAM_FUNC(movPrar, regP[(opcode >> 1) & 0x1].v, 6, 2) // MOV Px, MemRarOffsStep
MOVAM_FUNC(movPrars, shiftP[(opcode >> 1) & 0x1].v, 6, 2) // MOV Px, MemRarOffsStep, s
MOVAM_FUNC(movaAbrar, (this->*readAbS[(opcode >> 4) & 0x3])(), 0, 0) // MOVA Ab, MemRarOffsStep

// Move data memory to the high and low parts of an accumulator
#define MOVMA_FUNC(name, ars0, ars1, op1) int TeakInterp::name(uint16_t opcode) { \
    uint8_t rar = ((opcode >> ars0) & 0xC) | ((opcode >> ars1) & 0x3); \
    uint16_t l = core.dsp.readData(getRarOffsAr(rar)); \
    uint16_t h = core.dsp.readData(getRarStepAr(rar)); \
    (this->*op1)(int32_t(h << 16) | l); \
    return 1; \
}

MOVMA_FUNC(movRarp, 8, 5, writePx33[opcode & 0x1]) // MOV MemRarOffsStep, Px
MOVMA_FUNC(movaRarab, 0, 0, writeAb40M[(opcode >> 4) & 0x3]) // MOVA MemRarOffsStep, Ab

int TeakInterp::movpPmareg(uint16_t opcode) { // MOVP ProgMemAx, Register
    // Move program memory addressed by an A accumulator to a register
    uint32_t address = 0x1FF00000 + ((regA[(opcode >> 5) & 0x1].v & 0x1FFFF) << 1);
    (this->*writeRegM[opcode & 0x1F])(core.memory.read<uint16_t>(ARM11, address));
    return 1;
}

int TeakInterp::mov2Abhabh(uint16_t opcode) { // MOV2 Abh, Abh, MemRar1StepAr1
    // Move the high parts of two accumulators to data memory
    uint8_t rar = ((opcode << 1) & 0x4) | (opcode & 0x1);
    core.dsp.writeData(getRarOffsAr(rar), (this->*readAbS[(opcode >> 2) & 0x3])() >> 16);
    core.dsp.writeData(getRarStepAr(rar), (this->*readAbS[(opcode >> 4) & 0x3])() >> 16);
    return 1;
}

// Pop a value from the stack
#define POP_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    op0(core.dsp.readData(regSp++)); \
    return 1; \
}

POP_FUNC(popArsm, (this->*writeArpMod[(opcode >> 8) & 0xF])) // POP ArArpSttMod
POP_FUNC(popB, (this->*writeBx16M[(opcode >> 5) & 0x1])) // POP Bx
POP_FUNC(popReg, (this->*writeRegM[opcode & 0x1F])) // POP Register
POP_FUNC(popRepc, regRepc=) // POP REPC
POP_FUNC(popR6, regR[6]=) // POP R6
POP_FUNC(popX, regX[opcode & 0x1]=) // POP Xx
POP_FUNC(popY1, regY[1]=) // POP Y1

int TeakInterp::popAbe(uint16_t opcode) { // POP Abe
    // Pop an accumulator's extension bits from the stack
    int64_t value = (int64_t(core.dsp.readData(regSp++)) << 32) | (*readAb[opcode & 0x3] & 0xFFFFFFFF);
    (this->*writeAb40M[opcode & 0x3])(value);
    return 1;
}

// Pop the high and low parts of an accumulator from the stack
#define POPA_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    uint16_t h = core.dsp.readData(regSp++); \
    uint16_t l = core.dsp.readData(regSp++); \
    (this->*op0)(int32_t(h << 16) | l); \
    return 1; \
}

POPA_FUNC(popP, writePx33[opcode & 0x1]) // POP Px
POPA_FUNC(popaAb, writeAb40M[opcode & 0x3]) // POPA Ab

// Push a value to the stack
#define PUSH_FUNC(name, op0, cyc) int TeakInterp::name(uint16_t opcode) { \
    core.dsp.writeData(--regSp, op0); \
    return cyc; \
}

PUSH_FUNC(pushAbe, (this->*readAbS[(opcode >> 1) & 0x3])() >> 32, 1) // PUSH Abe
PUSH_FUNC(pushArsm, *readArpMod[opcode & 0xF], 1) // PUSH ArArpSttMod
PUSH_FUNC(pushI16, readParam(), 2) // PUSH Imm16
PUSH_FUNC(pushReg, (this->*readRegS[opcode & 0x1F])(), 1) // PUSH Register
PUSH_FUNC(pushRpc, regRepc, 1) // PUSH REPC
PUSH_FUNC(pushR6, regR[6], 1) // PUSH R6
PUSH_FUNC(pushX, regX[opcode & 0x1], 1) // PUSH Xx
PUSH_FUNC(pushY1, regY[1], 1) // PUSH Y1

// Push the high and low parts of an accumulator to the stack
#define PUSHA_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    int64_t value = op0; \
    core.dsp.writeData(--regSp, value >> 0); \
    core.dsp.writeData(--regSp, value >> 16); \
    return 1; \
}

PUSHA_FUNC(pushP, shiftP[(opcode >> 1) & 0x1].v) // PUSH Px
PUSHA_FUNC(pushaA, (this->*readAxS[(opcode >> 6) & 0x1])()) // PUSHA Ax
PUSHA_FUNC(pushaB, (this->*readBxS[(opcode >> 1) & 0x1])()) // PUSHA Bx

int TeakInterp::swap(uint16_t opcode) { // SWAP SwapTypes4
    // Swap accumulator values and set flags based on the type bits
    int64_t temp;
    switch (opcode & 0xF) {
        case 0x4: temp = regB[1].v, writeB40S<1>(regA[1].v), writeA40S<1>(temp); // (A0, B0), (A1, B1)
        case 0x0: temp = regB[0].v, writeB40S<0>(regA[0].v), writeA40M<0>(temp); return 1; // (A0, B0)
        case 0x5: temp = regB[0].v, writeB40S<0>(regA[1].v), writeA40S<1>(temp); // (A0, B1), (A1, B0)
        case 0x1: temp = regB[1].v, writeB40S<1>(regA[0].v), writeA40M<0>(temp); return 1; // (A0, B1)
        case 0x2: temp = regB[0].v, writeB40S<0>(regA[1].v), writeA40M<1>(temp); return 1; // (A1, B0)
        case 0x3: temp = regB[1].v, writeB40S<1>(regA[1].v), writeA40M<1>(temp); return 1; // (A1, B1)
        case 0x6: writeA40M<1>(regB[0].v), writeB40S<0>(regA[0].v); return 1; // (A0, B0, A1)
        case 0x7: writeA40M<1>(regB[1].v), writeB40S<1>(regA[0].v); return 1; // (A0, B1, A1)
        case 0x8: writeA40M<0>(regB[0].v), writeB40S<0>(regA[1].v); return 1; // (A1, B0, A0)
        case 0x9: writeA40M<0>(regB[1].v), writeB40S<1>(regA[1].v); return 1; // (A1, B1, A0)
        case 0xA: writeB40S<1>(regA[0].v), writeA40M<0>(regB[0].v); return 1; // (B0, A0, B1)
        case 0xB: writeB40S<1>(regA[1].v), writeA40M<1>(regB[0].v); return 1; // (B0, A1, B1)
        case 0xC: writeB40S<0>(regA[0].v), writeA40M<0>(regB[1].v); return 1; // (B1, A0, B0)
        case 0xD: writeB40S<0>(regA[1].v), writeA40M<1>(regB[1].v); return 1; // (B1, A1, B0)
        default: return unkOp(opcode);
    }
}
