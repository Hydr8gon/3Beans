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

class GpuShader {
public:
    virtual ~GpuShader() {}

    virtual void startList() = 0;
    virtual void processVtx(uint32_t idx = -1) = 0;

    virtual void setOutMap(uint8_t (*map)[2]) = 0;
    virtual void setGshInMap(uint8_t *map) = 0;
    virtual void setGshInCount(uint8_t count) = 0;

    virtual void setVshCode(int i, uint32_t value) = 0;
    virtual void setVshDesc(int i, uint32_t value) = 0;
    virtual void setVshEntry(uint16_t entry, uint16_t end) = 0;
    virtual void setVshBool(int i, bool value) = 0;
    virtual void setVshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) = 0;
    virtual void setVshFloats(int i, float *floats) = 0;

    virtual void setGshCode(int i, uint32_t value) = 0;
    virtual void setGshDesc(int i, uint32_t value) = 0;
    virtual void setGshEntry(uint16_t entry, uint16_t end) = 0;
    virtual void setGshBool(int i, bool value) = 0;
    virtual void setGshInts(int i, uint8_t int0, uint8_t int1, uint8_t int2) = 0;
    virtual void setGshFloats(int i, float *floats) = 0;
};
