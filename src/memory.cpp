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

#include <cstdio>
#include "core.h"

bool Memory::loadBootRoms()
{
    // Try to load the ARM11 boot ROM
    FILE *file = fopen("boot11.bin", "rb");
    if (!file) return false;
    fread(boot11, sizeof(uint8_t), 0x10000, file);
    fclose(file);

    // Try to load the ARM9 boot ROM
    file = fopen("boot9.bin", "rb");
    if (!file) return false;
    fread(boot9, sizeof(uint8_t), 0x10000, file);
    fclose(file);
    return true;
}

template uint8_t  Memory::read(CpuId id, uint32_t address);
template uint16_t Memory::read(CpuId id, uint32_t address);
template uint32_t Memory::read(CpuId id, uint32_t address);
template <typename T> T Memory::read(CpuId id, uint32_t address)
{
    // Get a pointer to readable memory on the ARM11/ARM9 if it exists
    uint8_t *data = nullptr;
    if (address >= 0x20000000 && address < 0x28000000)
        data = &fcram[address & 0x7FFFFFF]; // 128MB FCRAM
    else if (address >= 0x18000000 && address < 0x18600000)
        data = &vram[address & 0x7FFFFF]; // 6MB VRAM
    else if (address >= 0x1FF00000 && address < 0x1FF80000)
        data = &dspWram[address & 0x7FFFF]; // 512KB DSP code/data RAM
    else if (address >= 0x1FF80000 && address < 0x20000000)
        data = &axiWram[address & 0x7FFFF]; // 512KB AXI WRAM
    else if (id != ARM9 && (address < 0x20000 || address >= 0xFFFF0000))
        data = &boot11[address & 0xFFFF]; // 64KB ARM11 boot ROM
    else if (id == ARM9 && address >= 0xFFFF0000)
        data = &boot9[address & 0xFFFF]; // 64KB ARM9 boot ROM

    // Get a pointer to readable TCM memory on the ARM9 if it exists
    if (id == ARM9)
    {
        Cp15 &cp15 = core->cpus[ARM9].cp15;
        if (address < cp15.itcmSize && cp15.itcmRead)
            data = &itcm[address & 0x7FFF];
        else if (address >= cp15.dtcmAddr && address < cp15.dtcmAddr + cp15.dtcmSize && cp15.dtcmRead)
            data = &dtcm[(address - cp15.dtcmAddr) & 0x3FFF];
    }

    // Form an LSB-first value from the data at the pointer
    if (data)
    {
        T value = 0;
        for (size_t i = 0; i < sizeof(T); i++)
            value |= data[i] << (i << 3);
        return value;
    }

    // Catch reads from unmapped memory
    LOG_WARN("Unknown ARM%d memory read: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
    return 0;
}

template void Memory::write(CpuId id, uint32_t address, uint8_t  value);
template void Memory::write(CpuId id, uint32_t address, uint16_t value);
template void Memory::write(CpuId id, uint32_t address, uint32_t value);
template <typename T> void Memory::write(CpuId id, uint32_t address, T value)
{
    // Get a pointer to writable memory on the ARM11/ARM9 if it exists
    uint8_t *data = nullptr;
    if (address >= 0x20000000 && address < 0x28000000)
        data = &fcram[address & 0x7FFFFFF]; // 128MB FCRAM
    else if (address >= 0x18000000 && address < 0x18600000)
        data = &vram[address & 0x7FFFFF]; // 6MB VRAM
    else if (address >= 0x1FF00000 && address < 0x1FF80000)
        data = &dspWram[address & 0x7FFFF]; // 512KB DSP code/data RAM
    else if (address >= 0x1FF80000 && address < 0x20000000)
        data = &axiWram[address & 0x7FFFF]; // 512KB AXI WRAM

    // Get a pointer to writable TCM memory on the ARM9 if it exists
    if (id == ARM9)
    {
        Cp15 &cp15 = core->cpus[ARM9].cp15;
        if (address < cp15.itcmSize && cp15.itcmWrite)
            data = &itcm[address & 0x7FFF];
        else if (address >= cp15.dtcmAddr && address < cp15.dtcmAddr + cp15.dtcmSize && cp15.dtcmWrite)
            data = &dtcm[(address - cp15.dtcmAddr) & 0x3FFF];
    }

    // Write an LSB-first value to the data at the pointer
    if (data)
    {
        for (size_t i = 0; i < sizeof(T); i++)
            data[i] = value >> (i << 3);
        return;
    }

    // Catch writes to unmapped memory
    LOG_WARN("Unknown ARM%d memory write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
}
