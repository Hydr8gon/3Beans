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

class Cp15 {
public:
    uint32_t exceptAddr = 0;
    bool dtcmRead = false, dtcmWrite = false;
    bool itcmRead = false, itcmWrite = false;
    uint32_t dtcmAddr = 0, dtcmSize = 0;
    uint32_t itcmSize = 0;

    Cp15(Core *core, CpuId id): core(core), id(id) {}
    uint32_t read(uint8_t cn, uint8_t cm, uint8_t cp);
    void write(uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value);

private:
    Core *core;
    CpuId id;

    uint32_t ctrlReg = 0x78;
    uint32_t dtcmReg = 0;
    uint32_t itcmReg = 0;
};
