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

int TeakInterp::loadPage(uint16_t opcode) { // LOAD Imm8u, PAGE
    // Set the data page to an 8-bit immediate
    writeMod1((regMod[1] & 0xFF00) | (opcode & 0xFF));
    return 1;
}

int TeakInterp::movAblhi8(uint16_t opcode) { // MOV Ablh, MemImm8
    // Move part of an accumulator to memory addressed by an 8-bit immediate
    core->dsp.writeData((regMod[1] << 8) | (opcode & 0xFF), *readAblh[(opcode >> 9) & 0x7]);
    return 1;
}

int TeakInterp::movI16arap(uint16_t opcode) { // MOV Imm16, ArArp
    // Move a 16-bit immediate to an AR/ARP register
    (this->*writeArArp[opcode & 0x7])(readParam());
    return 2;
}

int TeakInterp::movI16reg(uint16_t opcode) { // MOV Imm16, Register
    // Move a 16-bit immediate to a register
    (this->*writeRegister[opcode & 0x1F])(readParam());
    return 2;
}

int TeakInterp::movI16sm(uint16_t opcode) { // MOV Imm16, SttMod
    // Move a 16-bit immediate to an STT/MOD register
    (this->*writeSttMod[opcode & 0x7])(readParam());
    return 2;
}

int TeakInterp::movI16stp(uint16_t opcode) { // MOV Imm16, Step
    // Move a 16-bit immediate to a step register
    regStep[(opcode >> 3) & 0x1] = readParam();
    return 2;
}

int TeakInterp::movRegreg(uint16_t opcode) { // MOV RegisterP0, Register
    // Move a register value to another register
    (this->*writeRegister[(opcode >> 5) & 0x1F])(*readRegisterP0[opcode & 0x1F]);
    return 1;
}

int TeakInterp::popReg(uint16_t opcode) { // POP Register
    // Pop a register from the stack
    (this->*writeRegister[opcode & 0x1F])(core->dsp.readData(regSp++));
    return 1;
}

int TeakInterp::pushReg(uint16_t opcode) { // PUSH Register
    // Push a register to the stack
    core->dsp.writeData(--regSp, *readRegister[opcode & 0x1F]);
    return 1;
}
