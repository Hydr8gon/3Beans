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

uint16_t TeakInterp::calcZmne(int64_t res) {
    // Calculate the common ZMNE flags for an accumulator operation
    res = (res << 24) >> 24;
    bool z = (res == 0);
    bool m = (res >> 39);
    bool e = (res != int32_t(res));
    bool n = (!e && (((res >> 1) ^ res) & BIT(30))) || z;
    return (z << 7) | (m << 6) | (n << 5) | (e << 2);
}

int TeakInterp::clrrA(uint16_t opcode) { // CLRR Ax, Cond
    // Set an A accumulator to 0x8000 and update flags if the condition is met
    if (checkCond(opcode)) {
        (this->*writeAx[(opcode >> 12) & 0x1])(0x8000);
        writeStt0(regStt[0] & ~0xE4); // ZMNE
    }
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

int TeakInterp::orAbaa(uint16_t opcode) { // OR Ab, Ax, Ax
    // Bitwise or an accumulator with an A accumulator and set flags
    int64_t res = *readAb[(opcode >> 10) & 0x3] | *readAx[(opcode >> 6) & 0x1];
    (this->*writeAx[(opcode >> 5) & 0x1])(res);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    return 1;
}

int TeakInterp::orI8a(uint16_t opcode) { // OR Imm8u, Ax
    // Bitwise or an A accumulator with an 8-bit immediate and set flags
    int64_t res = *readAx[(opcode >> 8) & 0x1] | (opcode & 0xFF);
    (this->*writeAx[(opcode >> 8) & 0x1])(res);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(res));
    return 1;
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
