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

void (TeakInterp::*TeakInterp::writeRegister[])(uint16_t) = {
    &TeakInterp::writeR<0>, &TeakInterp::writeR<1>, &TeakInterp::writeR<2>, &TeakInterp::writeR<3>,
    &TeakInterp::writeR<4>, &TeakInterp::writeR<5>, &TeakInterp::writeR<7>, &TeakInterp::writeY0,
    &TeakInterp::writeSt0, &TeakInterp::writeSt1, &TeakInterp::writeSt2, &TeakInterp::writeP0h,
    &TeakInterp::writePc, &TeakInterp::writeSp, &TeakInterp::writeCfg<0>, &TeakInterp::writeCfg<1>,
    &TeakInterp::writeBh<0>, &TeakInterp::writeBh<1>, &TeakInterp::writeBl<0>, &TeakInterp::writeBl<1>,
    &TeakInterp::writeExt<0>, &TeakInterp::writeExt<1>, &TeakInterp::writeExt<2>, &TeakInterp::writeExt<3>,
    &TeakInterp::writeA16<0>, &TeakInterp::writeA16<1>, &TeakInterp::writeAl<0>, &TeakInterp::writeAl<1>,
    &TeakInterp::writeAh<0>, &TeakInterp::writeAh<1>, &TeakInterp::writeLc, &TeakInterp::writeSv
};

void (TeakInterp::*TeakInterp::writeArArp[])(uint16_t) = {
    &TeakInterp::writeAr<0>, &TeakInterp::writeAr<1>, &TeakInterp::writeArp<0>, &TeakInterp::writeArp<1>,
    &TeakInterp::writeArp<2>, &TeakInterp::writeArp<3>, &TeakInterp::writeNone, &TeakInterp::writeNone
};

void (TeakInterp::*TeakInterp::writeSttMod[])(uint16_t) = {
    &TeakInterp::writeStt0, &TeakInterp::writeStt1, &TeakInterp::writeStt2, &TeakInterp::writeNone,
    &TeakInterp::writeMod0, &TeakInterp::writeMod1, &TeakInterp::writeMod2, &TeakInterp::writeMod3
};

void (TeakInterp::*TeakInterp::writeAb[])(int64_t) = {
    &TeakInterp::writeB40<0>, &TeakInterp::writeB40<1>, &TeakInterp::writeA40<0>, &TeakInterp::writeA40<1>
};

void (TeakInterp::**TeakInterp::writeAx)(int64_t) = &writeAb[2];
void (TeakInterp::**TeakInterp::writeBx)(int64_t) = &writeAb[0];

TeakInterp::TeakInterp(Core *core): core(core) {
    // Initialize the lookup table if it hasn't been done
    if (!teakInstrs[0])
        initLookup();
}

void TeakInterp::resetCycles() {
    // Adjust CPU cycles for a global cycle reset
    if (cycles != -1)
        cycles -= std::min(core->globalCycles, cycles);
}

void TeakInterp::incrementPc() {
    // Increment the program counter and check if it's at the end of a loop
    regPc = (regPc + 1) & 0x3FFFF;
    if (~regStt[2] & BIT(15)) return;
    uint8_t count = ((regStt[2] >> 12) & 0x7) - 1;
    if (regPc != bkEnd[count]) return;

    // Decrement the loop counter and jump to the loop start if non-zero
    if (regLc-- > 0) {
        regPc = bkStart[count];
        return;
    }

    // Finish a loop and return to the previous one if nested
    if (count > 0) { // Nested
        regLc = bkStack[count];
        regStt[2] -= BIT(12);
        regIcr -= BIT(5);
    }
    else { // Finished
        regLc = 0;
        regStt[2] &= ~0xF000;
        regIcr &= ~0xF0;
    }
}

int TeakInterp::runOpcode() {
    // Reset the program counter and decrement the repeat counter if non-zero
    if (repCount) {
        regPc = repAddr;
        repCount--;
    }

    // Look up an instruction to execute and increment the program counter
    uint16_t opcode = core->memory.read<uint16_t>(ARM11, 0x1FF00000 + (regPc << 1));
    incrementPc();
    return (this->*teakInstrs[opcode])(opcode);
}

uint16_t TeakInterp::readParam() {
    // Read an additional parameter word and increment the program counter
    uint16_t param = core->memory.read<uint16_t>(ARM11, 0x1FF00000 + (regPc << 1));
    incrementPc();
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

uint16_t TeakInterp::calcZmne(int64_t res) {
    // Calculate the common ZMNE flags for an accumulator operation
    res = (res << 24) >> 24;
    bool z = (res == 0);
    bool m = (res >> 39);
    bool e = (res != int32_t(res));
    bool n = (!e && (((res >> 1) ^ res) & BIT(30))) || z;
    return (z << 7) | (m << 6) | (n << 5) | (e << 2);
}

uint16_t TeakInterp::getRnStepZids(uint8_t rnStep) {
    // Get an address register value and post-adjust with a ZIDS step
    uint16_t &reg = regR[rnStep & 0x7];
    switch ((rnStep >> 3) & 0x3) {
        case 0: return reg; // Zero
        case 1: return reg++; // Increment
        case 2: return reg--; // Decrement

    default: // Step
        LOG_CRIT("Unhandled Teak DSP step mode used\n");
        return reg;
    }
}

template <int i> void TeakInterp::writeA40(int64_t value) {
    // Write a 40-bit value to one of the A accumulators and mirror extension bits to ST
    regA[i].v = (value << 24) >> 24;
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
}

template <int i> void TeakInterp::writeA16(uint16_t value) {
    // Write a 16-bit value to one of the A accumulators and mirror extension bits to ST
    regA[i].v = int16_t(value);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
}

template <int i> void TeakInterp::writeB40(int64_t value) {
    // Write a 40-bit value to one of the B accumulators
    regB[i].v = (value << 24) >> 24;
}

template <int i> void TeakInterp::writeArp(uint16_t value) {
    // Write to one of the ARP registers
    regArp[i] = (regArp[i] & ~0x6FFF) | (value & 0x6FFF);
}

void TeakInterp::writeP0h(uint16_t value) {
    // Write to the upper word of the P0 register and sign-extend the 33rd bit in STT1
    regP[0].h = value;
    regP[0].v = int32_t(regP[0].v);
    regStt[1] = (regStt[1] & ~0x4000) | ((value >> 1) & 0x4000);
}

void TeakInterp::writeSt0(uint16_t value) {
    // Write to the ST0 register and mirror bits to new registers and A0
    regSt[0] = value;
    regStt[0] = (regStt[0] & ~0xFF) | ((regSt[0] >> 4) & 0xFE) | ((regSt[0] >> 5) & 0x1);
    regStt[1] = (regStt[1] & ~0x10) | (regSt[0] & 0x10);
    regMod[0] = (regMod[0] & ~0x1) | (regSt[0] & 0x1);
    regMod[3] = (regMod[3] & ~0x380) | ((regSt[0] << 6) & 0x380);
    regA[0].e = int16_t(regSt[0]) >> 12;
}

void TeakInterp::writeSt1(uint16_t value) {
    // Write to the ST1 register and mirror bits to new registers and A1
    regSt[1] = (regSt[1] & ~0xFCFF) | (value & 0xFCFF);
    regMod[0] = (regMod[0] & ~0xC00) | (regSt[1] & 0xC00);
    regMod[1] = (regMod[1] & ~0xFF) | (regSt[1] & 0xFF);
    regA[1].e = int16_t(regSt[1]) >> 12;
}

void TeakInterp::writeSt2(uint16_t value) {
    // Write to the ST2 register and mirror bits to new registers
    regSt[2] = (regSt[2] & ~0x3FF) | (value & 0x3FF);
    regMod[0] = (regMod[0] & ~0x380) | (regSt[2] & 0x380);
    regMod[2] = (regMod[2] & ~0x3F) | (regSt[2] & 0x3F);
    regMod[3] = (regMod[3] & ~0x400) | ((regSt[2] << 4) & 0x400);
}

void TeakInterp::writeStt0(uint16_t value) {
    // Write to the STT0 register and mirror status flags to ST0
    regStt[0] = (regStt[0] & ~0x8FF) | (value & 0x8FF);
    regSt[0] = (regSt[0] & ~0xFE0) | ((regStt[0] << 5) & 0x20) | ((regStt[0] << 4) & 0xFE0);
}

void TeakInterp::writeStt1(uint16_t value) {
    // Write to the STT1 register and mirror the R flag and 33rd P bits
    regStt[1] = (regStt[1] & ~0xC010) | (value & 0xC010);
    regSt[0] = (regSt[0] & ~0x10) | (regStt[1] & 0x10);
    regP[0].e = int16_t(regStt[1] << 1) >> 15;
    regP[1].e = int16_t(regStt[1] << 0) >> 15;
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

// Define write functions for registers with no special cases
#define WRITE_FUNC(id, reg) id(uint16_t value) { reg = value; }
WRITE_FUNC(template <int i> void TeakInterp::writeAl, regA[i].l)
WRITE_FUNC(template <int i> void TeakInterp::writeAh, regA[i].h)
WRITE_FUNC(template <int i> void TeakInterp::writeBl, regB[i].l)
WRITE_FUNC(template <int i> void TeakInterp::writeBh, regB[i].h)
WRITE_FUNC(template <int i> void TeakInterp::writeR, regR[i])
WRITE_FUNC(template <int i> void TeakInterp::writeExt, regExt[i])
WRITE_FUNC(template <int i> void TeakInterp::writeCfg, regCfg[i])
WRITE_FUNC(template <int i> void TeakInterp::writeAr, regAr[i])
WRITE_FUNC(void TeakInterp::writeY0, regY[0])
WRITE_FUNC(void TeakInterp::writePc, regPc)
WRITE_FUNC(void TeakInterp::writeSp, regSp)
WRITE_FUNC(void TeakInterp::writeSv, regSv)
WRITE_FUNC(void TeakInterp::writeLc, regLc)

int TeakInterp::unkOp(uint16_t opcode) {
    // Handle an unknown Teak DSP opcode
    LOG_CRIT("Unknown Teak DSP opcode: 0x%X\n", opcode);
    return 1;
}
