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
    Memory(Core *core): core(core) {};
    bool loadFiles();
    void loadOtp(FILE *file);

    template <typename T> T read(CpuId id, uint32_t address);
    template <typename T> void write(CpuId id, uint32_t address, T value);

private:
    Core *core;

    uint8_t fcram[0x8000000] = {}; // 128MB FCRAM
    uint8_t vram[0x600000] = {}; // 6MB VRAM
    uint8_t arm9Ram[0x100000] = {}; // 1MB ARM9 internal RAM
    uint8_t dspWram[0x80000] = {}; // 512KB DSP code/data RAM
    uint8_t axiWram[0x80000] = {}; // 512KB AXI WRAM
    uint8_t boot11[0x10000] = {}; // 64KB ARM11 boot ROM
    uint8_t boot9[0x10000] = {}; // 64KB ARM9 boot ROM
    uint8_t itcm[0x8000] = {}; // 32KB ARM9 ITCM
    uint8_t dtcm[0x4000] = {}; // 16KB ARM9 DTCM

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
