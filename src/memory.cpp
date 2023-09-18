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

#include "core.h"

// Define a 16-bit register in an I/O switch statement
#define DEF_IO16(addr, func)      \
    case addr + 0: case addr + 1: \
        base -= addr;             \
        size = 2;                 \
        func;                     \
        goto next;

// Define a 32-bit register in an I/O switch statement
#define DEF_IO32(addr, func)      \
    case addr + 0: case addr + 1: \
    case addr + 2: case addr + 3: \
        base -= addr;             \
        size = 4;                 \
        func;                     \
        goto next;

// Define shared parameters for I/O register writes
#define IO_PARAMS mask << (base << 3), data << (base << 3)

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
    else if (id == ARM9 && address >= 0x8000000 && address < 0x8100000)
        data = &arm9Ram[address & 0xFFFFF]; // 1MB ARM9 internal RAM
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

    // Forward the read to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioRead<T>(id, address);

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
    else if (id == ARM9 && address >= 0x8000000 && address < 0x8100000)
        data = &arm9Ram[address & 0xFFFFF]; // 1MB ARM9 internal RAM
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

    // Forward the write to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioWrite<T>(id, address, value);

    // Catch writes to unmapped memory
    LOG_WARN("Unknown ARM%d memory write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
}

template <typename T> T Memory::ioRead(CpuId id, uint32_t address)
{
    T value = 0;
    size_t i = 0;

    // Read a value from one or more I/O registers
    while (i < sizeof(T))
    {
        uint32_t base = address + i;
        uint32_t size, data;

        if (id != ARM9)
        {
            // Check registers that are exclusive to the ARM11
            switch (base)
            {
                DEF_IO32(0x10163000, data = core->pxi.readPxiSync11()) // PXI_SYNC11
                DEF_IO32(0x17E0010C, data = 0x3FF) // IRQ_ACK (stub)
            }
        }
        else
        {
            // Check registers that are exclusive to the ARM9
            switch (base)
            {
                DEF_IO32(0x10001000, data = core->cpus[ARM9].readIrqIe()) // IRQ_IE
                DEF_IO32(0x10001004, data = core->cpus[ARM9].readIrqIf()) // IRQ_IF
                DEF_IO16(0x10006000, data = core->sdMmc.readSdCmd()) // SD_CMD
                DEF_IO32(0x1000600C, data = core->sdMmc.readSdResponse(0)) // SD_RESPONSE0
                DEF_IO32(0x10006010, data = core->sdMmc.readSdResponse(1)) // SD_RESPONSE1
                DEF_IO32(0x10006014, data = core->sdMmc.readSdResponse(2)) // SD_RESPONSE2
                DEF_IO32(0x10006018, data = core->sdMmc.readSdResponse(3)) // SD_RESPONSE3
                DEF_IO32(0x1000601C, data = core->sdMmc.readSdIrqStatus()) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, data = core->sdMmc.readSdIrqMask()) // SD_IRQ_MASK
                DEF_IO32(0x10008000, data = core->pxi.readPxiSync9()) // PXI_SYNC9
                DEF_IO32(0x10009000, data = 0x20) // AES_CNT (stub)
            }
        }

        // Catch reads from unmapped I/O registers
        LOG_WARN("Unknown ARM%d I/O read: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
        return value;

    next:
        // Process the data and loop until a full value is formed
        value |= (data >> (base << 3)) << (i << 3);
        i += size - base;
    }

    return value;
}

template <typename T> void Memory::ioWrite(CpuId id, uint32_t address, T value)
{
    size_t i = 0;

    // Write a value to one or more I/O registers
    while (i < sizeof(T))
    {
        uint32_t base = address + i, size;
        uint32_t mask = (1ULL << ((sizeof(T) - i) << 3)) - 1;
        uint32_t data = value >> (i << 3);

        if (id != ARM9)
        {
            // Check registers that are exclusive to the ARM11
            switch (base)
            {
                DEF_IO32(0x10163000, core->pxi.writePxiSync11(IO_PARAMS)) // PXI_SYNC11
            }
        }
        else
        {
            // Check registers that are exclusive to the ARM9
            switch (base)
            {
                DEF_IO32(0x10001000, core->cpus[ARM9].writeIrqIe(IO_PARAMS)) // IRQ_IE
                DEF_IO32(0x10001004, core->cpus[ARM9].writeIrqIf(IO_PARAMS)) // IRQ_IF
                DEF_IO16(0x10006000, core->sdMmc.writeSdCmd(IO_PARAMS)) // SD_CMD
                DEF_IO32(0x1000601C, core->sdMmc.writeSdIrqStatus(IO_PARAMS)) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmc.writeSdIrqMask(IO_PARAMS)) // SD_IRQ_MASK
                DEF_IO32(0x10008000, core->pxi.writePxiSync9(IO_PARAMS)) // PXI_SYNC9
            }
        }

        // Catch writes to unmapped I/O registers
        LOG_WARN("Unknown ARM%d I/O write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
        return;

    next:
        // Loop until the full value has been written
        i += size - base;
    }
}
