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

// Lookup table for register reads with accumulator saturation
uint16_t (TeakInterp::*TeakInterp::readRegS[])() = {
    &TeakInterp::readR<0>, &TeakInterp::readR<1>, &TeakInterp::readR<2>, &TeakInterp::readR<3>,
    &TeakInterp::readR<4>, &TeakInterp::readR<5>, &TeakInterp::readR<7>, &TeakInterp::readY0,
    &TeakInterp::readSt<0>, &TeakInterp::readSt<1>, &TeakInterp::readSt<2>, &TeakInterp::readP0hS,
    &TeakInterp::readPc, &TeakInterp::readSp, &TeakInterp::readCfg<0>, &TeakInterp::readCfg<1>,
    &TeakInterp::readBhS<0>, &TeakInterp::readBhS<1>, &TeakInterp::readBlS<0>, &TeakInterp::readBlS<1>,
    &TeakInterp::readExt<0>, &TeakInterp::readExt<1>, &TeakInterp::readExt<2>, &TeakInterp::readExt<3>,
    &TeakInterp::readA16<0>, &TeakInterp::readA16<1>, &TeakInterp::readAlS<0>, &TeakInterp::readAlS<1>,
    &TeakInterp::readAhS<0>, &TeakInterp::readAhS<1>, &TeakInterp::readLc, &TeakInterp::readSv
};

// Lookup table for register reads with P0 and accumulator saturation
uint16_t (TeakInterp::*TeakInterp::readRegP0S[])() = {
    &TeakInterp::readR<0>, &TeakInterp::readR<1>, &TeakInterp::readR<2>, &TeakInterp::readR<3>,
    &TeakInterp::readR<4>, &TeakInterp::readR<5>, &TeakInterp::readR<7>, &TeakInterp::readY0,
    &TeakInterp::readSt<0>, &TeakInterp::readSt<1>, &TeakInterp::readSt<2>, &TeakInterp::readP016S,
    &TeakInterp::readPc, &TeakInterp::readSp, &TeakInterp::readCfg<0>, &TeakInterp::readCfg<1>,
    &TeakInterp::readBhS<0>, &TeakInterp::readBhS<1>, &TeakInterp::readBlS<0>, &TeakInterp::readBlS<1>,
    &TeakInterp::readExt<0>, &TeakInterp::readExt<1>, &TeakInterp::readExt<2>, &TeakInterp::readExt<3>,
    &TeakInterp::readA16<0>, &TeakInterp::readA16<1>, &TeakInterp::readAlS<0>, &TeakInterp::readAlS<1>,
    &TeakInterp::readAhS<0>, &TeakInterp::readAhS<1>, &TeakInterp::readLc, &TeakInterp::readSv
};

// Lookup table for partial accumulator reads with saturation
uint16_t (TeakInterp::*TeakInterp::readAblhS[])()= {
    &TeakInterp::readBlS<0>, &TeakInterp::readBhS<0>, &TeakInterp::readBlS<1>, &TeakInterp::readBhS<1>,
    &TeakInterp::readAlS<0>, &TeakInterp::readAhS<0>, &TeakInterp::readAlS<1>, &TeakInterp::readAhS<1>
};

// Lookup table for full accumulator reads with saturation
int64_t (TeakInterp::*TeakInterp::readAbS[])()= {
    &TeakInterp::readB40S<0>, &TeakInterp::readB40S<1>, &TeakInterp::readA40S<0>, &TeakInterp::readA40S<1>
};

// Lookup table for full product reads with shifting
int64_t (TeakInterp::*TeakInterp::readPxS[])()= {
    &TeakInterp::readP33S<0>, &TeakInterp::readP33S<1>
};

// Lookup table for general register writes
void (TeakInterp::*TeakInterp::writeReg[])(uint16_t) = {
    &TeakInterp::writeR<0>, &TeakInterp::writeR<1>, &TeakInterp::writeR<2>, &TeakInterp::writeR<3>,
    &TeakInterp::writeR<4>, &TeakInterp::writeR<5>, &TeakInterp::writeR<7>, &TeakInterp::writeY0,
    &TeakInterp::writeSt0, &TeakInterp::writeSt1, &TeakInterp::writeSt2, &TeakInterp::writeP0h,
    &TeakInterp::writePc, &TeakInterp::writeSp, &TeakInterp::writeCfg<0>, &TeakInterp::writeCfg<1>,
    &TeakInterp::writeBh<0>, &TeakInterp::writeBh<1>, &TeakInterp::writeBl<0>, &TeakInterp::writeBl<1>,
    &TeakInterp::writeExt<0>, &TeakInterp::writeExt<1>, &TeakInterp::writeExt<2>, &TeakInterp::writeExt<3>,
    &TeakInterp::writeA16<0>, &TeakInterp::writeA16<1>, &TeakInterp::writeAl<0>, &TeakInterp::writeAl<1>,
    &TeakInterp::writeAh<0>, &TeakInterp::writeAh<1>, &TeakInterp::writeLc, &TeakInterp::writeSv
};

// Lookup table for register writes with MOV accumulator rules
void (TeakInterp::*TeakInterp::writeRegM[])(uint16_t) = {
    &TeakInterp::writeR<0>, &TeakInterp::writeR<1>, &TeakInterp::writeR<2>, &TeakInterp::writeR<3>,
    &TeakInterp::writeR<4>, &TeakInterp::writeR<5>, &TeakInterp::writeR<7>, &TeakInterp::writeY0,
    &TeakInterp::writeSt0, &TeakInterp::writeSt1, &TeakInterp::writeSt2, &TeakInterp::writeP0h,
    &TeakInterp::writePc, &TeakInterp::writeSp, &TeakInterp::writeCfg<0>, &TeakInterp::writeCfg<1>,
    &TeakInterp::writeBhM<0>, &TeakInterp::writeBhM<1>, &TeakInterp::writeBlM<0>, &TeakInterp::writeBlM<1>,
    &TeakInterp::writeExt<0>, &TeakInterp::writeExt<1>, &TeakInterp::writeExt<2>, &TeakInterp::writeExt<3>,
    &TeakInterp::writeA16M<0>, &TeakInterp::writeA16M<1>, &TeakInterp::writeAlM<0>, &TeakInterp::writeAlM<1>,
    &TeakInterp::writeAhM<0>, &TeakInterp::writeAhM<1>, &TeakInterp::writeLc, &TeakInterp::writeSv
};

// Lookup table for AR/ARP and STT/MOD register writes
void (TeakInterp::*TeakInterp::writeArpMod[])(uint16_t) = {
    &TeakInterp::writeAr<0>, &TeakInterp::writeAr<1>, &TeakInterp::writeArp<0>, &TeakInterp::writeArp<1>,
    &TeakInterp::writeArp<2>, &TeakInterp::writeArp<3>, &TeakInterp::writeNone, &TeakInterp::writeNone,
    &TeakInterp::writeStt0, &TeakInterp::writeStt1, &TeakInterp::writeStt2, &TeakInterp::writeNone,
    &TeakInterp::writeMod0, &TeakInterp::writeMod1, &TeakInterp::writeMod2, &TeakInterp::writeMod3
};

// Lookup table for partial accumulator writes with MOV rules
void (TeakInterp::*TeakInterp::writeAblhM[])(uint16_t) = {
    &TeakInterp::writeBlM<0>, &TeakInterp::writeBhM<0>, &TeakInterp::writeBlM<1>, &TeakInterp::writeBhM<1>,
    &TeakInterp::writeAlM<0>, &TeakInterp::writeAhM<0>, &TeakInterp::writeAlM<1>, &TeakInterp::writeAhM<1>
};

// Lookup table for 40-bit accumulator writes
void (TeakInterp::*TeakInterp::writeAb40[])(int64_t) = {
    &TeakInterp::writeB40<0>, &TeakInterp::writeB40<1>, &TeakInterp::writeA40<0>, &TeakInterp::writeA40<1>
};

// Lookup table for 40-bit accumulator writes with MOV rules
void (TeakInterp::*TeakInterp::writeAb40M[])(int64_t) = {
    &TeakInterp::writeB40M<0>, &TeakInterp::writeB40M<1>, &TeakInterp::writeA40M<0>, &TeakInterp::writeA40M<1>
};

// Lookup table for 16-bit accumulator writes with MOV rules
void (TeakInterp::*TeakInterp::writeAb16M[])(uint16_t) = {
    &TeakInterp::writeB16M<0>, &TeakInterp::writeB16M<1>, &TeakInterp::writeA16M<0>, &TeakInterp::writeA16M<1>
};

// Lookup table for lower accumulator writes with MOV rules
void (TeakInterp::*TeakInterp::writeAblM[])(uint16_t) = {
    &TeakInterp::writeBlM<0>, &TeakInterp::writeBlM<1>, &TeakInterp::writeAlM<0>, &TeakInterp::writeAlM<1>
};

// Lookup table for 33-bit product writes
void (TeakInterp::*TeakInterp::writePx33[])(int64_t)= {
    &TeakInterp::writeP33<0>, &TeakInterp::writeP33<1>
};

int (TeakInterp::*TeakInterp::teakInstrs[])(uint16_t) = {};
int64_t (TeakInterp::**TeakInterp::readAxS)() = &readAbS[2];
int64_t (TeakInterp::**TeakInterp::readBxS)() = &readAbS[0];
void (TeakInterp::**TeakInterp::writeCfgx)(uint16_t) = &writeReg[14];
void (TeakInterp::**TeakInterp::writeArArp)(uint16_t) = &writeArpMod[0];
void (TeakInterp::**TeakInterp::writeSttMod)(uint16_t) = &writeArpMod[8];
void (TeakInterp::**TeakInterp::writeAx40)(int64_t) = &writeAb40[2];
void (TeakInterp::**TeakInterp::writeBx40)(int64_t) = &writeAb40[0];
void (TeakInterp::**TeakInterp::writeAx16M)(uint16_t) = &writeAb16M[2];
void (TeakInterp::**TeakInterp::writeBx16M)(uint16_t) = &writeAb16M[0];
void (TeakInterp::**TeakInterp::writeAxlM)(uint16_t) = &writeAblM[2];

int8_t TeakInterp::offsTable[] = { 0, 1, -1, -1 };
int32_t TeakInterp::stepTable[] = { 0, 1, -1, STEP_S, 2, -2, 2, -2 };

void TeakInterp::initLookup() {
    // Build an instruction lookup table using all 16 opcode bits
    for (uint32_t op = 0; op < 0x10000; op++) {
        if ((op & 0xF3FE) == 0xD2DA)
            teakInstrs[op] = &TeakInterp::addAbb;
        else if ((op & 0xFFFC) == 0x5DF0)
            teakInstrs[op] = &TeakInterp::addBa;
        else if ((op & 0xFEFF) == 0x86C0)
            teakInstrs[op] = &TeakInterp::addI16a;
        else if ((op & 0xFE00) == 0xC600)
            teakInstrs[op] = &TeakInterp::addI8a;
        else if ((op & 0xFEFF) == 0xD4FB)
            teakInstrs[op] = &TeakInterp::addMi16a;
        else if ((op & 0xFE00) == 0xA600)
            teakInstrs[op] = &TeakInterp::addMi8a;
        else if ((op & 0xFEFF) == 0xD4DB)
            teakInstrs[op] = &TeakInterp::addM7i16a;
        else if ((op & 0xFE80) == 0x4600)
            teakInstrs[op] = &TeakInterp::addM7i7a;
        else if ((op & 0xFEE0) == 0x8680)
            teakInstrs[op] = &TeakInterp::addMrna;
        else if ((op & 0xFFFC) == 0x5DF8)
            teakInstrs[op] = &TeakInterp::addPb;
        else if ((op & 0xFFF3) == 0x5DC0)
            teakInstrs[op] = &TeakInterp::addPpab;
        else if ((op & 0xFEE0) == 0x86A0)
            teakInstrs[op] = &TeakInterp::addRega;
        else if ((op & 0xFFEF) == 0xD38B)
            teakInstrs[op] = &TeakInterp::addR6a;
        else if ((op & 0xFE00) == 0xB200)
            teakInstrs[op] = &TeakInterp::addhMi8;
        else if ((op & 0xFEE0) == 0x9280)
            teakInstrs[op] = &TeakInterp::addhMrn;
        else if ((op & 0xFEE0) == 0x92A0)
            teakInstrs[op] = &TeakInterp::addhReg;
        else if ((op & 0xFFFE) == 0x9464)
            teakInstrs[op] = &TeakInterp::addhR6;
        else if ((op & 0xFE00) == 0xB400)
            teakInstrs[op] = &TeakInterp::addlMi8;
        else if ((op & 0xFEE0) == 0x9480)
            teakInstrs[op] = &TeakInterp::addlMrn;
        else if ((op & 0xFEE0) == 0x94A0)
            teakInstrs[op] = &TeakInterp::addlReg;
        else if ((op & 0xFFFE) == 0x9466)
            teakInstrs[op] = &TeakInterp::addlR6;
        else if ((op & 0xFF00) == 0xE700)
            teakInstrs[op] = &TeakInterp::addvMi8;
        else if ((op & 0xFFE0) == 0x86E0)
            teakInstrs[op] = &TeakInterp::addvMrn;
        else if ((op & 0xFFE0) == 0x87E0)
            teakInstrs[op] = &TeakInterp::addvReg;
        else if (op == 0x47BB)
            teakInstrs[op] = &TeakInterp::addvR6;
        else if ((op & 0xFFF3) == 0x5DC1)
            teakInstrs[op] = &TeakInterp::adda;
        else if ((op & 0xFFF3) == 0x4590)
            teakInstrs[op] = &TeakInterp::add3;
        else if ((op & 0xFFF3) == 0x4592)
            teakInstrs[op] = &TeakInterp::add3a;
        else if ((op & 0xFFF3) == 0x4593)
            teakInstrs[op] = &TeakInterp::add3aa;
        else if ((op & 0xEFF0) == 0x6770)
            teakInstrs[op] = &TeakInterp::andAbab;
        else if ((op & 0xFEFF) == 0x82C0)
            teakInstrs[op] = &TeakInterp::andI16;
        else if ((op & 0xFE00) == 0xC200)
            teakInstrs[op] = &TeakInterp::andI8;
        else if ((op & 0xFEFF) == 0xD4F9)
            teakInstrs[op] = &TeakInterp::andMi16;
        else if ((op & 0xFE00) == 0xA200)
            teakInstrs[op] = &TeakInterp::andMi8;
        else if ((op & 0xFEFF) == 0xD4D9)
            teakInstrs[op] = &TeakInterp::andM7i16;
        else if ((op & 0xFE80) == 0x4200)
            teakInstrs[op] = &TeakInterp::andM7i7;
        else if ((op & 0xFEE0) == 0x8280)
            teakInstrs[op] = &TeakInterp::andMrn;
        else if ((op & 0xFEE0) == 0x82A0)
            teakInstrs[op] = &TeakInterp::andReg;
        else if ((op & 0xFFEF) == 0xD389)
            teakInstrs[op] = &TeakInterp::andR6;
        else if ((op & 0xFFC0) == 0x4B80)
            teakInstrs[op] = &TeakInterp::banke;
        else if ((op & 0xFF00) == 0x5C00)
            teakInstrs[op] = &TeakInterp::bkrepI8;
        else if ((op & 0xFF80) == 0x5D00)
            teakInstrs[op] = &TeakInterp::bkrepReg;
        else if ((op & 0xFFFC) == 0x8FDC)
            teakInstrs[op] = &TeakInterp::bkrepR6;
        else if ((op & 0xFFFC) == 0xDA9C)
            teakInstrs[op] = &TeakInterp::bkreprstMrar;
        else if ((op & 0xFFFC) == 0x5F48)
            teakInstrs[op] = &TeakInterp::bkreprstMsp;
        else if ((op & 0xFBFC) == 0xDADC)
            teakInstrs[op] = &TeakInterp::bkrepstoMrar;
        else if ((op & 0xFFF8) == 0x9468)
            teakInstrs[op] = &TeakInterp::bkrepstoMsp;
        else if (op == 0xD3C0)
            teakInstrs[op] = &TeakInterp::_break;
        else if ((op & 0xFFC0) == 0x4180)
            teakInstrs[op] = &TeakInterp::br;
        else if ((op & 0xF800) == 0x5000)
            teakInstrs[op] = &TeakInterp::brr;
        else if ((op & 0xFFC0) == 0x41C0)
            teakInstrs[op] = &TeakInterp::call;
        else if ((op & 0xFFEF) == 0xD381)
            teakInstrs[op] = &TeakInterp::callaA;
        else if ((op & 0xFEFF) == 0xD480)
            teakInstrs[op] = &TeakInterp::callaAl;
        else if ((op & 0xF800) == 0x1000)
            teakInstrs[op] = &TeakInterp::callr;
        else if ((op & 0xFF00) == 0xE500)
            teakInstrs[op] = &TeakInterp::chngMi8;
        else if ((op & 0xFFE0) == 0x84E0)
            teakInstrs[op] = &TeakInterp::chngMrn;
        else if ((op & 0xFFE0) == 0x85E0)
            teakInstrs[op] = &TeakInterp::chngReg;
        else if (op == 0x47BA)
            teakInstrs[op] = &TeakInterp::chngR6;
        else if ((op & 0xFFF8) == 0x0038)
            teakInstrs[op] = &TeakInterp::chngSm;
        else if ((op & 0xEFF0) == 0x6760)
            teakInstrs[op] = &TeakInterp::clrA;
        else if ((op & 0xEFF0) == 0x6F60)
            teakInstrs[op] = &TeakInterp::clrB;
        else if (op == 0x5DFE)
            teakInstrs[op] = &TeakInterp::clrp0;
        else if (op == 0x5DFD)
            teakInstrs[op] = &TeakInterp::clrp1;
        else if (op == 0x5DFF)
            teakInstrs[op] = &TeakInterp::clrp01;
        else if ((op & 0xEFF0) == 0x67C0)
            teakInstrs[op] = &TeakInterp::clrrA;
        else if ((op & 0xEFF0) == 0x6F70)
            teakInstrs[op] = &TeakInterp::clrrB;
        else if ((op & 0xFFFC) == 0x4D8C)
            teakInstrs[op] = &TeakInterp::cmpAb;
        else if ((op & 0xFBFE) == 0xDA9A)
            teakInstrs[op] = &TeakInterp::cmpBa;
        else if (op == 0xD483)
            teakInstrs[op] = &TeakInterp::cmpB0b1;
        else if (op == 0xD583)
            teakInstrs[op] = &TeakInterp::cmpB1b0;
        else if ((op & 0xFEFF) == 0x8CC0)
            teakInstrs[op] = &TeakInterp::cmpI16a;
        else if ((op & 0xFE00) == 0xCC00)
            teakInstrs[op] = &TeakInterp::cmpI8a;
        else if ((op & 0xFEFF) == 0xD4FE)
            teakInstrs[op] = &TeakInterp::cmpMi16a;
        else if ((op & 0xFE00) == 0xAC00)
            teakInstrs[op] = &TeakInterp::cmpMi8a;
        else if ((op & 0xFEFF) == 0xD4DE)
            teakInstrs[op] = &TeakInterp::cmpM7i16a;
        else if ((op & 0xFE80) == 0x4C00)
            teakInstrs[op] = &TeakInterp::cmpM7i7a;
        else if ((op & 0xFEE0) == 0x8C80)
            teakInstrs[op] = &TeakInterp::cmpMrna;
        else if ((op & 0xFEE0) == 0x8CA0)
            teakInstrs[op] = &TeakInterp::cmpRega;
        else if ((op & 0xFFEF) == 0xD38E)
            teakInstrs[op] = &TeakInterp::cmpR6a;
        else if ((op & 0xFE00) == 0xBE00)
            teakInstrs[op] = &TeakInterp::cmpuMi8;
        else if ((op & 0xFEE0) == 0x9E80)
            teakInstrs[op] = &TeakInterp::cmpuMrn;
        else if ((op & 0xFEE0) == 0x9EA0)
            teakInstrs[op] = &TeakInterp::cmpuReg;
        else if ((op & 0xFFF7) == 0x8A63)
            teakInstrs[op] = &TeakInterp::cmpuR6;
        else if ((op & 0xFF00) == 0xED00)
            teakInstrs[op] = &TeakInterp::cmpvMi8;
        else if ((op & 0xFFE0) == 0x8CE0)
            teakInstrs[op] = &TeakInterp::cmpvMrn;
        else if ((op & 0xFFE0) == 0x8DE0)
            teakInstrs[op] = &TeakInterp::cmpvReg;
        else if (op == 0x47BE)
            teakInstrs[op] = &TeakInterp::cmpvR6;
        else if (op == 0xD390)
            teakInstrs[op] = &TeakInterp::cntxR;
        else if (op == 0xD380)
            teakInstrs[op] = &TeakInterp::cntxS;
        else if ((op & 0xEFF0) == 0x67F0)
            teakInstrs[op] = &TeakInterp::copy;
        else if ((op & 0xEFF0) == 0x67E0)
            teakInstrs[op] = &TeakInterp::dec;
        else if (op == 0x43C0)
            teakInstrs[op] = &TeakInterp::dint;
        else if (op == 0x4380)
            teakInstrs[op] = &TeakInterp::eint;
        else if ((op & 0xFFFE) == 0x9460)
            teakInstrs[op] = &TeakInterp::expB;
        else if ((op & 0xFEFE) == 0x9060)
            teakInstrs[op] = &TeakInterp::expBa;
        else if ((op & 0xFFE0) == 0x9C40)
            teakInstrs[op] = &TeakInterp::expMrn;
        else if ((op & 0xFEE0) == 0x9840)
            teakInstrs[op] = &TeakInterp::expMrna;
        else if ((op & 0xFFE0) == 0x9440)
            teakInstrs[op] = &TeakInterp::expReg;
        else if ((op & 0xFEE0) == 0x9040)
            teakInstrs[op] = &TeakInterp::expRega;
        else if (op == 0xD7C1)
            teakInstrs[op] = &TeakInterp::expR6;
        else if ((op & 0xFFEF) == 0xD382)
            teakInstrs[op] = &TeakInterp::expR6a;
        else if ((op & 0xEFF0) == 0x67D0)
            teakInstrs[op] = &TeakInterp::inc;
        else if ((op & 0xF600) == 0x0200)
            teakInstrs[op] = &TeakInterp::loadMod;
        else if ((op & 0xFFF8) == 0xD7D8)
            teakInstrs[op] = &TeakInterp::loadMpd;
        else if ((op & 0xFF00) == 0x0400)
            teakInstrs[op] = &TeakInterp::loadPage;
        else if ((op & 0xFFFC) == 0x4D80)
            teakInstrs[op] = &TeakInterp::loadPs;
        else if ((op & 0xFFF0) == 0x0010)
            teakInstrs[op] = &TeakInterp::loadPs01;
        else if ((op & 0xFB80) == 0xDB80)
            teakInstrs[op] = &TeakInterp::loadStep;
        else if ((op & 0xF780) == 0xD400)
            teakInstrs[op] = &TeakInterp::maaMrmr;
        else if ((op & 0xF7E0) == 0x8400)
            teakInstrs[op] = &TeakInterp::maaMrni16;
        else if ((op & 0xF700) == 0xE400)
            teakInstrs[op] = &TeakInterp::maaY0mi8;
        else if ((op & 0xF7E0) == 0x8420)
            teakInstrs[op] = &TeakInterp::maaY0mrn;
        else if ((op & 0xF7E0) == 0x8440)
            teakInstrs[op] = &TeakInterp::maaY0reg;
        else if ((op & 0xFFFE) == 0x5EA8)
            teakInstrs[op] = &TeakInterp::maaY0r6;
        else if ((op & 0xFEE7) == 0x8460)
            teakInstrs[op] = &TeakInterp::maxGe;
        else if ((op & 0xFEE7) == 0x8660)
            teakInstrs[op] = &TeakInterp::maxGt;
        else if ((op & 0xFEE7) == 0x8860)
            teakInstrs[op] = &TeakInterp::minLe;
        else if ((op & 0xFEE7) == 0x8A60)
            teakInstrs[op] = &TeakInterp::minLt;
        else if ((op & 0xF3F8) == 0x82C8)
            teakInstrs[op] = &TeakInterp::mma;
        else if ((op & 0xF3F8) == 0x83C8)
            teakInstrs[op] = &TeakInterp::mmaa;
        else if ((op & 0xFF00) == 0xC800)
            teakInstrs[op] = &TeakInterp::mma3;
        else if ((op & 0xF0FE) == 0x80C2)
            teakInstrs[op] = &TeakInterp::mma3a;
        else if ((op & 0xFF07) == 0xCB04)
            teakInstrs[op] = &TeakInterp::mmsua3;
        else if ((op & 0xFF07) == 0xCB05)
            teakInstrs[op] = &TeakInterp::mmusa3;
        else if ((op & 0xFF07) == 0xCB06)
            teakInstrs[op] = &TeakInterp::mmsua3a;
        else if ((op & 0xFF07) == 0xCB07)
            teakInstrs[op] = &TeakInterp::mmusa3a;
        else if ((op & 0xFF07) == 0xCA04)
            teakInstrs[op] = &TeakInterp::msumsua3a;
        else if ((op & 0xFF07) == 0xCA05)
            teakInstrs[op] = &TeakInterp::msumusa3a;
        else if ((op & 0xFF07) == 0xCA06)
            teakInstrs[op] = &TeakInterp::msumsua3aa;
        else if ((op & 0xFF07) == 0xCA07)
            teakInstrs[op] = &TeakInterp::msumusa3aa;
        else if ((op & 0xFFF8) == 0x5DA0)
            teakInstrs[op] = &TeakInterp::modrD2;
        else if ((op & 0xFFF8) == 0x5DA8)
            teakInstrs[op] = &TeakInterp::modrD2d;
        else if ((op & 0xFFF8) == 0x4990)
            teakInstrs[op] = &TeakInterp::modrI2;
        else if ((op & 0xFFF8) == 0x4998)
            teakInstrs[op] = &TeakInterp::modrI2d;
        else if ((op & 0xFFE0) == 0x0080)
            teakInstrs[op] = &TeakInterp::modrZids;
        else if ((op & 0xFFE0) == 0x00A0)
            teakInstrs[op] = &TeakInterp::modrZidsd;
        else if ((op & 0xF39C) == 0xD294)
            teakInstrs[op] = &TeakInterp::modrMrmr;
        else if ((op & 0xFF81) == 0x0D80)
            teakInstrs[op] = &TeakInterp::modrMrmrd;
        else if ((op & 0xFCE4) == 0x8464)
            teakInstrs[op] = &TeakInterp::modrMrdmr;
        else if ((op & 0xFF81) == 0x0D81)
            teakInstrs[op] = &TeakInterp::modrMrdmrd;
        else if ((op & 0xFEFF) == 0x886B)
            teakInstrs[op] = &TeakInterp::movApc;
        else if ((op & 0xFEFF) == 0xD49B)
            teakInstrs[op] = &TeakInterp::movA0hstp;
        else if ((op & 0xF39F) == 0xD290)
            teakInstrs[op] = &TeakInterp::movAbab;
        else if ((op & 0xFFFC) == 0x8FD4)
            teakInstrs[op] = &TeakInterp::movAbp0;
        else if ((op & 0xF100) == 0x3000)
            teakInstrs[op] = &TeakInterp::movAblhmi8;
        else if ((op & 0xFFE0) == 0x9540)
            teakInstrs[op] = &TeakInterp::movAblarap;
        else if ((op & 0xFFE0) == 0x9C60)
            teakInstrs[op] = &TeakInterp::movAblsm;
        else if ((op & 0xFFFC) == 0xD394)
            teakInstrs[op] = &TeakInterp::movAblx1;
        else if ((op & 0xFFFC) == 0xD384)
            teakInstrs[op] = &TeakInterp::movAbly1;
        else if ((op & 0xFEFF) == 0xD4BC)
            teakInstrs[op] = &TeakInterp::movAlmi16;
        else if ((op & 0xFEFF) == 0xD49C)
            teakInstrs[op] = &TeakInterp::movAlm7i16;
        else if ((op & 0xFE80) == 0xDC80)
            teakInstrs[op] = &TeakInterp::movAlm7i7;
        else if ((op & 0xFFE0) == 0x9560)
            teakInstrs[op] = &TeakInterp::movArapabl;
        else if ((op & 0xFFF8) == 0x0008)
            teakInstrs[op] = &TeakInterp::movI16arap;
        else if ((op & 0xFEFF) == 0x5E20)
            teakInstrs[op] = &TeakInterp::movI16b;
        else if ((op & 0xFFE0) == 0x5E00)
            teakInstrs[op] = &TeakInterp::movI16reg;
        else if (op == 0x0023)
            teakInstrs[op] = &TeakInterp::movI16r6;
        else if ((op & 0xFFF8) == 0x0030)
            teakInstrs[op] = &TeakInterp::movI16sm;
        else if ((op & 0xFFF7) == 0x8971)
            teakInstrs[op] = &TeakInterp::movI16stp;
        else if ((op & 0xEF00) == 0x2100)
            teakInstrs[op] = &TeakInterp::movI8al;
        else if ((op & 0xE300) == 0x2300)
            teakInstrs[op] = &TeakInterp::movI8ry;
        else if ((op & 0xFF00) == 0x0500)
            teakInstrs[op] = &TeakInterp::movI8sv;
        else if ((op & 0xFEFF) == 0xD4B8)
            teakInstrs[op] = &TeakInterp::movMi16a;
        else if ((op & 0xE700) == 0x6100)
            teakInstrs[op] = &TeakInterp::movMi8ab;
        else if ((op & 0xE300) == 0x6200)
            teakInstrs[op] = &TeakInterp::movMi8ablh;
        else if ((op & 0xE300) == 0x6000)
            teakInstrs[op] = &TeakInterp::movMi8ry;
        else if ((op & 0xFF00) == 0x6D00)
            teakInstrs[op] = &TeakInterp::movMi8sv;
        else if ((op & 0xFEFF) == 0xD498)
            teakInstrs[op] = &TeakInterp::movM7i16a;
        else if ((op & 0xFE80) == 0xD880)
            teakInstrs[op] = &TeakInterp::movM7i7a;
        else if ((op & 0xFEE0) == 0x98C0)
            teakInstrs[op] = &TeakInterp::movMrnb;
        else if ((op & 0xFC00) == 0x1C00)
            teakInstrs[op] = &TeakInterp::movMrnreg;
        else if ((op & 0xFFE0) == 0x47C0)
            teakInstrs[op] = &TeakInterp::movMxpreg;
        else if ((op & 0xFCF1) == 0x88D0)
            teakInstrs[op] = &TeakInterp::movPrar;
        else if ((op & 0xFCF1) == 0x88D1)
            teakInstrs[op] = &TeakInterp::movPrars;
        else if ((op & 0xF39E) == 0xD292)
            teakInstrs[op] = &TeakInterp::movRarp;
        else if ((op & 0xFFC0) == 0x5EC0)
            teakInstrs[op] = &TeakInterp::movRegb;
        else if ((op & 0xFC00) == 0x1800)
            teakInstrs[op] = &TeakInterp::movRegmrn;
        else if ((op & 0xFFE0) == 0x5E80)
            teakInstrs[op] = &TeakInterp::movRegmxp;
        else if ((op & 0xFC00) == 0x5800)
            teakInstrs[op] = &TeakInterp::movRegreg;
        else if ((op & 0xFFE0) == 0x5F60)
            teakInstrs[op] = &TeakInterp::movRegr6;
        else if ((op & 0xF100) == 0x2000)
            teakInstrs[op] = &TeakInterp::movRymi8;
        else if ((op & 0xFFE0) == 0x5F00)
            teakInstrs[op] = &TeakInterp::movR6reg;
        else if ((op & 0xF3F8) == 0xD2F8)
            teakInstrs[op] = &TeakInterp::movSmabl;
        else if ((op & 0xFEFF) == 0xD482)
            teakInstrs[op] = &TeakInterp::movStpa0h;
        else if ((op & 0xFF00) == 0x7D00)
            teakInstrs[op] = &TeakInterp::movSvmi8;
        else if ((op & 0xFFC0) == 0x4DC0)
            teakInstrs[op] = &TeakInterp::movaAbrar;
        else if ((op & 0xFFC0) == 0x4BC0)
            teakInstrs[op] = &TeakInterp::movaRarab;
        else if ((op & 0xFFC0) == 0x0D40)
            teakInstrs[op] = &TeakInterp::movpPmareg;
        else if ((op & 0xE700) == 0x6300)
            teakInstrs[op] = &TeakInterp::movsMi8ab;
        else if ((op & 0xFF80) == 0x0180)
            teakInstrs[op] = &TeakInterp::movsMrnab;
        else if ((op & 0xFF80) == 0x0100)
            teakInstrs[op] = &TeakInterp::movsRegab;
        else if ((op & 0xFFFE) == 0x5F42)
            teakInstrs[op] = &TeakInterp::movsR6a;
        else if ((op & 0xF180) == 0x4080)
            teakInstrs[op] = &TeakInterp::movsi;
        else if ((op & 0xFF00) == 0xE000)
            teakInstrs[op] = &TeakInterp::mpyY0mi8;
        else if ((op & 0xFFE0) == 0x8020)
            teakInstrs[op] = &TeakInterp::mpyY0mrn;
        else if ((op & 0xFFE0) == 0x8040)
            teakInstrs[op] = &TeakInterp::mpyY0reg;
        else if (op == 0x5EA0)
            teakInstrs[op] = &TeakInterp::mpyY0r6;
        else if ((op & 0xFF00) == 0x0800)
            teakInstrs[op] = &TeakInterp::mpyi;
        else if ((op & 0xEFF0) == 0x6790)
            teakInstrs[op] = &TeakInterp::neg;
        else if (op == 0x0000)
            teakInstrs[op] = &TeakInterp::nop;
        else if ((op & 0xEFF0) == 0x6780)
            teakInstrs[op] = &TeakInterp::_not;
        else if ((op & 0xF39F) == 0xD291)
            teakInstrs[op] = &TeakInterp::orAba;
        else if ((op & 0xFEFC) == 0xD4A4)
            teakInstrs[op] = &TeakInterp::orAb;
        else if ((op & 0xFBFC) == 0xD3C4)
            teakInstrs[op] = &TeakInterp::orBb;
        else if ((op & 0xFEFF) == 0x80C0)
            teakInstrs[op] = &TeakInterp::orI16;
        else if ((op & 0xFE00) == 0xC000)
            teakInstrs[op] = &TeakInterp::orI8;
        else if ((op & 0xFEFF) == 0xD4F8)
            teakInstrs[op] = &TeakInterp::orMi16;
        else if ((op & 0xFE00) == 0xA000)
            teakInstrs[op] = &TeakInterp::orMi8;
        else if ((op & 0xFEFF) == 0xD4D8)
            teakInstrs[op] = &TeakInterp::orM7i16;
        else if ((op & 0xFE80) == 0x4000)
            teakInstrs[op] = &TeakInterp::orM7i7;
        else if ((op & 0xFEE0) == 0x8080)
            teakInstrs[op] = &TeakInterp::orMrn;
        else if ((op & 0xFEE0) == 0x80A0)
            teakInstrs[op] = &TeakInterp::orReg;
        else if ((op & 0xFFEF) == 0xD388)
            teakInstrs[op] = &TeakInterp::orR6;
        else if ((op & 0xFFFC) == 0x47B4)
            teakInstrs[op] = &TeakInterp::popAbe;
        else if ((op & 0xF0FF) == 0x80C7)
            teakInstrs[op] = &TeakInterp::popArsm;
        else if ((op & 0xFFDE) == 0x0006)
            teakInstrs[op] = &TeakInterp::popB;
        else if ((op & 0xFFFE) == 0xD496)
            teakInstrs[op] = &TeakInterp::popP;
        else if ((op & 0xFFE0) == 0x5E60)
            teakInstrs[op] = &TeakInterp::popReg;
        else if ((op & 0xFFFC) == 0xD7F0)
            teakInstrs[op] = &TeakInterp::popRepc;
        else if ((op & 0xFFFE) == 0x0024)
            teakInstrs[op] = &TeakInterp::popR6;
        else if ((op & 0xFFFE) == 0xD494)
            teakInstrs[op] = &TeakInterp::popX;
        else if ((op & 0xFFFE) == 0x0004)
            teakInstrs[op] = &TeakInterp::popY1;
        else if ((op & 0xFFFC) == 0x47B0)
            teakInstrs[op] = &TeakInterp::popaAb;
        else if ((op & 0xFFF8) == 0xD7C8)
            teakInstrs[op] = &TeakInterp::pushAbe;
        else if ((op & 0xFFF0) == 0xD3D0)
            teakInstrs[op] = &TeakInterp::pushArsm;
        else if (op == 0x5F40)
            teakInstrs[op] = &TeakInterp::pushI16;
        else if ((op & 0xFFFC) == 0xD78C)
            teakInstrs[op] = &TeakInterp::pushP;
        else if ((op & 0xFFE0) == 0x5E40)
            teakInstrs[op] = &TeakInterp::pushReg;
        else if ((op & 0xFFFC) == 0xD7F8)
            teakInstrs[op] = &TeakInterp::pushRpc;
        else if ((op & 0xFFDF) == 0xD4D7)
            teakInstrs[op] = &TeakInterp::pushR6;
        else if ((op & 0xFFDE) == 0xD4D4)
            teakInstrs[op] = &TeakInterp::pushX;
        else if ((op & 0xFFDF) == 0xD4D6)
            teakInstrs[op] = &TeakInterp::pushY1;
        else if ((op & 0xFFBC) == 0x4384)
            teakInstrs[op] = &TeakInterp::pushaA;
        else if ((op & 0xFFFC) == 0xD788)
            teakInstrs[op] = &TeakInterp::pushaB;
        else if ((op & 0xFF00) == 0x0C00)
            teakInstrs[op] = &TeakInterp::repI8;
        else if ((op & 0xFFE0) == 0x0D00)
            teakInstrs[op] = &TeakInterp::repReg;
        else if ((op & 0xFFFE) == 0x0002)
            teakInstrs[op] = &TeakInterp::repR6;
        else if ((op & 0xFFF0) == 0x4580)
            teakInstrs[op] = &TeakInterp::ret;
        else if ((op & 0xFFEF) == 0x45C0)
            teakInstrs[op] = &TeakInterp::reti;
        else if ((op & 0xFF00) == 0x0900)
            teakInstrs[op] = &TeakInterp::rets;
        else if ((op & 0xFF00) == 0xE300)
            teakInstrs[op] = &TeakInterp::rstMi8;
        else if ((op & 0xFFE0) == 0x82E0)
            teakInstrs[op] = &TeakInterp::rstMrn;
        else if ((op & 0xFFE0) == 0x83E0)
            teakInstrs[op] = &TeakInterp::rstReg;
        else if (op == 0x47B9)
            teakInstrs[op] = &TeakInterp::rstR6;
        else if ((op & 0xFFF8) == 0x4388)
            teakInstrs[op] = &TeakInterp::rstSm;
        else if ((op & 0xFF00) == 0xE100)
            teakInstrs[op] = &TeakInterp::setMi8;
        else if ((op & 0xFFE0) == 0x80E0)
            teakInstrs[op] = &TeakInterp::setMrn;
        else if ((op & 0xFFE0) == 0x81E0)
            teakInstrs[op] = &TeakInterp::setReg;
        else if (op == 0x47B8)
            teakInstrs[op] = &TeakInterp::setR6;
        else if ((op & 0xFFF8) == 0x43C8)
            teakInstrs[op] = &TeakInterp::setSm;
        else if ((op & 0xF390) == 0xD280)
            teakInstrs[op] = &TeakInterp::shfc;
        else if ((op & 0xF240) == 0x9240)
            teakInstrs[op] = &TeakInterp::shfi;
        else if ((op & 0xEFF0) == 0x6720)
            teakInstrs[op] = &TeakInterp::shlA;
        else if ((op & 0xEFF0) == 0x6F20)
            teakInstrs[op] = &TeakInterp::shlB;
        else if ((op & 0xEFF0) == 0x6730)
            teakInstrs[op] = &TeakInterp::shl4A;
        else if ((op & 0xEFF0) == 0x6F30)
            teakInstrs[op] = &TeakInterp::shl4B;
        else if ((op & 0xEFF0) == 0x6700)
            teakInstrs[op] = &TeakInterp::shrA;
        else if ((op & 0xEFF0) == 0x6F00)
            teakInstrs[op] = &TeakInterp::shrB;
        else if ((op & 0xEFF0) == 0x6710)
            teakInstrs[op] = &TeakInterp::shr4A;
        else if ((op & 0xEFF0) == 0x6F10)
            teakInstrs[op] = &TeakInterp::shr4B;
        else if ((op & 0xFEE7) == 0x8A61)
            teakInstrs[op] = &TeakInterp::subAbb;
        else if ((op & 0xFFE7) == 0x8861)
            teakInstrs[op] = &TeakInterp::subBa;
        else if ((op & 0xFEFF) == 0x8EC0)
            teakInstrs[op] = &TeakInterp::subI16a;
        else if ((op & 0xFE00) == 0xCE00)
            teakInstrs[op] = &TeakInterp::subI8a;
        else if ((op & 0xFEFF) == 0xD4FF)
            teakInstrs[op] = &TeakInterp::subMi16a;
        else if ((op & 0xFE00) == 0xAE00)
            teakInstrs[op] = &TeakInterp::subMi8a;
        else if ((op & 0xFEFF) == 0xD4DF)
            teakInstrs[op] = &TeakInterp::subM7i16a;
        else if ((op & 0xFE80) == 0x4E00)
            teakInstrs[op] = &TeakInterp::subM7i7a;
        else if ((op & 0xFEE0) == 0x8E80)
            teakInstrs[op] = &TeakInterp::subMrna;
        else if ((op & 0xFEE0) == 0x8EA0)
            teakInstrs[op] = &TeakInterp::subRega;
        else if ((op & 0xFFEF) == 0xD38F)
            teakInstrs[op] = &TeakInterp::subR6a;
        else if ((op & 0xFE00) == 0xB600)
            teakInstrs[op] = &TeakInterp::subhMi8;
        else if ((op & 0xFEE0) == 0x9680)
            teakInstrs[op] = &TeakInterp::subhMrn;
        else if ((op & 0xFEE0) == 0x96A0)
            teakInstrs[op] = &TeakInterp::subhReg;
        else if ((op & 0xFEFF) == 0x5E23)
            teakInstrs[op] = &TeakInterp::subhR6;
        else if ((op & 0xFFC0) == 0x4980)
            teakInstrs[op] = &TeakInterp::swap;
        else if ((op & 0xF000) == 0xF000)
            teakInstrs[op] = &TeakInterp::tstbMi8;
        else if ((op & 0xF0E0) == 0x9020)
            teakInstrs[op] = &TeakInterp::tstbMrn;
        else if ((op & 0xF0FF) == 0x9018) // Override
            teakInstrs[op] = &TeakInterp::tstbR6;
        else if ((op & 0xF0E0) == 0x9000)
            teakInstrs[op] = &TeakInterp::tstbReg;
        else if ((op & 0xFFF8) == 0x0028)
            teakInstrs[op] = &TeakInterp::tstbSm;
        else if ((op & 0xFE00) == 0xA800)
            teakInstrs[op] = &TeakInterp::tst0Almi8;
        else if ((op & 0xFEE0) == 0x8880)
            teakInstrs[op] = &TeakInterp::tst0Almrn;
        else if ((op & 0xFEE0) == 0x88A0)
            teakInstrs[op] = &TeakInterp::tst0Alreg;
        else if ((op & 0xFFEF) == 0xD38C)
            teakInstrs[op] = &TeakInterp::tst0Alr6;
        else if ((op & 0xFF00) == 0xE900)
            teakInstrs[op] = &TeakInterp::tst0I16mi8;
        else if ((op & 0xFFE0) == 0x88E0)
            teakInstrs[op] = &TeakInterp::tst0I16mrn;
        else if ((op & 0xFFE0) == 0x89E0)
            teakInstrs[op] = &TeakInterp::tst0I16reg;
        else if (op == 0x47BC)
            teakInstrs[op] = &TeakInterp::tst0I16r6;
        else if ((op & 0xFFF8) == 0x9470)
            teakInstrs[op] = &TeakInterp::tst0I16sm;
        else if ((op & 0xFEFF) == 0x84C0)
            teakInstrs[op] = &TeakInterp::xorI16;
        else if ((op & 0xFE00) == 0xC400)
            teakInstrs[op] = &TeakInterp::xorI8;
        else if ((op & 0xFEFF) == 0xD4FA)
            teakInstrs[op] = &TeakInterp::xorMi16;
        else if ((op & 0xFE00) == 0xA400)
            teakInstrs[op] = &TeakInterp::xorMi8;
        else if ((op & 0xFEFF) == 0xD4DA)
            teakInstrs[op] = &TeakInterp::xorM7i16;
        else if ((op & 0xFE80) == 0x4400)
            teakInstrs[op] = &TeakInterp::xorM7i7;
        else if ((op & 0xFEE0) == 0x8480)
            teakInstrs[op] = &TeakInterp::xorMrn;
        else if ((op & 0xFEE0) == 0x84A0)
            teakInstrs[op] = &TeakInterp::xorReg;
        else if ((op & 0xFFEF) == 0xD38A)
            teakInstrs[op] = &TeakInterp::xorR6;
        else
            teakInstrs[op] = &TeakInterp::unkOp;
    }
}
