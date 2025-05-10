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

int TeakInterp::addvReg(uint16_t opcode) { // ADDV Imm16, Register
    // Add a 16-bit immediate to a register and set flags
    int32_t res = int16_t(*readRegister[opcode & 0x1F]) + int16_t(readParam());
    bool z = (res == 0);
    bool m = (res & BIT(15));
    bool c = (res & BIT(16));
    writeStt0((regStt[0] & ~0xC8) | (z << 7) | (m << 6) | (c << 3));
    (this->*writeRegister[opcode & 0x1F])(res);
    return 1;
}

int TeakInterp::clrA(uint16_t opcode) { // CLR Ax, Cond
    // Set an A accumulator to zero and update flags if the condition is met
    if (checkCond(opcode)) {
        writeStt0((regStt[0] & ~0xE4) | 0xA0); // ZMNE
        (this->*writeAx[(opcode >> 12) & 0x1])(0);
    }
    return 1;
}

int TeakInterp::clrrA(uint16_t opcode) { // CLRR Ax, Cond
    // Set an A accumulator to 0x8000 and update flags if the condition is met
    if (checkCond(opcode)) {
        writeStt0(regStt[0] & ~0xE4); // ZMNE
        (this->*writeAx[(opcode >> 12) & 0x1])(0x8000);
    }
    return 1;
}

int TeakInterp::cmpuReg(uint16_t opcode) { // CMPU Register, Ax
    // Compare a register with an A accumulator and set flags
    int64_t op1 = regA[(opcode >> 8) & 0x1].v;
    uint16_t op2 = *readRegister[opcode & 0x1F];
    int64_t res = ((op1 - op2) << 24) >> 24;
    bool v = ((op2 ^ op1) & ~(res ^ op2)) >> 39;
    bool c = (op1 >= res);
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
    return 1;
}

int TeakInterp::cmpuMi8(uint16_t opcode) { // CMPU MemImm8, Ax
    // Compare memory addressed by an 8-bit immediate with an A accumulator and set flags
    int64_t op1 = regA[(opcode >> 8) & 0x1].v;
    uint16_t op2 = core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF));
    int64_t res = ((op1 - op2) << 24) >> 24;
    bool v = ((op2 ^ op1) & ~(res ^ op2)) >> 39;
    bool c = (op1 >= res);
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
    return 1;
}

int TeakInterp::dec(uint16_t opcode) { // DEC Ax, Cond
    // Decrement an A accumulator by 1 and set flags if the condition is met
    if (checkCond(opcode)) {
        int64_t op1 = regA[(opcode >> 12) & 0x1].v;
        int64_t res = ((op1 - 1) << 24) >> 24;
        bool v = ((1 ^ op1) & ~(res ^ 1)) >> 39;
        bool c = (op1 >= res);
        writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
        (this->*writeAx[(opcode >> 12) & 0x1])(res);
    }
    return 1;
}

int TeakInterp::orAba(uint16_t opcode) { // OR Ab, Ax, Ax
    // Bitwise or an accumulator with an A accumulator and set flags
    int64_t res = *readAb[(opcode >> 10) & 0x3] | regA[(opcode >> 6) & 0x1].v;
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    (this->*writeAx[(opcode >> 5) & 0x1])(res);
    return 1;
}

int TeakInterp::orI16(uint16_t opcode) { // OR Imm16, Ax
    // Bitwise or an A accumulator with a 16-bit immediate and set flags
    int64_t res = regA[(opcode >> 8) & 0x1].v | readParam();
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    (this->*writeAx[(opcode >> 8) & 0x1])(res);
    return 1;
}

int TeakInterp::orI8(uint16_t opcode) { // OR Imm8u, Ax
    // Bitwise or an A accumulator with an 8-bit immediate and set flags
    int64_t res = regA[(opcode >> 8) & 0x1].v | (opcode & 0xFF);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    (this->*writeAx[(opcode >> 8) & 0x1])(res);
    return 1;
}

int TeakInterp::rstI16sm(uint16_t opcode) { // RST Imm16, SttMod
    // Clear bits in an STT/MOD register using a 16-bit immediate and set flags
    uint16_t res = *readSttMod[opcode & 0x7] & ~readParam();
    bool z = (res == 0);
    bool m = (res & BIT(15));
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6));
    (this->*writeSttMod[opcode & 0x7])(res);
    return 2;
}

int TeakInterp::setI16sm(uint16_t opcode) { // SET Imm16, SttMod
    // Set bits in an STT/MOD register using a 16-bit immediate and set flags
    uint16_t res = *readSttMod[opcode & 0x7] | readParam();
    bool z = (res == 0);
    bool m = (res & BIT(15));
    writeStt0((regStt[0] & ~0xC0) | (z << 7) | (m << 6));
    (this->*writeSttMod[opcode & 0x7])(res);
    return 2;
}

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

int TeakInterp::subRega(uint16_t opcode) { // SUB RegisterP0, Ax
    // Subtract a register from an A accumulator and set flags
    int64_t op1 = regA[(opcode >> 8) & 0x1].v;
    int16_t op2 = *readRegisterP0[opcode & 0x1F];
    int64_t res = ((op1 - op2) << 24) >> 24;
    bool v = ((op2 ^ op1) & ~(res ^ op2)) >> 39;
    bool c = (op1 >= res);
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));
    (this->*writeAx[(opcode >> 8) & 0x1])(res);
    return 1;
}
