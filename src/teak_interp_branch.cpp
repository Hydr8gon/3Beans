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

int TeakInterp::br(uint16_t opcode) { // BR Address18, Cond
    // Branch to an 18-bit address if the condition is met
    uint16_t param = readParam();
    if (checkCond(opcode))
        regPc = ((opcode << 12) & 0x30000) | param;
    return 2;
}

int TeakInterp::nop(uint16_t opcode) { // NOP
    // Do nothing
    return 1;
}
