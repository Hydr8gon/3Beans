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

int TeakInterp::clrrA(uint16_t opcode) { // CLRR Ax, Cond
    // Set an A register to 0x8000 and update flags if the condition is met
    if (checkCond(opcode)) {
        writeA(opcode & BIT(12), 0x8000);
        writeStt0(regStt[0] & ~0xE4); // ZMNE
    }
    return 1;
}

int TeakInterp::cmpuMi8(uint16_t opcode) { // CMPU MemImm8, Ax
    // Compare memory addressed by an 8-bit immediate with an A register and set flags
    int64_t op1 = regA[(opcode >> 8) & 0x1];
    uint16_t op2 = core->dsp.readData((regMod[1] << 8) | (opcode & 0xFF));
    int64_t res = ((op1 - op2) << 24) >> 24;
    bool z = (res == 0);
    bool m = (res >> 39);
    bool e = (res != int32_t(res));
    bool n = (!e && (((res >> 1) ^ res) & BIT(30))) || z;
    bool v = ((op2 ^ op1) & ~(res ^ op2)) >> 39;
    bool c = (op1 >= res);
    writeStt0((regStt[0] & ~0xFC) | (z << 7) | (m << 6) | (n << 5) | (v << 4) | (c << 3) | (e << 2) | (v << 1));
    return 1;
}
