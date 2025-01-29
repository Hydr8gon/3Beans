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

class Gpu {
public:
    Gpu(Core *core): core(core) {}

    uint32_t readMemfillDstAddr(int i) { return gpuMemfillDstAddr[i]; }
    uint32_t readMemfillDstEnd(int i) { return gpuMemfillDstEnd[i]; }
    uint32_t readMemfillData(int i) { return gpuMemfillData[i]; }
    uint32_t readMemfillCnt(int i) { return gpuMemfillCnt[i]; }

    void writeMemfillDstAddr(int i, uint32_t mask, uint32_t value);
    void writeMemfillDstEnd(int i, uint32_t mask, uint32_t value);
    void writeMemfillData(int i, uint32_t mask, uint32_t value);
    void writeMemfillCnt(int i, uint32_t mask, uint32_t value);

private:
    Core *core;

    uint32_t gpuMemfillDstAddr[2] = {};
    uint32_t gpuMemfillDstEnd[2] = {};
    uint32_t gpuMemfillData[2] = {};
    uint32_t gpuMemfillCnt[2] = {};
};
