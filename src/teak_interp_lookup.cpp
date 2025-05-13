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
        if ((op & 0xFF00) == 0xE700)
            teakInstrs[op] = &TeakInterp::addvMi8;
        else if ((op & 0xFFE0) == 0x86E0)
            teakInstrs[op] = &TeakInterp::addvMrn;
        else if ((op & 0xFFE0) == 0x87E0)
            teakInstrs[op] = &TeakInterp::addvReg;
        else if (op == 0x47BB)
            teakInstrs[op] = &TeakInterp::addvR6;
        else if ((op & 0xFF00) == 0x5C00)
            teakInstrs[op] = &TeakInterp::bkrepI8;
        else if ((op & 0xFF80) == 0x5D00)
            teakInstrs[op] = &TeakInterp::bkrepReg;
        else if ((op & 0xFFFC) == 0x8FDC)
            teakInstrs[op] = &TeakInterp::bkrepR6;
        else if ((op & 0xFFC0) == 0x4180)
            teakInstrs[op] = &TeakInterp::br;
        else if ((op & 0xF800) == 0x5000)
            teakInstrs[op] = &TeakInterp::brr;
        else if ((op & 0xFFC0) == 0x41C0)
            teakInstrs[op] = &TeakInterp::call;
        else if ((op & 0xEFF0) == 0x6760)
            teakInstrs[op] = &TeakInterp::clrA;
        else if ((op & 0xEFF0) == 0x6F60)
            teakInstrs[op] = &TeakInterp::clrB;
        else if ((op & 0xEFF0) == 0x67C0)
            teakInstrs[op] = &TeakInterp::clrrA;
        else if ((op & 0xEFF0) == 0x6F70)
            teakInstrs[op] = &TeakInterp::clrrB;
        else if ((op & 0xFE00) == 0xBE00)
            teakInstrs[op] = &TeakInterp::cmpuMi8;
        else if ((op & 0xFEE0) == 0x9E80)
            teakInstrs[op] = &TeakInterp::cmpuMrn;
        else if ((op & 0xFEE0) == 0x9EA0)
            teakInstrs[op] = &TeakInterp::cmpuReg;
        else if ((op & 0xFFF7) == 0x8A63)
            teakInstrs[op] = &TeakInterp::cmpuR6;
        else if (op == 0xD390)
            teakInstrs[op] = &TeakInterp::cntxR;
        else if (op == 0xD380)
            teakInstrs[op] = &TeakInterp::cntxS;
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
        else if ((op & 0xFF00) == 0x0400)
            teakInstrs[op] = &TeakInterp::loadPage;
        else if ((op & 0xFB80) == 0xDB80)
            teakInstrs[op] = &TeakInterp::loadStep;
        else if ((op & 0xF100) == 0x3000)
            teakInstrs[op] = &TeakInterp::movAblhmi8;
        else if ((op & 0xFEFF) == 0xD4BC)
            teakInstrs[op] = &TeakInterp::movAlmi16;
        else if ((op & 0xFFF8) == 0x0008)
            teakInstrs[op] = &TeakInterp::movI16arap;
        else if ((op & 0xFFE0) == 0x5E00)
            teakInstrs[op] = &TeakInterp::movI16reg;
        else if (op == 0x0023)
            teakInstrs[op] = &TeakInterp::movI16r6;
        else if ((op & 0xFFF8) == 0x0030)
            teakInstrs[op] = &TeakInterp::movI16sm;
        else if ((op & 0xFFF7) == 0x8971)
            teakInstrs[op] = &TeakInterp::movI16stp;
        else if ((op & 0xFEFF) == 0xD4B8)
            teakInstrs[op] = &TeakInterp::movMi16a;
        else if ((op & 0xFC00) == 0x1800)
            teakInstrs[op] = &TeakInterp::movRegmrn;
        else if ((op & 0xFC00) == 0x5800)
            teakInstrs[op] = &TeakInterp::movRegreg;
        else if ((op & 0xFFE0) == 0x5F60)
            teakInstrs[op] = &TeakInterp::movRegr6;
        else if ((op & 0xFFC0) == 0x0D40)
            teakInstrs[op] = &TeakInterp::movpPmareg;
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
        else if ((op & 0xFEE0) == 0x8080)
            teakInstrs[op] = &TeakInterp::orMrn;
        else if ((op & 0xFEE0) == 0x80A0)
            teakInstrs[op] = &TeakInterp::orReg;
        else if ((op & 0xFFEF) == 0xD388)
            teakInstrs[op] = &TeakInterp::orR6;
        else if ((op & 0xFFE0) == 0x5E60)
            teakInstrs[op] = &TeakInterp::popReg;
        else if ((op & 0xFFE0) == 0x5E40)
            teakInstrs[op] = &TeakInterp::pushReg;
        else if ((op & 0xFF00) == 0x0C00)
            teakInstrs[op] = &TeakInterp::repI8;
        else if ((op & 0xFFE0) == 0x0D00)
            teakInstrs[op] = &TeakInterp::repReg;
        else if ((op & 0xFFFE) == 0x0002)
            teakInstrs[op] = &TeakInterp::repR6;
        else if ((op & 0xFFF0) == 0x4580)
            teakInstrs[op] = &TeakInterp::ret;
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
        else if ((op & 0xF240) == 0x9240)
            teakInstrs[op] = &TeakInterp::shfi;
        else if ((op & 0xFEF7) == 0x8A61)
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
        else if ((op & 0xFEE0) == 0x8E80)
            teakInstrs[op] = &TeakInterp::subMrna;
        else if ((op & 0xFEE0) == 0x8EA0)
            teakInstrs[op] = &TeakInterp::subRega;
        else if ((op & 0xFFEF) == 0xD38F)
            teakInstrs[op] = &TeakInterp::subR6a;
        else
            teakInstrs[op] = &TeakInterp::unkOp;
    }
}
