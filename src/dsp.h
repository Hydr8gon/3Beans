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

class Dsp {
public:
    Dsp(Core *core): core(core) {}

    uint16_t readPcfg() { return dspPcfg; }
    uint16_t readPsts() { return dspPsts; }

    void writePcfg(uint16_t mask, uint16_t value);

private:
    Core *core;

    uint16_t dspPcfg = 0x1;
    uint16_t dspPsts = 0xE100;
};
