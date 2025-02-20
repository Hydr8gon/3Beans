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

    uint32_t readSingleS(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp);
    uint32_t readSingleD(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp);
    void writeSingleS(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value);
    void writeSingleD(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value);

    uint64_t readDoubleS(uint8_t cpopc, uint8_t cm);
    uint64_t readDoubleD(uint8_t cpopc, uint8_t cm);
    void writeDoubleS(uint8_t cpopc, uint8_t cm, uint64_t value);
    void writeDoubleD(uint8_t cpopc, uint8_t cm, uint64_t value);

    void loadMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs);
    void loadMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs);
    void storeMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs);
    void storeMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs);

    void dataOperS(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp);
    void dataOperD(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp);

private:
    Core *core;
    CpuId id;
};
