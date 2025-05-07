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

#pragma once

#include <cstdint>

class Core;

class TeakInterp {
public:
    uint64_t cycles = -1;
    uint32_t regPc = 0;

    TeakInterp(Core *core);

    void resetCycles();
    int runOpcode();

private:
    Core *core;

    static int (TeakInterp::*teakInstrs[0x10000])(uint16_t);
    static void (TeakInterp::*writeSttMod[0x8])(uint16_t);

    int64_t regA[2] = {};
    uint16_t regSt[3] = {};
    uint16_t regIcr = 0;
    uint16_t regStt[3] = {};
    uint16_t regMod[4] = { 0x4 };

    uint16_t readParam();
    bool checkCond(uint8_t cond);

    void writeA(bool i, int64_t value);
    void writeStt0(uint16_t value);
    void writeStt1(uint16_t value);
    void writeStt2(uint16_t value);
    void writeMod0(uint16_t value);
    void writeMod1(uint16_t value);
    void writeMod2(uint16_t value);
    void writeMod3(uint16_t value);
    void writeNone(uint16_t value);

    int unkOp(uint16_t opcode);

    int clrrA(uint16_t opcode);
    int cmpuMi8(uint16_t opcode);

    int br(uint16_t opcode);
    int nop(uint16_t opcode);

    int loadPage(uint16_t opcode);
    int movI16sm(uint16_t opcode);
};
