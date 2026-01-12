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
#include <queue>

class Core;

class Pxi {
public:
    Pxi(Core &core): core(core) {}

    uint32_t readSync(bool arm9) { return pxiSync[arm9]; }
    uint16_t readCnt(bool arm9) { return pxiCnt[arm9]; }
    uint32_t readRecv(bool arm9);

    void writeSync(bool arm9, uint32_t mask, uint32_t value);
    void writeCnt(bool arm9, uint16_t mask, uint16_t value);
    void writeSend(bool arm9, uint32_t mask, uint32_t value);

private:
    Core &core;
    std::queue<uint32_t> fifos[2];

    uint32_t pxiSync[2] = {};
    uint16_t pxiCnt[2] = { 0x101, 0x101 };
    uint32_t pxiRecv[2] = {};
};
