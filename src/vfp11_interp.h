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
#include "defines.h"

class Core;

class Vfp11Interp {
public:
    Vfp11Interp(Core *core, CpuId id): core(core), id(id) {}

    void readSingleS(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp);
    void readSingleD(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp);
    void writeSingleS(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp);
    void writeSingleD(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp);

    void readDoubleS(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm);
    void readDoubleD(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm);
    void writeDoubleS(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm);
    void writeDoubleD(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm);

    void loadMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs);
    void loadMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs);
    void storeMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs);
    void storeMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs);

    void dataOperS(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp);
    void dataOperD(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp);

private:
    Core *core;
    CpuId id;

    union {
        uint32_t u32[32] = {};
        int32_t i32[32];
        float flt[32];
        double dbl[16];
    } regs;

    uint32_t fpscr = 0;
    uint32_t fpexc = 0;

    uint8_t vecLength = 1;
    bool vecStride = false;

    bool checkEnable();

    void fmrs(uint32_t *rd, uint8_t sn);
    void fmrx(uint32_t *rd, uint8_t sys);
    void fmsr(uint8_t sn, uint32_t rd);
    void fmxr(uint8_t sys, uint32_t rd);

    void fldsP(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fldsM(uint8_t fd, uint32_t rn, uint8_t ofs);
    void flddP(uint8_t fd, uint32_t rn, uint8_t ofs);
    void flddM(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fldmia(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fldmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs);
    void fldmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs);

    void fstsP(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fstsM(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fstdP(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fstdM(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fstmia(uint8_t fd, uint32_t rn, uint8_t ofs);
    void fstmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs);
    void fstmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs);

    void fadds(uint8_t fd, uint8_t fn, uint8_t fm);
    void fsubs(uint8_t fd, uint8_t fn, uint8_t fm);
    void fdivs(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmuls(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmuls(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmacs(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmacs(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmscs(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmscs(uint8_t fd, uint8_t fn, uint8_t fm);

    void faddd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fsubd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fdivd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmuld(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmuld(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmacd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmacd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fmscd(uint8_t fd, uint8_t fn, uint8_t fm);
    void fnmscd(uint8_t fd, uint8_t fn, uint8_t fm);

    void fcpys(uint8_t fd, uint8_t fm);
    void fabss(uint8_t fd, uint8_t fm);
    void fnegs(uint8_t fd, uint8_t fm);
    void fsqrts(uint8_t fd, uint8_t fm);
    void fcmps(uint8_t fd, uint8_t fm);
    void fcmpzs(uint8_t fd, uint8_t fm);
    void fcvtds(uint8_t fd, uint8_t fm);
    void fuitos(uint8_t fd, uint8_t fm);
    void fsitos(uint8_t fd, uint8_t fm);
    void ftouis(uint8_t fd, uint8_t fm);
    void ftosis(uint8_t fd, uint8_t fm);

    void fcpyd(uint8_t fd, uint8_t fm);
    void fabsd(uint8_t fd, uint8_t fm);
    void fnegd(uint8_t fd, uint8_t fm);
    void fsqrtd(uint8_t fd, uint8_t fm);
    void fcmpd(uint8_t fd, uint8_t fm);
    void fcmpzd(uint8_t fd, uint8_t fm);
    void fcvtsd(uint8_t fd, uint8_t fm);
    void fuitod(uint8_t fd, uint8_t fm);
    void fsitod(uint8_t fd, uint8_t fm);
    void ftouid(uint8_t fd, uint8_t fm);
    void ftosid(uint8_t fd, uint8_t fm);
};
