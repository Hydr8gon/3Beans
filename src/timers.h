/*
    Copyright 2023-2024 Hydr8gon

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

class Timers {
public:
    Timers(Core *core): core(core) {}

    void resetCycles();
    void overflow(int i);

    uint16_t readTmCntL(int i);
    uint16_t readTmCntH(int i) { return tmCntH[i]; }

    void writeTmCntL(int i, uint16_t mask, uint16_t value);
    void writeTmCntH(int i, uint16_t mask, uint16_t value);

private:
    Core *core;

    uint32_t endCycles[4] = {};
    uint16_t timers[4] = {};
    uint8_t shifts[4] = { 1, 1, 1, 1 };
    bool countUp[4] = {};

    uint16_t tmCntL[4] = {};
    uint16_t tmCntH[4] = {};
};
