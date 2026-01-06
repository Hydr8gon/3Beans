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

template void TeakInterp::multiplyXY<int16_t, int16_t>(int);
template void TeakInterp::multiplyXY<int16_t, uint16_t>(int);
template void TeakInterp::multiplyXY<uint16_t, int16_t>(int);
template void TeakInterp::multiplyXY<uint16_t, uint16_t>(int);

TEMPLATE2(int64_t TeakInterp::readA40S, 0)
TEMPLATE2(uint16_t TeakInterp::readAl, 0)
TEMPLATE2(uint16_t TeakInterp::readAlS, 0)
TEMPLATE2(uint16_t TeakInterp::readAh, 0)
TEMPLATE2(uint16_t TeakInterp::readAhS, 0)
TEMPLATE2(int64_t TeakInterp::readB40S, 0)
TEMPLATE2(uint16_t TeakInterp::readBl, 0)
TEMPLATE2(uint16_t TeakInterp::readBlS, 0)
TEMPLATE2(uint16_t TeakInterp::readBh, 0)
TEMPLATE2(uint16_t TeakInterp::readBhS, 0)
TEMPLATE4(uint16_t TeakInterp::readR, 0)
TEMPLATE4(uint16_t TeakInterp::readR, 4)
TEMPLATE4(uint16_t TeakInterp::readExt, 0)
TEMPLATE3(uint16_t TeakInterp::readSt, 0)
TEMPLATE2(uint16_t TeakInterp::readCfg, 0)
TEMPLATE2(void TeakInterp::writeA40, 0, int64_t)
TEMPLATE2(void TeakInterp::writeA40S, 0, int64_t)
TEMPLATE2(void TeakInterp::writeA40M, 0, int64_t)
TEMPLATE2(void TeakInterp::writeA16, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeA16M, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeAl, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeAlM, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeAh, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeAhM, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeB40, 0, int64_t)
TEMPLATE2(void TeakInterp::writeB40S, 0, int64_t)
TEMPLATE2(void TeakInterp::writeB40M, 0, int64_t)
TEMPLATE2(void TeakInterp::writeB16M, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeBl, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeBlM, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeBh, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeBhM, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeP33, 0, int64_t)
TEMPLATE4(void TeakInterp::writeR, 0, uint16_t)
TEMPLATE4(void TeakInterp::writeR, 4, uint16_t)
TEMPLATE4(void TeakInterp::writeExt, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeCfg, 0, uint16_t)
TEMPLATE2(void TeakInterp::writeAr, 0, uint16_t)
TEMPLATE4(void TeakInterp::writeArp, 0, uint16_t)

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

void TeakInterp::stopCycles() {
    // Halt by setting the next cycle to an unreachable value
    cycles = -1;
    halted = true;
}

void TeakInterp::incrementPc() {
    // Increment the program counter and check if it's at the end of a loop
    regPc = (regPc + 1) & 0x1FFFF;
    if (~regStt[2] & BIT(15)) return;
    uint8_t count = ((regStt[2] >> 12) & 0x7) - 1;
    if (regPc != bkEnd[count]) return;

    // Decrement the loop counter and restart or finish the loop
    if (regLc > 0) {
        regPc = bkStart[count];
        regLc--;
    }
    else {
        bkrepPop(count);
    }
}

int TeakInterp::runOpcode() {
    // Reset the program counter and decrement the repeat counter if repeating
    if (repAddr != -1) {
        regPc = repAddr;
        if (regRepc-- == 0) {
            regRepc = 0;
            repAddr = -1;
        }
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

void TeakInterp::setPendingIrqs(uint8_t mask) {
    // Set interrupt pending bits in the status registers
    regStt[2] |= (mask & 0xF);
    regSt[2] |= ((mask & 0x4) << 11) | ((mask & 0x3) << 14);

    // Schedule an interrupt if one is enabled and pending
    if (!(regMod[3] & BIT(7)) || scheduled) return;
    for (int i = 0; i < 4; i++) {
        if (!(regStt[2] & (regMod[3] >> 8) & BIT(i))) continue;
        core->schedule(Task(TEAK_INTERRUPT0 + i), 2);
        scheduled = true;
        return;
    }
}

void TeakInterp::interrupt(int i) {
    // Ensure the interrupt condition still holds
    scheduled = false;
    if (!(regMod[3] & BIT(7)) || !(regStt[2] & (regMod[3] >> 8) & BIT(i))) return;

    // Resume execution after an idle loop
    if (cycles == -1 && halted) {
        cycles = 0;
        halted = false;
    }

    // Run until not repeating an opcode to avoid dealing with it
    while (repAddr != -1)
        cycles += (runOpcode() << 1);

    // Push PC to the stack and clear the IE and IPx bits
    core->dsp.writeData(--regSp, regPc >> ((regMod[3] & BIT(14)) ? 16 : 0));
    core->dsp.writeData(--regSp, regPc >> ((regMod[3] & BIT(14)) ? 0 : 16));
    regSt[0] &= ~BIT(1);
    regMod[3] &= ~BIT(7);
    regStt[2] &= ~BIT(i);

    // Clear the IPx mirror and jump to an interrupt handler with optional context save
    if (i < 3) {
        regSt[2] &= ~BIT((i == 2) ? 13 : (14 + i));
        if (regMod[3] & BIT(1 + i)) cntxS(0);
        regPc = 0x6 + i * 8;
        return;
    }

    // For vector interrupts, jump to a vector handler with optional context save
    uint32_t vector = core->dsp.getIcuVector();
    if (vector & BIT(31)) cntxS(0);
    regPc = vector & 0x1FFFF;
}

void TeakInterp::bkrepPop(uint8_t count) {
    // Finish a BKREP loop and return to the previous one if nested
    if (count > 0) { // Nested
        regLc = bkStack[count];
        regStt[2] -= BIT(12);
        regIcr -= BIT(5);
    }
    else { // Finished
        regStt[2] &= ~0xF000;
        regIcr &= ~0xF0;
    }
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

int64_t TeakInterp::saturate(int64_t value) {
    // Clamp an accumulator value within a signed 32-bit range
    if (value > 0x7FFFFFFFLL)
        value = 0x7FFFFFFFLL;
    else if (value < -0x80000000LL)
        value = -0x80000000LL;
    else
        return value;

    // Set the limit flag if saturation occurred
    regStt[0] |= BIT(0);
    regSt[0] |= BIT(5);
    return value;
}

int64_t TeakInterp::shift(int64_t value, int16_t amount) {
    // Limit the shift amount to well-defined values
    amount = std::min<int16_t>(63, std::max<int16_t>(-63, amount));
    int64_t res;

    // If enabled, logically shift a 40-bit value left or right and set flags
    if (regMod[0] & BIT(7)) {
        res = (amount > 0) ? ((value & 0xFFFFFFFFFF) << amount) : ((value & 0xFFFFFFFFFF) >> -amount);
        bool c = (amount > 0 && (res & BITL(40))) || (amount < 0 && (value & 0xFFFFFFFFFF & BITL(-1 - amount)));
        writeStt0((regStt[0] & ~0xEC) | calcZmne(res) | (c << 3));
        return res;
    }

    // Otherwise, arithmetically shift the value left or right and set flags
    res = (amount > 0) ? (value << amount) : (value >> -amount);
    bool v = amount > 0 && (((res << 24) >> (24 + amount)) != value);
    bool c = (amount > 0 && (res & BITL(40))) || (amount < 0 && (value & BITL(-1 - amount)));
    writeStt0((regStt[0] & ~0xFC) | calcZmne(res) | (v << 4) | (c << 3) | (v << 1));

    // Perform special saturation using shift details and set the limit flag
    if ((regMod[0] & BIT(1)) || (!v && res == int32_t(res))) return res;
    regStt[0] |= BIT(0);
    regSt[0] |= BIT(5);
    return (value < 0) ? -0x80000000LL : 0x7FFFFFFFLL;
}

template <typename Tx, typename Ty> void TeakInterp::multiplyXY(int i) {
    // Get the X and Y values to multiply, with Y modified based on HWM bits
    Tx valX = regX[i]; Ty valY;
    switch ((regMod[0] >> 5) & 0x3) {
        case 0x0: valY = regY[i]; break;
        case 0x1: valY = (regY[i] >> 8); break;
        case 0x2: valY = (regY[i] & 0xFF); break;
        default: valY = i ? (regY[i] & 0xFF) : (regY[i] >> 8); break;
    }

    // Multiply and store the result in a product register
    (this->*writePx33[i])(valX * valY);
}

void TeakInterp::updateShiftP(int i) {
    // Update the shifted copy of a product register
    switch ((regMod[0] >> (10 + i * 3)) & 0x3) { // Shifter
        case 0x0: shiftP[i].v = regP[i].v >> 0; return;
        case 0x1: shiftP[i].v = regP[i].v >> 1; return;
        case 0x2: shiftP[i].v = regP[i].v << 1; return;
        default: shiftP[i].v = regP[i].v << 2; return;
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

uint16_t TeakInterp::revBits(uint16_t value) {
    // Reverse the bits of a 16-bit value using a partial lookup table
    static uint8_t r[0x10] = { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF };
    return (r[value & 0xF] << 12) | (r[(value >> 4) & 0xF] << 8) | (r[(value >> 8) & 0xF] << 4) | r[value >> 12];
}

uint16_t TeakInterp::offsReg(uint8_t reg, int8_t offs) {
    // Apply an offset to an address register without modulo, bit-reversed if enabled
    bool md = (regMod[2] & BIT(reg));
    bool br = (regMod[2] & BIT(reg + 8));
    if (!md || br || dmod)
        return ((br && !md) ? revBits(regR[reg]) : regR[reg]) + offs;

    // Apply an offset to an address register with modulo, wrapping at bounds
    int i = (reg >> 2);
    if (offs > 0 && (regR[reg] & modMasks[i]) == (regCfg[i] >> 7))
        return (regR[reg] & ~modMasks[i]) | 0;
    else if (offs < 0 && (regR[reg] & modMasks[i]) == 0)
        return (regR[reg] & ~modMasks[i]) | (regCfg[i] >> 7);
    return regR[reg] + offs;
}

uint16_t TeakInterp::stepReg(uint8_t reg, int32_t step) {
    // Get the current address to return, bit-reversed if enabled
    bool md = (regMod[2] & BIT(reg));
    bool br = (regMod[2] & BIT(reg + 8));
    uint16_t address = (br && !md) ? revBits(regR[reg]) : regR[reg];

    // Clear R3/R7 instead of single-stepping them if enabled
    int i = (reg >> 2);
    if ((reg & 0x3) == 3 && (regMod[1] & BIT(14 + i)) && abs(step) != 2) {
        regR[reg] = 0;
        return address;
    }

    // Get the actual value to step with based on configuration
    int32_t value;
    if (step != STEP_S) {
        // Use a constant step value directly
        value = step;
    }
    else if ((regMod[1] & 0x3000) == 0x1000 || (br && !md)) {
        // Use a 16-bit step register, truncated to 9-bit with modulo
        value = md ? (int16_t(regStep0[i] << 7) >> 7) : regStep0[i];
    }
    else {
        // Use a 7-bit step register in TL1 mode or when 16-bit is disabled
        value = int8_t(regCfg[i] << 1) >> 1;
    }

    // Apply the step to the address register without modulo
    if (!md || br || dmod) {
        regR[reg] += value;
        return address;
    }

    // Break the step into two parts if it's a double constant
    int count = 1;
    if (abs(step) == 2)
        value >>= 1, count++;

    // Apply the step to the address register with modulo, wrapping at bounds
    // TODO: handle TL1/TL2 modes properly
    while (count--) {
        if (step > 0 && (regR[reg] & modMasks[i]) == (regCfg[i] >> 7))
            regR[reg] = (regR[reg] & ~modMasks[i]) | 0;
        else if (step < 0 && (regR[reg] & modMasks[i]) == 0)
            regR[reg] = (regR[reg] & ~modMasks[i]) | (regCfg[i] >> 7);
        else regR[reg] += value;
    }
    return address;
}

int64_t TeakInterp::readRegP0(int i, bool sign) {
    // Read a register operand with P0, substituting full-width values when applicable
    switch (i) {
        case 0x0B: return shiftP[0].v;
        case 0x18: return regA[0].v;
        case 0x19: return regA[1].v;
        default: return sign ? int16_t(*readReg[i]) : uint16_t(*readReg[i]);
    }
}

int64_t TeakInterp::readRegP0S(int i) {
    // Read a register operand with P0 and saturation, substituting full-width values when applicable
    switch (i) {
        case 0x0B: return shiftP[0].v;
        case 0x18: return readA40S<0>();
        case 0x19: return readA40S<1>();
        default: return int16_t((this->*readRegS[i])());
    }
}

int64_t TeakInterp::readRegExp(int i) {
    // Read a register operand for EXP opcodes, converting P0 and 16-bit values to signed 32-bit
    switch (i) {
        case 0x0B: return int32_t(shiftP[0].v);
        case 0x18: return regA[0].v;
        case 0x19: return regA[1].v;
        default: return int32_t(*readReg[i] << 16);
    }
}

template <int i> int64_t TeakInterp::readA40S() {
    // Read from a full A accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regA[i].v : saturate(regA[i].v);
}

template <int i> uint16_t TeakInterp::readAlS() {
    // Read from the low part of an A accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regA[i].l : saturate(regA[i].v);
}

template <int i> uint16_t TeakInterp::readAhS() {
    // Read from the high part of an A accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regA[i].h : (saturate(regA[i].v) >> 16);
}

template <int i> int64_t TeakInterp::readB40S() {
    // Read from a full B accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regB[i].v : saturate(regB[i].v);
}

template <int i> uint16_t TeakInterp::readBlS() {
    // Read from the low part of a B accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regB[i].l : saturate(regB[i].v);
}

template <int i> uint16_t TeakInterp::readBhS() {
    // Read from the high part of a B accumulator and saturate if enabled
    return (regMod[0] & BIT(0)) ? regB[i].h : (saturate(regB[i].v) >> 16);
}

// Define read functions for registers with no special cases
#define READ_FUNC(id, reg) id() { return reg; }
READ_FUNC(template <int i> uint16_t TeakInterp::readAl, regA[i].l)
READ_FUNC(template <int i> uint16_t TeakInterp::readAh, regA[i].h)
READ_FUNC(template <int i> uint16_t TeakInterp::readBl, regB[i].l)
READ_FUNC(template <int i> uint16_t TeakInterp::readBh, regB[i].h)
READ_FUNC(template <int i> uint16_t TeakInterp::readR, regR[i])
READ_FUNC(template <int i> uint16_t TeakInterp::readExt, regExt[i])
READ_FUNC(template <int i> uint16_t TeakInterp::readSt, regSt[i])
READ_FUNC(template <int i> uint16_t TeakInterp::readCfg, regCfg[i])
READ_FUNC(uint16_t TeakInterp::readP0h, shiftP[0].h)
READ_FUNC(uint16_t TeakInterp::readY0, regY[0])
READ_FUNC(uint16_t TeakInterp::readPc, regPc)
READ_FUNC(uint16_t TeakInterp::readSp, regSp)
READ_FUNC(uint16_t TeakInterp::readSv, regSv)
READ_FUNC(uint16_t TeakInterp::readLc, regLc)

template <int i> void TeakInterp::writeA40(int64_t value) {
    // Write a 40-bit value to an A accumulator and mirror extension bits to ST
    regA[i].v = (value << 24) >> 24;
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
}

template <int i> void TeakInterp::writeA40S(int64_t value) {
    // Write a 40-bit value to an A accumulator, saturate if enabled, and mirror extension bits to ST
    regA[i].v = (regMod[0] & BIT(1)) ? ((value << 24) >> 24) : saturate((value << 24) >> 24);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
}

template <int i> void TeakInterp::writeA40M(int64_t value) {
    // Write a 40-bit value to an A accumulator, saturate if enabled, mirror extension bits, and set flags for MOV
    regA[i].v = (regMod[0] & BIT(1)) ? ((value << 24) >> 24) : saturate((value << 24) >> 24);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(value));
}

template <int i> void TeakInterp::writeA16(uint16_t value) {
    // Write a 16-bit value to an A accumulator and mirror extension bits to ST
    regA[i].v = int16_t(value);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
}

template <int i> void TeakInterp::writeA16M(uint16_t value) {
    // Write a 16-bit value to an A accumulator, mirror extension bits, and set flags for MOV
    regA[i].v = int16_t(value);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regA[i].v));
}

template <int i> void TeakInterp::writeAlM(uint16_t value) {
    // Write a 16-bit value to the low part of an A accumulator, clear high bits, and set flags for MOV
    regA[i].v = value;
    regSt[i] &= ~0xF000;
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regA[i].v));
}

template <int i> void TeakInterp::writeAhM(uint16_t value) {
    // Write a 16-bit value to the high part of an A accumulator, sign-extend, clear low bits, and set flags for MOV
    regA[i].v = int32_t(value << 16);
    regSt[i] = (regSt[i] & ~0xF000) | ((regA[i].v >> 20) & 0xF000);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regA[i].v));
}

template <int i> void TeakInterp::writeB40(int64_t value) {
    // Write a 40-bit value to a B accumulator
    regB[i].v = (value << 24) >> 24;
}

template <int i> void TeakInterp::writeB40S(int64_t value) {
    // Write a 40-bit value to a B accumulator and saturate if enabled
    regB[i].v = (regMod[0] & BIT(1)) ? ((value << 24) >> 24) : saturate((value << 24) >> 24);
}

template <int i> void TeakInterp::writeB40M(int64_t value) {
    // Write a 40-bit value to a B accumulator, saturate if enabled, and set flags for MOV
    regB[i].v = (regMod[0] & BIT(1)) ? ((value << 24) >> 24) : saturate((value << 24) >> 24);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(value));
}

template <int i> void TeakInterp::writeB16M(uint16_t value) {
    // Write a 16-bit value to a B accumulator and set flags for MOV
    regB[i].v = int16_t(value);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regB[i].v));
}

template <int i> void TeakInterp::writeBlM(uint16_t value) {
    // Write a 16-bit value to the low part of a B accumulator, clear high bits, and set flags for MOV
    regB[i].v = value;
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regB[i].v));
}

template <int i> void TeakInterp::writeBhM(uint16_t value) {
    // Write a 16-bit value to the high part of a B accumulator, sign-extend, clear low bits, and set flags for MOV
    regB[i].v = int32_t(value << 16);
    writeStt0((regStt[0] & ~0xE4) | calcZmne(regB[i].v));
}

template <int i> void TeakInterp::writeP33(int64_t value) {
    // Write a 33-bit value to one of the P registers and mirror the 33rd bit to STT1
    regP[i].v = ((value << 31) >> 31);
    regStt[1] = (regStt[1] & ~BIT(14 + i)) | ((regP[i].v >> (18 - i)) & BIT(14 + i));
    updateShiftP(i);
}

template <int i> void TeakInterp::writeCfg(uint16_t value) {
    // Write to one of the config registers
    regCfg[i] = value;

    // Calculate a modulo mask based on the base modulo value
    modMasks[i] = 2;
    while (modMasks[i] < (regCfg[i] >> 7)) modMasks[i] <<= 1;
    modMasks[i]--;
}

template <int i> void TeakInterp::writeAr(uint16_t value) {
    // Write to one of the AR registers and reconfigure operands
    regAr[i] = value;
    arRn[i * 2 + 0] = (value >> 13) & 0x7;
    arRn[i * 2 + 1] = (value >> 10) & 0x7;
    arCs[i * 2 + 0] = offsTable[(value >> 8) & 0x3];
    arCs[i * 2 + 1] = offsTable[(value >> 3) & 0x3];
    arPm[i * 2 + 0] = stepTable[(value >> 5) & 0x7];
    arPm[i * 2 + 1] = stepTable[(value >> 0) & 0x7];
}

template <int i> void TeakInterp::writeArp(uint16_t value) {
    // Write to one of the ARP registers and reconfigure operands
    regArp[i] = (value & 0x6FFF);
    arpRi[i] = ((value >> 10) & 0x3);
    arpRj[i] = ((value >> 13) & 0x3) + 4;
    arpCi[i] = offsTable[(value >> 3) & 0x3];
    arpCj[i] = offsTable[(value >> 8) & 0x3];
    arpPi[i] = stepTable[(value >> 0) & 0x7];
    arpPj[i] = stepTable[(value >> 5) & 0x7];
}

void TeakInterp::writeP0h(uint16_t value) {
    // Write to the upper word of the P0 register and sign-extend the 33rd bit in STT1
    regP[0].h = value;
    regP[0].v = int32_t(regP[0].v);
    regStt[1] = (regStt[1] & ~0x4000) | ((value >> 1) & 0x4000);
    updateShiftP(0);
}

void TeakInterp::writeSt0(uint16_t value) {
    // Write to the ST0 register and mirror bits to new registers and A0
    regSt[0] = value;
    regStt[0] = (regStt[0] & ~0xFF) | ((regSt[0] >> 4) & 0xFE) | ((regSt[0] >> 5) & 0x1);
    regStt[1] = (regStt[1] & ~0x10) | (regSt[0] & 0x10);
    regMod[0] = (regMod[0] & ~0x1) | (regSt[0] & 0x1);
    regMod[3] = (regMod[3] & ~0x380) | ((regSt[0] << 6) & 0x380);
    regA[0].e = int16_t(regSt[0]) >> 12;
    core->dsp.updateIcuState();
}

void TeakInterp::writeSt1(uint16_t value) {
    // Write to the ST1 register and mirror bits to new registers and A1
    regSt[1] = (regSt[1] & ~0xFCFF) | (value & 0xFCFF);
    regMod[0] = (regMod[0] & ~0xC00) | (regSt[1] & 0xC00);
    regMod[1] = (regMod[1] & ~0xFF) | (regSt[1] & 0xFF);
    regA[1].e = int16_t(regSt[1]) >> 12;
    updateShiftP(0);
}

void TeakInterp::writeSt2(uint16_t value) {
    // Write to the ST2 register and mirror bits to new registers
    regSt[2] = (regSt[2] & ~0x3FF) | (value & 0x3FF);
    regMod[0] = (regMod[0] & ~0x380) | (regSt[2] & 0x380);
    regMod[2] = (regMod[2] & ~0x3F) | (regSt[2] & 0x3F);
    regMod[3] = (regMod[3] & ~0x400) | ((regSt[2] << 4) & 0x400);
    core->dsp.updateIcuState();
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
    for (int i = 0; i < 2; i++) {
        regP[i].e = int16_t(regStt[1] << !i) >> 15;
        updateShiftP(i);
    }
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
    updateShiftP(0), updateShiftP(1);
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
    core->dsp.updateIcuState();
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
