/*
    Copyright 2023 Hydr8gon

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

class Memory
{
    public:
        Memory(Core *core): core(core) {};
        bool loadBootRoms();

        template <typename T> T read(CpuId id, uint32_t address);
        template <typename T> void write(CpuId id, uint32_t address, T value);

    private:
        Core *core;

        uint8_t fcram[0x8000000] = {}; // 128MB FCRAM
        uint8_t vram[0x600000]   = {}; // 6MB VRAM
        uint8_t dspWram[0x80000] = {}; // 512KB DSP code/data RAM
        uint8_t axiWram[0x80000] = {}; // 512KB AXI WRAM
        uint8_t boot11[0x10000]  = {}; // 64KB ARM11 boot ROM
        uint8_t boot9[0x10000]   = {}; // 64KB ARM9 boot ROM
        uint8_t itcm[0x8000]     = {}; // 32KB ARM9 ITCM
        uint8_t dtcm[0x4000]     = {}; // 16KB ARM9 DTCM
};
