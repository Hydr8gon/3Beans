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

// Set up a block repeat with a loop count and end address
#define BKREP_FUNC(name, op0, op1h) int TeakInterp::name(uint16_t opcode) { \
    uint16_t param = readParam(); \
    uint8_t count = (regStt[2] >> 12) & 0x7; \
    if (count < 4) { \
        bkStart[count] = regPc; \
        bkEnd[count] = ((((op1h) & 0x30000) | param) + 1) & 0x3FFFF; \
        bkStack[count] = regLc; \
        regLc = (op0); \
        regStt[2] = (regStt[2] | BIT(15)) + BIT(12); \
        regIcr = (regIcr | BIT(4)) + BIT(5); \
    } \
    return 2; \
}

BKREP_FUNC(bkrepI8, (opcode & 0xFF), regPc) // BKREP Imm8u, Address16
BKREP_FUNC(bkrepReg, *readReg[opcode & 0x1F], (opcode << 11)) // BKREP Register, Address18
BKREP_FUNC(bkrepR6, regR[6], (opcode << 16)) // BKREP R6, Address18

int TeakInterp::br(uint16_t opcode) { // BR Address18, Cond
    // Branch to an 18-bit address if the condition is met
    uint16_t param = readParam();
    if (checkCond(opcode))
        regPc = ((opcode << 12) & 0x30000) | param;
    return 2;
}

int TeakInterp::brr(uint16_t opcode) { // BRR RelAddr7, Cond
    // Branch to a relative 7-bit signed offset if the condition is met
    if (checkCond(opcode))
        regPc += int8_t(opcode >> 3) >> 1;
    return 1;
}

int TeakInterp::call(uint16_t opcode) { // CALL Address18, Cond
    // Branch to an 18-bit address and push PC to the stack if the condition is met
    uint16_t param = readParam();
    if (checkCond(opcode)) {
        core->dsp.writeData(--regSp, regPc >> ((regMod[3] & BIT(14)) ? 16 : 0));
        core->dsp.writeData(--regSp, regPc >> ((regMod[3] & BIT(14)) ? 0 : 16));
        regPc = ((opcode << 12) & 0x30000) | param;
    }
    return 2;
}

int TeakInterp::cntxR(uint16_t opcode) { // CNTX R
    // Pop status bits from shadow registers and swap page bits
    uint8_t page = regSt[1];
    writeSt0((regSt[0] & ~0xFFD) | (shadSt[0] & 0xFFD));
    writeSt1((regSt[1] & ~0xCFF) | (shadSt[1] & 0xCFF));
    writeSt2((regSt[2] & ~0xFF) | (shadSt[2] & 0xFF));
    shadSt[1] = (shadSt[1] & ~0xFF) | page;

    // Swap AR/ARP registers and A1/B1 accumulators
    for (int i = 0; i < 2; i++)
        SWAP(regAr[i], shadAr[i]);
    for (int i = 0; i < 4; i++)
        SWAP(regArp[i], shadArp[i]);
    SWAP(regA[1].v, regB[1].v);
    return 1;
}

int TeakInterp::cntxS(uint16_t opcode) { // CNTX S
    // Push status bits to shadow registers and swap page bits
    uint8_t page = shadSt[1];
    for (int i = 0; i < 3; i++)
        shadSt[i] = regSt[i];
    writeSt1((regSt[1] & ~0xFF) | page);

    // Swap AR/ARP registers and A1/B1 accumulators, and set flags
    for (int i = 0; i < 2; i++)
        SWAP(regAr[i], shadAr[i]);
    for (int i = 0; i < 4; i++)
        SWAP(regArp[i], shadArp[i]);
    SWAP(regA[1].v, regB[1].v);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regA[1].v));
    return 1;
}

int TeakInterp::dint(uint16_t opcode) { // DINT
    // Clear the interrupt enable bit
    regSt[0] &= ~BIT(1);
    regMod[3] &= ~BIT(7);
    return 1;
}

int TeakInterp::eint(uint16_t opcode) { // EINT
    // Set the interrupt enable bit
    regSt[0] |= BIT(1);
    regMod[3] |= BIT(7);
    return 1;
}

int TeakInterp::nop(uint16_t opcode) { // NOP
    // Do nothing
    return 1;
}

// Set up the next opcode to repeat with a count value
#define REP_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    repCount = (op0) + 1; \
    repAddr = regPc; \
    return 1; \
}

REP_FUNC(repI8, (opcode & 0xFF)) // REP Imm8u
REP_FUNC(repReg, *readReg[opcode & 0x1F]) // REP Register
REP_FUNC(repR6, regR[6]) // REP R6

int TeakInterp::ret(uint16_t opcode) { // RET, Cond
    // Pop PC from the stack if the condition is met
    if (checkCond(opcode)) {
        uint16_t h = core->dsp.readData(regSp++);
        uint16_t l = core->dsp.readData(regSp++);
        regPc = ((regMod[3] & BIT(14)) ? ((l << 16) | h) : ((h << 16) | l)) & 0x3FFFF;
    }
    return 1;
}
