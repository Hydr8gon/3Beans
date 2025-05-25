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

// Add a 16-bit immediate to a memory value and set flags
#define ADDVM_FUNC(name, op1a) int TeakInterp::name(uint16_t opcode) { \
    uint16_t addr = op1a; \
    int32_t res = int16_t(core->dsp.readData(addr)) + int16_t(readParam()); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    bool c = (res & BIT(16)); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    core->dsp.writeData(addr, res); \
    return 2; \
}

ADDVM_FUNC(addvMi8, (regMod[1] << 8) | (opcode & 0xFF)) // ADDV Imm16, MemImm8
ADDVM_FUNC(addvMrn, getRnStepZids(opcode)) // ADDV Imm16, MemRnStepZids

// Add a 16-bit immediate to a register value and set flags
#define ADDVR_FUNC(name, op1r, op1w) int TeakInterp::name(uint16_t opcode) { \
    int32_t res = int16_t(op1r) + int16_t(readParam()); \
    bool z = (res == 0); \
    bool m = (res & BIT(15)); \
    bool c = (res & BIT(16)); \
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3)); \
    op1w(res); \
    return 2; \
}

ADDVR_FUNC(addvReg, *readReg[opcode & 0x1F], (this->*writeReg[opcode & 0x1F])) // ADDV Imm16, Register
ADDVR_FUNC(addvR6, regR[6], regR[6]=) // ADDV Imm16, R6

// Set an accumulator to zero and update flags if the condition is met
#define CLR_FUNC(name, op0b) int TeakInterp::name(uint16_t opcode) { \
    if (checkCond(opcode)) { \
        writeStt0((regStt[0] & ~0xE4) | 0xA0); \
        (this->*op0b[(opcode >> 12) & 0x1])(0); \
    } \
    return 1; \
}

CLR_FUNC(clrA, writeAx) // CLR Ax, Cond
CLR_FUNC(clrB, writeBx) // CLR Bx, Cond

// Set an accumulator to 0x8000 and update flags if the condition is met
#define CLRR_FUNC(name, op0b) int TeakInterp::name(uint16_t opcode) { \
    if (checkCond(opcode)) { \
        writeStt0(regStt[0] & ~0xE4); \
        (this->*op0b[(opcode >> 12) & 0x1])(0x8000); \
    } \
    return 1; \
}

CLRR_FUNC(clrrA, writeAx) // CLRR Ax, Cond
CLRR_FUNC(clrrB, writeBx) // CLRR Bx, Cond

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

int TeakInterp::dec(uint16_t opcode) { // DEC Ax, Cond
    // Decrement an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t val = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((val - 1) << 24) >> 24;
        bool v = ((1 ^ val) & ~(res ^ 1)) >> 39;
        bool c = (uint64_t(val) >= uint64_t(res));
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx[(opcode >> 12) & 0x1])(res);
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
        (this->*writeAx[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

// Bitwise or an accumulator with a value and set flags
#define OR_FUNC(name, op0, op1b, op1s, op2s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t res = op1b[(opcode >> op1s) & 0x1].v | op0; \
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res)); \
    (this->*writeAx[(opcode >> op2s) & 0x1])(res); \
    return cyc; \
}

OR_FUNC(orAba, *readAb[(opcode >> 10) & 0x3], regA, 6, 5, 1) // OR Ab, Ax, Ax
OR_FUNC(orAb, regA[(opcode >> 8) & 0x1].v, regB, 1, 0, 1) // OR Ax, Bx, Ax
OR_FUNC(orBb, regB[(opcode >> 10) & 0x1].v, regB, 1, 0, 1) // OR Bx, Bx, Ax
OR_FUNC(orI16, readParam(), regA, 8, 8, 2) // OR Imm16, Ax
OR_FUNC(orI8, (opcode & 0xFF), regA, 8, 8, 1) // OR Imm8u, Ax
OR_FUNC(orMi16, core->dsp.readData(readParam()), regA, 8, 8, 2) // OR MemImm16, Ax
OR_FUNC(orMi8, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), regA, 8, 8, 1) // OR MemImm8, Ax
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

int TeakInterp::shfi(uint16_t opcode) { // SHFI Ab, Ab, Imm6s
    // Shift an accumulator by a 6-bit signed immediate and set flags
    int8_t shift = int8_t(opcode << 2) >> 2;
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
    (this->*writeAb[(opcode >> 7) & 0x3])(res);
    return 1;
}

// Subtract one accumulator from another and set flags
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

SUB40_FUNC(subAbb, *readAb[(opcode >> 3) & 0x3], regB, writeBx, 8) // SUB Ab, Bx
SUB40_FUNC(subBa, regB[(opcode >> 4) & 0x1].v, regA, writeAx, 3) // SUB Bx, Ax

// Subtract a 16-bit value from an accumulator and set flags
#define SUB16_FUNC(name, op0, op1s, cyc) int TeakInterp::name(uint16_t opcode) { \
    int64_t val1 = regA[(opcode >> op1s) & 0x1].v; \
    int16_t val2 = op0; \
    int64_t res = ((val1 - val2) << 24) >> 24; \
    bool v = ((val2 ^ val1) & ~(res ^ val2)) >> 39; \
    bool c = (uint64_t(val1) >= uint64_t(res)); \
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1)); \
    (this->*writeAx[(opcode >> op1s) & 0x1])(res); \
    return cyc; \
}

SUB16_FUNC(subI16a, readParam(), 8, 2) // SUB Imm16, Ax
SUB16_FUNC(subI8a, (opcode & 0xFF), 8, 1) // SUB Imm8u, Ax
SUB16_FUNC(subMi16a, core->dsp.readData(readParam()), 8, 2) // SUB MemImm16, Ax
SUB16_FUNC(subMi8a, core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF)), 8, 1) // SUB MemImm8, Ax
SUB16_FUNC(subMrna, core->dsp.readData(getRnStepZids(opcode)), 8, 1) // SUB MemRnStepZids, Ax
SUB16_FUNC(subRega, *readRegP0[opcode & 0x1F], 8, 1) // SUB RegisterP0, Ax
SUB16_FUNC(subR6a, regR[6], 4, 1) // SUB R6, Ax
