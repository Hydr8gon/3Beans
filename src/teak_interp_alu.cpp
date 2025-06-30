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

// Add a 40-bit value to an accumulator and set flags
#define ADD40_FUNC(name, op0, op1rb, op1wb, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = op1rb[(opcode >> op1s) & 0x1].v; \
    int64_t val2 = op0; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*op1wb[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

ADD40_FUNC(addAbb, *readAb[(opcode >> 10) & 0x3], regB, writeBx40, 0) // ADD Ab, Bx
ADD40_FUNC(addBa, regB[(opcode >> 1) & 0x1].v, regA, writeAx40, 0) // ADD Bx, Ax
ADD40_FUNC(addPb, (this->*readPxS[(opcode >> 1) & 0x1])(), regB, writeBx40, 0) // ADD Px, Bx

// Add a 16-bit value to an accumulator and set flags
#define ADD16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

ADD16_FUNC(addI16a, readParam(), 8, 2) // ADD Imm16, Ax
ADD16_FUNC(addI8a, (opcode & 0xFF), 8, 1) // ADD Imm8u, Ax
ADD16_FUNC(addMi16a, core->dsp.readData(readParam()), 8, 2) // ADD MemImm16, Ax
ADD16_FUNC(addMi8a, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // ADD MemImm8, Ax
ADD16_FUNC(addM7i16a, core->dsp.readData(regR[7] + readParam()), 8, 2) // ADD MemR7Imm16, Ax
ADD16_FUNC(addM7i7a, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // ADD MemR7Imm7s, Ax
ADD16_FUNC(addMrna, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // ADD MemRnStepZids, Ax
ADD16_FUNC(addRega, *readRegP0[opcode & 0x1F], 8, 1) // ADD RegisterP0, Ax
ADD16_FUNC(addR6a, regR[6], 4, 1) // ADD R6, Ax

// Add a value to the high part of an A accumulator and set flags
#define ADDH_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 8) & 0x1].v; \
    int32_t val2 = (op0) << 16; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

ADDH_FUNC(addhMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // ADDH MemImm8, Ax
ADDH_FUNC(addhMrn, core->dsp.readData(getRnStepZids(opcode)), 8) // ADDH MemRnStepZids, Ax
ADDH_FUNC(addhReg, *readReg[opcode & 0x1F], 8) // ADDH Register, Ax
ADDH_FUNC(addhR6, regR[6], 0) // ADDH R6, Ax

// Add a 16-bit immediate to a memory value and set flags
#define ADDVM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t val = core->dsp.readData(addr); \
    uint16_t res = val + readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    bool c = (val > res); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    core->dsp.writeData(addr, res); \
    return 2; \
}

ADDVM_FUNC(addvMi8, (regMod[1] << 8) | (opcode & 0xFF)) // ADDV Imm16, MemImm8
ADDVM_FUNC(addvMrn, getRnStepZids(opcode)) // ADDV Imm16, MemRnStepZids

// Add a 16-bit immediate to a register value and set flags
#define ADDVR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    uint16_t val = op1r; \
    uint16_t res = val + readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    bool c = (val > res); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    op1w(res); \
    return 2; \
}

ADDVR_FUNC(addvReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // ADDV Imm16, Register
ADDVR_FUNC(addvR6, regR[6], regR[6]=) // ADDV Imm16, R6

int TeakInterp::andAbab(uint16_t opcode) {
    // Bitwise and one accumulator with another and set flags
    int64_t res = *readAb[opcode & 0x3] & *readAb[(opcode >> 2) & 0x3];
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    (this->*writeAx40[(opcode >> 12) & 0x1])(res);
    return 1;
}

// Bitwise and a value with an A accumulator and set flags
#define AND_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = regA[(opcode >> op1s) & 0x1].v & (op0); \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

AND_FUNC(andI16, readParam(), 8, 2) // AND Imm16, Ax
AND_FUNC(andI8, (opcode & 0xFF), 8, 1) // AND Imm8u, Ax
AND_FUNC(andMi16, core->dsp.readData(readParam()), 8, 2) // AND MemImm16, Ax
AND_FUNC(andMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // AND MemImm8, Ax
AND_FUNC(andM7i16, core->dsp.readData(regR[7] + readParam()), 8, 2) // AND MemR7Imm16, Ax
AND_FUNC(andM7i7, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // AND MemR7Imm7s, Ax
AND_FUNC(andMrn, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // AND MemRnStepZids, Ax
AND_FUNC(andReg, *readRegP0[opcode & 0x1F], 8, 1) // AND RegisterP0, Ax
AND_FUNC(andR6, regR[6], 4, 1) // AND R6, Ax

// Set an accumulator to zero and update flags if the condition is met
#define CLR_FUNC(name, op0b) int TeakInterp::name(uint16_t opcode) { \
    if (checkCond(opcode)) { \
        writeStt0((regStt[0] & ~0xE4) | 0xA0); \
        (this->*op0b[(opcode >> 12) & 0x1])(0); \
    } \
    return 1; \
}

CLR_FUNC(clrA, writeAx40) // CLR Ax, Cond
CLR_FUNC(clrB, writeBx40) // CLR Bx, Cond

// Set an accumulator to 0x8000 and update flags if the condition is met
#define CLRR_FUNC(name, op0b) int TeakInterp::name(uint16_t opcode) { \
    if (checkCond(opcode)) { \
        writeStt0(regStt[0] & ~0xE4); \
        (this->*op0b[(opcode >> 12) & 0x1])(0x8000); \
    } \
    return 1; \
}

CLRR_FUNC(clrrA, writeAx40) // CLRR Ax, Cond
CLRR_FUNC(clrrB, writeBx40) // CLRR Bx, Cond

// Compare a 40-bit value with an accumulator and set flags
#define CMP40_FUNC(name, op0, op1) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = op1; \
    int64_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return 1; \
}

CMP40_FUNC(cmpAb, regA[(opcode >> 1) & 0x1].v, regB[opcode & 0x1].v) // CMP Ax, Bx
CMP40_FUNC(cmpBa, regB[(opcode >> 10) & 0x1].v, regA[opcode & 0x1].v) // CMP Bx, Ax
CMP40_FUNC(cmpB0b1, regB[0].v, regB[1].v) // CMP B0, B1
CMP40_FUNC(cmpB1b0, regB[1].v, regB[0].v) // CMP B1, B0

// Compare a 16-bit value with an accumulator and set flags
#define CMP16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return cyc; \
}

CMP16_FUNC(cmpI16a, readParam(), 8, 2) // CMP Imm16, Ax
CMP16_FUNC(cmpI8a, (opcode & 0xFF), 8, 1) // CMP Imm8u, Ax
CMP16_FUNC(cmpMi16a, core->dsp.readData(readParam()), 8, 2) // CMP MemImm16, Ax
CMP16_FUNC(cmpMi8a, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // CMP MemImm8, Ax
CMP16_FUNC(cmpM7i16a, core->dsp.readData(regR[7] + readParam()), 8, 2) // CMP MemR7Imm16, Ax
CMP16_FUNC(cmpM7i7a, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // CMP MemR7Imm7s, Ax
CMP16_FUNC(cmpMrna, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // CMP MemRnStepZids, Ax
CMP16_FUNC(cmpRega, *readRegP0[opcode & 0x1F], 8, 1) // CMP RegisterP0, Ax
CMP16_FUNC(cmpR6a, regR[6], 4, 1) // CMP R6, Ax

// Compare an unsigned value with an A accumulator and set flags
#define CMPU_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    uint16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return 1; \
}

CMPU_FUNC(cmpuMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // CMPU MemImm8, Ax
CMPU_FUNC(cmpuMrn, core->dsp.readData(getRnStepZids(opcode)), 8) // CMPU MemRnStepZids, Ax
CMPU_FUNC(cmpuReg, *readReg[opcode & 0x1F], 8) // CMPU Register, Ax
CMPU_FUNC(cmpuR6, regR[6], 3) // CMPU R6, Ax

// Compare a 16-bit immediate with a register or memory value and set flags
#define CMPV_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    uint16_t val = op1; \
    uint16_t res = val - readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    bool c = (val >= res); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    return 2; \
}

CMPV_FUNC(cmpvMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // CMPV Imm16, MemImm8
CMPV_FUNC(cmpvMrn, core->dsp.readData(getRnStepZids(opcode))) // CMPV Imm16, MemRnStepZids
CMPV_FUNC(cmpvReg, *readReg[opcode & 0x1F]) // CMPV Imm16, Register
CMPV_FUNC(cmpvR6, regR[6]) // CMPV Imm16, R6

int TeakInterp::copy(uint16_t opcode) { // COPY Ax, Cond
    // Copy a value from the other A accumulator and set flags
    int64_t val = regA[(~opcode >> 12) & 0x1].v;
    writeStt0((regStt[0] & ~0xE4) | calcZmne(val));
    (this->*writeAx40[(opcode >> 12) & 0x1])(val);
    return 1;
}

int TeakInterp::dec(uint16_t opcode) { // DEC Ax, Cond
    // Decrement an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val - 1) << 24) >> 24;
        bool v = ((1 ^ val) & ~(res ^ 1)) >> 39;
        bool c = (uint64_t(val) >= uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

int TeakInterp::inc(uint16_t opcode) { // INC Ax, Cond
    // Increment an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val + 1) << 24) >> 24;
        bool v = (~(1 ^ val) & (res ^ 1)) >> 39;
        bool c = (uint64_t(val) >= uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Modify an address register as if memory was accessed and set the R flag if zero
#define MODR_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    op0; \
    bool r = (regR[opcode & 0x7] == 0); \
    writeStt1((regStt[1] & ~0x10) | (r << 4)); \
    return 1; \
}

MODR_FUNC(modrD2, stepReg(opcode & 0x7, -2)) // MODR MemRnStepD2
MODR_FUNC(modrI2, stepReg(opcode & 0x7, 2)) // MODR MemRnStepI2
MODR_FUNC(modrZids, getRnStepZids(opcode)) // MODR MemRnStepZids

// Move a value shifted by the shift register into an accumulator and set flags
#define MOVS_FUNC(name, op0, op1i) int TeakInterp::name(uint16_t opcode) { \
    int16_t shift = regSv; \
    int64_t res = int16_t(op0); \
    if (regMod[0] & BIT(7)) { \
        res = (shift > 0) ? ((res & 0xFFFFFFFFFF) << shift) : ((res & 0xFFFFFFFFFF) >> -shift); \
        bool c = (res >> 40) & 0x1; \
        writeStt0((regStt[0] & ~0xEC) | calcZmne(res) | (c << 3)); \
    } \
    else { \
        res = (shift > 0) ? (res << shift) : (res >> -shift); \
        bool v = (res != ((res << 24) >> 24)); \
        bool c = (res >> 40) & 0x1; \
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    } \
    (this->*writeAb40[op1i])(res); \
    return 1; \
}

MOVS_FUNC(movsMi8ab, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), (opcode >> 11) & 0x3) // MOVS MemImm8, Ab
MOVS_FUNC(movsMrnab, core->dsp.readData(getRnStepZids(opcode)), (opcode >> 5) & 0x3) // MOVS MemRnStepZids, Ab
MOVS_FUNC(movsRegab, *readRegP0[opcode & 0x1F], (opcode >> 5) & 0x3) // MOVS RegisterP0, Ab
MOVS_FUNC(movsR6a, regR[6], (opcode & 0x1) + 2) // MOVS R6, Ax

int TeakInterp::mpyi(uint16_t opcode) { // MPYI Y0, Imm8s
    // Load an 8-bit signed immediate into X0 and multiply it with Y0
    regX[0] = int8_t(opcode);
    multiplyXY(false, false);
    return 1;
}

int TeakInterp::neg(uint16_t opcode) { // NEG Ax, Cond
    // Negate an A accumulator by subtracting it from 0 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val1 = 0;
        int64_t val2 = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val1 - val2) << 24) >> 24;
        bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39;
        bool c = (uint64_t(val1) >= uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

int TeakInterp::_not(uint16_t opcode) { // NOT Ax, Cond
    // Invert the bits of an A accumulator and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t res = ~regA[(opcode >> 12) & 0x1].v;
        writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
        (this->*writeAx40[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Bitwise or a value with an A accumulator and set flags
#define OR_FUNC(name, op0, op1b, op1s, op2s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = op1b[(opcode >> op1s) & 0x1].v | (op0); \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    (this->*writeAx40[(opcode >> op2s) & 0x1])(res); \
    return cyc; \
}

OR_FUNC(orAba, *readAb[(opcode >> 10) & 0x3], regA, 6, 5, 1) // OR Ab, Ax, Ax
OR_FUNC(orAb, regA[(opcode >> 8) & 0x1].v, regB, 1, 0, 1) // OR Ax, Bx, Ax
OR_FUNC(orBb, regB[(opcode >> 10) & 0x1].v, regB, 1, 0, 1) // OR Bx, Bx, Ax
OR_FUNC(orI16, readParam(), regA, 8, 8, 2) // OR Imm16, Ax
OR_FUNC(orI8, (opcode & 0xFF), regA, 8, 8, 1) // OR Imm8u, Ax
OR_FUNC(orMi16, core->dsp.readData(readParam()), regA, 8, 8, 2) // OR MemImm16, Ax
OR_FUNC(orMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), regA, 8, 8, 1) // OR MemImm8, Ax
OR_FUNC(orM7i16, core->dsp.readData(regR[7] + readParam()), regA, 8, 8, 2) // OR MemR7Imm16, Ax
OR_FUNC(orM7i7, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), regA, 8, 8, 1) // OR MemR7Imm7s, Ax
OR_FUNC(orMrn, core->dsp.readData(getRnStepZids(opcode)), regA, 8, 8, 1) // OR MemRnStepZids, Ax
OR_FUNC(orReg, *readRegP0[opcode & 0x1F], regA, 8, 8, 1) // OR RegisterP0, Ax
OR_FUNC(orR6, regR[6], regA, 4, 4, 1) // OR R6, Ax

// Clear bits in a memory value using a 16-bit immediate and set flags
#define RSTM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t res = core->dsp.readData(op1a) & ~readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    core->dsp.writeData(addr, res); \
    return 2; \
}

RSTM_FUNC(rstMi8, (regMod[1] << 8) | (opcode & 0xFF)) // RST Imm16, MemImm8
RSTM_FUNC(rstMrn, getRnStepZids(opcode)) // RST Imm16, MemRnStepZids

// Clear bits in a register value using a 16-bit immediate and set flags
#define RSTR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    uint16_t res = (op1r) & ~readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    op1w(res); \
    return 2; \
}

RSTR_FUNC(rstReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // RST Imm16, Register
RSTR_FUNC(rstR6, regR[6], regR[6]=) // RST Imm16, R6
RSTR_FUNC(rstSm, *readSttMod[opcode & 0x7], (this->*writeSttMod[opcode & 0x7])) // RST Imm16, SttMod

// Set bits in a memory value using a 16-bit immediate and set flags
#define SETM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t res = core->dsp.readData(op1a) | readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    core->dsp.writeData(addr, res); \
    return 2; \
}

SETM_FUNC(setMi8, (regMod[1] << 8) | (opcode & 0xFF)) // SET Imm16, MemImm8
SETM_FUNC(setMrn, getRnStepZids(opcode)) // SET Imm16, MemRnStepZids

// Set bits in a register value using a 16-bit immediate and set flags
#define SETR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    uint16_t res = (op1r) | readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    op1w(res); \
    return 2; \
}

SETR_FUNC(setReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // SET Imm16, Register
SETR_FUNC(setR6, regR[6], regR[6]=) // SET Imm16, R6
SETR_FUNC(setSm, *readSttMod[opcode & 0x7], (this->*writeSttMod[opcode & 0x7])) // SET Imm16, SttMod

int TeakInterp::shfc(uint16_t opcode) { // SHFC Ab, Ab, Cond
    // Shift an accumulator by the shift register and set flags if the condition is met
    if (checkCond(opcode)) {
        int16_t shift = regSv;
        int64_t res = *readAb[(opcode >> 10) & 0x3];
        if (regMod[0] & BIT(7)) { // Logical
            res = (shift > 0) ? ((res & 0xFFFFFFFFFF) << shift) : ((res & 0xFFFFFFFFFF) >> -shift);
            bool c = (res >> 40) & 0x1;
            writeStt0((regStt[0] & ~0xEC) | calcZmne(res) | (c << 3));
        }
        else { // Arithmetic
            res = (shift > 0) ? (res << shift) : (res >> -shift);
            bool v = (res != ((res << 24) >> 24));
            bool c = (res >> 40) & 0x1;
            writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        }
        (this->*writeAb40[(opcode >> 5) & 0x3])(res);
    }
    return 1;
}

// Move a value shifted by a signed immediate into an accumulator and set flags
#define SHFI_FUNC(name, op0, op1s, op2s) int TeakInterp::name(uint16_t opcode) { \
    int8_t shift = int8_t(opcode << op2s) >> op2s; \
    int64_t res = op0; \
    if (regMod[0] & BIT(7)) { \
        res = (shift > 0) ? ((res & 0xFFFFFFFFFF) << shift) : ((res & 0xFFFFFFFFFF) >> -shift); \
        bool c = (res >> 40) & 0x1; \
        writeStt0((regStt[0] & ~0xEC) | calcZmne(res) | (c << 3)); \
    } \
    else { \
        res = (shift > 0) ? (res << shift) : (res >> -shift); \
        bool v = (res != ((res << 24) >> 24)); \
        bool c = (res >> 40) & 0x1; \
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    } \
    (this->*writeAb40[(opcode >> op1s) & 0x3])(res); \
    return 1; \
}

SHFI_FUNC(movsi, int16_t(*readReg[(opcode >> 9) & 0x7]), 5, 3) // MOVSI R0123457y0, Ab, Imm5s
SHFI_FUNC(shfi, *readAb[(opcode >> 10) & 0x3], 7, 2) // SHFI Ab, Ab, Imm6s

// Subtract a 40-bit value from an accumulator and set flags
#define SUB40_FUNC(name, op0, op1rb, op1wb, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = op1rb[(opcode >> op1s) & 0x1].v; \
    int64_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*op1wb[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

SUB40_FUNC(subAbb, *readAb[(opcode >> 3) & 0x3], regB, writeBx40, 8) // SUB Ab, Bx
SUB40_FUNC(subBa, regB[(opcode >> 4) & 0x1].v, regA, writeAx40, 3) // SUB Bx, Ax

// Subtract a 16-bit value from an accumulator and set flags
#define SUB16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

SUB16_FUNC(subI16a, readParam(), 8, 2) // SUB Imm16, Ax
SUB16_FUNC(subI8a, (opcode & 0xFF), 8, 1) // SUB Imm8u, Ax
SUB16_FUNC(subMi16a, core->dsp.readData(readParam()), 8, 2) // SUB MemImm16, Ax
SUB16_FUNC(subMi8a, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // SUB MemImm8, Ax
SUB16_FUNC(subM7i16a, core->dsp.readData(regR[7] + readParam()), 8, 2) // SUB MemR7Imm16, Ax
SUB16_FUNC(subM7i7a, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // SUB MemR7Imm7s, Ax
SUB16_FUNC(subMrna, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // SUB MemRnStepZids, Ax
SUB16_FUNC(subRega, *readRegP0[opcode & 0x1F], 8, 1) // SUB RegisterP0, Ax
SUB16_FUNC(subR6a, regR[6], 4, 1) // SUB R6, Ax

// Subtract a value from the high part of an A accumulator and set flags
#define SUBH_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 8) & 0x1].v; \
    int32_t val2 = (op0) << 16; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40[(opcode >> 8) & 0x1])(res); \
    return 1; \
}

SUBH_FUNC(subhMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // SUBH MemImm8, Ax
SUBH_FUNC(subhMrn, core->dsp.readData(getRnStepZids(opcode))) // SUBH MemRnStepZids, Ax
SUBH_FUNC(subhReg, *readReg[opcode & 0x1F]) // SUBH Register, Ax
SUBH_FUNC(subhR6, regR[6]) // SUBH R6, Ax

// Set the Z flag to a specified bit in a register or memory value
#define TSTB_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    bool z = (op0) & BIT((op1) & 0xF); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return cyc; \
}

TSTB_FUNC(tstbMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), opcode >> 8, 1) // TSTB MemImm8, Imm4
TSTB_FUNC(tstbMrn, core->dsp.readData(getRnStepZids(opcode)), opcode >> 8, 1) // TSTB MemRnStepZids, Imm4
TSTB_FUNC(tstbReg, *readReg[opcode & 0x1F], opcode >> 8, 1) // TSTB Register, Imm4
TSTB_FUNC(tstbR6, regR[6], opcode >> 8, 1) // TSTB R6, Imm4
TSTB_FUNC(tstbSm, *readSttMod[opcode & 0x7], readParam(), 2) // TSTB SttMod, Imm4

// Set the Z flag if the bits in a value masked by an A accumulator are 0
#define TST0A_FUNC(name, op0s, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !((op1) & regA[(opcode >> op0s) & 0x1].l); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 1; \
}

TST0A_FUNC(tst0Almi8, 8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST0 Axl, MemImm8
TST0A_FUNC(tst0Almrn, 8, getRnStepZids(opcode)) // TST0 Axl, MemRnStepZids
TST0A_FUNC(tst0Alreg, 8, *readReg[opcode & 0x1F]) // TST0 Axl, Register
TST0A_FUNC(tst0Alr6, 4, regR[6]) // TST0 Axl, R6

// Set the Z flag if the bits in a value masked by a 16-bit immediate are 0
#define TST0I_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !((op1) & readParam()); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 2; \
}

TST0I_FUNC(tst0I16mi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST0 Imm16, MemImm8
TST0I_FUNC(tst0I16mrn, getRnStepZids(opcode)) // TST0 Imm16, MemRnStepZids
TST0I_FUNC(tst0I16reg, *readReg[opcode & 0x1F]) // TST0 Imm16, Register
TST0I_FUNC(tst0I16r6, regR[6]) // TST0 Imm16, R6
TST0I_FUNC(tst0I16sm, *readSttMod[opcode & 0x7]) // TST0 Imm16, SttMod

// Bitwise exclusive or a value with an A accumulator and set flags
#define XOR_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = regA[(opcode >> op1s) & 0x1].v ^ (op0); \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

XOR_FUNC(xorI16, readParam(), 8, 2) // XOR Imm16, Ax
XOR_FUNC(xorI8, (opcode & 0xFF), 8, 1) // XOR Imm8u, Ax
XOR_FUNC(xorMi16, core->dsp.readData(readParam()), 8, 2) // XOR MemImm16, Ax
XOR_FUNC(xorMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // XOR MemImm8, Ax
XOR_FUNC(xorM7i16, core->dsp.readData(regR[7] + readParam()), 8, 2) // XOR MemR7Imm16, Ax
XOR_FUNC(xorM7i7, core->dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // XOR MemR7Imm7s, Ax
XOR_FUNC(xorMrn, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // XOR MemRnStepZids, Ax
XOR_FUNC(xorReg, *readRegP0[opcode & 0x1F], 8, 1) // XOR RegisterP0, Ax
XOR_FUNC(xorR6, regR[6], 4, 1) // XOR R6, Ax
