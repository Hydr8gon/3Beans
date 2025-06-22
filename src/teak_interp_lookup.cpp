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
        else if ((op & 0xFF00) == 0xE700)
            teakInstrs[op] = &TeakInterp::addvMi8;
        else if ((op & 0xFFE0) == 0x86E0)
            teakInstrs[op] = &TeakInterp::addvMrn;
        else if ((op & 0xFFE0) == 0x87E0)
            teakInstrs[op] = &TeakInterp::addvReg;
        else if (op == 0x47BB)
            teakInstrs[op] = &TeakInterp::addvR6;
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
        else if ((op & 0xEFF0) == 0x6760)
            teakInstrs[op] = &TeakInterp::clrA;
        else if ((op & 0xEFF0) == 0x6F60)
            teakInstrs[op] = &TeakInterp::clrB;
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
        else if ((op & 0xFFF8) == 0x5DA0)
            teakInstrs[op] = &TeakInterp::modrD2;
        else if ((op & 0xFFF8) == 0x4990)
            teakInstrs[op] = &TeakInterp::modrI2;
        else if ((op & 0xFFE0) == 0x0080)
            teakInstrs[op] = &TeakInterp::modrZids;
        else if ((op & 0xFEFF) == 0x886B)
            teakInstrs[op] = &TeakInterp::movApc;
        else if ((op & 0xFEFF) == 0xD49B)
            teakInstrs[op] = &TeakInterp::movA0hstp;
        else if ((op & 0xF100) == 0x3000)
            teakInstrs[op] = &TeakInterp::movAblhmi8;
        else if ((op & 0xFFE0) == 0x9540)
            teakInstrs[op] = &TeakInterp::movAblarap;
        else if ((op & 0xFFE0) == 0x9C60)
            teakInstrs[op] = &TeakInterp::movAblsm;
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
        else if ((op & 0xFF00) == 0x0800)
            teakInstrs[op] = &TeakInterp::mpyi;
        else if ((op & 0xEFF0) == 0x6790)
            teakInstrs[op] = &TeakInterp::neg;
        else if (op == 0x0000)
            teakInstrs[op] = &TeakInterp::nop;
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
        else
            teakInstrs[op] = &TeakInterp::unkOp;
    }
}
