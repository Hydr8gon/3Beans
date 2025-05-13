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

union SplitReg {
    int64_t v;
    struct {
        uint16_t l;
        uint16_t h;
        int32_t e;
    };
};

class TeakInterp {
public:
    uint64_t cycles = -1;
    uint32_t regPc = 0;

    TeakInterp(Core *core);

    void resetCycles();
    int runOpcode();

private:
    Core *core;

    uint16_t *readRegister[0x20] = { &regR[0], &regR[1], &regR[2], &regR[3], &regR[4], &regR[5], &regR[7],
        &regY[0], &regSt[0], &regSt[1], &regSt[2], &regP[0].h, (uint16_t*)&regPc, &regSp, &regCfg[0],
        &regCfg[1], &regB[0].h, &regB[1].h, &regB[0].l, &regB[1].l, &regExt[0], &regExt[1], &regExt[2],
        &regExt[3], &regA[0].l, &regA[1].l, &regA[0].l, &regA[1].l, &regA[0].h, &regA[1].h, &regLc, &regSv };
    uint16_t *readRegisterP0[0x20] = { &regR[0], &regR[1], &regR[2], &regR[3], &regR[4], &regR[5], &regR[7],
        &regY[0], &regSt[0], &regSt[1], &regSt[2], &regP[0].l, (uint16_t*)&regPc, &regSp, &regCfg[0],
        &regCfg[1], &regB[0].h, &regB[1].h, &regB[0].l, &regB[1].l, &regExt[0], &regExt[1], &regExt[2],
        &regExt[3], &regA[0].l, &regA[1].l, &regA[0].l, &regA[1].l, &regA[0].h, &regA[1].h, &regLc, &regSv };

    uint16_t *readSttMod[0x8] = { &regStt[0], &regStt[1], &regStt[2],
        &regNone, &regMod[0], &regMod[1], &regMod[2], &regMod[3] };
    uint16_t *readAblh[0x8] = { &regB[0].l, &regB[0].h, &regB[1].l,
        &regB[1].h, &regA[0].l, &regA[0].h, &regA[1].l, &regA[1].h };
    int64_t *readAb[0x4] = { &regB[0].v, &regB[1].v, &regA[0].v, &regA[1].v };

    static int (TeakInterp::*teakInstrs[0x10000])(uint16_t);
    static void (TeakInterp::*writeRegister[0x20])(uint16_t);
    static void (TeakInterp::*writeArArp[0x8])(uint16_t);
    static void (TeakInterp::*writeSttMod[0x8])(uint16_t);
    static void (TeakInterp::*writeAb[0x4])(int64_t);
    static void (TeakInterp::**writeAx)(int64_t);
    static void (TeakInterp::**writeBx)(int64_t);

    SplitReg regA[2] = {};
    SplitReg regB[2] = {};
    SplitReg regP[2] = {};
    uint16_t regY[2] = {};
    uint16_t regR[8] = {};
    uint16_t regExt[4] = {};
    uint16_t regSp = 0;
    uint16_t regSv = 0;
    uint16_t regLc = 0;
    uint16_t regIcr = 0xFF00;
    uint16_t regSt[3] = { 0x0, 0x300, 0x1000 };
    uint16_t regStt[3] = {};
    uint16_t regMod[4] = { 0x4 };
    uint16_t regCfg[2] = {};
    uint16_t regStep[2] = {};
    uint16_t regAr[2] = {};
    uint16_t regArp[4] = {};
    uint16_t regNone = 0;

    uint16_t shadSt[3] = {};
    uint16_t shadAr[2] = {};
    uint16_t shadArp[4] = {};
    uint16_t bkStack[4] = {};
    uint32_t bkStart[4] = {};
    uint32_t bkEnd[4] = {};
    uint32_t repCount = 0;
    uint32_t repAddr = 0;

    void incrementPc();
    uint16_t readParam();
    bool checkCond(uint8_t cond);
    static uint16_t calcZmne(int64_t res);
    uint16_t getRnStepZids(uint8_t rnStep);

    template <int i> void writeA40(int64_t value);
    template <int i> void writeA16(uint16_t value);
    template <int i> void writeAl(uint16_t value);
    template <int i> void writeAh(uint16_t value);
    template <int i> void writeB40(int64_t value);
    template <int i> void writeBl(uint16_t value);
    template <int i> void writeBh(uint16_t value);
    template <int i> void writeR(uint16_t value);
    template <int i> void writeExt(uint16_t value);
    template <int i> void writeCfg(uint16_t value);
    template <int i> void writeAr(uint16_t value);
    template <int i> void writeArp(uint16_t value);

    void writeP0h(uint16_t value);
    void writeY0(uint16_t value);
    void writePc(uint16_t value);
    void writeSp(uint16_t value);
    void writeSv(uint16_t value);
    void writeLc(uint16_t value);
    void writeSt0(uint16_t value);
    void writeSt1(uint16_t value);
    void writeSt2(uint16_t value);
    void writeStt0(uint16_t value);
    void writeStt1(uint16_t value);
    void writeStt2(uint16_t value);
    void writeMod0(uint16_t value);
    void writeMod1(uint16_t value);
    void writeMod2(uint16_t value);
    void writeMod3(uint16_t value);
    void writeNone(uint16_t value);

    static void initLookup();
    int unkOp(uint16_t opcode);

    int addvMi8(uint16_t opcode);
    int addvMrn(uint16_t opcode);
    int addvReg(uint16_t opcode);
    int addvR6(uint16_t opcode);
    int clrA(uint16_t opcode);
    int clrB(uint16_t opcode);
    int clrrA(uint16_t opcode);
    int clrrB(uint16_t opcode);
    int cmpuMi8(uint16_t opcode);
    int cmpuMrn(uint16_t opcode);
    int cmpuReg(uint16_t opcode);
    int cmpuR6(uint16_t opcode);
    int dec(uint16_t opcode);
    int inc(uint16_t opcode);
    int orAba(uint16_t opcode);
    int orAb(uint16_t opcode);
    int orBb(uint16_t opcode);
    int orI16(uint16_t opcode);
    int orI8(uint16_t opcode);
    int orMi16(uint16_t opcode);
    int orMi8(uint16_t opcode);
    int orMrn(uint16_t opcode);
    int orReg(uint16_t opcode);
    int orR6(uint16_t opcode);
    int rstMi8(uint16_t opcode);
    int rstMrn(uint16_t opcode);
    int rstReg(uint16_t opcode);
    int rstR6(uint16_t opcode);
    int rstSm(uint16_t opcode);
    int setMi8(uint16_t opcode);
    int setMrn(uint16_t opcode);
    int setReg(uint16_t opcode);
    int setR6(uint16_t opcode);
    int setSm(uint16_t opcode);
    int shfi(uint16_t opcode);
    int subAbb(uint16_t opcode);
    int subBa(uint16_t opcode);
    int subI16a(uint16_t opcode);
    int subI8a(uint16_t opcode);
    int subMi16a(uint16_t opcode);
    int subMi8a(uint16_t opcode);
    int subMrna(uint16_t opcode);
    int subRega(uint16_t opcode);
    int subR6a(uint16_t opcode);

    int bkrepI8(uint16_t opcode);
    int bkrepReg(uint16_t opcode);
    int bkrepR6(uint16_t opcode);
    int br(uint16_t opcode);
    int brr(uint16_t opcode);
    int call(uint16_t opcode);
    int cntxR(uint16_t opcode);
    int cntxS(uint16_t opcode);
    int dint(uint16_t opcode);
    int eint(uint16_t opcode);
    int nop(uint16_t opcode);
    int repI8(uint16_t opcode);
    int repReg(uint16_t opcode);
    int repR6(uint16_t opcode);
    int ret(uint16_t opcode);

    int loadMod(uint16_t opcode);
    int loadPage(uint16_t opcode);
    int loadStep(uint16_t opcode);
    int movAblhmi8(uint16_t opcode);
    int movAlmi16(uint16_t opcode);
    int movI16arap(uint16_t opcode);
    int movI16reg(uint16_t opcode);
    int movI16r6(uint16_t opcode);
    int movI16sm(uint16_t opcode);
    int movI16stp(uint16_t opcode);
    int movMi16a(uint16_t opcode);
    int movRegmrn(uint16_t opcode);
    int movRegreg(uint16_t opcode);
    int movRegr6(uint16_t opcode);
    int movpPmareg(uint16_t opcode);
    int popReg(uint16_t opcode);
    int pushReg(uint16_t opcode);
};
