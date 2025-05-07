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

int (TeakInterp::*TeakInterp::teakInstrs[])(uint16_t) = {};

void (TeakInterp::*TeakInterp::writeSttMod[0x8])(uint16_t) = {
    &TeakInterp::writeStt0, &TeakInterp::writeStt1, &TeakInterp::writeStt2, &TeakInterp::writeNone,
    &TeakInterp::writeMod0, &TeakInterp::writeMod1, &TeakInterp::writeMod2, &TeakInterp::writeMod3
};

TeakInterp::TeakInterp(Core *core): core(core) {
    // Initialize the instruction lookup table if it hasn't been already
    if (teakInstrs[0]) return;
    for (uint32_t op = 0; op < 0x10000; op++) {
        if ((op & 0xFFC0) == 0x4180)
            teakInstrs[op] = &TeakInterp::br;
        else if ((op & 0xEFF0) == 0x67C0)
            teakInstrs[op] = &TeakInterp::clrrA;
        else if ((op & 0xFE00) == 0xBE00)
            teakInstrs[op] = &TeakInterp::cmpuMi8;
        else if ((op & 0xFF00) == 0x0400)
            teakInstrs[op] = &TeakInterp::loadPage;
        else if ((op & 0xFFF8) == 0x0030)
            teakInstrs[op] = &TeakInterp::movI16sm;
        else if (op == 0x0000)
            teakInstrs[op] = &TeakInterp::nop;
        else
            teakInstrs[op] = &TeakInterp::unkOp;
    }
}

void TeakInterp::resetCycles() {
    // Adjust CPU cycles for a global cycle reset
    if (cycles != -1)
        cycles -= std::min(core->globalCycles, cycles);
}

int TeakInterp::runOpcode() {
    // Look up an instruction to execute and increment the program counter
    uint16_t opcode = core->memory.read<uint16_t>(ARM11, 0x1FF00000 + (regPc << 1));
    regPc = (regPc + 1) & 0x3FFFF;
    return (this->*teakInstrs[opcode])(opcode);
}

uint16_t TeakInterp::readParam() {
    // Read an additional parameter word and increment the program counter
    uint16_t param = core->memory.read<uint16_t>(ARM11, 0x1FF00000 + (regPc << 1));
    regPc = (regPc + 1) & 0x3FFFF;
    return param;
}

bool TeakInterp::checkCond(uint8_t cond) {
    // Evaluate a condition code based on status flags
    switch (cond & 0xF) {
        case 0x0: return true; // TRUE
        case 0x1: return (regSt[0] & BIT(11)); // EQ
        case 0x2: return !(regSt[0] & BIT(11)); // NEQ
        case 0x3: return !(regSt[0] & (BIT(10) | BIT(11))); // GT
        case 0x4: return !(regSt[0] & BIT(10)); // GE
        case 0x5: return (regSt[0] & BIT(10)); // LT
        case 0x6: return (regSt[0] & (BIT(10) | BIT(11))); // LE
        case 0x7: return !(regSt[0] & BIT(9)); // NN
        case 0x8: return (regSt[0] & BIT(7)); // C
        case 0x9: return (regSt[0] & BIT(8)); // V
        case 0xA: return (regSt[0] & BIT(6)); // E
        case 0xB: return (regSt[0] & BIT(5)); // L
        case 0xC: return !(regSt[0] & BIT(4)); // NR
        case 0xD: return !(regSt[2] & BIT(10)); // NIU0
        case 0xE: return (regSt[2] & BIT(10)); // IU0
        default: return (regSt[2] & BIT(11)); // IU1
    }
}

void TeakInterp::writeA(bool i, int64_t value) {
    // Write to one of the 40-bit A registers and mirror extension bits to ST
    regA[i] = (value << 24) >> 24;
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i] >> 20) & 0xF000);
}

void TeakInterp::writeStt0(uint16_t value) {
    // Write to the STT0 register and mirror status flags to ST0
    regStt[0] = (regStt[0] & ~0x8FF) | (value & 0x8FF);
    regSt[0] = (regSt[0] & ~0xFE0) | ((regStt[0] << 5) & 0x20) | ((regStt[0] << 4) & 0xFE0);
}

void TeakInterp::writeStt1(uint16_t value) {
    // Write to the STT1 register and mirror the R flag to ST0
    regStt[1] = (regStt[1] & ~0xC010) | (value & 0xC010);
    regSt[0] = (regSt[0] & ~0x10) | (regStt[1] & 0x10);
}

void TeakInterp::writeStt2(uint16_t value) {
    // Write to the STT2 register
    regStt[2] = (regStt[2] & ~0xC0) | (value & 0xC0);
}

void TeakInterp::writeMod0(uint16_t value) {
    // Write to the MOD0 register and mirror various bits to ST0-2
    regMod[0] = (regMod[0] & ~0x6FE3) | (value & 0x6FE3);
    regSt[0] = (regSt[0] & ~0x1) | (regMod[0] & 0x1);
    regSt[1] = (regSt[1] & ~0xC00) | (regMod[0] & 0xC00);
    regSt[2] = (regSt[2] & ~0x380) | (regMod[0] & 0x380);
}

void TeakInterp::writeMod1(uint16_t value) {
    // Write to the MOD1 register and mirror the data page to ST1
    regMod[1] = (regMod[1] & ~0xF0FF) | (value & 0xF0FF);
    regSt[1] = (regSt[1] & ~0xFF) | (regMod[1] & 0xFF);
}

void TeakInterp::writeMod2(uint16_t value) {
    // Write to the MOD2 register and mirror enable bits to ST2
    regMod[2] = value;
    regSt[2] = (regSt[2] & ~0x3F) | (regMod[2] & 0x3F);
}

void TeakInterp::writeMod3(uint16_t value) {
    // Write to the MOD3 register and mirror bits to old registers
    regMod[3] = (regMod[3] & ~0xEFBF) | (value & 0xEFBF);
    regSt[0] = (regSt[0] & ~0xE) | ((regMod[3] >> 6) & 0xE);
    regSt[2] = (regSt[2] & ~0x40) | ((regMod[3] >> 4) & 0x40);
    regIcr = (regIcr & ~0xF) | (regMod[3] & 0xF);
}

void TeakInterp::writeNone(uint16_t value) {
    // Ignore writes to reserved registers
}

int TeakInterp::unkOp(uint16_t opcode) {
    // Handle an unknown Teak DSP opcode
    LOG_CRIT("Unknown Teak DSP opcode: 0x%X\n", opcode);
    regPc = (regPc - 1) & 0x3FFFF;
    return 1;
}
