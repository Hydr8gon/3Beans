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

ADD40_FUNC(addAbb, *readAb[(opcode >> 10) & 0x3], regB, writeBx40S, 0) // ADD Ab, Bx
ADD40_FUNC(addBa, regB[(opcode >> 1) & 0x1].v, regA, writeAx40S, 0) // ADD Bx, Ax
ADD40_FUNC(addPb, shiftP[(opcode >> 1) & 0x1].v, regB, writeBx40S, 0) // ADD Px, Bx
ADD40_FUNC(addP1a, shiftP[1].v, regA, writeAx40S, 0) // ADD P1, Ax
ADD40_FUNC(addRega, readRegP0(opcode & 0x1F), regA, writeAx40S, 8) // ADD RegisterP0, Ax

// Add a 16-bit value to an accumulator and set flags
#define ADD16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

ADD16_FUNC(addI16a, readParam(), 8, 2) // ADD Imm16, Ax
ADD16_FUNC(addI8a, (opcode & 0xFF), 8, 1) // ADD Imm8u, Ax
ADD16_FUNC(addMi16a, core.dsp.readData(readParam()), 8, 2) // ADD MemImm16, Ax
ADD16_FUNC(addMi8a, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // ADD MemImm8, Ax
ADD16_FUNC(addM7i16a, core.dsp.readData(regR[7] + readParam()), 8, 2) // ADD MemR7Imm16, Ax
ADD16_FUNC(addM7i7a, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // ADD MemR7Imm7s, Ax
ADD16_FUNC(addMrna, core.dsp.readData(getRnStepZids(opcode)), 8, 1) // ADD MemRnStepZids, Ax
ADD16_FUNC(addR6a, regR[6], 4, 1) // ADD R6, Ax

// Add a value to the high part of an A accumulator and set flags
#define ADDH_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int32_t val2 = (op0) << 16; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

ADDH_FUNC(addhMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // ADDH MemImm8, Ax
ADDH_FUNC(addhMrn, core.dsp.readData(getRnStepZids(opcode)), 8) // ADDH MemRnStepZids, Ax
ADDH_FUNC(addhReg, *readReg[opcode & 0x1F], 8) // ADDH Register, Ax
ADDH_FUNC(addhR6, regR[6], 0) // ADDH R6, Ax

// Add a value to the low part of an A accumulator and set flags
#define ADDL_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    uint16_t val2 = op0; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

ADDL_FUNC(addlMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // ADDL MemImm8, Ax
ADDL_FUNC(addlMrn, core.dsp.readData(getRnStepZids(opcode)), 8) // ADDL MemRnStepZids, Ax
ADDL_FUNC(addlReg, *readReg[opcode & 0x1F], 8) // ADDL Register, Ax
ADDL_FUNC(addlR6, regR[6], 0) // ADDL R6, Ax

// Add a 16-bit immediate to a memory value and set flags
#define ADDVM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t val = core.dsp.readData(addr); \
    uint32_t res = int16_t(val) + int16_t(readParam()); \
    bool z = !(res & 0xFFFF); \
    bool m = (res & BIT(31)); \
    bool c = (val > uint16_t(res)); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    core.dsp.writeData(addr, res); \
    return 2; \
}

ADDVM_FUNC(addvMi8, (regMod[1] << 8) | (opcode & 0xFF)) // ADDV Imm16, MemImm8
ADDVM_FUNC(addvMrn, getRnStepZids(opcode)) // ADDV Imm16, MemRnStepZids

// Add a 16-bit immediate to a register value and set flags
#define ADDVR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    uint16_t val = op1r; \
    uint32_t res = int16_t(val) + int16_t(readParam()); \
    bool z = !(res & 0xFFFF); \
    bool m = (res & BIT(31)); \
    bool c = (val > uint16_t(res)); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    op1w(res); \
    return 2; \
}

ADDVR_FUNC(addvReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // ADDV Imm16, Register
ADDVR_FUNC(addvR6, regR[6], regR[6]=) // ADDV Imm16, R6

// Add the product registers with a base value in an accumulator and set flags
#define ADD3_FUNC(name, base, p0a, p1a) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = (base) + (shiftP[0].v >> p0a); \
    int64_t val2 = (shiftP[1].v >> p1a); \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAb40S[(opcode >> 2) & 0x3])(res); \
    return 1; \
}

ADD3_FUNC(addPpab, 0, 0, 0) // ADD P0, P1, Ab
ADD3_FUNC(adda, 0, 0, 16) // ADDA P0, P1, Ab
ADD3_FUNC(add3, *readAb[(opcode >> 2) & 0x3], 0, 0) // ADD3 P0, P1, Ab
ADD3_FUNC(add3a, *readAb[(opcode >> 2) & 0x3], 0, 16) // ADD3A P0, P1, Ab
ADD3_FUNC(add3aa, *readAb[(opcode >> 2) & 0x3], 16, 16) // ADD3AA P0, P1, Ab

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
AND_FUNC(andMi16, core.dsp.readData(readParam()), 8, 2) // AND MemImm16, Ax
AND_FUNC(andMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // AND MemImm8, Ax
AND_FUNC(andM7i16, core.dsp.readData(regR[7] + readParam()), 8, 2) // AND MemR7Imm16, Ax
AND_FUNC(andM7i7, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // AND MemR7Imm7s, Ax
AND_FUNC(andMrn, core.dsp.readData(getRnStepZids(opcode)), 8, 1) // AND MemRnStepZids, Ax
AND_FUNC(andReg, readRegP0(opcode & 0x1F, false), 8, 1) // AND RegisterP0, Ax
AND_FUNC(andR6, regR[6], 4, 1) // AND R6, Ax

int TeakInterp::andI8(uint16_t opcode) { // AND Imm8u, Ax
    // Bitwise and an 8-bit immediate with an A accumulator and set flags, but preserve bits 8-15
    int64_t res = regA[(opcode >> 8) & 0x1].v & (0xFF00 | (opcode & 0xFF));
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res & 0xFF));
    (this->*writeAx40[(opcode >> 8) & 0x1])(res);
    return 1;
}

// Change bits in a memory value using a 16-bit immediate mask and set flags
#define CHNGM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t res = core.dsp.readData(addr) ^ readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    core.dsp.writeData(addr, res); \
    return 2; \
}

CHNGM_FUNC(chngMi8, (regMod[1] << 8) | (opcode & 0xFF)) // CHNG Imm16, MemImm8
CHNGM_FUNC(chngMrn, getRnStepZids(opcode)) // CHNG Imm16, MemRnStepZids

// Change bits in a register value using a 16-bit immediate mask and set flags
#define CHNGR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    uint16_t res = (op1r) ^ readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    op1w(res); \
    return 2; \
}

CHNGR_FUNC(chngReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // CHNG Imm16, Register
CHNGR_FUNC(chngR6, regR[6], regR[6]=) // CHNG Imm16, R6
CHNGR_FUNC(chngSm, *readSttMod[opcode & 0x7], (this->*writeSttMod[opcode & 0x7])) // CHNG Imm16, SttMod

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

// Set a product register to zero
#define CLRP_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    writeP33<op0>(0); \
    return 1; \
}

CLRP_FUNC(clrp0, 0) // CLRP P0
CLRP_FUNC(clrp1, 1) // CLRP P1

int TeakInterp::clrp01(uint16_t opcode) { // CLRP P0, P1
    // Set both product registers to zero
    writeP33<0>(0);
    writeP33<1>(0);
    return 1;
}

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
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return 1; \
}

CMP40_FUNC(cmpAb, regA[(opcode >> 1) & 0x1].v, regB[opcode & 0x1].v) // CMP Ax, Bx
CMP40_FUNC(cmpBa, regB[(opcode >> 10) & 0x1].v, regA[opcode & 0x1].v) // CMP Bx, Ax
CMP40_FUNC(cmpB0b1, regB[0].v, regB[1].v) // CMP B0, B1
CMP40_FUNC(cmpB1b0, regB[1].v, regB[0].v) // CMP B1, B0
CMP40_FUNC(cmpRega, readRegP0(opcode & 0x1F), regA[(opcode >> 8) & 0x1].v) // CMP RegisterP0, Ax

// Compare a 16-bit value with an accumulator and set flags
#define CMP16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return cyc; \
}

CMP16_FUNC(cmpI16a, readParam(), 8, 2) // CMP Imm16, Ax
CMP16_FUNC(cmpI8a, (opcode & 0xFF), 8, 1) // CMP Imm8u, Ax
CMP16_FUNC(cmpMi16a, core.dsp.readData(readParam()), 8, 2) // CMP MemImm16, Ax
CMP16_FUNC(cmpMi8a, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // CMP MemImm8, Ax
CMP16_FUNC(cmpM7i16a, core.dsp.readData(regR[7] + readParam()), 8, 2) // CMP MemR7Imm16, Ax
CMP16_FUNC(cmpM7i7a, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // CMP MemR7Imm7s, Ax
CMP16_FUNC(cmpMrna, core.dsp.readData(getRnStepZids(opcode)), 8, 1) // CMP MemRnStepZids, Ax
CMP16_FUNC(cmpR6a, regR[6], 4, 1) // CMP R6, Ax

// Compare an unsigned value with an A accumulator and set flags
#define CMPU_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    uint16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    return 1; \
}

CMPU_FUNC(cmpuMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // CMPU MemImm8, Ax
CMPU_FUNC(cmpuMrn, core.dsp.readData(getRnStepZids(opcode)), 8) // CMPU MemRnStepZids, Ax
CMPU_FUNC(cmpuReg, *readReg[opcode & 0x1F], 8) // CMPU Register, Ax
CMPU_FUNC(cmpuR6, regR[6], 3) // CMPU R6, Ax

// Compare a 16-bit immediate with a register or memory value and set flags
#define CMPV_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    uint16_t val = op1; \
    uint32_t res = int16_t(val) - int16_t(readParam()); \
    bool z = !(res & 0xFFFF); \
    bool m = (res & BIT(31)); \
    bool c = (val < uint16_t(res)); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    return 2; \
}

CMPV_FUNC(cmpvMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // CMPV Imm16, MemImm8
CMPV_FUNC(cmpvMrn, core.dsp.readData(getRnStepZids(opcode))) // CMPV Imm16, MemRnStepZids
CMPV_FUNC(cmpvReg, *readReg[opcode & 0x1F]) // CMPV Imm16, Register
CMPV_FUNC(cmpvR6, regR[6]) // CMPV Imm16, R6

int TeakInterp::copy(uint16_t opcode) { // COPY Ax, Cond
    // Copy a value from the other A accumulator and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(~opcode >> 12) & 0x1].v;
        writeStt0((regStt[0] & ~0xE4) | calcZmne(val));
        (this->*writeAx40S[(opcode >> 12) & 0x1])(val);
    }
    return 1;
}

int TeakInterp::dec(uint16_t opcode) { // DEC Ax, Cond
    // Decrement an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val - 1) << 24) >> 24;
        bool v = ((1 ^ val) & ~(res ^ 1)) >> 39;
        bool c = (uint64_t(val) < uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40S[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Calculate the shift that would normalize a value and store it in the shift register
#define EXP_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    int64_t value = op0; \
    int16_t exp = 1; \
    while (!((value ^ (value << exp)) & BITL(40)) && exp <= 40) exp++; \
    regSv = (exp - 10); \
    return 1; \
}

EXP_FUNC(expB, regB[opcode & 0x1].v) // EXP Bx
EXP_FUNC(expMrn, int32_t(core.dsp.readData(getRnStepZids(opcode)) << 16)) // EXP MemRnStepZids
EXP_FUNC(expReg, readRegExp(opcode & 0x1F)) // EXP RegisterP0
EXP_FUNC(expR6, int32_t(regR[6] << 16)) // EXP R6

// Calculate the shift that would normalize a value and store it in the shift register and an A accumulator
#define EXPA_FUNC(name, op0, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t value = op0; \
    int16_t exp = 1; \
    while (!((value ^ (value << exp)) & BITL(40)) && exp <= 40) exp++; \
    (this->*writeAx16[(opcode >> op1s) & 0x1])(regSv = exp - 10); \
    return 1; \
}

EXPA_FUNC(expBa, regB[opcode & 0x1].v, 8) // EXP Bx, Ax
EXPA_FUNC(expMrna, int32_t(core.dsp.readData(getRnStepZids(opcode)) << 16), 8) // EXP MemRnStepZids, Ax
EXPA_FUNC(expRega, readRegExp(opcode & 0x1F), 8) // EXP RegisterP0, Ax
EXPA_FUNC(expR6a, int32_t(regR[6] << 16), 4) // EXP R6, Ax

int TeakInterp::inc(uint16_t opcode) { // INC Ax, Cond
    // Increment an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val + 1) << 24) >> 24;
        bool v = (~(1 ^ val) & (res ^ 1)) >> 39;
        bool c = (uint64_t(val) >= uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40S[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Saturate an A accumulator and set flags
#define LIM_FUNC(name, i0, i1) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = saturate(regA[i0].v); \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    writeA40<i1>(res); \
    return 1; \
}

LIM_FUNC(limA0, 0, 0) // LIM A0, A0
LIM_FUNC(limA1, 1, 1) // LIM A1, A1
LIM_FUNC(limA0a1, 0, 1) // LIM A0, A1
LIM_FUNC(limA1a0, 1, 0) // LIM A1, A0

// Add aligned P0 to an A accumulator, set flags, load X0 and Y0, and multiply them
#define MAA_FUNC(name, xu, yu, op0a, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 11) & 0x1].v; \
    int32_t val2 = (shiftP[0].v >> 16); \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    regX[0] = op1; \
    regY[0] = core.dsp.readData(getRnStepZids(op0a)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> 11) & 0x1])(res); \
    multiplyXY<xu##int16_t, yu##int16_t>(0); \
    return cyc; \
}

MAA_FUNC(maaMrmr, ,, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MAA MR, MR, Ax
MAA_FUNC(maaMrni16, ,, (opcode & 0x1F), readParam(), 2) // MAA MemRnStepZids, Imm16, Ax
MAA_FUNC(maasuMrmr, u,, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MAASU M,M,Ax
MAA_FUNC(maasuMrni16, u,, (opcode & 0x1F), readParam(), 2) // MAASU MemRnStepZids, Imm16, Ax

// Add aligned P0 to an A accumulator, set flags, load X0, and multiply it with Y0
#define MAAY_FUNC(name, xu, yu, op1, op2s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op2s) & 0x1].v; \
    int32_t val2 = (shiftP[0].v >> 16); \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    regX[0] = op1; \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op2s) & 0x1])(res); \
    multiplyXY<xu##int16_t, yu##int16_t>(0); \
    return 1; \
}

MAAY_FUNC(maaY0mi8, ,, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 11) // MAA Y0, MemImm8, Ax
MAAY_FUNC(maaY0mrn, ,, core.dsp.readData(getRnStepZids(opcode)), 11) // MAA Y0, MemRnStepZids, Ax
MAAY_FUNC(maaY0reg, ,, *readReg[opcode & 0x1F], 11) // MAA Y0, Register, Ax
MAAY_FUNC(maaY0r6, ,, regR[6], 0) // MAA Y0, R6, Ax
MAAY_FUNC(maasuY0mrn, u,, core.dsp.readData(getRnStepZids(opcode)), 11) // MAASU Y0, MemRnStepZids, Ax
MAAY_FUNC(maasuY0reg, u,, *readReg[opcode & 0x1F], 11) // MAASU Y0, Register, Ax
MAAY_FUNC(maasuY0r6, u,, regR[6], 0) // MAASU Y0, R6, Ax

// Add shifted P0 to an A accumulator, set flags, load X0 and Y0, and multiply them
#define MAC_FUNC(name, xu, yu, op0a, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 11) & 0x1].v; \
    int64_t val2 = shiftP[0].v; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    regX[0] = op1; \
    regY[0] = core.dsp.readData(getRnStepZids(op0a)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> 11) & 0x1])(res); \
    multiplyXY<xu##int16_t, yu##int16_t>(0); \
    return cyc; \
}

MAC_FUNC(macMrmr, ,, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MAC MR, MR, Ax
MAC_FUNC(macMrni16, ,, (opcode & 0x1F), readParam(), 2) // MAC MemRnStepZids, Imm16, Ax
MAC_FUNC(macsuMrmr, u,, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MACSU M,M,Ax
MAC_FUNC(macsuMrni16, u,, (opcode & 0x1F), readParam(), 2) // MACSU MemRnStepZids, Imm16, Ax
MAC_FUNC(macusMrmr, ,u, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MACUS M,M,Ax
MAC_FUNC(macusMrni16, ,u, (opcode & 0x1F), readParam(), 2) // MACUS MemRnStepZids, Imm16, Ax
MAC_FUNC(macuuMrmr, u,u, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MACUU M,M,A
MAC_FUNC(macuuMrni16, u,u, (opcode & 0x1F), readParam(), 2) // MACUU MemRnStepZids, Imm16, Ax

// Add shifted P0 to an A accumulator, set flags, load X0, and multiply it with Y0
#define MACY_FUNC(name, xu, yu, op1, op2s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op2s) & 0x1].v; \
    int64_t val2 = shiftP[0].v; \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    regX[0] = op1; \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op2s) & 0x1])(res); \
    multiplyXY<xu##int16_t, yu##int16_t>(0); \
    return 1; \
}

MACY_FUNC(macY0mi8, ,, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 11) // MAC Y0, MemImm8, Ax
MACY_FUNC(macY0mrn, ,, core.dsp.readData(getRnStepZids(opcode)), 11) // MAC Y0, MemRnStepZids, Ax
MACY_FUNC(macY0reg, ,, *readReg[opcode & 0x1F], 11) // MAC Y0, Register, Ax
MACY_FUNC(macY0r6, ,, regR[6], 0) // MAC Y0, R6, Ax
MACY_FUNC(macsuY0mi8, u,, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 11) // MACSU Y0, MemImm8, Ax
MACY_FUNC(macsuY0mrn, u,, core.dsp.readData(getRnStepZids(opcode)), 11) // MACSU Y0, MemRnStepZids, Ax
MACY_FUNC(macsuY0reg, u,, *readReg[opcode & 0x1F], 11) // MACSU Y0, Register, Ax
MACY_FUNC(macsuY0r6, u,, regR[6], 0) // MACSU Y0, R6, Ax
MACY_FUNC(macusY0mrn, ,u, core.dsp.readData(getRnStepZids(opcode)), 11) // MACUS Y0, MemRnStepZids, Ax
MACY_FUNC(macusY0reg, ,u, *readReg[opcode & 0x1F], 11) // MACUS Y0, Register, Ax
MACY_FUNC(macusY0r6, ,u, regR[6], 0) // MACUS Y0, R6, Ax
MACY_FUNC(macuuY0mrn, u,u, core.dsp.readData(getRnStepZids(opcode)), 11) // MACUU Y0, MemRnStepZids, Ax
MACY_FUNC(macuuY0reg, u,u, *readReg[opcode & 0x1F], 11) // MACUU Y0, Register, Ax
MACY_FUNC(macuuY0r6, u,u, regR[6], 0) // MACUU Y0, R6, Ax

// Take the min/max of A accumulators, set MIXP if changed, and post-adjust R0
#define MINMAX_FUNC(name, op) int TeakInterp::name(uint16_t opcode) { \
    bool a = (opcode >> 8) & 0x1; \
    if (regA[!a].v op regA[a].v) { \
        regA[a].v = regA[!a].v; \
        regMixp = regR[0]; \
        writeStt0(regStt[0] | BIT(6)); \
    } \
    else { \
        writeStt0(regStt[0] & ~BIT(6)); \
    } \
    getRnStepZids(opcode); \
    return 1; \
}

MINMAX_FUNC(maxGe, >=) // MAX Ax, R0StepZids, GE
MINMAX_FUNC(maxGt, >) // MAX Ax, R0StepZids, GT
MINMAX_FUNC(minLe, <=) // MIN Ax, R0StepZids, LE
MINMAX_FUNC(minLt, <) // MIN Ax, R0StepZids, LT

// Add the previous product registers with a base value in an accumulator and set flags
// Then, load memory values into both X and Y registers with offset/step and multiply them
#define MMA_FUNC(n, b, p0a, p1a, x0u, y0u, x1u, y1u, ops, opm, op0s, op1s, op2s) int TeakInterp::n(uint16_t opcode) { \
    int64_t val1 = (b) + (shiftP[0].v >> p0a); \
    int64_t val2 = (shiftP[1].v >> p1a); \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    regX[1] = core.dsp.readData(offsReg(arpRi[(opcode >> ops) & opm], arpCi[(opcode >> op1s) & opm])); \
    regY[1] = core.dsp.readData(offsReg(arpRj[(opcode >> ops) & opm], arpCj[(opcode >> op0s) & opm])); \
    regX[0] = core.dsp.readData(stepReg(arpRi[(opcode >> ops) & opm], arpPi[(opcode >> op1s) & opm])); \
    regY[0] = core.dsp.readData(stepReg(arpRj[(opcode >> ops) & opm], arpPj[(opcode >> op0s) & opm])); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAb40S[(opcode >> op2s) & 0x3])(res); \
    multiplyXY<x1u##int16_t, y1u##int16_t>(1); \
    multiplyXY<x0u##int16_t, y0u##int16_t>(0); \
    return 1; \
}

MMA_FUNC(mma, 0, 0, 0, ,,,, 2, 1, 1, 0, 10) // MMA MemRj1StepArp1, MemRi1StepArp1, Ab
MMA_FUNC(mmaa, 0, 0, 16, ,,,, 2, 1, 1, 0, 10) // MMAA MemRj1StepArp1, MemRi1StepArp1, Ab
MMA_FUNC(mma3, *readAb[(opcode >> 6) & 0x3], 0, 0, ,,,, 4, 3, 2, 0, 6) // MMA3 MemRjStepArp, MemRiStepArp, Ab
MMA_FUNC(mma3a, *readAb[(opcode >> 10) & 0x3], 0, 16, ,,,, 0, 1, 9, 8, 10) // MMA3A MemRj1StepArp1, MemRi1StepArp1, Ab
MMA_FUNC(mmsua3, *readAb[(opcode >> 6) & 0x3], 0, 0, ,,u,, 5, 1, 4, 3, 6) // MMSUA3 MemRj1StepArp1, MemRi1StepArp1, Ab
MMA_FUNC(mmusa3, *readAb[(opcode >> 6) & 0x3], 0, 0, ,,,u, 5, 1, 4, 3, 6) // MMUSA3 MemRj1StepArp1, MemRi1StepArp1, Ab
MMA_FUNC(mmsua3a, *readAb[(opcode >> 6) & 0x3], 0, 16, ,,u,, 5, 1, 4, 3, 6) // MMSUA3A MemRj1StpArp1, MemRi1StpArp1, Ab
MMA_FUNC(mmusa3a, *readAb[(opcode >> 6) & 0x3], 0, 16, ,,,u, 5, 1, 4, 3, 6) // MMUSA3A MemRj1StpArp1, MemRi1StpArp1, Ab
MMA_FUNC(msumsua3a, *readAb[(opcode >> 6) & 0x3], 0, 16, u,,u,, 5, 1, 4, 3, 6) // MSUMSUA3A MRj1SArp1, MRi1SArp1, Ab
MMA_FUNC(msumusa3a, *readAb[(opcode >> 6) & 0x3], 0, 16, u,,,u, 5, 1, 4, 3, 6) // MSUMUSA3A MRj1SArp1, MRi1SArp1, Ab
MMA_FUNC(msumsua3aa, *readAb[(opcode >> 6) & 0x3], 16, 16, u,,u,, 5, 1, 4, 3, 6) // MSUMSUA3AA MRj1SArp1, MRi1SArp1, Ab
MMA_FUNC(msumusa3aa, *readAb[(opcode >> 6) & 0x3], 16, 16, u,,,u, 5, 1, 4, 3, 6) // MSUMUSA3AA MRj1SArp1, MRi1SArp1, Ab

// Add the previous product registers with a base value in an accumulator and set flags
// Then, load memory values into X registers with offset/step and multiply with Y registers
#define MMAY_FUNC(name, p1a, x0u, y0u, x1u, y1u, ars0, ars1, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v + shiftP[0].v; \
    int64_t val2 = (shiftP[1].v >> p1a); \
    int64_t res = ((val1 + val2) << 24) >> 24; \
    bool v = (~(val2 ^ val1) & (res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) > uint64_t(res)); \
    uint8_t rar = ((opcode >> ars0) & 0x4) | ((opcode >> ars1) & 0x1); \
    regX[1] = core.dsp.readData(getRarOffsAr(rar)); \
    regX[0] = core.dsp.readData(getRarStepAr(rar)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op1s) & 0x1])(res); \
    multiplyXY<x1u##int16_t, y1u##int16_t>(1); \
    multiplyXY<x0u##int16_t, y0u##int16_t>(0); \
    return 1; \
}

MMAY_FUNC(mma3Y, 0, ,,,, 2, 3, 8) // MMA3 Y, MemRar1StepAr1, Ax
MMAY_FUNC(mma3aY, 16, ,,,, 2, 3, 8) // MMA3A Y, MemRar1StepAr1, Ax
MMAY_FUNC(mmsua3Y, 0, ,,u,, 2, 3, 8) // MMSUA3 Y, MemRar1StepAr1, Ax
MMAY_FUNC(mmusa3Y, 0, ,,,u, 1, 2, 4) // MMUSA3 Y, MemRar1StepAr1, Ax
MMAY_FUNC(mmsua3aY, 16, ,,u,, 2, 3, 8) // MMSUA3A Y, MemRar1StepAr1, Ax
MMAY_FUNC(mmusa3aY, 16, ,,,u, 1, 2, 4) // MMUSA3A Y, MemRar1StepAr1, Ax

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

// Modify an address register with modulo disabled and set the R flag if zero
#define MODRD_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    dmod = true; \
    op0; \
    dmod = false; \
    bool r = (regR[opcode & 0x7] == 0); \
    writeStt1((regStt[1] & ~0x10) | (r << 4)); \
    return 1; \
}

MODRD_FUNC(modrD2d, stepReg(opcode & 0x7, -2)) // MODR MemRnStepD2, DMOD
MODRD_FUNC(modrI2d, stepReg(opcode & 0x7, 2)) // MODR MemRnStepI2, DMOD
MODRD_FUNC(modrZidsd, getRnStepZids(opcode)) // MODR MemRnStepZids, DMOD

int TeakInterp::modrMrmr(uint16_t opcode) { // MODR MemRiStepArp, MemRjStepArp
    // Modify an I address register and a J address register
    stepReg(arpRi[(opcode >> 10) & 0x3], arpPi[(opcode >> 0) & 0x3]);
    stepReg(arpRj[(opcode >> 10) & 0x3], arpPj[(opcode >> 5) & 0x3]);
    return 1;
}

// Modify an I address register and a J address register with modulo disabled for one
#define MODR2_FUNC(name, op0ib, op0jb, op0s, op1ib, op1jb, op1is, op1js) int TeakInterp::name(uint16_t opcode) { \
    stepReg(op0ib[(opcode >> op0s) & 0x3], op1ib[(opcode >> op1is) & 0x3]); \
    dmod = true; \
    stepReg(op0jb[(opcode >> op0s) & 0x3], op1jb[(opcode >> op1js) & 0x3]); \
    dmod = false; \
    return 1; \
}

MODR2_FUNC(modrMrmrd, arpRi, arpRj, 5, arpPi, arpPj, 1, 3) // MODR MemRiStepArp, MemRjStepArp, DMOD
MODR2_FUNC(modrMrdmr, arpRj, arpRi, 8, arpPj, arpPi, 3, 0) // MODR MemRiStepArp, DMOD, MemRjStepArp

int TeakInterp::modrMrdmrd(uint16_t opcode) { // MODR MemRiStepArp, DMOD, MemRjStepArp, DMOD
    // Modify an I address register and a J address register with modulo disabled for both
    dmod = true;
    stepReg(arpRi[(opcode >> 5) & 0x3], arpPi[(opcode >> 1) & 0x3]);
    stepReg(arpRj[(opcode >> 5) & 0x3], arpPj[(opcode >> 3) & 0x3]);
    dmod = false;
    return 1;
}

// Move a memory value shifted by the shift register into an accumulator
#define MOVSM_FUNC(name, op0a, op1s) int TeakInterp::name(uint16_t opcode) { \
    (this->*writeAb40[(opcode >> op1s) & 0x3])(shift(int16_t(core.dsp.readData(op0a)), regSv)); \
    return 1; \
}

MOVSM_FUNC(movsMi8ab, (regMod[1] << 8) | (opcode & 0xFF), 11) // MOVS MemImm8, Ab
MOVSM_FUNC(movsMrnab, getRnStepZids(opcode), 5) // MOVS MemRnStepZids, Ab

// Move a register value shifted by the shift register into an accumulator
#define MOVSR_FUNC(name, op0, op1) int TeakInterp::name(uint16_t opcode) { \
    (this->*op1)(shift(int16_t(op0), regSv)); \
    return 1; \
}

MOVSR_FUNC(movsRegab, *readReg[opcode & 0x1F], writeAb40[(opcode >> 5) & 0x3]) // MOVS Register, Ab
MOVSR_FUNC(movsR6a, regR[6], writeAx40[opcode & 0x1]) // MOVS R6, Ax

// Load a value into X0 and multiply it with Y0
#define MPY_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    regX[0] = op1; \
    multiplyXY<int16_t, int16_t>(0); \
    return 1; \
}

MPY_FUNC(mpyY0mi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // MPY Y0, MemImm8
MPY_FUNC(mpyY0mrn, core.dsp.readData(getRnStepZids(opcode))) // MPY Y0, MemRnStepZids
MPY_FUNC(mpyY0reg, *readReg[opcode & 0x1F]) // MPY Y0, Register
MPY_FUNC(mpyY0r6, regR[6]) // MPY Y0, R6
MPY_FUNC(mpyi, int8_t(opcode)) // MPYI Y0, Imm8s

// Load a value into X0 and multiply it with Y0, treating X as unsigned
#define MPYSU_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    regX[0] = op1; \
    multiplyXY<uint16_t, int16_t>(0); \
    return 1; \
}

MPYSU_FUNC(mpysuY0mrn, core.dsp.readData(getRnStepZids(opcode))) // MPYSU Y0, MemRnStepZids
MPYSU_FUNC(mpysuY0reg, *readReg[opcode & 0x1F]) // MPYSU Y0, Register
MPYSU_FUNC(mpysuY0r6, regR[6]) // MPYSU Y0, R6

int TeakInterp::mpysuMrmr(uint16_t opcode) { // MPYSU MemR45StepZids, MemR0123StepZids
    // Load memory values into X0/Y0 and multiply them, treating X as unsigned
    regX[0] = core.dsp.readData(getRnStepZids(opcode & 0x1B));
    regY[0] = core.dsp.readData(getRnStepZids(((opcode >> 2) & 0x19) + 4));
    multiplyXY<uint16_t, int16_t>(0);
    return 1;
}

// Subtract the previous product from an A accumulator, set flags, load X0 and Y0, and multiply them
#define MSU_FUNC(name, op0a, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 8) & 0x1].v; \
    int64_t val2 = shiftP[0].v; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    regX[0] = op1; \
    regY[0] = core.dsp.readData(getRnStepZids(op0a)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> 8) & 0x1])(res); \
    multiplyXY<int16_t, int16_t>(0); \
    return cyc; \
}

MSU_FUNC(msuMrmr, ((opcode >> 2) & 0x19) + 4, core.dsp.readData(getRnStepZids(opcode & 0x1B)), 1) // MSU MR45, MR03, Ax
MSU_FUNC(msuMrni16, (opcode & 0x1F), readParam(), 2) // MSU MemRnStepZids, Imm16, Ax

// Subtract the previous product from an A accumulator, set flags, load X0, and multiply it with Y0
#define MSUY_FUNC(name, op1, op2s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op2s) & 0x1].v; \
    int64_t val2 = shiftP[0].v; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    regX[0] = op1; \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op2s) & 0x1])(res); \
    multiplyXY<int16_t, int16_t>(0); \
    return 1; \
}

MSUY_FUNC(msuY0mi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8) // MSU Y0, MemImm8, Ax
MSUY_FUNC(msuY0mrn, core.dsp.readData(getRnStepZids(opcode)), 8) // MSU Y0, MemRnStepZids, Ax
MSUY_FUNC(msuY0reg, *readReg[opcode & 0x1F], 8) // MSU Y0, Register, Ax
MSUY_FUNC(msuY0r6, regR[6], 0) // MSU Y0, R6, Ax

int TeakInterp::neg(uint16_t opcode) { // NEG Ax, Cond
    // Negate an A accumulator by subtracting it from 0 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val1 = 0;
        int64_t val2 = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val1 - val2) << 24) >> 24;
        bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39;
        bool c = (uint64_t(val1) < uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40S[(opcode >> 12) & 0x1])(res);
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
OR_FUNC(orMi16, core.dsp.readData(readParam()), regA, 8, 8, 2) // OR MemImm16, Ax
OR_FUNC(orMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), regA, 8, 8, 1) // OR MemImm8, Ax
OR_FUNC(orM7i16, core.dsp.readData(regR[7] + readParam()), regA, 8, 8, 2) // OR MemR7Imm16, Ax
OR_FUNC(orM7i7, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), regA, 8, 8, 1) // OR MemR7Imm7s, Ax
OR_FUNC(orMrn, core.dsp.readData(getRnStepZids(opcode)), regA, 8, 8, 1) // OR MemRnStepZids, Ax
OR_FUNC(orReg, readRegP0(opcode & 0x1F, false), regA, 8, 8, 1) // OR RegisterP0, Ax
OR_FUNC(orR6, regR[6], regA, 4, 4, 1) // OR R6, Ax

int TeakInterp::rnd(uint16_t opcode) { // RND Ax, Cond
    // Round the upper part of an A accumulator and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val + 0x8000) << 24) >> 24;
        bool v = (~val & res) >> 39;
        bool c = (uint64_t(val) > uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx40S[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Clear bits in a memory value using a 16-bit immediate and set flags
#define RSTM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    uint16_t res = core.dsp.readData(addr) & ~readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    core.dsp.writeData(addr, res); \
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
    uint16_t res = core.dsp.readData(addr) | readParam(); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6)); \
    core.dsp.writeData(addr, res); \
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
    // Shift an accumulator by the shift register if the condition is met
    if (checkCond(opcode))
        (this->*writeAb40[(opcode >> 5) & 0x3])(shift(*readAb[(opcode >> 10) & 0x3], regSv));
    return 1;
}

// Move a value shifted by a signed immediate into an accumulator
#define SHFI_FUNC(name, op0, op1s, op2s) int TeakInterp::name(uint16_t opcode) { \
    (this->*writeAb40[(opcode >> op1s) & 0x3])(shift(op0, int8_t(opcode << op2s) >> op2s)); \
    return 1; \
}

SHFI_FUNC(movsi, int16_t(*readReg[(opcode >> 9) & 0x7]), 5, 3) // MOVSI R0123457y0, Ab, Imm5s
SHFI_FUNC(shfi, *readAb[(opcode >> 10) & 0x3], 7, 2) // SHFI Ab, Ab, Imm6s

// Shift an accumulator by a constant amount if the condition is met
#define SHLR_FUNC(name, op0r, op0w, amt) int TeakInterp::name(uint16_t opcode) { \
    if (checkCond(opcode)) \
        (this->*op0w[(opcode >> 12) & 0x1])(shift(op0r[(opcode >> 12) & 0x1].v, amt)); \
    return 1; \
}

SHLR_FUNC(shlA, regA, writeAx40, 1) // SHL Ax, Cond
SHLR_FUNC(shlB, regB, writeBx40, 1) // SHL Bx, Cond
SHLR_FUNC(shl4A, regA, writeAx40, 4) // SHL4 Ax, Cond
SHLR_FUNC(shl4B, regB, writeBx40, 4) // SHL4 Bx, Cond
SHLR_FUNC(shrA, regA, writeAx40, -1) // SHR Ax, Cond
SHLR_FUNC(shrB, regB, writeBx40, -1) // SHR Bx, Cond
SHLR_FUNC(shr4A, regA, writeAx40, -4) // SHR4 Ax, Cond
SHLR_FUNC(shr4B, regB, writeBx40, -4) // SHR4 Bx, Cond

// Load a value into both X0 and Y0 and multiply them
#define SQR_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    regX[0] = regY[0] = op0; \
    multiplyXY<int16_t, int16_t>(0); \
    return 1; \
}

SQR_FUNC(sqrMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // SQR MemImm8
SQR_FUNC(sqrMrn, core.dsp.readData(getRnStepZids(opcode))) // SQR MemRnStepZids
SQR_FUNC(sqrReg, *readReg[opcode & 0x1F]) // SQR Register
SQR_FUNC(sqrR6, regR[6]) // SQR R6

// Subtract a 40-bit value from an accumulator and set flags
#define SUB40_FUNC(name, op0, op1rb, op1wb, op1s) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = op1rb[(opcode >> op1s) & 0x1].v; \
    int64_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*op1wb[(opcode >> op1s) & 0x1])(res); \
    return 1; \
}

SUB40_FUNC(subAbb, *readAb[(opcode >> 3) & 0x3], regB, writeBx40S, 8) // SUB Ab, Bx
SUB40_FUNC(subBa, regB[(opcode >> 4) & 0x1].v, regA, writeAx40S, 3) // SUB Bx, Ax
SUB40_FUNC(subRega, readRegP0(opcode & 0x1F), regA, writeAx40S, 8) // SUB RegisterP0, Ax

// Subtract a 16-bit value from an accumulator and set flags
#define SUB16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

SUB16_FUNC(subI16a, readParam(), 8, 2) // SUB Imm16, Ax
SUB16_FUNC(subI8a, (opcode & 0xFF), 8, 1) // SUB Imm8u, Ax
SUB16_FUNC(subMi16a, core.dsp.readData(readParam()), 8, 2) // SUB MemImm16, Ax
SUB16_FUNC(subMi8a, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // SUB MemImm8, Ax
SUB16_FUNC(subM7i16a, core.dsp.readData(regR[7] + readParam()), 8, 2) // SUB MemR7Imm16, Ax
SUB16_FUNC(subM7i7a, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // SUB MemR7Imm7s, Ax
SUB16_FUNC(subMrna, core.dsp.readData(getRnStepZids(opcode)), 8, 1) // SUB MemRnStepZids, Ax
SUB16_FUNC(subR6a, regR[6], 4, 1) // SUB R6, Ax

// Subtract a value from the high part of an A accumulator and set flags
#define SUBH_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 8) & 0x1].v; \
    int32_t val2 = (op0) << 16; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> 8) & 0x1])(res); \
    return 1; \
}

SUBH_FUNC(subhMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // SUBH MemImm8, Ax
SUBH_FUNC(subhMrn, core.dsp.readData(getRnStepZids(opcode))) // SUBH MemRnStepZids, Ax
SUBH_FUNC(subhReg, *readReg[opcode & 0x1F]) // SUBH Register, Ax
SUBH_FUNC(subhR6, regR[6]) // SUBH R6, Ax

// Subtract a value from the low part of an A accumulator and set flags
#define SUBL_FUNC(name, op0) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> 8) & 0x1].v; \
    uint16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) < uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx40S[(opcode >> 8) & 0x1])(res); \
    return 1; \
}

SUBL_FUNC(sublMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // SUBL MemImm8, Ax
SUBL_FUNC(sublMrn, core.dsp.readData(getRnStepZids(opcode))) // SUBL MemRnStepZids, Ax
SUBL_FUNC(sublReg, *readReg[opcode & 0x1F]) // SUBL Register, Ax
SUBL_FUNC(sublR6, regR[6]) // SUBL R6, Ax

// Set the Z flag to a specified bit in a register or memory value
#define TSTB_FUNC(name, op0, op1, cyc) int TeakInterp::name(uint16_t opcode) { \
    bool z = (op0) & BIT((op1) & 0xF); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return cyc; \
}

TSTB_FUNC(tstbMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), opcode >> 8, 1) // TSTB MemImm8, Imm4
TSTB_FUNC(tstbMrn, core.dsp.readData(getRnStepZids(opcode)), opcode >> 8, 1) // TSTB MemRnStepZids, Imm4
TSTB_FUNC(tstbReg, *readReg[opcode & 0x1F], opcode >> 8, 1) // TSTB Register, Imm4
TSTB_FUNC(tstbR6, regR[6], opcode >> 8, 1) // TSTB R6, Imm4
TSTB_FUNC(tstbSm, *readSttMod[opcode & 0x7], readParam(), 2) // TSTB SttMod, Imm4

// Set the Z flag if the bits in a value masked by an A accumulator are 0
#define TST0A_FUNC(name, op0s, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !((op1) & regA[(opcode >> op0s) & 0x1].l); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 1; \
}

TST0A_FUNC(tst0Almi8, 8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST0 Axl, MemImm8
TST0A_FUNC(tst0Almrn, 8, getRnStepZids(opcode)) // TST0 Axl, MemRnStepZids
TST0A_FUNC(tst0Alreg, 8, *readReg[opcode & 0x1F]) // TST0 Axl, Register
TST0A_FUNC(tst0Alr6, 4, regR[6]) // TST0 Axl, R6

// Set the Z flag if the bits in a value masked by a 16-bit immediate are 0
#define TST0I_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !((op1) & readParam()); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 2; \
}

TST0I_FUNC(tst0I16mi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST0 Imm16, MemImm8
TST0I_FUNC(tst0I16mrn, getRnStepZids(opcode)) // TST0 Imm16, MemRnStepZids
TST0I_FUNC(tst0I16reg, *readReg[opcode & 0x1F]) // TST0 Imm16, Register
TST0I_FUNC(tst0I16r6, regR[6]) // TST0 Imm16, R6
TST0I_FUNC(tst0I16sm, *readSttMod[opcode & 0x7]) // TST0 Imm16, SttMod

// Set the Z flag if the bits in a value masked by an A accumulator are 1
#define TST1A_FUNC(name, op0s, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !(~(op1) & regA[(opcode >> op0s) & 0x1].l); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 1; \
}

TST1A_FUNC(tst1Almi8, 8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST1 Axl, MemImm8
TST1A_FUNC(tst1Almrn, 8, getRnStepZids(opcode)) // TST1 Axl, MemRnStepZids
TST1A_FUNC(tst1Alreg, 8, *readReg[opcode & 0x1F]) // TST1 Axl, Register
TST1A_FUNC(tst1Alr6, 4, regR[6]) // TST1 Axl, R6

// Set the Z flag if the bits in a value masked by a 16-bit immediate are 1
#define TST1I_FUNC(name, op1) int TeakInterp::name(uint16_t opcode) { \
    bool z = !(~(op1) & readParam()); \
    writeStt0((regStt[0] & ~0x80) | (z << 7)); \
    return 2; \
}

TST1I_FUNC(tst1I16mi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF))) // TST1 Imm16, MemImm8
TST1I_FUNC(tst1I16mrn, getRnStepZids(opcode)) // TST1 Imm16, MemRnStepZids
TST1I_FUNC(tst1I16reg, *readReg[opcode & 0x1F]) // TST1 Imm16, Register
TST1I_FUNC(tst1I16r6, regR[6]) // TST1 Imm16, R6
TST1I_FUNC(tst1I16sm, *readSttMod[opcode & 0x7]) // TST1 Imm16, SttMod

// Bitwise exclusive or a value with an A accumulator and set flags
#define XOR_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = regA[(opcode >> op1s) & 0x1].v ^ (op0); \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    (this->*writeAx40[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

XOR_FUNC(xorI16, readParam(), 8, 2) // XOR Imm16, Ax
XOR_FUNC(xorI8, (opcode & 0xFF), 8, 1) // XOR Imm8u, Ax
XOR_FUNC(xorMi16, core.dsp.readData(readParam()), 8, 2) // XOR MemImm16, Ax
XOR_FUNC(xorMi8, core.dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // XOR MemImm8, Ax
XOR_FUNC(xorM7i16, core.dsp.readData(regR[7] + readParam()), 8, 2) // XOR MemR7Imm16, Ax
XOR_FUNC(xorM7i7, core.dsp.readData(regR[7] + (int8_t(opcode << 1) >> 1)), 8, 1) // XOR MemR7Imm7s, Ax
XOR_FUNC(xorMrn, core.dsp.readData(getRnStepZids(opcode)), 8, 1) // XOR MemRnStepZids, Ax
XOR_FUNC(xorReg, readRegP0(opcode & 0x1F, false), 8, 1) // XOR RegisterP0, Ax
XOR_FUNC(xorR6, regR[6], 4, 1) // XOR R6, Ax
