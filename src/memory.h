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

class Memory {
public:
    Memory(Core *core);
    bool loadFiles();
    void loadOtp(FILE *file);

    template <typename T> T read(CpuId id, uint32_t address);
    template <typename T> void write(CpuId id, uint32_t address, T value);

private:
    Core *core;

    // 32-bit address space, split into 64KB pages
    uint8_t *readMap11[0x10000] = {};
    uint8_t *readMap9[0x10000] = {};
    uint8_t *writeMap11[0x10000] = {};
    uint8_t *writeMap9[0x10000] = {};

    uint8_t arm9Ram[0x100000] = {}; // 1MB ARM9 internal RAM
    uint8_t fcram[0x8000000] = {}; // 128MB FCRAM
    uint8_t vram[0x600000] = {}; // 6MB VRAM
    uint8_t dspWram[0x80000] = {}; // 512KB DSP code/data RAM
    uint8_t axiWram[0x80000] = {}; // 512KB AXI WRAM
    uint8_t boot11[0x10000] = {}; // 64KB ARM11 boot ROM
    uint8_t boot9[0x10000] = {}; // 64KB ARM9 boot ROM

    uint8_t cfg9Sysprot9 = 0;
    uint8_t cfg9Sysprot11 = 0;
    uint32_t otpEncrypted[0x40] = {};

    template <typename T> T ioRead(CpuId id, uint32_t address);
    template <typename T> void ioWrite(CpuId id, uint32_t address, T value);

    uint8_t readCfg9Sysprot9() { return cfg9Sysprot9; }
    uint8_t readCfg9Sysprot11() { return cfg9Sysprot11; }
    uint32_t readOtpEncrypted(int i) { return otpEncrypted[i]; }

    void writeCfg9Sysprot9(uint8_t value);
    void writeCfg9Sysprot11(uint8_t value);
};

template uint8_t Memory::read(CpuId id, uint32_t address);
template uint16_t Memory::read(CpuId id, uint32_t address);
template uint32_t Memory::read(CpuId id, uint32_t address);
template <typename T> FORCE_INLINE T Memory::read(CpuId id, uint32_t address) {
    // Look up a readable memory pointer and load an LSB-first value from it
    if (uint8_t *data = (id == ARM9 ? readMap9 : readMap11)[address >> 16]) {
        T value = 0;
        data += address & (0x10000 - sizeof(T));
        for (uint32_t i = 0; i < sizeof(T); i++)
            value |= data[i] << (i << 3);
        return value;
    }

    // Forward the read to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioRead<T>(id, address);

    // Catch reads from unmapped memory
    LOG_WARN("Unmapped ARM%d memory read: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
    return 0;
}

template void Memory::write(CpuId id, uint32_t address, uint8_t value);
template void Memory::write(CpuId id, uint32_t address, uint16_t value);
template void Memory::write(CpuId id, uint32_t address, uint32_t value);
template <typename T> FORCE_INLINE void Memory::write(CpuId id, uint32_t address, T value) {
    // Look up a writable memory pointer and store an LSB-first value to it
    if (uint8_t *data = (id == ARM9 ? writeMap9 : writeMap11)[address >> 16]) {
        data += address & (0x10000 - sizeof(T));
        for (uint32_t i = 0; i < sizeof(T); i++)
            data[i] = value >> (i << 3);
        return;
    }

    // Forward the write to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioWrite<T>(id, address, value);

    // Catch writes to unmapped memory
    LOG_WARN("Unmapped ARM%d memory write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
}
