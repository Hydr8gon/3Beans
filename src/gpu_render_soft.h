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

class GpuRenderSoft {
public:
    GpuRenderSoft(Core *core): core(core) {}
    void runShader(uint16_t entry, float (*input)[4]);

    void writeVshCode(uint16_t address, uint32_t value);
    void writeVshDesc(uint16_t address, uint32_t value);

    void setVshBool(int i, bool value);
    void setVshInt(int i, int j, uint8_t value);
    void setVshFloat(int i, int j, float value);

private:
    Core *core;
    static void (GpuRenderSoft::*vshInstrs[0x40])(uint32_t);

    float (*shdIn)[4] = nullptr;
    bool shdRunning = false;

    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};

    bool vshBools[16] = {};
    uint8_t vshInts[4][3] = {};
    float vshFloats[96][4] = {};

    void shdNop(uint32_t opcode);
    void shdEnd(uint32_t opcode);
    void vshUnk(uint32_t opcode);
};
