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

#include <cstring>
#include "core.h"

// Define an 8-bit register in an I/O switch statement
#define DEF_IO08(addr, func) \
    case addr + 0: \
        base -= addr; \
        size = 1; \
        func; \
        goto next;

// Define a 16-bit register in an I/O switch statement
#define DEF_IO16(addr, func) \
    case addr + 0: case addr + 1: \
        base -= addr; \
        size = 2; \
        func; \
        goto next;

// Define a 32-bit register in an I/O switch statement
#define DEF_IO32(addr, func) \
    case addr + 0: case addr + 1: \
    case addr + 2: case addr + 3: \
        base -= addr; \
        size = 4; \
        func; \
        goto next;

// Define shared parameters for I/O register writes
#define IO_PARAMS mask << (base << 3), data << (base << 3)
#define IO_PARAMS8 data << (base << 3)

template uint8_t Memory::readFallback(CpuId, uint32_t);
template uint16_t Memory::readFallback(CpuId, uint32_t);
template uint32_t Memory::readFallback(CpuId, uint32_t);
template void Memory::writeFallback(CpuId, uint32_t, uint8_t);
template void Memory::writeFallback(CpuId, uint32_t, uint16_t);
template void Memory::writeFallback(CpuId, uint32_t, uint32_t);

Memory::~Memory() {
    // Free extended FCRAM and VRAM
    delete[] fcramExt;
    delete[] vramExt;
}

bool Memory::init() {
    // Allocate extended FCRAM and VRAM if running in new 3DS mode
    if (core->n3dsMode) {
        fcramExt = new uint8_t[0x8000000];
        memset(fcramExt, 0, 0x8000000);
        vramExt = new uint8_t[0x400000];
        memset(vramExt, 0, 0x400000);
    }

    // Initialize the memory maps
    updateMap(false, 0x0, 0xFFFFFFFF);
    updateMap(true, 0x0, 0xFFFFFFFF);

    // Try to load the ARM11 boot ROM
    FILE *file = fopen(Settings::boot11Path.c_str(), "rb");
    if (!file) return false;
    fread(boot11, sizeof(uint8_t), 0x10000, file);
    fclose(file);

    // Try to load the ARM9 boot ROM
    file = fopen(Settings::boot9Path.c_str(), "rb");
    if (!file) return false;
    fread(boot9, sizeof(uint8_t), 0x10000, file);
    fclose(file);
    return true;
}

void Memory::loadOtp(FILE *file) {
    // Load encrypted OTP data from a file
    fread(otpEncrypted, sizeof(uint32_t), 0x40, file);
}

void Memory::updateMap(bool arm9, uint32_t start, uint32_t end) {
    // Update the ARM9 or ARM11 physical memory maps with 4KB pages
    bool extend = (core->interrupts.readCfg11MpClkcnt() & 0x70000);
    for (uint64_t address = start; address <= end; address += 0x1000) {
        // Look up and reset pointers for the current address
        uint8_t *&read = (arm9 ? readMap9 : readMap11)[address >> 12];
        uint8_t *&write = (arm9 ? writeMap9 : writeMap11)[address >> 12];
        read = write = nullptr;

        // Set a pointer to readable memory if it exists at the current address
        if (arm9 && address >= 0x8000000 && address < (cfg9Extmemcnt9 ? 0x8180000 : 0x8100000))
            read = &arm9Ram[address & 0x1FFFFF]; // 1.5MB ARM9 internal RAM
        else if (address >= 0x18000000 && address < 0x18600000)
            read = &vram[address & 0x7FFFFF]; // 6MB VRAM
        else if (!arm9 && (cfg11MpCnt & BIT(0)) && address >= 0x1F000000 && address < 0x1F400000)
            read = &vramExt[address & 0x3FFFFF]; // 4MB extended VRAM
        else if (address >= 0x1FF00000 && address < 0x1FF80000)
            read = &dspWram[address & 0x7FFFF]; // 512KB DSP code/data RAM
        else if (address >= 0x1FF80000 && address < 0x20000000)
            read = &axiWram[address & 0x7FFFF]; // 512KB AXI WRAM
        else if (address >= 0x20000000 && address < 0x28000000)
            read = &fcram[address & 0x7FFFFFF]; // 128MB FCRAM
        else if (extend && address >= 0x28000000 && address < 0x30000000)
            read = &fcramExt[address & 0x7FFFFFF]; // 128MB extended FCRAM
        else if (!arm9 && (address < 0x20000 || address >= 0xFFFF0000) && ((address & 0xF000) || !cfg11BrOverlayCnt))
            read = &boot11[address & 0xFFFF]; // 64KB ARM11 boot ROM
        else if (arm9 && address >= 0xFFFF0000)
            read = &boot9[address & 0xFFFF]; // 64KB ARM9 boot ROM

        // Set a pointer to writable memory if it exists at the current address
        if (arm9 && address >= 0x8000000 && address < (cfg9Extmemcnt9 ? 0x8180000 : 0x8100000))
            write = &arm9Ram[address & 0x1FFFFF]; // 1.5MB ARM9 internal RAM
        else if (address >= 0x18000000 && address < 0x18600000)
            write = &vram[address & 0x7FFFFF]; // 6MB VRAM
        else if (!arm9 && (cfg11MpCnt & BIT(0)) && address >= 0x1F000000 && address < 0x1F400000)
            write = &vramExt[address & 0x3FFFFF]; // 4MB extended VRAM
        else if (address >= 0x1FF00000 && address < 0x1FF80000)
            write = &dspWram[address & 0x7FFFF]; // 512KB DSP code/data RAM
        else if (address >= 0x1FF80000 && address < 0x20000000)
            write = &axiWram[address & 0x7FFFF]; // 512KB AXI WRAM
        else if (address >= 0x20000000 && address < 0x28000000)
            write = &fcram[address & 0x7FFFFFF]; // 128MB FCRAM
        else if (extend && address >= 0x28000000 && address < 0x30000000)
            write = &fcramExt[address & 0x7FFFFFF]; // 128MB extended FCRAM
    }
}

template <typename T> T Memory::readFallback(CpuId id, uint32_t address) {
    // Forward a read to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioRead<T>(id, address);

    // Handle the ARM11 bootrom overlay if reads to it have fallen through
    if (id != ARM9 && (address < 0x20000 || address >= 0xFFFF0000))
        return (address == *core->arms[id].registers[15]) ? 0xE59FF018 : cfg11BrOverlayVal;

    // Catch reads from unmapped memory
    if (id == ARM9)
        LOG_CRIT("Unmapped ARM9 memory read: 0x%X\n", address);
    else
        LOG_CRIT("Unmapped ARM11 core %d memory read: 0x%X\n", id, address);
    return 0;
}

template <typename T> void Memory::writeFallback(CpuId id, uint32_t address, T value) {
    // Forward a write to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioWrite<T>(id, address, value);

    // Catch writes to unmapped memory
    if (id == ARM9)
        LOG_CRIT("Unmapped ARM9 memory write: 0x%X\n", address);
    else
        LOG_CRIT("Unmapped ARM11 core %d memory write: 0x%X\n", id, address);
}

template <typename T> T Memory::ioRead(CpuId id, uint32_t address) {
    // Mirror the ARM11 DSP register area
    if (id != ARM9 && (address >> 12) == 0x10203)
        address &= 0xFFFFF03F;

    // Read a value from one or more I/O registers
    T value = 0;
    for (uint32_t i = 0; i < sizeof(T);) {
        uint32_t base = address + i, size, data;

        // Check registers that are shared between CPUs
        switch (base) {
            DEF_IO32(0x10101000, data = core->shas[0].readCnt()) // SHA_CNT11
            DEF_IO32(0x10101004, data = core->shas[0].readBlkcnt()) // SHA_BLKCNT11
            DEF_IO32(0x10101040, data = core->shas[0].readHash(0)) // SHA_HASH0_11
            DEF_IO32(0x10101044, data = core->shas[0].readHash(1)) // SHA_HASH1_11
            DEF_IO32(0x10101048, data = core->shas[0].readHash(2)) // SHA_HASH2_11
            DEF_IO32(0x1010104C, data = core->shas[0].readHash(3)) // SHA_HASH3_11
            DEF_IO32(0x10101050, data = core->shas[0].readHash(4)) // SHA_HASH4_11
            DEF_IO32(0x10101054, data = core->shas[0].readHash(5)) // SHA_HASH5_11
            DEF_IO32(0x10101058, data = core->shas[0].readHash(6)) // SHA_HASH6_11
            DEF_IO32(0x1010105C, data = core->shas[0].readHash(7)) // SHA_HASH7_11
            DEF_IO16(0x10103000, data = core->csnd.readMainVol()) // CSND_MAIN_VOL
            DEF_IO16(0x10103002, data = core->csnd.readMainCnt()) // CSND_MAIN_CNT
            DEF_IO16(0x10103400, data = core->csnd.readChanCnt(0)) // CSND_CHAN0_CNT
            DEF_IO16(0x10103404, data = core->csnd.readChanRvol(0)) // CSND_CHAN0_RVOL
            DEF_IO16(0x10103406, data = core->csnd.readChanLvol(0)) // CSND_CHAN0_LVOL
            DEF_IO32(0x1010340C, data = core->csnd.readChanStart(0)) // CSND_CHAN0_START
            DEF_IO16(0x10103420, data = core->csnd.readChanCnt(1)) // CSND_CHAN1_CNT
            DEF_IO16(0x10103424, data = core->csnd.readChanRvol(1)) // CSND_CHAN1_RVOL
            DEF_IO16(0x10103426, data = core->csnd.readChanLvol(1)) // CSND_CHAN1_LVOL
            DEF_IO32(0x1010342C, data = core->csnd.readChanStart(1)) // CSND_CHAN1_START
            DEF_IO16(0x10103440, data = core->csnd.readChanCnt(2)) // CSND_CHAN2_CNT
            DEF_IO16(0x10103444, data = core->csnd.readChanRvol(2)) // CSND_CHAN2_RVOL
            DEF_IO16(0x10103446, data = core->csnd.readChanLvol(2)) // CSND_CHAN2_LVOL
            DEF_IO32(0x1010344C, data = core->csnd.readChanStart(2)) // CSND_CHAN2_START
            DEF_IO16(0x10103460, data = core->csnd.readChanCnt(3)) // CSND_CHAN3_CNT
            DEF_IO16(0x10103464, data = core->csnd.readChanRvol(3)) // CSND_CHAN3_RVOL
            DEF_IO16(0x10103466, data = core->csnd.readChanLvol(3)) // CSND_CHAN3_LVOL
            DEF_IO32(0x1010346C, data = core->csnd.readChanStart(3)) // CSND_CHAN3_START
            DEF_IO16(0x10103480, data = core->csnd.readChanCnt(4)) // CSND_CHAN4_CNT
            DEF_IO16(0x10103484, data = core->csnd.readChanRvol(4)) // CSND_CHAN4_RVOL
            DEF_IO16(0x10103486, data = core->csnd.readChanLvol(4)) // CSND_CHAN4_LVOL
            DEF_IO32(0x1010348C, data = core->csnd.readChanStart(4)) // CSND_CHAN4_START
            DEF_IO16(0x101034A0, data = core->csnd.readChanCnt(5)) // CSND_CHAN5_CNT
            DEF_IO16(0x101034A4, data = core->csnd.readChanRvol(5)) // CSND_CHAN5_RVOL
            DEF_IO16(0x101034A6, data = core->csnd.readChanLvol(5)) // CSND_CHAN5_LVOL
            DEF_IO32(0x101034AC, data = core->csnd.readChanStart(5)) // CSND_CHAN5_START
            DEF_IO16(0x101034C0, data = core->csnd.readChanCnt(6)) // CSND_CHAN6_CNT
            DEF_IO16(0x101034C4, data = core->csnd.readChanRvol(6)) // CSND_CHAN6_RVOL
            DEF_IO16(0x101034C6, data = core->csnd.readChanLvol(6)) // CSND_CHAN6_LVOL
            DEF_IO32(0x101034CC, data = core->csnd.readChanStart(6)) // CSND_CHAN6_START
            DEF_IO16(0x101034E0, data = core->csnd.readChanCnt(7)) // CSND_CHAN7_CNT
            DEF_IO16(0x101034E4, data = core->csnd.readChanRvol(7)) // CSND_CHAN7_RVOL
            DEF_IO16(0x101034E6, data = core->csnd.readChanLvol(7)) // CSND_CHAN7_LVOL
            DEF_IO32(0x101034EC, data = core->csnd.readChanStart(7)) // CSND_CHAN7_START
            DEF_IO16(0x10103500, data = core->csnd.readChanCnt(8)) // CSND_CHAN8_CNT
            DEF_IO16(0x10103504, data = core->csnd.readChanRvol(8)) // CSND_CHAN8_RVOL
            DEF_IO16(0x10103506, data = core->csnd.readChanLvol(8)) // CSND_CHAN8_LVOL
            DEF_IO32(0x1010350C, data = core->csnd.readChanStart(8)) // CSND_CHAN8_START
            DEF_IO16(0x10103520, data = core->csnd.readChanCnt(9)) // CSND_CHAN9_CNT
            DEF_IO16(0x10103524, data = core->csnd.readChanRvol(9)) // CSND_CHAN9_RVOL
            DEF_IO16(0x10103526, data = core->csnd.readChanLvol(9)) // CSND_CHAN9_LVOL
            DEF_IO32(0x1010352C, data = core->csnd.readChanStart(9)) // CSND_CHAN9_START
            DEF_IO16(0x10103540, data = core->csnd.readChanCnt(10)) // CSND_CHAN10_CNT
            DEF_IO16(0x10103544, data = core->csnd.readChanRvol(10)) // CSND_CHAN10_RVOL
            DEF_IO16(0x10103546, data = core->csnd.readChanLvol(10)) // CSND_CHAN10_LVOL
            DEF_IO32(0x1010354C, data = core->csnd.readChanStart(10)) // CSND_CHAN10_START
            DEF_IO16(0x10103560, data = core->csnd.readChanCnt(11)) // CSND_CHAN11_CNT
            DEF_IO16(0x10103564, data = core->csnd.readChanRvol(11)) // CSND_CHAN11_RVOL
            DEF_IO16(0x10103566, data = core->csnd.readChanLvol(11)) // CSND_CHAN11_LVOL
            DEF_IO32(0x1010356C, data = core->csnd.readChanStart(11)) // CSND_CHAN11_START
            DEF_IO16(0x10103580, data = core->csnd.readChanCnt(12)) // CSND_CHAN12_CNT
            DEF_IO16(0x10103584, data = core->csnd.readChanRvol(12)) // CSND_CHAN12_RVOL
            DEF_IO16(0x10103586, data = core->csnd.readChanLvol(12)) // CSND_CHAN12_LVOL
            DEF_IO32(0x1010358C, data = core->csnd.readChanStart(12)) // CSND_CHAN12_START
            DEF_IO16(0x101035A0, data = core->csnd.readChanCnt(13)) // CSND_CHAN13_CNT
            DEF_IO16(0x101035A4, data = core->csnd.readChanRvol(13)) // CSND_CHAN13_RVOL
            DEF_IO16(0x101035A6, data = core->csnd.readChanLvol(13)) // CSND_CHAN13_LVOL
            DEF_IO32(0x101035AC, data = core->csnd.readChanStart(13)) // CSND_CHAN13_START
            DEF_IO16(0x101035C0, data = core->csnd.readChanCnt(14)) // CSND_CHAN14_CNT
            DEF_IO16(0x101035C4, data = core->csnd.readChanRvol(14)) // CSND_CHAN14_RVOL
            DEF_IO16(0x101035C6, data = core->csnd.readChanLvol(14)) // CSND_CHAN14_LVOL
            DEF_IO32(0x101035CC, data = core->csnd.readChanStart(14)) // CSND_CHAN14_START
            DEF_IO16(0x101035E0, data = core->csnd.readChanCnt(15)) // CSND_CHAN15_CNT
            DEF_IO16(0x101035E4, data = core->csnd.readChanRvol(15)) // CSND_CHAN15_RVOL
            DEF_IO16(0x101035E6, data = core->csnd.readChanLvol(15)) // CSND_CHAN15_LVOL
            DEF_IO32(0x101035EC, data = core->csnd.readChanStart(15)) // CSND_CHAN15_START
            DEF_IO16(0x10103600, data = core->csnd.readChanCnt(16)) // CSND_CHAN16_CNT
            DEF_IO16(0x10103604, data = core->csnd.readChanRvol(16)) // CSND_CHAN16_RVOL
            DEF_IO16(0x10103606, data = core->csnd.readChanLvol(16)) // CSND_CHAN16_LVOL
            DEF_IO32(0x1010360C, data = core->csnd.readChanStart(16)) // CSND_CHAN16_START
            DEF_IO16(0x10103620, data = core->csnd.readChanCnt(17)) // CSND_CHAN17_CNT
            DEF_IO16(0x10103624, data = core->csnd.readChanRvol(17)) // CSND_CHAN17_RVOL
            DEF_IO16(0x10103626, data = core->csnd.readChanLvol(17)) // CSND_CHAN17_LVOL
            DEF_IO32(0x1010362C, data = core->csnd.readChanStart(17)) // CSND_CHAN17_START
            DEF_IO16(0x10103640, data = core->csnd.readChanCnt(18)) // CSND_CHAN18_CNT
            DEF_IO16(0x10103644, data = core->csnd.readChanRvol(18)) // CSND_CHAN18_RVOL
            DEF_IO16(0x10103646, data = core->csnd.readChanLvol(18)) // CSND_CHAN18_LVOL
            DEF_IO32(0x1010364C, data = core->csnd.readChanStart(18)) // CSND_CHAN18_START
            DEF_IO16(0x10103660, data = core->csnd.readChanCnt(19)) // CSND_CHAN19_CNT
            DEF_IO16(0x10103664, data = core->csnd.readChanRvol(19)) // CSND_CHAN19_RVOL
            DEF_IO16(0x10103666, data = core->csnd.readChanLvol(19)) // CSND_CHAN19_LVOL
            DEF_IO32(0x1010366C, data = core->csnd.readChanStart(19)) // CSND_CHAN19_START
            DEF_IO16(0x10103680, data = core->csnd.readChanCnt(20)) // CSND_CHAN20_CNT
            DEF_IO16(0x10103684, data = core->csnd.readChanRvol(20)) // CSND_CHAN20_RVOL
            DEF_IO16(0x10103686, data = core->csnd.readChanLvol(20)) // CSND_CHAN20_LVOL
            DEF_IO32(0x1010368C, data = core->csnd.readChanStart(20)) // CSND_CHAN20_START
            DEF_IO16(0x101036A0, data = core->csnd.readChanCnt(21)) // CSND_CHAN21_CNT
            DEF_IO16(0x101036A4, data = core->csnd.readChanRvol(21)) // CSND_CHAN21_RVOL
            DEF_IO16(0x101036A6, data = core->csnd.readChanLvol(21)) // CSND_CHAN21_LVOL
            DEF_IO32(0x101036AC, data = core->csnd.readChanStart(21)) // CSND_CHAN21_START
            DEF_IO16(0x101036C0, data = core->csnd.readChanCnt(22)) // CSND_CHAN22_CNT
            DEF_IO16(0x101036C4, data = core->csnd.readChanRvol(22)) // CSND_CHAN22_RVOL
            DEF_IO16(0x101036C6, data = core->csnd.readChanLvol(22)) // CSND_CHAN22_LVOL
            DEF_IO32(0x101036CC, data = core->csnd.readChanStart(22)) // CSND_CHAN22_START
            DEF_IO16(0x101036E0, data = core->csnd.readChanCnt(23)) // CSND_CHAN23_CNT
            DEF_IO16(0x101036E4, data = core->csnd.readChanRvol(23)) // CSND_CHAN23_RVOL
            DEF_IO16(0x101036E6, data = core->csnd.readChanLvol(23)) // CSND_CHAN23_LVOL
            DEF_IO32(0x101036EC, data = core->csnd.readChanStart(23)) // CSND_CHAN23_START
            DEF_IO16(0x10103700, data = core->csnd.readChanCnt(24)) // CSND_CHAN24_CNT
            DEF_IO16(0x10103704, data = core->csnd.readChanRvol(24)) // CSND_CHAN24_RVOL
            DEF_IO16(0x10103706, data = core->csnd.readChanLvol(24)) // CSND_CHAN24_LVOL
            DEF_IO32(0x1010370C, data = core->csnd.readChanStart(24)) // CSND_CHAN24_START
            DEF_IO16(0x10103720, data = core->csnd.readChanCnt(25)) // CSND_CHAN25_CNT
            DEF_IO16(0x10103724, data = core->csnd.readChanRvol(25)) // CSND_CHAN25_RVOL
            DEF_IO16(0x10103726, data = core->csnd.readChanLvol(25)) // CSND_CHAN25_LVOL
            DEF_IO32(0x1010372C, data = core->csnd.readChanStart(25)) // CSND_CHAN25_START
            DEF_IO16(0x10103740, data = core->csnd.readChanCnt(26)) // CSND_CHAN26_CNT
            DEF_IO16(0x10103744, data = core->csnd.readChanRvol(26)) // CSND_CHAN26_RVOL
            DEF_IO16(0x10103746, data = core->csnd.readChanLvol(26)) // CSND_CHAN26_LVOL
            DEF_IO32(0x1010374C, data = core->csnd.readChanStart(26)) // CSND_CHAN26_START
            DEF_IO16(0x10103760, data = core->csnd.readChanCnt(27)) // CSND_CHAN27_CNT
            DEF_IO16(0x10103764, data = core->csnd.readChanRvol(27)) // CSND_CHAN27_RVOL
            DEF_IO16(0x10103766, data = core->csnd.readChanLvol(27)) // CSND_CHAN27_LVOL
            DEF_IO32(0x1010376C, data = core->csnd.readChanStart(27)) // CSND_CHAN27_START
            DEF_IO16(0x10103780, data = core->csnd.readChanCnt(28)) // CSND_CHAN28_CNT
            DEF_IO16(0x10103784, data = core->csnd.readChanRvol(28)) // CSND_CHAN28_RVOL
            DEF_IO16(0x10103786, data = core->csnd.readChanLvol(28)) // CSND_CHAN28_LVOL
            DEF_IO32(0x1010378C, data = core->csnd.readChanStart(28)) // CSND_CHAN28_START
            DEF_IO16(0x101037A0, data = core->csnd.readChanCnt(29)) // CSND_CHAN29_CNT
            DEF_IO16(0x101037A4, data = core->csnd.readChanRvol(29)) // CSND_CHAN29_RVOL
            DEF_IO16(0x101037A6, data = core->csnd.readChanLvol(29)) // CSND_CHAN29_LVOL
            DEF_IO32(0x101037AC, data = core->csnd.readChanStart(29)) // CSND_CHAN29_START
            DEF_IO16(0x101037C0, data = core->csnd.readChanCnt(30)) // CSND_CHAN30_CNT
            DEF_IO16(0x101037C4, data = core->csnd.readChanRvol(30)) // CSND_CHAN30_RVOL
            DEF_IO16(0x101037C6, data = core->csnd.readChanLvol(30)) // CSND_CHAN30_LVOL
            DEF_IO32(0x101037CC, data = core->csnd.readChanStart(30)) // CSND_CHAN30_START
            DEF_IO16(0x101037E0, data = core->csnd.readChanCnt(31)) // CSND_CHAN31_CNT
            DEF_IO16(0x101037E4, data = core->csnd.readChanRvol(31)) // CSND_CHAN31_RVOL
            DEF_IO16(0x101037E6, data = core->csnd.readChanLvol(31)) // CSND_CHAN31_LVOL
            DEF_IO32(0x101037EC, data = core->csnd.readChanStart(31)) // CSND_CHAN31_START
            DEF_IO16(0x10122000, data = core->wifi.readCmd()) // WIFI_CMD
            DEF_IO32(0x10122004, data = core->wifi.readCmdParam()) // WIFI_CMD_PARAM
            DEF_IO32(0x1012200C, data = core->wifi.readResponse(0)) // WIFI_RESPONSE0
            DEF_IO32(0x10122010, data = core->wifi.readResponse(1)) // WIFI_RESPONSE1
            DEF_IO32(0x10122014, data = core->wifi.readResponse(2)) // WIFI_RESPONSE2
            DEF_IO32(0x10122018, data = core->wifi.readResponse(3)) // WIFI_RESPONSE3
            DEF_IO32(0x1012201C, data = core->wifi.readIrqStatus()) // WIFI_IRQ_STATUS
            DEF_IO32(0x10122020, data = core->wifi.readIrqMask()) // WIFI_IRQ_MASK
            DEF_IO16(0x10122026, data = core->wifi.readData16Blklen()) // WIFI_DATA16_BLKLEN
            DEF_IO32(0x1012202C, data = core->wifi.readErrDetail()) // WIFI_ERR_DETAIL
            DEF_IO16(0x10122030, data = core->wifi.readData16Fifo()) // WIFI_DATA16_FIFO
            DEF_IO16(0x101220D8, data = core->wifi.readDataCtl()) // WIFI_DATA_CTL
            DEF_IO16(0x10122100, data = core->wifi.readData32Irq()) // WIFI_DATA32_IRQ
            DEF_IO16(0x10122104, data = core->wifi.readData32Blklen()) // WIFI_DATA32_BLKLEN
            DEF_IO32(0x10140420, data = readCfg11BrOverlayCnt()) // CFG11_BR_OVERLAY_CNT
            DEF_IO32(0x10140424, data = readCfg11BrOverlayVal()) // CFG11_BR_OVERLAY_VAL
            DEF_IO16(0x10140FFC, data = core->interrupts.readCfg11Socinfo()) // CFG11_SOCINFO
            DEF_IO32(0x10141200, data = core->gpu.readCfg11GpuCnt()) // CFG11_GPU_CNT
            DEF_IO32(0x10141300, data = core->interrupts.readCfg11MpClkcnt()) // CFG11_MP_CLKCNT
            DEF_IO32(0x10141304, data = readCfg11MpCnt()) // CFG11_MP_CNT
            DEF_IO08(0x10141310, data = core->interrupts.readCfg11MpBootcnt(0)) // CFG11_MP_BOOTCNT0
            DEF_IO08(0x10141311, data = core->interrupts.readCfg11MpBootcnt(1)) // CFG11_MP_BOOTCNT1
            DEF_IO08(0x10141312, data = core->interrupts.readCfg11MpBootcnt(2)) // CFG11_MP_BOOTCNT2
            DEF_IO08(0x10141313, data = core->interrupts.readCfg11MpBootcnt(3)) // CFG11_MP_BOOTCNT3
            DEF_IO08(0x10144000, data = core->i2c.readBusData(1)) // I2C_BUS1_DATA
            DEF_IO08(0x10144001, data = core->i2c.readBusCnt(1)) // I2C_BUS1_CNT
            DEF_IO16(0x10146000, data = core->input.readHidPad()) // HID_PAD
            DEF_IO08(0x10148000, data = core->i2c.readBusData(2)) // I2C_BUS2_DATA
            DEF_IO08(0x10148001, data = core->i2c.readBusCnt(2)) // I2C_BUS2_CNT
            DEF_IO08(0x10161000, data = core->i2c.readBusData(0)) // I2C_BUS0_DATA
            DEF_IO08(0x10161001, data = core->i2c.readBusCnt(0)) // I2C_BUS0_CNT
            DEF_IO32(0x10163000, data = core->pxi.readSync(0)) // PXI_SYNC11
            DEF_IO32(0x10163004, data = core->pxi.readCnt(0)) // PXI_CNT11
            DEF_IO32(0x1016300C, data = core->pxi.readRecv(0)) // PXI_RECV11
            DEF_IO16(0x10164000, data = core->cartridge.readNtrMcnt()) // NTRCARD_MCNT
            DEF_IO32(0x10164004, data = core->cartridge.readNtrRomcnt()) // NTRCARD_ROMCNT
            DEF_IO32(0x1016401C, data = core->cartridge.readNtrData()) // NTRCARD_DATA
        }

        // Check registers that are exclusive to one CPU
        if (id != ARM9) { // ARM11
            switch (base) {
                DEF_IO32(0x10200000, data = core->cdmas[CDMA0].readCsr(8)) // CDMA0_DSR
                DEF_IO32(0x10200004, data = core->cdmas[CDMA0].readCpc(8)) // CDMA0_DPC
                DEF_IO32(0x10200020, data = core->cdmas[CDMA0].readInten()) // CDMA0_INTEN
                DEF_IO32(0x10200024, data = core->cdmas[CDMA0].readIntEventRis()) // CDMA0_INT_EVENT_RIS
                DEF_IO32(0x10200028, data = core->cdmas[CDMA0].readIntmis()) // CDMA0_INTMIS
                DEF_IO32(0x10200030, data = core->cdmas[CDMA0].readFsrd()) // CDMA0_FSRD
                DEF_IO32(0x10200034, data = core->cdmas[CDMA0].readFsrc()) // CDMA0_FSRC
                DEF_IO32(0x10200038, data = core->cdmas[CDMA0].readFtr(8)) // CDMA0_FTRD
                DEF_IO32(0x10200040, data = core->cdmas[CDMA0].readFtr(0)) // CDMA0_FTR0
                DEF_IO32(0x10200044, data = core->cdmas[CDMA0].readFtr(1)) // CDMA0_FTR1
                DEF_IO32(0x10200048, data = core->cdmas[CDMA0].readFtr(2)) // CDMA0_FTR2
                DEF_IO32(0x1020004C, data = core->cdmas[CDMA0].readFtr(3)) // CDMA0_FTR3
                DEF_IO32(0x10200050, data = core->cdmas[CDMA0].readFtr(4)) // CDMA0_FTR4
                DEF_IO32(0x10200054, data = core->cdmas[CDMA0].readFtr(5)) // CDMA0_FTR5
                DEF_IO32(0x10200058, data = core->cdmas[CDMA0].readFtr(6)) // CDMA0_FTR6
                DEF_IO32(0x1020005C, data = core->cdmas[CDMA0].readFtr(7)) // CDMA0_FTR7
                DEF_IO32(0x10200100, data = core->cdmas[CDMA0].readCsr(0)) // CDMA0_CSR0
                DEF_IO32(0x10200104, data = core->cdmas[CDMA0].readCpc(0)) // CDMA0_CPC0
                DEF_IO32(0x10200108, data = core->cdmas[CDMA0].readCsr(1)) // CDMA0_CSR1
                DEF_IO32(0x1020010C, data = core->cdmas[CDMA0].readCpc(1)) // CDMA0_CPC1
                DEF_IO32(0x10200110, data = core->cdmas[CDMA0].readCsr(2)) // CDMA0_CSR2
                DEF_IO32(0x10200114, data = core->cdmas[CDMA0].readCpc(2)) // CDMA0_CPC2
                DEF_IO32(0x10200118, data = core->cdmas[CDMA0].readCsr(3)) // CDMA0_CSR3
                DEF_IO32(0x1020011C, data = core->cdmas[CDMA0].readCpc(3)) // CDMA0_CPC3
                DEF_IO32(0x10200120, data = core->cdmas[CDMA0].readCsr(4)) // CDMA0_CSR4
                DEF_IO32(0x10200124, data = core->cdmas[CDMA0].readCpc(4)) // CDMA0_CPC4
                DEF_IO32(0x10200128, data = core->cdmas[CDMA0].readCsr(5)) // CDMA0_CSR5
                DEF_IO32(0x1020012C, data = core->cdmas[CDMA0].readCpc(5)) // CDMA0_CPC5
                DEF_IO32(0x10200130, data = core->cdmas[CDMA0].readCsr(6)) // CDMA0_CSR6
                DEF_IO32(0x10200134, data = core->cdmas[CDMA0].readCpc(6)) // CDMA0_CPC6
                DEF_IO32(0x10200138, data = core->cdmas[CDMA0].readCsr(7)) // CDMA0_CSR7
                DEF_IO32(0x1020013C, data = core->cdmas[CDMA0].readCpc(7)) // CDMA0_CPC7
                DEF_IO32(0x10200400, data = core->cdmas[CDMA0].readSar(0)) // CDMA0_SAR0
                DEF_IO32(0x10200404, data = core->cdmas[CDMA0].readDar(0)) // CDMA0_DAR0
                DEF_IO32(0x10200408, data = core->cdmas[CDMA0].readCcr(0)) // CDMA0_CCR0
                DEF_IO32(0x1020040C, data = core->cdmas[CDMA0].readLc0(0)) // CDMA0_LC0_0
                DEF_IO32(0x10200410, data = core->cdmas[CDMA0].readLc1(0)) // CDMA0_LC1_0
                DEF_IO32(0x10200420, data = core->cdmas[CDMA0].readSar(1)) // CDMA0_SAR1
                DEF_IO32(0x10200424, data = core->cdmas[CDMA0].readDar(1)) // CDMA0_DAR1
                DEF_IO32(0x10200428, data = core->cdmas[CDMA0].readCcr(1)) // CDMA0_CCR1
                DEF_IO32(0x1020042C, data = core->cdmas[CDMA0].readLc0(1)) // CDMA0_LC0_1
                DEF_IO32(0x10200430, data = core->cdmas[CDMA0].readLc1(1)) // CDMA0_LC1_1
                DEF_IO32(0x10200440, data = core->cdmas[CDMA0].readSar(2)) // CDMA0_SAR2
                DEF_IO32(0x10200444, data = core->cdmas[CDMA0].readDar(2)) // CDMA0_DAR2
                DEF_IO32(0x10200448, data = core->cdmas[CDMA0].readCcr(2)) // CDMA0_CCR2
                DEF_IO32(0x1020044C, data = core->cdmas[CDMA0].readLc0(2)) // CDMA0_LC0_2
                DEF_IO32(0x10200450, data = core->cdmas[CDMA0].readLc1(2)) // CDMA0_LC1_2
                DEF_IO32(0x10200460, data = core->cdmas[CDMA0].readSar(3)) // CDMA0_SAR3
                DEF_IO32(0x10200464, data = core->cdmas[CDMA0].readDar(3)) // CDMA0_DAR3
                DEF_IO32(0x10200468, data = core->cdmas[CDMA0].readCcr(3)) // CDMA0_CCR3
                DEF_IO32(0x1020046C, data = core->cdmas[CDMA0].readLc0(3)) // CDMA0_LC0_3
                DEF_IO32(0x10200470, data = core->cdmas[CDMA0].readLc1(3)) // CDMA0_LC1_3
                DEF_IO32(0x10200480, data = core->cdmas[CDMA0].readSar(4)) // CDMA0_SAR4
                DEF_IO32(0x10200484, data = core->cdmas[CDMA0].readDar(4)) // CDMA0_DAR4
                DEF_IO32(0x10200488, data = core->cdmas[CDMA0].readCcr(4)) // CDMA0_CCR4
                DEF_IO32(0x1020048C, data = core->cdmas[CDMA0].readLc0(4)) // CDMA0_LC0_4
                DEF_IO32(0x10200490, data = core->cdmas[CDMA0].readLc1(4)) // CDMA0_LC1_4
                DEF_IO32(0x102004A0, data = core->cdmas[CDMA0].readSar(5)) // CDMA0_SAR5
                DEF_IO32(0x102004A4, data = core->cdmas[CDMA0].readDar(5)) // CDMA0_DAR5
                DEF_IO32(0x102004A8, data = core->cdmas[CDMA0].readCcr(5)) // CDMA0_CCR5
                DEF_IO32(0x102004AC, data = core->cdmas[CDMA0].readLc0(5)) // CDMA0_LC0_5
                DEF_IO32(0x102004B0, data = core->cdmas[CDMA0].readLc1(5)) // CDMA0_LC1_5
                DEF_IO32(0x102004C0, data = core->cdmas[CDMA0].readSar(6)) // CDMA0_SAR6
                DEF_IO32(0x102004C4, data = core->cdmas[CDMA0].readDar(6)) // CDMA0_DAR6
                DEF_IO32(0x102004C8, data = core->cdmas[CDMA0].readCcr(6)) // CDMA0_CCR6
                DEF_IO32(0x102004CC, data = core->cdmas[CDMA0].readLc0(6)) // CDMA0_LC0_6
                DEF_IO32(0x102004D0, data = core->cdmas[CDMA0].readLc1(6)) // CDMA0_LC1_6
                DEF_IO32(0x102004E0, data = core->cdmas[CDMA0].readSar(7)) // CDMA0_SAR7
                DEF_IO32(0x102004E4, data = core->cdmas[CDMA0].readDar(7)) // CDMA0_DAR7
                DEF_IO32(0x102004E8, data = core->cdmas[CDMA0].readCcr(7)) // CDMA0_CCR7
                DEF_IO32(0x102004EC, data = core->cdmas[CDMA0].readLc0(7)) // CDMA0_LC0_7
                DEF_IO32(0x102004F0, data = core->cdmas[CDMA0].readLc1(7)) // CDMA0_LC1_7
                DEF_IO32(0x10200D00, data = core->cdmas[CDMA0].readDbgstatus()) // CDMA0_DBGSTATUS
                DEF_IO32(0x10200D08, data = core->cdmas[CDMA0].readDbginst0()) // CDMA0_DBGINST0
                DEF_IO32(0x10200D0C, data = core->cdmas[CDMA0].readDbginst1()) // CDMA0_DBGINST1
                DEF_IO16(0x10203000, data = core->dsp.readPdata()) // DSP_PDATA
                DEF_IO16(0x10203008, data = core->dsp.readPcfg()) // DSP_PCFG
                DEF_IO16(0x1020300C, data = core->dsp.readPsts()) // DSP_PSTS
                DEF_IO16(0x10203010, data = core->dsp.readPsem()) // DSP_PSEM
                DEF_IO16(0x10203014, data = core->dsp.readPmask()) // DSP_PMASK
                DEF_IO16(0x1020301C, data = core->dsp.readSem()) // DSP_SEM
                DEF_IO16(0x10203020, data = core->dsp.readCmd(0)) // DSP_CMD0
                DEF_IO16(0x10203024, data = core->dsp.readRep(0)) // DSP_REP0
                DEF_IO16(0x10203028, data = core->dsp.readCmd(1)) // DSP_CMD1
                DEF_IO16(0x1020302C, data = core->dsp.readRep(1)) // DSP_REP1
                DEF_IO16(0x10203030, data = core->dsp.readCmd(2)) // DSP_CMD2
                DEF_IO16(0x10203034, data = core->dsp.readRep(2)) // DSP_REP2
                DEF_IO32(0x10206000, data = core->cdmas[CDMA1].readCsr(8)) // CDMA1_DSR
                DEF_IO32(0x10206004, data = core->cdmas[CDMA1].readCpc(8)) // CDMA1_DPC
                DEF_IO32(0x10206020, data = core->cdmas[CDMA1].readInten()) // CDMA1_INTEN
                DEF_IO32(0x10206024, data = core->cdmas[CDMA1].readIntEventRis()) // CDMA1_INT_EVENT_RIS
                DEF_IO32(0x10206028, data = core->cdmas[CDMA1].readIntmis()) // CDMA1_INTMIS
                DEF_IO32(0x10206030, data = core->cdmas[CDMA1].readFsrd()) // CDMA1_FSRD
                DEF_IO32(0x10206034, data = core->cdmas[CDMA1].readFsrc()) // CDMA1_FSRC
                DEF_IO32(0x10206038, data = core->cdmas[CDMA1].readFtr(8)) // CDMA1_FTRD
                DEF_IO32(0x10206040, data = core->cdmas[CDMA1].readFtr(0)) // CDMA1_FTR0
                DEF_IO32(0x10206044, data = core->cdmas[CDMA1].readFtr(1)) // CDMA1_FTR1
                DEF_IO32(0x10206048, data = core->cdmas[CDMA1].readFtr(2)) // CDMA1_FTR2
                DEF_IO32(0x1020604C, data = core->cdmas[CDMA1].readFtr(3)) // CDMA1_FTR3
                DEF_IO32(0x10206050, data = core->cdmas[CDMA1].readFtr(4)) // CDMA1_FTR4
                DEF_IO32(0x10206054, data = core->cdmas[CDMA1].readFtr(5)) // CDMA1_FTR5
                DEF_IO32(0x10206058, data = core->cdmas[CDMA1].readFtr(6)) // CDMA1_FTR6
                DEF_IO32(0x1020605C, data = core->cdmas[CDMA1].readFtr(7)) // CDMA1_FTR7
                DEF_IO32(0x10206100, data = core->cdmas[CDMA1].readCsr(0)) // CDMA1_CSR0
                DEF_IO32(0x10206104, data = core->cdmas[CDMA1].readCpc(0)) // CDMA1_CPC0
                DEF_IO32(0x10206108, data = core->cdmas[CDMA1].readCsr(1)) // CDMA1_CSR1
                DEF_IO32(0x1020610C, data = core->cdmas[CDMA1].readCpc(1)) // CDMA1_CPC1
                DEF_IO32(0x10206110, data = core->cdmas[CDMA1].readCsr(2)) // CDMA1_CSR2
                DEF_IO32(0x10206114, data = core->cdmas[CDMA1].readCpc(2)) // CDMA1_CPC2
                DEF_IO32(0x10206118, data = core->cdmas[CDMA1].readCsr(3)) // CDMA1_CSR3
                DEF_IO32(0x1020611C, data = core->cdmas[CDMA1].readCpc(3)) // CDMA1_CPC3
                DEF_IO32(0x10206120, data = core->cdmas[CDMA1].readCsr(4)) // CDMA1_CSR4
                DEF_IO32(0x10206124, data = core->cdmas[CDMA1].readCpc(4)) // CDMA1_CPC4
                DEF_IO32(0x10206128, data = core->cdmas[CDMA1].readCsr(5)) // CDMA1_CSR5
                DEF_IO32(0x1020612C, data = core->cdmas[CDMA1].readCpc(5)) // CDMA1_CPC5
                DEF_IO32(0x10206130, data = core->cdmas[CDMA1].readCsr(6)) // CDMA1_CSR6
                DEF_IO32(0x10206134, data = core->cdmas[CDMA1].readCpc(6)) // CDMA1_CPC6
                DEF_IO32(0x10206138, data = core->cdmas[CDMA1].readCsr(7)) // CDMA1_CSR7
                DEF_IO32(0x1020613C, data = core->cdmas[CDMA1].readCpc(7)) // CDMA1_CPC7
                DEF_IO32(0x10206400, data = core->cdmas[CDMA1].readSar(0)) // CDMA1_SAR0
                DEF_IO32(0x10206404, data = core->cdmas[CDMA1].readDar(0)) // CDMA1_DAR0
                DEF_IO32(0x10206408, data = core->cdmas[CDMA1].readCcr(0)) // CDMA1_CCR0
                DEF_IO32(0x1020640C, data = core->cdmas[CDMA1].readLc0(0)) // CDMA1_LC0_0
                DEF_IO32(0x10206410, data = core->cdmas[CDMA1].readLc1(0)) // CDMA1_LC1_0
                DEF_IO32(0x10206420, data = core->cdmas[CDMA1].readSar(1)) // CDMA1_SAR1
                DEF_IO32(0x10206424, data = core->cdmas[CDMA1].readDar(1)) // CDMA1_DAR1
                DEF_IO32(0x10206428, data = core->cdmas[CDMA1].readCcr(1)) // CDMA1_CCR1
                DEF_IO32(0x1020642C, data = core->cdmas[CDMA1].readLc0(1)) // CDMA1_LC0_1
                DEF_IO32(0x10206430, data = core->cdmas[CDMA1].readLc1(1)) // CDMA1_LC1_1
                DEF_IO32(0x10206440, data = core->cdmas[CDMA1].readSar(2)) // CDMA1_SAR2
                DEF_IO32(0x10206444, data = core->cdmas[CDMA1].readDar(2)) // CDMA1_DAR2
                DEF_IO32(0x10206448, data = core->cdmas[CDMA1].readCcr(2)) // CDMA1_CCR2
                DEF_IO32(0x1020644C, data = core->cdmas[CDMA1].readLc0(2)) // CDMA1_LC0_2
                DEF_IO32(0x10206450, data = core->cdmas[CDMA1].readLc1(2)) // CDMA1_LC1_2
                DEF_IO32(0x10206460, data = core->cdmas[CDMA1].readSar(3)) // CDMA1_SAR3
                DEF_IO32(0x10206464, data = core->cdmas[CDMA1].readDar(3)) // CDMA1_DAR3
                DEF_IO32(0x10206468, data = core->cdmas[CDMA1].readCcr(3)) // CDMA1_CCR3
                DEF_IO32(0x1020646C, data = core->cdmas[CDMA1].readLc0(3)) // CDMA1_LC0_3
                DEF_IO32(0x10206470, data = core->cdmas[CDMA1].readLc1(3)) // CDMA1_LC1_3
                DEF_IO32(0x10206480, data = core->cdmas[CDMA1].readSar(4)) // CDMA1_SAR4
                DEF_IO32(0x10206484, data = core->cdmas[CDMA1].readDar(4)) // CDMA1_DAR4
                DEF_IO32(0x10206488, data = core->cdmas[CDMA1].readCcr(4)) // CDMA1_CCR4
                DEF_IO32(0x1020648C, data = core->cdmas[CDMA1].readLc0(4)) // CDMA1_LC0_4
                DEF_IO32(0x10206490, data = core->cdmas[CDMA1].readLc1(4)) // CDMA1_LC1_4
                DEF_IO32(0x102064A0, data = core->cdmas[CDMA1].readSar(5)) // CDMA1_SAR5
                DEF_IO32(0x102064A4, data = core->cdmas[CDMA1].readDar(5)) // CDMA1_DAR5
                DEF_IO32(0x102064A8, data = core->cdmas[CDMA1].readCcr(5)) // CDMA1_CCR5
                DEF_IO32(0x102064AC, data = core->cdmas[CDMA1].readLc0(5)) // CDMA1_LC0_5
                DEF_IO32(0x102064B0, data = core->cdmas[CDMA1].readLc1(5)) // CDMA1_LC1_5
                DEF_IO32(0x102064C0, data = core->cdmas[CDMA1].readSar(6)) // CDMA1_SAR6
                DEF_IO32(0x102064C4, data = core->cdmas[CDMA1].readDar(6)) // CDMA1_DAR6
                DEF_IO32(0x102064C8, data = core->cdmas[CDMA1].readCcr(6)) // CDMA1_CCR6
                DEF_IO32(0x102064CC, data = core->cdmas[CDMA1].readLc0(6)) // CDMA1_LC0_6
                DEF_IO32(0x102064D0, data = core->cdmas[CDMA1].readLc1(6)) // CDMA1_LC1_6
                DEF_IO32(0x102064E0, data = core->cdmas[CDMA1].readSar(7)) // CDMA1_SAR7
                DEF_IO32(0x102064E4, data = core->cdmas[CDMA1].readDar(7)) // CDMA1_DAR7
                DEF_IO32(0x102064E8, data = core->cdmas[CDMA1].readCcr(7)) // CDMA1_CCR7
                DEF_IO32(0x102064EC, data = core->cdmas[CDMA1].readLc0(7)) // CDMA1_LC0_7
                DEF_IO32(0x102064F0, data = core->cdmas[CDMA1].readLc1(7)) // CDMA1_LC1_7
                DEF_IO32(0x10206D00, data = core->cdmas[CDMA1].readDbgstatus()) // CDMA1_DBGSTATUS
                DEF_IO32(0x10206D08, data = core->cdmas[CDMA1].readDbginst0()) // CDMA1_DBGINST0
                DEF_IO32(0x10206D0C, data = core->cdmas[CDMA1].readDbginst1()) // CDMA1_DBGINST1
                DEF_IO32(0x10301000, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301004, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301008, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x1030100C, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301010, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301014, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301018, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x1030101C, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301020, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301024, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301028, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x1030102C, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301030, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301034, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10301038, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x1030103C, data = core->shas[0].readFifo()) // SHA_FIFO11
                DEF_IO32(0x10322000, data = core->wifi.readData32Fifo()) // WIFI_DATA32_FIFO
                DEF_IO32(0x10400010, data = core->gpu.readMemsetDstAddr(0)) // GPU_MEMSET_DST_ADDR0
                DEF_IO32(0x10400014, data = core->gpu.readMemsetDstEnd(0)) // GPU_MEMSET_DST_END0
                DEF_IO32(0x10400018, data = core->gpu.readMemsetData(0)) // GPU_MEMSET_DATA0
                DEF_IO32(0x1040001C, data = core->gpu.readMemsetCnt(0)) // GPU_MEMSET_CNT0
                DEF_IO32(0x10400020, data = core->gpu.readMemsetDstAddr(1)) // GPU_MEMSET_DST_ADDR1
                DEF_IO32(0x10400024, data = core->gpu.readMemsetDstEnd(1)) // GPU_MEMSET_DST_END1
                DEF_IO32(0x10400028, data = core->gpu.readMemsetData(1)) // GPU_MEMSET_DATA1
                DEF_IO32(0x1040002C, data = core->gpu.readMemsetCnt(1)) // GPU_MEMSET_CNT1
                DEF_IO32(0x10400468, data = core->pdc.readFramebufLt0(0)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x1040046C, data = core->pdc.readFramebufLt1(0)) // PDC0_FRAMEBUF_LT1
                DEF_IO32(0x10400470, data = core->pdc.readFramebufFormat(0)) // PDC0_FRAMEBUF_FORMAT
                DEF_IO32(0x10400474, data = core->pdc.readInterruptType(0)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400478, data = core->pdc.readFramebufSelAck(0)) // PDC0_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400568, data = core->pdc.readFramebufLt0(1)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x1040056C, data = core->pdc.readFramebufLt1(1)) // PDC1_FRAMEBUF_LT1
                DEF_IO32(0x10400570, data = core->pdc.readFramebufFormat(1)) // PDC1_FRAMEBUF_FORMAT
                DEF_IO32(0x10400574, data = core->pdc.readInterruptType(1)) // PDC1_INTERRUPT_TYPE
                DEF_IO32(0x10400578, data = core->pdc.readFramebufSelAck(1)) // PDC1_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400C00, data = core->gpu.readMemcpySrcAddr()) // GPU_MEMCPY_SRC_ADDR
                DEF_IO32(0x10400C04, data = core->gpu.readMemcpyDstAddr()) // GPU_MEMCPY_DST_ADDR
                DEF_IO32(0x10400C08, data = core->gpu.readMemcpyDispDstSize()) // GPU_MEMCPY_DISP_DST_SIZE
                DEF_IO32(0x10400C0C, data = core->gpu.readMemcpyDispSrcSize()) // GPU_MEMCPY_DISP_SRC_SIZE
                DEF_IO32(0x10400C10, data = core->gpu.readMemcpyFlags()) // GPU_MEMCPY_FLAGS
                DEF_IO32(0x10400C18, data = core->gpu.readMemcpyCnt()) // GPU_MEMCPY_CNT
                DEF_IO32(0x10400C20, data = core->gpu.readMemcpyTexSize()) // GPU_MEMCPY_TEX_SIZE
                DEF_IO32(0x10400C24, data = core->gpu.readMemcpyTexSrcWidth()) // GPU_MEMCPY_TEX_SRC_WIDTH
                DEF_IO32(0x10400C28, data = core->gpu.readMemcpyTexDstWidth()) // GPU_MEMCPY_TEX_DST_WIDTH
                DEF_IO32(0x10401000, data = core->gpu.readIrqAck(0)) // GPU_IRQ_ACK0
                DEF_IO32(0x10401004, data = core->gpu.readIrqAck(1)) // GPU_IRQ_ACK1
                DEF_IO32(0x10401008, data = core->gpu.readIrqAck(2)) // GPU_IRQ_ACK2
                DEF_IO32(0x1040100C, data = core->gpu.readIrqAck(3)) // GPU_IRQ_ACK3
                DEF_IO32(0x10401010, data = core->gpu.readIrqAck(4)) // GPU_IRQ_ACK4
                DEF_IO32(0x10401014, data = core->gpu.readIrqAck(5)) // GPU_IRQ_ACK5
                DEF_IO32(0x10401018, data = core->gpu.readIrqAck(6)) // GPU_IRQ_ACK6
                DEF_IO32(0x1040101C, data = core->gpu.readIrqAck(7)) // GPU_IRQ_ACK7
                DEF_IO32(0x10401020, data = core->gpu.readIrqAck(8)) // GPU_IRQ_ACK8
                DEF_IO32(0x10401024, data = core->gpu.readIrqAck(9)) // GPU_IRQ_ACK9
                DEF_IO32(0x10401028, data = core->gpu.readIrqAck(10)) // GPU_IRQ_ACK10
                DEF_IO32(0x1040102C, data = core->gpu.readIrqAck(11)) // GPU_IRQ_ACK11
                DEF_IO32(0x10401030, data = core->gpu.readIrqAck(12)) // GPU_IRQ_ACK12
                DEF_IO32(0x10401034, data = core->gpu.readIrqAck(13)) // GPU_IRQ_ACK13
                DEF_IO32(0x10401038, data = core->gpu.readIrqAck(14)) // GPU_IRQ_ACK14
                DEF_IO32(0x1040103C, data = core->gpu.readIrqAck(15)) // GPU_IRQ_ACK15
                DEF_IO32(0x10401040, data = core->gpu.readIrqReq(0)) // GPU_IRQ_REQ0
                DEF_IO32(0x10401044, data = core->gpu.readIrqReq(1)) // GPU_IRQ_REQ1
                DEF_IO32(0x10401048, data = core->gpu.readIrqReq(2)) // GPU_IRQ_REQ2
                DEF_IO32(0x1040104C, data = core->gpu.readIrqReq(3)) // GPU_IRQ_REQ3
                DEF_IO32(0x10401050, data = core->gpu.readIrqReq(4)) // GPU_IRQ_REQ4
                DEF_IO32(0x10401054, data = core->gpu.readIrqReq(5)) // GPU_IRQ_REQ5
                DEF_IO32(0x10401058, data = core->gpu.readIrqReq(6)) // GPU_IRQ_REQ6
                DEF_IO32(0x1040105C, data = core->gpu.readIrqReq(7)) // GPU_IRQ_REQ7
                DEF_IO32(0x10401060, data = core->gpu.readIrqReq(8)) // GPU_IRQ_REQ8
                DEF_IO32(0x10401064, data = core->gpu.readIrqReq(9)) // GPU_IRQ_REQ9
                DEF_IO32(0x10401068, data = core->gpu.readIrqReq(10)) // GPU_IRQ_REQ10
                DEF_IO32(0x1040106C, data = core->gpu.readIrqReq(11)) // GPU_IRQ_REQ11
                DEF_IO32(0x10401070, data = core->gpu.readIrqReq(12)) // GPU_IRQ_REQ12
                DEF_IO32(0x10401074, data = core->gpu.readIrqReq(13)) // GPU_IRQ_REQ13
                DEF_IO32(0x10401078, data = core->gpu.readIrqReq(14)) // GPU_IRQ_REQ14
                DEF_IO32(0x1040107C, data = core->gpu.readIrqReq(15)) // GPU_IRQ_REQ15
                DEF_IO32(0x10401080, data = core->gpu.readIrqCmp(0)) // GPU_IRQ_CMP0
                DEF_IO32(0x10401084, data = core->gpu.readIrqCmp(1)) // GPU_IRQ_CMP1
                DEF_IO32(0x10401088, data = core->gpu.readIrqCmp(2)) // GPU_IRQ_CMP2
                DEF_IO32(0x1040108C, data = core->gpu.readIrqCmp(3)) // GPU_IRQ_CMP3
                DEF_IO32(0x10401090, data = core->gpu.readIrqCmp(4)) // GPU_IRQ_CMP4
                DEF_IO32(0x10401094, data = core->gpu.readIrqCmp(5)) // GPU_IRQ_CMP5
                DEF_IO32(0x10401098, data = core->gpu.readIrqCmp(6)) // GPU_IRQ_CMP6
                DEF_IO32(0x1040109C, data = core->gpu.readIrqCmp(7)) // GPU_IRQ_CMP7
                DEF_IO32(0x104010A0, data = core->gpu.readIrqCmp(8)) // GPU_IRQ_CMP8
                DEF_IO32(0x104010A4, data = core->gpu.readIrqCmp(9)) // GPU_IRQ_CMP9
                DEF_IO32(0x104010A8, data = core->gpu.readIrqCmp(10)) // GPU_IRQ_CMP10
                DEF_IO32(0x104010AC, data = core->gpu.readIrqCmp(11)) // GPU_IRQ_CMP11
                DEF_IO32(0x104010B0, data = core->gpu.readIrqCmp(12)) // GPU_IRQ_CMP12
                DEF_IO32(0x104010B4, data = core->gpu.readIrqCmp(13)) // GPU_IRQ_CMP13
                DEF_IO32(0x104010B8, data = core->gpu.readIrqCmp(14)) // GPU_IRQ_CMP14
                DEF_IO32(0x104010BC, data = core->gpu.readIrqCmp(15)) // GPU_IRQ_CMP15
                DEF_IO32(0x104010C0, data = core->gpu.readIrqMaskL()) // GPU_IRQ_MASK_L
                DEF_IO32(0x104010C4, data = core->gpu.readIrqMaskH()) // GPU_IRQ_MASK_H
                DEF_IO32(0x104010C8, data = core->gpu.readIrqStatL()) // GPU_IRQ_STAT_L
                DEF_IO32(0x104010CC, data = core->gpu.readIrqStatH()) // GPU_IRQ_STAT_H
                DEF_IO32(0x104010D0, data = core->gpu.readIrqAutostop()) // GPU_IRQ_AUTOSTOP
                DEF_IO32(0x10401100, data = core->gpu.readFaceCulling()) // GPU_FACE_CULLING
                DEF_IO32(0x10401104, data = core->gpu.readViewScaleH()) // GPU_VIEW_SCALE_H
                DEF_IO32(0x10401108, data = core->gpu.readViewStepH()) // GPU_VIEW_STEP_H
                DEF_IO32(0x1040110C, data = core->gpu.readViewScaleV()) // GPU_VIEW_SCALE_V
                DEF_IO32(0x10401110, data = core->gpu.readViewStepV()) // GPU_VIEW_STEP_V
                DEF_IO32(0x1040113C, data = core->gpu.readShdOutTotal()) // GPU_SHD_OUT_TOTAL
                DEF_IO32(0x10401140, data = core->gpu.readShdOutMap(0)) // GPU_SHD_OUT_MAP0
                DEF_IO32(0x10401144, data = core->gpu.readShdOutMap(1)) // GPU_SHD_OUT_MAP1
                DEF_IO32(0x10401148, data = core->gpu.readShdOutMap(2)) // GPU_SHD_OUT_MAP2
                DEF_IO32(0x1040114C, data = core->gpu.readShdOutMap(3)) // GPU_SHD_OUT_MAP3
                DEF_IO32(0x10401150, data = core->gpu.readShdOutMap(4)) // GPU_SHD_OUT_MAP4
                DEF_IO32(0x10401154, data = core->gpu.readShdOutMap(5)) // GPU_SHD_OUT_MAP5
                DEF_IO32(0x10401158, data = core->gpu.readShdOutMap(6)) // GPU_SHD_OUT_MAP6
                DEF_IO32(0x10401208, data = core->gpu.readTexDim(0)) // GPU_TEX0_DIM
                DEF_IO32(0x10401214, data = core->gpu.readTexAddr1(0)) // GPU_TEX0_ADDR1
                DEF_IO32(0x10401238, data = core->gpu.readTexType(0)) // GPU_TEX0_TYPE
                DEF_IO32(0x10401248, data = core->gpu.readTexDim(1)) // GPU_TEX1_DIM
                DEF_IO32(0x10401254, data = core->gpu.readTexAddr1(1)) // GPU_TEX1_ADDR
                DEF_IO32(0x10401258, data = core->gpu.readTexType(1)) // GPU_TEX1_TYPE
                DEF_IO32(0x10401268, data = core->gpu.readTexDim(2)) // GPU_TEX2_DIM
                DEF_IO32(0x10401274, data = core->gpu.readTexAddr1(2)) // GPU_TEX2_ADDR
                DEF_IO32(0x10401278, data = core->gpu.readTexType(2)) // GPU_TEX2_TYPE
                DEF_IO32(0x10401300, data = core->gpu.readCombSrc(0)) // GPU_COMB0_SRC
                DEF_IO32(0x10401304, data = core->gpu.readCombOper(0)) // GPU_COMB0_OPER
                DEF_IO32(0x10401308, data = core->gpu.readCombMode(0)) // GPU_COMB0_MODE
                DEF_IO32(0x1040130C, data = core->gpu.readCombColor(0)) // GPU_COMB0_COLOR
                DEF_IO32(0x10401320, data = core->gpu.readCombSrc(1)) // GPU_COMB1_SRC
                DEF_IO32(0x10401324, data = core->gpu.readCombOper(1)) // GPU_COMB1_OPER
                DEF_IO32(0x10401328, data = core->gpu.readCombMode(1)) // GPU_COMB1_MODE
                DEF_IO32(0x1040132C, data = core->gpu.readCombColor(1)) // GPU_COMB1_COLOR
                DEF_IO32(0x10401340, data = core->gpu.readCombSrc(2)) // GPU_COMB2_SRC
                DEF_IO32(0x10401344, data = core->gpu.readCombOper(2)) // GPU_COMB2_OPER
                DEF_IO32(0x10401348, data = core->gpu.readCombMode(2)) // GPU_COMB2_MODE
                DEF_IO32(0x1040134C, data = core->gpu.readCombColor(2)) // GPU_COMB2_COLOR
                DEF_IO32(0x10401360, data = core->gpu.readCombSrc(3)) // GPU_COMB3_SRC
                DEF_IO32(0x10401364, data = core->gpu.readCombOper(3)) // GPU_COMB3_OPER
                DEF_IO32(0x10401368, data = core->gpu.readCombMode(3)) // GPU_COMB3_MODE
                DEF_IO32(0x1040136C, data = core->gpu.readCombColor(3)) // GPU_COMB3_COLOR
                DEF_IO32(0x104013C0, data = core->gpu.readCombSrc(4)) // GPU_COMB4_SRC
                DEF_IO32(0x104013C4, data = core->gpu.readCombOper(4)) // GPU_COMB4_OPER
                DEF_IO32(0x104013C8, data = core->gpu.readCombMode(4)) // GPU_COMB4_MODE
                DEF_IO32(0x104013CC, data = core->gpu.readCombColor(4)) // GPU_COMB4_COLOR
                DEF_IO32(0x104013E0, data = core->gpu.readCombSrc(5)) // GPU_COMB5_SRC
                DEF_IO32(0x104013E4, data = core->gpu.readCombOper(5)) // GPU_COMB5_OPER
                DEF_IO32(0x104013E8, data = core->gpu.readCombMode(5)) // GPU_COMB5_MODE
                DEF_IO32(0x104013EC, data = core->gpu.readCombColor(5)) // GPU_COMB5_COLOR
                DEF_IO32(0x1040141C, data = core->gpu.readDepcolMask()) // GPU_DEPCOL_MASK
                DEF_IO32(0x1040144C, data = core->gpu.readColbufWrite()) // GPU_COLBUF_WRITE
                DEF_IO32(0x10401454, data = core->gpu.readDepbufWrite()) // GPU_DEPBUF_WRITE
                DEF_IO32(0x10401458, data = core->gpu.readDepbufFmt()) // GPU_DEPBUF_FMT
                DEF_IO32(0x1040145C, data = core->gpu.readColbufFmt()) // GPU_COLBUF_FMT
                DEF_IO32(0x10401470, data = core->gpu.readDepbufLoc()) // GPU_DEPBUF_LOC
                DEF_IO32(0x10401474, data = core->gpu.readColbufLoc()) // GPU_COLBUF_LOC
                DEF_IO32(0x10401478, data = core->gpu.readBufferDim()) // GPU_BUFFER_DIM
                DEF_IO32(0x10401800, data = core->gpu.readAttrBase()) // GPU_ATTR_BASE
                DEF_IO32(0x10401804, data = core->gpu.readAttrFmtL()) // GPU_ATTR_FMT_L
                DEF_IO32(0x10401808, data = core->gpu.readAttrFmtH()) // GPU_ATTR_FMT_H
                DEF_IO32(0x1040180C, data = core->gpu.readAttrOfs(0)) // GPU_ATTR_OFS0
                DEF_IO32(0x10401810, data = core->gpu.readAttrCfgL(0)) // GPU_ATTR_CFG0_L
                DEF_IO32(0x10401814, data = core->gpu.readAttrCfgH(0)) // GPU_ATTR_CFG0_H
                DEF_IO32(0x10401818, data = core->gpu.readAttrOfs(1)) // GPU_ATTR_OFS1
                DEF_IO32(0x1040181C, data = core->gpu.readAttrCfgL(1)) // GPU_ATTR_CFG1_L
                DEF_IO32(0x10401820, data = core->gpu.readAttrCfgH(1)) // GPU_ATTR_CFG1_H
                DEF_IO32(0x10401824, data = core->gpu.readAttrOfs(2)) // GPU_ATTR_OFS2
                DEF_IO32(0x10401828, data = core->gpu.readAttrCfgL(2)) // GPU_ATTR_CFG2_L
                DEF_IO32(0x1040182C, data = core->gpu.readAttrCfgH(2)) // GPU_ATTR_CFG2_H
                DEF_IO32(0x10401830, data = core->gpu.readAttrOfs(3)) // GPU_ATTR_OFS3
                DEF_IO32(0x10401834, data = core->gpu.readAttrCfgL(3)) // GPU_ATTR_CFG3_L
                DEF_IO32(0x10401838, data = core->gpu.readAttrCfgH(3)) // GPU_ATTR_CFG3_H
                DEF_IO32(0x1040183C, data = core->gpu.readAttrOfs(4)) // GPU_ATTR_OFS4
                DEF_IO32(0x10401840, data = core->gpu.readAttrCfgL(4)) // GPU_ATTR_CFG4_L
                DEF_IO32(0x10401844, data = core->gpu.readAttrCfgH(4)) // GPU_ATTR_CFG4_H
                DEF_IO32(0x10401848, data = core->gpu.readAttrOfs(5)) // GPU_ATTR_OFS5
                DEF_IO32(0x1040184C, data = core->gpu.readAttrCfgL(5)) // GPU_ATTR_CFG5_L
                DEF_IO32(0x10401850, data = core->gpu.readAttrCfgH(5)) // GPU_ATTR_CFG5_H
                DEF_IO32(0x10401854, data = core->gpu.readAttrOfs(6)) // GPU_ATTR_OFS6
                DEF_IO32(0x10401858, data = core->gpu.readAttrCfgL(6)) // GPU_ATTR_CFG6_L
                DEF_IO32(0x1040185C, data = core->gpu.readAttrCfgH(6)) // GPU_ATTR_CFG6_H
                DEF_IO32(0x10401860, data = core->gpu.readAttrOfs(7)) // GPU_ATTR_OFS7
                DEF_IO32(0x10401864, data = core->gpu.readAttrCfgL(7)) // GPU_ATTR_CFG7_L
                DEF_IO32(0x10401868, data = core->gpu.readAttrCfgH(7)) // GPU_ATTR_CFG7_H
                DEF_IO32(0x1040186C, data = core->gpu.readAttrOfs(8)) // GPU_ATTR_OFS8
                DEF_IO32(0x10401870, data = core->gpu.readAttrCfgL(8)) // GPU_ATTR_CFG8_L
                DEF_IO32(0x10401874, data = core->gpu.readAttrCfgH(8)) // GPU_ATTR_CFG8_H
                DEF_IO32(0x10401878, data = core->gpu.readAttrOfs(9)) // GPU_ATTR_OFS9
                DEF_IO32(0x1040187C, data = core->gpu.readAttrCfgL(9)) // GPU_ATTR_CFG9_L
                DEF_IO32(0x10401880, data = core->gpu.readAttrCfgH(9)) // GPU_ATTR_CFG9_H
                DEF_IO32(0x10401884, data = core->gpu.readAttrOfs(10)) // GPU_ATTR_OFS10
                DEF_IO32(0x10401888, data = core->gpu.readAttrCfgL(10)) // GPU_ATTR_CFG10_L
                DEF_IO32(0x1040188C, data = core->gpu.readAttrCfgH(10)) // GPU_ATTR_CFG10_H
                DEF_IO32(0x10401890, data = core->gpu.readAttrOfs(11)) // GPU_ATTR_OFS11
                DEF_IO32(0x10401894, data = core->gpu.readAttrCfgL(11)) // GPU_ATTR_CFG11_L
                DEF_IO32(0x10401898, data = core->gpu.readAttrCfgH(11)) // GPU_ATTR_CFG11_H
                DEF_IO32(0x1040189C, data = core->gpu.readAttrIdxList()) // GPU_ATTR_IDX_LIST
                DEF_IO32(0x104018A0, data = core->gpu.readAttrNumVerts()) // GPU_ATTR_NUM_VERTS
                DEF_IO32(0x104018A8, data = core->gpu.readAttrFirstIdx()) // GPU_ATTR_FIRST_IDX
                DEF_IO32(0x104018E0, data = core->gpu.readCmdSize(0)) // GPU_CMD_SIZE0
                DEF_IO32(0x104018E4, data = core->gpu.readCmdSize(1)) // GPU_CMD_SIZE1
                DEF_IO32(0x104018E8, data = core->gpu.readCmdAddr(0)) // GPU_CMD_ADDR0
                DEF_IO32(0x104018EC, data = core->gpu.readCmdAddr(1)) // GPU_CMD_ADDR1
                DEF_IO32(0x10401978, data = core->gpu.readPrimConfig()) // GPU_PRIM_CONFIG
                DEF_IO32(0x1040197C, data = core->gpu.readPrimRestart()) // GPU_PRIM_RESTART
                DEF_IO32(0x10401AC0, data = core->gpu.readVshBools()) // GPU_VSH_BOOLS
                DEF_IO32(0x10401AC4, data = core->gpu.readVshInts(0)) // GPU_VSH_INTS0
                DEF_IO32(0x10401AC8, data = core->gpu.readVshInts(1)) // GPU_VSH_INTS1
                DEF_IO32(0x10401ACC, data = core->gpu.readVshInts(2)) // GPU_VSH_INTS2
                DEF_IO32(0x10401AD0, data = core->gpu.readVshInts(3)) // GPU_VSH_INTS3
                DEF_IO32(0x10401AE8, data = core->gpu.readVshEntry()) // GPU_VSH_ENTRY
                DEF_IO32(0x10401AEC, data = core->gpu.readVshAttrIdsL()) // GPU_VSH_ATTR_IDS_L
                DEF_IO32(0x10401AF0, data = core->gpu.readVshAttrIdsH()) // GPU_VSH_ATTR_IDS_H
                DEF_IO32(0x10401AF4, data = core->gpu.readVshOutMask()) // GPU_VSH_OUT_MASK
                DEF_IO32(0x17E00004, data = core->interrupts.readMpScuConfig()) // MP_SCU_CONFIG
                DEF_IO32(0x17E00100, data = core->interrupts.readMpIle(id)) // MP_ILE
                DEF_IO32(0x17E0010C, data = core->interrupts.readMpAck(id)) // MP_ACK
                DEF_IO32(0x17E00118, data = core->interrupts.readMpPending(id)) // MP_PENDING
                DEF_IO32(0x17E00600, data = core->timers.readMpReload(id, 0)) // MP_RELOAD0
                DEF_IO32(0x17E00604, data = core->timers.readMpCounter(id, 0)) // MP_COUNTER0
                DEF_IO32(0x17E00608, data = core->timers.readMpTmcnt(id, 0)) // MP_TMCNT0
                DEF_IO32(0x17E0060C, data = core->timers.readMpTmirq(id, 0)) // MP_TMIRQ0
                DEF_IO32(0x17E00620, data = core->timers.readMpReload(id, 1)) // MP_RELOAD1
                DEF_IO32(0x17E00624, data = core->timers.readMpCounter(id, 1)) // MP_COUNTER1
                DEF_IO32(0x17E00628, data = core->timers.readMpTmcnt(id, 1)) // MP_TMCNT1
                DEF_IO32(0x17E0062C, data = core->timers.readMpTmirq(id, 1)) // MP_TMIRQ1
                DEF_IO32(0x17E00700, data = core->timers.readMpReload(ARM11A, 0)) // MP0_RELOAD0
                DEF_IO32(0x17E00704, data = core->timers.readMpCounter(ARM11A, 0)) // MP0_COUNTER0
                DEF_IO32(0x17E00708, data = core->timers.readMpTmcnt(ARM11A, 0)) // MP0_TMCNT0
                DEF_IO32(0x17E0070C, data = core->timers.readMpTmirq(ARM11A, 0)) // MP0_TMIRQ0
                DEF_IO32(0x17E00720, data = core->timers.readMpReload(ARM11A, 1)) // MP0_RELOAD1
                DEF_IO32(0x17E00724, data = core->timers.readMpCounter(ARM11A, 1)) // MP0_COUNTER1
                DEF_IO32(0x17E00728, data = core->timers.readMpTmcnt(ARM11A, 1)) // MP0_TMCNT1
                DEF_IO32(0x17E0072C, data = core->timers.readMpTmirq(ARM11A, 1)) // MP0_TMIRQ1
                DEF_IO32(0x17E00800, data = core->timers.readMpReload(ARM11B, 0)) // MP1_RELOAD0
                DEF_IO32(0x17E00804, data = core->timers.readMpCounter(ARM11B, 0)) // MP1_COUNTER0
                DEF_IO32(0x17E00808, data = core->timers.readMpTmcnt(ARM11B, 0)) // MP1_TMCNT0
                DEF_IO32(0x17E0080C, data = core->timers.readMpTmirq(ARM11B, 0)) // MP1_TMIRQ0
                DEF_IO32(0x17E00820, data = core->timers.readMpReload(ARM11B, 1)) // MP1_RELOAD1
                DEF_IO32(0x17E00824, data = core->timers.readMpCounter(ARM11B, 1)) // MP1_COUNTER1
                DEF_IO32(0x17E00828, data = core->timers.readMpTmcnt(ARM11B, 1)) // MP1_TMCNT1
                DEF_IO32(0x17E0082C, data = core->timers.readMpTmirq(ARM11B, 1)) // MP1_TMIRQ1
                DEF_IO32(0x17E00900, data = core->timers.readMpReload(ARM11C, 0)) // MP2_RELOAD0
                DEF_IO32(0x17E00904, data = core->timers.readMpCounter(ARM11C, 0)) // MP2_COUNTER0
                DEF_IO32(0x17E00908, data = core->timers.readMpTmcnt(ARM11C, 0)) // MP2_TMCNT0
                DEF_IO32(0x17E0090C, data = core->timers.readMpTmirq(ARM11C, 0)) // MP2_TMIRQ0
                DEF_IO32(0x17E00920, data = core->timers.readMpReload(ARM11C, 1)) // MP2_RELOAD1
                DEF_IO32(0x17E00924, data = core->timers.readMpCounter(ARM11C, 1)) // MP2_COUNTER1
                DEF_IO32(0x17E00928, data = core->timers.readMpTmcnt(ARM11C, 1)) // MP2_TMCNT1
                DEF_IO32(0x17E0092C, data = core->timers.readMpTmirq(ARM11C, 1)) // MP2_TMIRQ1
                DEF_IO32(0x17E00A00, data = core->timers.readMpReload(ARM11D, 0)) // MP3_RELOAD0
                DEF_IO32(0x17E00A04, data = core->timers.readMpCounter(ARM11D, 0)) // MP3_COUNTER0
                DEF_IO32(0x17E00A08, data = core->timers.readMpTmcnt(ARM11D, 0)) // MP3_TMCNT0
                DEF_IO32(0x17E00A0C, data = core->timers.readMpTmirq(ARM11D, 0)) // MP3_TMIRQ0
                DEF_IO32(0x17E00A20, data = core->timers.readMpReload(ARM11D, 1)) // MP3_RELOAD1
                DEF_IO32(0x17E00A24, data = core->timers.readMpCounter(ARM11D, 1)) // MP3_COUNTER1
                DEF_IO32(0x17E00A28, data = core->timers.readMpTmcnt(ARM11D, 1)) // MP3_TMCNT1
                DEF_IO32(0x17E00A2C, data = core->timers.readMpTmirq(ARM11D, 1)) // MP3_TMIRQ1
                DEF_IO32(0x17E01000, data = core->interrupts.readMpIge()) // MP_IGE
                DEF_IO32(0x17E01004, data = core->interrupts.readMpCtrlType()) // MP_CTRL_TYPE
                DEF_IO32(0x17E01100, data = core->interrupts.readMpIe(0)) // MP_IE0
                DEF_IO32(0x17E01104, data = core->interrupts.readMpIe(1)) // MP_IE1
                DEF_IO32(0x17E01108, data = core->interrupts.readMpIe(2)) // MP_IE2
                DEF_IO32(0x17E0110C, data = core->interrupts.readMpIe(3)) // MP_IE3
                DEF_IO32(0x17E01180, data = core->interrupts.readMpIe(0)) // MP_IE0
                DEF_IO32(0x17E01184, data = core->interrupts.readMpIe(1)) // MP_IE1
                DEF_IO32(0x17E01188, data = core->interrupts.readMpIe(2)) // MP_IE2
                DEF_IO32(0x17E0118C, data = core->interrupts.readMpIe(3)) // MP_IE3
                DEF_IO32(0x17E01200, data = core->interrupts.readMpIp(id, 0)) // MP_IP0
                DEF_IO32(0x17E01204, data = core->interrupts.readMpIp(id, 1)) // MP_IP1
                DEF_IO32(0x17E01208, data = core->interrupts.readMpIp(id, 2)) // MP_IP2
                DEF_IO32(0x17E0120C, data = core->interrupts.readMpIp(id, 3)) // MP_IP3
                DEF_IO32(0x17E01280, data = core->interrupts.readMpIp(id, 0)) // MP_IP0
                DEF_IO32(0x17E01284, data = core->interrupts.readMpIp(id, 1)) // MP_IP1
                DEF_IO32(0x17E01288, data = core->interrupts.readMpIp(id, 2)) // MP_IP2
                DEF_IO32(0x17E0128C, data = core->interrupts.readMpIp(id, 3)) // MP_IP3
                DEF_IO32(0x17E01300, data = core->interrupts.readMpIa(id, 0)) // MP_IA0
                DEF_IO32(0x17E01304, data = core->interrupts.readMpIa(id, 1)) // MP_IA1
                DEF_IO32(0x17E01308, data = core->interrupts.readMpIa(id, 2)) // MP_IA2
                DEF_IO32(0x17E0130C, data = core->interrupts.readMpIa(id, 3)) // MP_IA3
                DEF_IO08(0x17E01800, data = core->interrupts.readMpTarget(id, 0)) // MP_TARGET0
                DEF_IO08(0x17E01801, data = core->interrupts.readMpTarget(id, 1)) // MP_TARGET1
                DEF_IO08(0x17E01802, data = core->interrupts.readMpTarget(id, 2)) // MP_TARGET2
                DEF_IO08(0x17E01803, data = core->interrupts.readMpTarget(id, 3)) // MP_TARGET3
                DEF_IO08(0x17E01804, data = core->interrupts.readMpTarget(id, 4)) // MP_TARGET4
                DEF_IO08(0x17E01805, data = core->interrupts.readMpTarget(id, 5)) // MP_TARGET5
                DEF_IO08(0x17E01806, data = core->interrupts.readMpTarget(id, 6)) // MP_TARGET6
                DEF_IO08(0x17E01807, data = core->interrupts.readMpTarget(id, 7)) // MP_TARGET7
                DEF_IO08(0x17E01808, data = core->interrupts.readMpTarget(id, 8)) // MP_TARGET8
                DEF_IO08(0x17E01809, data = core->interrupts.readMpTarget(id, 9)) // MP_TARGET9
                DEF_IO08(0x17E0180A, data = core->interrupts.readMpTarget(id, 10)) // MP_TARGET10
                DEF_IO08(0x17E0180B, data = core->interrupts.readMpTarget(id, 11)) // MP_TARGET11
                DEF_IO08(0x17E0180C, data = core->interrupts.readMpTarget(id, 12)) // MP_TARGET12
                DEF_IO08(0x17E0180D, data = core->interrupts.readMpTarget(id, 13)) // MP_TARGET13
                DEF_IO08(0x17E0180E, data = core->interrupts.readMpTarget(id, 14)) // MP_TARGET14
                DEF_IO08(0x17E0180F, data = core->interrupts.readMpTarget(id, 15)) // MP_TARGET15
                DEF_IO08(0x17E01810, data = core->interrupts.readMpTarget(id, 16)) // MP_TARGET16
                DEF_IO08(0x17E01811, data = core->interrupts.readMpTarget(id, 17)) // MP_TARGET17
                DEF_IO08(0x17E01812, data = core->interrupts.readMpTarget(id, 18)) // MP_TARGET18
                DEF_IO08(0x17E01813, data = core->interrupts.readMpTarget(id, 19)) // MP_TARGET19
                DEF_IO08(0x17E01814, data = core->interrupts.readMpTarget(id, 20)) // MP_TARGET20
                DEF_IO08(0x17E01815, data = core->interrupts.readMpTarget(id, 21)) // MP_TARGET21
                DEF_IO08(0x17E01816, data = core->interrupts.readMpTarget(id, 22)) // MP_TARGET22
                DEF_IO08(0x17E01817, data = core->interrupts.readMpTarget(id, 23)) // MP_TARGET23
                DEF_IO08(0x17E01818, data = core->interrupts.readMpTarget(id, 24)) // MP_TARGET24
                DEF_IO08(0x17E01819, data = core->interrupts.readMpTarget(id, 25)) // MP_TARGET25
                DEF_IO08(0x17E0181A, data = core->interrupts.readMpTarget(id, 26)) // MP_TARGET26
                DEF_IO08(0x17E0181B, data = core->interrupts.readMpTarget(id, 27)) // MP_TARGET27
                DEF_IO08(0x17E0181C, data = core->interrupts.readMpTarget(id, 28)) // MP_TARGET28
                DEF_IO08(0x17E0181D, data = core->interrupts.readMpTarget(id, 29)) // MP_TARGET29
                DEF_IO08(0x17E0181E, data = core->interrupts.readMpTarget(id, 30)) // MP_TARGET30
                DEF_IO08(0x17E0181F, data = core->interrupts.readMpTarget(id, 31)) // MP_TARGET31
                DEF_IO08(0x17E01820, data = core->interrupts.readMpTarget(id, 32)) // MP_TARGET32
                DEF_IO08(0x17E01821, data = core->interrupts.readMpTarget(id, 33)) // MP_TARGET33
                DEF_IO08(0x17E01822, data = core->interrupts.readMpTarget(id, 34)) // MP_TARGET34
                DEF_IO08(0x17E01823, data = core->interrupts.readMpTarget(id, 35)) // MP_TARGET35
                DEF_IO08(0x17E01824, data = core->interrupts.readMpTarget(id, 36)) // MP_TARGET36
                DEF_IO08(0x17E01825, data = core->interrupts.readMpTarget(id, 37)) // MP_TARGET37
                DEF_IO08(0x17E01826, data = core->interrupts.readMpTarget(id, 38)) // MP_TARGET38
                DEF_IO08(0x17E01827, data = core->interrupts.readMpTarget(id, 39)) // MP_TARGET39
                DEF_IO08(0x17E01828, data = core->interrupts.readMpTarget(id, 40)) // MP_TARGET40
                DEF_IO08(0x17E01829, data = core->interrupts.readMpTarget(id, 41)) // MP_TARGET41
                DEF_IO08(0x17E0182A, data = core->interrupts.readMpTarget(id, 42)) // MP_TARGET42
                DEF_IO08(0x17E0182B, data = core->interrupts.readMpTarget(id, 43)) // MP_TARGET43
                DEF_IO08(0x17E0182C, data = core->interrupts.readMpTarget(id, 44)) // MP_TARGET44
                DEF_IO08(0x17E0182D, data = core->interrupts.readMpTarget(id, 45)) // MP_TARGET45
                DEF_IO08(0x17E0182E, data = core->interrupts.readMpTarget(id, 46)) // MP_TARGET46
                DEF_IO08(0x17E0182F, data = core->interrupts.readMpTarget(id, 47)) // MP_TARGET47
                DEF_IO08(0x17E01830, data = core->interrupts.readMpTarget(id, 48)) // MP_TARGET48
                DEF_IO08(0x17E01831, data = core->interrupts.readMpTarget(id, 49)) // MP_TARGET49
                DEF_IO08(0x17E01832, data = core->interrupts.readMpTarget(id, 50)) // MP_TARGET50
                DEF_IO08(0x17E01833, data = core->interrupts.readMpTarget(id, 51)) // MP_TARGET51
                DEF_IO08(0x17E01834, data = core->interrupts.readMpTarget(id, 52)) // MP_TARGET52
                DEF_IO08(0x17E01835, data = core->interrupts.readMpTarget(id, 53)) // MP_TARGET53
                DEF_IO08(0x17E01836, data = core->interrupts.readMpTarget(id, 54)) // MP_TARGET54
                DEF_IO08(0x17E01837, data = core->interrupts.readMpTarget(id, 55)) // MP_TARGET55
                DEF_IO08(0x17E01838, data = core->interrupts.readMpTarget(id, 56)) // MP_TARGET56
                DEF_IO08(0x17E01839, data = core->interrupts.readMpTarget(id, 57)) // MP_TARGET57
                DEF_IO08(0x17E0183A, data = core->interrupts.readMpTarget(id, 58)) // MP_TARGET58
                DEF_IO08(0x17E0183B, data = core->interrupts.readMpTarget(id, 59)) // MP_TARGET59
                DEF_IO08(0x17E0183C, data = core->interrupts.readMpTarget(id, 60)) // MP_TARGET60
                DEF_IO08(0x17E0183D, data = core->interrupts.readMpTarget(id, 61)) // MP_TARGET61
                DEF_IO08(0x17E0183E, data = core->interrupts.readMpTarget(id, 62)) // MP_TARGET62
                DEF_IO08(0x17E0183F, data = core->interrupts.readMpTarget(id, 63)) // MP_TARGET63
                DEF_IO08(0x17E01840, data = core->interrupts.readMpTarget(id, 64)) // MP_TARGET64
                DEF_IO08(0x17E01841, data = core->interrupts.readMpTarget(id, 65)) // MP_TARGET65
                DEF_IO08(0x17E01842, data = core->interrupts.readMpTarget(id, 66)) // MP_TARGET66
                DEF_IO08(0x17E01843, data = core->interrupts.readMpTarget(id, 67)) // MP_TARGET67
                DEF_IO08(0x17E01844, data = core->interrupts.readMpTarget(id, 68)) // MP_TARGET68
                DEF_IO08(0x17E01845, data = core->interrupts.readMpTarget(id, 69)) // MP_TARGET69
                DEF_IO08(0x17E01846, data = core->interrupts.readMpTarget(id, 70)) // MP_TARGET70
                DEF_IO08(0x17E01847, data = core->interrupts.readMpTarget(id, 71)) // MP_TARGET71
                DEF_IO08(0x17E01848, data = core->interrupts.readMpTarget(id, 72)) // MP_TARGET72
                DEF_IO08(0x17E01849, data = core->interrupts.readMpTarget(id, 73)) // MP_TARGET73
                DEF_IO08(0x17E0184A, data = core->interrupts.readMpTarget(id, 74)) // MP_TARGET74
                DEF_IO08(0x17E0184B, data = core->interrupts.readMpTarget(id, 75)) // MP_TARGET75
                DEF_IO08(0x17E0184C, data = core->interrupts.readMpTarget(id, 76)) // MP_TARGET76
                DEF_IO08(0x17E0184D, data = core->interrupts.readMpTarget(id, 77)) // MP_TARGET77
                DEF_IO08(0x17E0184E, data = core->interrupts.readMpTarget(id, 78)) // MP_TARGET78
                DEF_IO08(0x17E0184F, data = core->interrupts.readMpTarget(id, 79)) // MP_TARGET79
                DEF_IO08(0x17E01850, data = core->interrupts.readMpTarget(id, 80)) // MP_TARGET80
                DEF_IO08(0x17E01851, data = core->interrupts.readMpTarget(id, 81)) // MP_TARGET81
                DEF_IO08(0x17E01852, data = core->interrupts.readMpTarget(id, 82)) // MP_TARGET82
                DEF_IO08(0x17E01853, data = core->interrupts.readMpTarget(id, 83)) // MP_TARGET83
                DEF_IO08(0x17E01854, data = core->interrupts.readMpTarget(id, 84)) // MP_TARGET84
                DEF_IO08(0x17E01855, data = core->interrupts.readMpTarget(id, 85)) // MP_TARGET85
                DEF_IO08(0x17E01856, data = core->interrupts.readMpTarget(id, 86)) // MP_TARGET86
                DEF_IO08(0x17E01857, data = core->interrupts.readMpTarget(id, 87)) // MP_TARGET87
                DEF_IO08(0x17E01858, data = core->interrupts.readMpTarget(id, 88)) // MP_TARGET88
                DEF_IO08(0x17E01859, data = core->interrupts.readMpTarget(id, 89)) // MP_TARGET89
                DEF_IO08(0x17E0185A, data = core->interrupts.readMpTarget(id, 90)) // MP_TARGET90
                DEF_IO08(0x17E0185B, data = core->interrupts.readMpTarget(id, 91)) // MP_TARGET91
                DEF_IO08(0x17E0185C, data = core->interrupts.readMpTarget(id, 92)) // MP_TARGET92
                DEF_IO08(0x17E0185D, data = core->interrupts.readMpTarget(id, 93)) // MP_TARGET93
                DEF_IO08(0x17E0185E, data = core->interrupts.readMpTarget(id, 94)) // MP_TARGET94
                DEF_IO08(0x17E0185F, data = core->interrupts.readMpTarget(id, 95)) // MP_TARGET95
                DEF_IO08(0x17E01860, data = core->interrupts.readMpTarget(id, 96)) // MP_TARGET96
                DEF_IO08(0x17E01861, data = core->interrupts.readMpTarget(id, 97)) // MP_TARGET97
                DEF_IO08(0x17E01862, data = core->interrupts.readMpTarget(id, 98)) // MP_TARGET98
                DEF_IO08(0x17E01863, data = core->interrupts.readMpTarget(id, 99)) // MP_TARGET99
                DEF_IO08(0x17E01864, data = core->interrupts.readMpTarget(id, 100)) // MP_TARGET100
                DEF_IO08(0x17E01865, data = core->interrupts.readMpTarget(id, 101)) // MP_TARGET101
                DEF_IO08(0x17E01866, data = core->interrupts.readMpTarget(id, 102)) // MP_TARGET102
                DEF_IO08(0x17E01867, data = core->interrupts.readMpTarget(id, 103)) // MP_TARGET103
                DEF_IO08(0x17E01868, data = core->interrupts.readMpTarget(id, 104)) // MP_TARGET104
                DEF_IO08(0x17E01869, data = core->interrupts.readMpTarget(id, 105)) // MP_TARGET105
                DEF_IO08(0x17E0186A, data = core->interrupts.readMpTarget(id, 106)) // MP_TARGET106
                DEF_IO08(0x17E0186B, data = core->interrupts.readMpTarget(id, 107)) // MP_TARGET107
                DEF_IO08(0x17E0186C, data = core->interrupts.readMpTarget(id, 108)) // MP_TARGET108
                DEF_IO08(0x17E0186D, data = core->interrupts.readMpTarget(id, 109)) // MP_TARGET109
                DEF_IO08(0x17E0186E, data = core->interrupts.readMpTarget(id, 110)) // MP_TARGET110
                DEF_IO08(0x17E0186F, data = core->interrupts.readMpTarget(id, 111)) // MP_TARGET111
                DEF_IO08(0x17E01870, data = core->interrupts.readMpTarget(id, 112)) // MP_TARGET112
                DEF_IO08(0x17E01871, data = core->interrupts.readMpTarget(id, 113)) // MP_TARGET113
                DEF_IO08(0x17E01872, data = core->interrupts.readMpTarget(id, 114)) // MP_TARGET114
                DEF_IO08(0x17E01873, data = core->interrupts.readMpTarget(id, 115)) // MP_TARGET115
                DEF_IO08(0x17E01874, data = core->interrupts.readMpTarget(id, 116)) // MP_TARGET116
                DEF_IO08(0x17E01875, data = core->interrupts.readMpTarget(id, 117)) // MP_TARGET117
                DEF_IO08(0x17E01876, data = core->interrupts.readMpTarget(id, 118)) // MP_TARGET118
                DEF_IO08(0x17E01877, data = core->interrupts.readMpTarget(id, 119)) // MP_TARGET119
                DEF_IO08(0x17E01878, data = core->interrupts.readMpTarget(id, 120)) // MP_TARGET120
                DEF_IO08(0x17E01879, data = core->interrupts.readMpTarget(id, 121)) // MP_TARGET121
                DEF_IO08(0x17E0187A, data = core->interrupts.readMpTarget(id, 122)) // MP_TARGET122
                DEF_IO08(0x17E0187B, data = core->interrupts.readMpTarget(id, 123)) // MP_TARGET123
                DEF_IO08(0x17E0187C, data = core->interrupts.readMpTarget(id, 124)) // MP_TARGET124
                DEF_IO08(0x17E0187D, data = core->interrupts.readMpTarget(id, 125)) // MP_TARGET125
                DEF_IO08(0x17E0187E, data = core->interrupts.readMpTarget(id, 126)) // MP_TARGET126
                DEF_IO08(0x17E0187F, data = core->interrupts.readMpTarget(id, 127)) // MP_TARGET127
            }
        }
        else { // ARM9
            switch (base) {
                DEF_IO08(0x10000000, data = readCfg9Sysprot9()) // CFG9_SYSPROT9
                DEF_IO08(0x10000001, data = readCfg9Sysprot11()) // CFG9_SYSPROT11
                DEF_IO16(0x10000010, data = core->cartridge.readCfg9CardPower()) // CFG9_CARD_POWER
                DEF_IO32(0x10000200, data = readCfg9Extmemcnt9()) // CFG9_EXTMEMCNT9
                DEF_IO16(0x10000FFC, data = core->interrupts.readCfg11Socinfo()) // CFG9_MPCORECFG
                DEF_IO32(0x10001000, data = core->interrupts.readIrqIe()) // IRQ_IE
                DEF_IO32(0x10001004, data = core->interrupts.readIrqIf()) // IRQ_IF
                DEF_IO32(0x10002004, data = core->ndma.readSad(0)) // NDMA0SAD
                DEF_IO32(0x10002008, data = core->ndma.readDad(0)) // NDMA0DAD
                DEF_IO32(0x1000200C, data = core->ndma.readTcnt(0)) // NDMA0TCNT
                DEF_IO32(0x10002010, data = core->ndma.readWcnt(0)) // NDMA0WCNT
                DEF_IO32(0x10002018, data = core->ndma.readFdata(0)) // NDMA0FDATA
                DEF_IO32(0x1000201C, data = core->ndma.readCnt(0)) // NDMA0CNT
                DEF_IO32(0x10002020, data = core->ndma.readSad(1)) // NDMA1SAD
                DEF_IO32(0x10002024, data = core->ndma.readDad(1)) // NDMA1DAD
                DEF_IO32(0x10002028, data = core->ndma.readTcnt(1)) // NDMA1TCNT
                DEF_IO32(0x1000202C, data = core->ndma.readWcnt(1)) // NDMA1WCNT
                DEF_IO32(0x10002034, data = core->ndma.readFdata(1)) // NDMA1FDATA
                DEF_IO32(0x10002038, data = core->ndma.readCnt(1)) // NDMA1CNT
                DEF_IO32(0x1000203C, data = core->ndma.readSad(2)) // NDMA2SAD
                DEF_IO32(0x10002040, data = core->ndma.readDad(2)) // NDMA2DAD
                DEF_IO32(0x10002044, data = core->ndma.readTcnt(2)) // NDMA2TCNT
                DEF_IO32(0x10002048, data = core->ndma.readWcnt(2)) // NDMA2WCNT
                DEF_IO32(0x10002050, data = core->ndma.readFdata(2)) // NDMA2FDATA
                DEF_IO32(0x10002054, data = core->ndma.readCnt(2)) // NDMA2CNT
                DEF_IO32(0x10002058, data = core->ndma.readSad(3)) // NDMA3SAD
                DEF_IO32(0x1000205C, data = core->ndma.readDad(3)) // NDMA3DAD
                DEF_IO32(0x10002060, data = core->ndma.readTcnt(3)) // NDMA3TCNT
                DEF_IO32(0x10002064, data = core->ndma.readWcnt(3)) // NDMA3WCNT
                DEF_IO32(0x1000206C, data = core->ndma.readFdata(3)) // NDMA3FDATA
                DEF_IO32(0x10002070, data = core->ndma.readCnt(3)) // NDMA3CNT
                DEF_IO32(0x10002074, data = core->ndma.readSad(4)) // NDMA4SAD
                DEF_IO32(0x10002078, data = core->ndma.readDad(4)) // NDMA4DAD
                DEF_IO32(0x1000207C, data = core->ndma.readTcnt(4)) // NDMA4TCNT
                DEF_IO32(0x10002080, data = core->ndma.readWcnt(4)) // NDMA4WCNT
                DEF_IO32(0x10002088, data = core->ndma.readFdata(4)) // NDMA4FDATA
                DEF_IO32(0x1000208C, data = core->ndma.readCnt(4)) // NDMA4CNT
                DEF_IO32(0x10002090, data = core->ndma.readSad(5)) // NDMA5SAD
                DEF_IO32(0x10002094, data = core->ndma.readDad(5)) // NDMA5DAD
                DEF_IO32(0x10002098, data = core->ndma.readTcnt(5)) // NDMA5TCNT
                DEF_IO32(0x1000209C, data = core->ndma.readWcnt(5)) // NDMA5WCNT
                DEF_IO32(0x100020A4, data = core->ndma.readFdata(5)) // NDMA5FDATA
                DEF_IO32(0x100020A8, data = core->ndma.readCnt(5)) // NDMA5CNT
                DEF_IO32(0x100020AC, data = core->ndma.readSad(6)) // NDMA6SAD
                DEF_IO32(0x100020B0, data = core->ndma.readDad(6)) // NDMA6DAD
                DEF_IO32(0x100020B4, data = core->ndma.readTcnt(6)) // NDMA6TCNT
                DEF_IO32(0x100020B8, data = core->ndma.readWcnt(6)) // NDMA6WCNT
                DEF_IO32(0x100020C0, data = core->ndma.readFdata(6)) // NDMA6FDATA
                DEF_IO32(0x100020C4, data = core->ndma.readCnt(6)) // NDMA6CNT
                DEF_IO32(0x100020C8, data = core->ndma.readSad(7)) // NDMA7SAD
                DEF_IO32(0x100020CC, data = core->ndma.readDad(7)) // NDMA7DAD
                DEF_IO32(0x100020D0, data = core->ndma.readTcnt(7)) // NDMA7TCNT
                DEF_IO32(0x100020D4, data = core->ndma.readWcnt(7)) // NDMA7WCNT
                DEF_IO32(0x100020DC, data = core->ndma.readFdata(7)) // NDMA7FDATA
                DEF_IO32(0x100020E0, data = core->ndma.readCnt(7)) // NDMA7CNT
                DEF_IO16(0x10003000, data = core->timers.readTmCntL(0)) // TM0CNT_L
                DEF_IO16(0x10003002, data = core->timers.readTmCntH(0)) // TM0CNT_H
                DEF_IO16(0x10003004, data = core->timers.readTmCntL(1)) // TM1CNT_L
                DEF_IO16(0x10003006, data = core->timers.readTmCntH(1)) // TM1CNT_H
                DEF_IO16(0x10003008, data = core->timers.readTmCntL(2)) // TM2CNT_L
                DEF_IO16(0x1000300A, data = core->timers.readTmCntH(2)) // TM2CNT_H
                DEF_IO16(0x1000300C, data = core->timers.readTmCntL(3)) // TM3CNT_L
                DEF_IO16(0x1000300E, data = core->timers.readTmCntH(3)) // TM3CNT_H
                DEF_IO32(0x10004000, data = core->cartridge.readCtrCnt()) // CTRCARD_CNT
                DEF_IO32(0x10004004, data = core->cartridge.readCtrBlkcnt()) // CTRCARD_BLKCNT
                DEF_IO32(0x10004008, data = core->cartridge.readCtrSeccnt()) // CTRCARD_SECCNT
                DEF_IO32(0x10004030, data = core->cartridge.readCtrFifo()) // CTRCARD_FIFO
                DEF_IO16(0x10006000, data = core->sdMmcs[0].readCmd()) // SD0_CMD
                DEF_IO16(0x10006002, data = core->sdMmcs[0].readPortSelect()) // SD0_PORT_SELECT
                DEF_IO32(0x10006004, data = core->sdMmcs[0].readCmdParam()) // SD0_CMD_PARAM
                DEF_IO16(0x1000600A, data = core->sdMmcs[0].readData16Blkcnt()) // SD0_DATA16_BLKCNT
                DEF_IO32(0x1000600C, data = core->sdMmcs[0].readResponse(0)) // SD0_RESPONSE0
                DEF_IO32(0x10006010, data = core->sdMmcs[0].readResponse(1)) // SD0_RESPONSE1
                DEF_IO32(0x10006014, data = core->sdMmcs[0].readResponse(2)) // SD0_RESPONSE2
                DEF_IO32(0x10006018, data = core->sdMmcs[0].readResponse(3)) // SD0_RESPONSE3
                DEF_IO32(0x1000601C, data = core->sdMmcs[0].readIrqStatus()) // SD0_IRQ_STATUS
                DEF_IO32(0x10006020, data = core->sdMmcs[0].readIrqMask()) // SD0_IRQ_MASK
                DEF_IO16(0x10006026, data = core->sdMmcs[0].readData16Blklen()) // SD0_DATA16_BLKLEN
                DEF_IO32(0x1000602C, data = core->sdMmcs[0].readErrDetail()) // SD0_ERR_DETAIL
                DEF_IO16(0x10006030, data = core->sdMmcs[0].readData16Fifo()) // SD0_DATA16_FIFO
                DEF_IO16(0x100060D8, data = core->sdMmcs[0].readDataCtl()) // SD0_DATA_CTL
                DEF_IO16(0x10006100, data = core->sdMmcs[0].readData32Irq()) // SD0_DATA32_IRQ
                DEF_IO16(0x10006104, data = core->sdMmcs[0].readData32Blklen()) // SD0_DATA32_BLKLEN
                DEF_IO32(0x1000610C, data = core->sdMmcs[0].readData32Fifo()) // SD0_DATA32_FIFO
                DEF_IO16(0x10007000, data = core->sdMmcs[1].readCmd()) // SD1_CMD
                DEF_IO16(0x10007002, data = core->sdMmcs[1].readPortSelect()) // SD1_PORT_SELECT
                DEF_IO32(0x10007004, data = core->sdMmcs[1].readCmdParam()) // SD1_CMD_PARAM
                DEF_IO16(0x1000700A, data = core->sdMmcs[1].readData16Blkcnt()) // SD1_DATA16_BLKCNT
                DEF_IO32(0x1000700C, data = core->sdMmcs[1].readResponse(0)) // SD1_RESPONSE0
                DEF_IO32(0x10007010, data = core->sdMmcs[1].readResponse(1)) // SD1_RESPONSE1
                DEF_IO32(0x10007014, data = core->sdMmcs[1].readResponse(2)) // SD1_RESPONSE2
                DEF_IO32(0x10007018, data = core->sdMmcs[1].readResponse(3)) // SD1_RESPONSE3
                DEF_IO32(0x1000701C, data = core->sdMmcs[1].readIrqStatus()) // SD1_IRQ_STATUS
                DEF_IO32(0x10007020, data = core->sdMmcs[1].readIrqMask()) // SD1_IRQ_MASK
                DEF_IO16(0x10007026, data = core->sdMmcs[1].readData16Blklen()) // SD1_DATA16_BLKLEN
                DEF_IO32(0x1000702C, data = core->sdMmcs[1].readErrDetail()) // SD1_ERR_DETAIL
                DEF_IO16(0x10007030, data = core->sdMmcs[1].readData16Fifo()) // SD1_DATA16_FIFO
                DEF_IO16(0x100070D8, data = core->sdMmcs[1].readDataCtl()) // SD1_DATA_CTL
                DEF_IO16(0x10007100, data = core->sdMmcs[1].readData32Irq()) // SD1_DATA32_IRQ
                DEF_IO16(0x10007104, data = core->sdMmcs[1].readData32Blklen()) // SD1_DATA32_BLKLEN
                DEF_IO32(0x1000710C, data = core->sdMmcs[1].readData32Fifo()) // SD1_DATA32_FIFO
                DEF_IO32(0x10008000, data = core->pxi.readSync(1)) // PXI_SYNC9
                DEF_IO32(0x10008004, data = core->pxi.readCnt(1)) // PXI_CNT9
                DEF_IO32(0x1000800C, data = core->pxi.readRecv(1)) // PXI_RECV9
                DEF_IO32(0x10009000, data = core->aes.readCnt()) // AES_CNT
                DEF_IO32(0x1000900C, data = core->aes.readRdfifo()) // AES_RDFIFO
                DEF_IO08(0x10009010, data = core->aes.readKeysel()) // AES_KEYSEL
                DEF_IO08(0x10009011, data = core->aes.readKeycnt()) // AES_KEYCNT
                DEF_IO32(0x1000A000, data = core->shas[1].readCnt()) // SHA_CNT9
                DEF_IO32(0x1000A004, data = core->shas[1].readBlkcnt()) // SHA_BLKCNT9
                DEF_IO32(0x1000A040, data = core->shas[1].readHash(0)) // SHA_HASH0_9
                DEF_IO32(0x1000A044, data = core->shas[1].readHash(1)) // SHA_HASH1_9
                DEF_IO32(0x1000A048, data = core->shas[1].readHash(2)) // SHA_HASH2_9
                DEF_IO32(0x1000A04C, data = core->shas[1].readHash(3)) // SHA_HASH3_9
                DEF_IO32(0x1000A050, data = core->shas[1].readHash(4)) // SHA_HASH4_9
                DEF_IO32(0x1000A054, data = core->shas[1].readHash(5)) // SHA_HASH5_9
                DEF_IO32(0x1000A058, data = core->shas[1].readHash(6)) // SHA_HASH6_9
                DEF_IO32(0x1000A05C, data = core->shas[1].readHash(7)) // SHA_HASH7_9
                DEF_IO32(0x1000A080, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A084, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A088, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A08C, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A090, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A094, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A098, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A09C, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A0, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A4, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A8, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0AC, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B0, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B4, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B8, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0BC, data = core->shas[1].readFifo()) // SHA_FIFO9
                DEF_IO32(0x1000B000, data = core->rsa.readCnt()) // RSA_CNT
                DEF_IO32(0x1000B100, data = core->rsa.readSlotcnt(0)) // RSA_SLOTCNT0
                DEF_IO32(0x1000B104, data = core->rsa.readSlotsize(0)) // RSA_SLOTSIZE0
                DEF_IO32(0x1000B110, data = core->rsa.readSlotcnt(1)) // RSA_SLOTCNT1
                DEF_IO32(0x1000B114, data = core->rsa.readSlotsize(1)) // RSA_SLOTSIZE1
                DEF_IO32(0x1000B120, data = core->rsa.readSlotcnt(2)) // RSA_SLOTCNT2
                DEF_IO32(0x1000B124, data = core->rsa.readSlotsize(2)) // RSA_SLOTSIZE2
                DEF_IO32(0x1000B130, data = core->rsa.readSlotcnt(3)) // RSA_SLOTCNT3
                DEF_IO32(0x1000B134, data = core->rsa.readSlotsize(3)) // RSA_SLOTSIZE3
                DEF_IO32(0x1000B400, data = core->rsa.readMod(0)) // RSA_MOD0
                DEF_IO32(0x1000B404, data = core->rsa.readMod(1)) // RSA_MOD1
                DEF_IO32(0x1000B408, data = core->rsa.readMod(2)) // RSA_MOD2
                DEF_IO32(0x1000B40C, data = core->rsa.readMod(3)) // RSA_MOD3
                DEF_IO32(0x1000B410, data = core->rsa.readMod(4)) // RSA_MOD4
                DEF_IO32(0x1000B414, data = core->rsa.readMod(5)) // RSA_MOD5
                DEF_IO32(0x1000B418, data = core->rsa.readMod(6)) // RSA_MOD6
                DEF_IO32(0x1000B41C, data = core->rsa.readMod(7)) // RSA_MOD7
                DEF_IO32(0x1000B420, data = core->rsa.readMod(8)) // RSA_MOD8
                DEF_IO32(0x1000B424, data = core->rsa.readMod(9)) // RSA_MOD9
                DEF_IO32(0x1000B428, data = core->rsa.readMod(10)) // RSA_MOD10
                DEF_IO32(0x1000B42C, data = core->rsa.readMod(11)) // RSA_MOD11
                DEF_IO32(0x1000B430, data = core->rsa.readMod(12)) // RSA_MOD12
                DEF_IO32(0x1000B434, data = core->rsa.readMod(13)) // RSA_MOD13
                DEF_IO32(0x1000B438, data = core->rsa.readMod(14)) // RSA_MOD14
                DEF_IO32(0x1000B43C, data = core->rsa.readMod(15)) // RSA_MOD15
                DEF_IO32(0x1000B440, data = core->rsa.readMod(16)) // RSA_MOD16
                DEF_IO32(0x1000B444, data = core->rsa.readMod(17)) // RSA_MOD17
                DEF_IO32(0x1000B448, data = core->rsa.readMod(18)) // RSA_MOD18
                DEF_IO32(0x1000B44C, data = core->rsa.readMod(19)) // RSA_MOD19
                DEF_IO32(0x1000B450, data = core->rsa.readMod(20)) // RSA_MOD20
                DEF_IO32(0x1000B454, data = core->rsa.readMod(21)) // RSA_MOD21
                DEF_IO32(0x1000B458, data = core->rsa.readMod(22)) // RSA_MOD22
                DEF_IO32(0x1000B45C, data = core->rsa.readMod(23)) // RSA_MOD23
                DEF_IO32(0x1000B460, data = core->rsa.readMod(24)) // RSA_MOD24
                DEF_IO32(0x1000B464, data = core->rsa.readMod(25)) // RSA_MOD25
                DEF_IO32(0x1000B468, data = core->rsa.readMod(26)) // RSA_MOD26
                DEF_IO32(0x1000B46C, data = core->rsa.readMod(27)) // RSA_MOD27
                DEF_IO32(0x1000B470, data = core->rsa.readMod(28)) // RSA_MOD28
                DEF_IO32(0x1000B474, data = core->rsa.readMod(29)) // RSA_MOD29
                DEF_IO32(0x1000B478, data = core->rsa.readMod(30)) // RSA_MOD30
                DEF_IO32(0x1000B47C, data = core->rsa.readMod(31)) // RSA_MOD31
                DEF_IO32(0x1000B480, data = core->rsa.readMod(32)) // RSA_MOD32
                DEF_IO32(0x1000B484, data = core->rsa.readMod(33)) // RSA_MOD33
                DEF_IO32(0x1000B488, data = core->rsa.readMod(34)) // RSA_MOD34
                DEF_IO32(0x1000B48C, data = core->rsa.readMod(35)) // RSA_MOD35
                DEF_IO32(0x1000B490, data = core->rsa.readMod(36)) // RSA_MOD36
                DEF_IO32(0x1000B494, data = core->rsa.readMod(37)) // RSA_MOD37
                DEF_IO32(0x1000B498, data = core->rsa.readMod(38)) // RSA_MOD38
                DEF_IO32(0x1000B49C, data = core->rsa.readMod(39)) // RSA_MOD39
                DEF_IO32(0x1000B4A0, data = core->rsa.readMod(40)) // RSA_MOD40
                DEF_IO32(0x1000B4A4, data = core->rsa.readMod(41)) // RSA_MOD41
                DEF_IO32(0x1000B4A8, data = core->rsa.readMod(42)) // RSA_MOD42
                DEF_IO32(0x1000B4AC, data = core->rsa.readMod(43)) // RSA_MOD43
                DEF_IO32(0x1000B4B0, data = core->rsa.readMod(44)) // RSA_MOD44
                DEF_IO32(0x1000B4B4, data = core->rsa.readMod(45)) // RSA_MOD45
                DEF_IO32(0x1000B4B8, data = core->rsa.readMod(46)) // RSA_MOD46
                DEF_IO32(0x1000B4BC, data = core->rsa.readMod(47)) // RSA_MOD47
                DEF_IO32(0x1000B4C0, data = core->rsa.readMod(48)) // RSA_MOD48
                DEF_IO32(0x1000B4C4, data = core->rsa.readMod(49)) // RSA_MOD49
                DEF_IO32(0x1000B4C8, data = core->rsa.readMod(50)) // RSA_MOD50
                DEF_IO32(0x1000B4CC, data = core->rsa.readMod(51)) // RSA_MOD51
                DEF_IO32(0x1000B4D0, data = core->rsa.readMod(52)) // RSA_MOD52
                DEF_IO32(0x1000B4D4, data = core->rsa.readMod(53)) // RSA_MOD53
                DEF_IO32(0x1000B4D8, data = core->rsa.readMod(54)) // RSA_MOD54
                DEF_IO32(0x1000B4DC, data = core->rsa.readMod(55)) // RSA_MOD55
                DEF_IO32(0x1000B4E0, data = core->rsa.readMod(56)) // RSA_MOD56
                DEF_IO32(0x1000B4E4, data = core->rsa.readMod(57)) // RSA_MOD57
                DEF_IO32(0x1000B4E8, data = core->rsa.readMod(58)) // RSA_MOD58
                DEF_IO32(0x1000B4EC, data = core->rsa.readMod(59)) // RSA_MOD59
                DEF_IO32(0x1000B4F0, data = core->rsa.readMod(60)) // RSA_MOD60
                DEF_IO32(0x1000B4F4, data = core->rsa.readMod(61)) // RSA_MOD61
                DEF_IO32(0x1000B4F8, data = core->rsa.readMod(62)) // RSA_MOD62
                DEF_IO32(0x1000B4FC, data = core->rsa.readMod(63)) // RSA_MOD63
                DEF_IO32(0x1000B800, data = core->rsa.readData(0)) // RSA_DATA0
                DEF_IO32(0x1000B804, data = core->rsa.readData(1)) // RSA_DATA1
                DEF_IO32(0x1000B808, data = core->rsa.readData(2)) // RSA_DATA2
                DEF_IO32(0x1000B80C, data = core->rsa.readData(3)) // RSA_DATA3
                DEF_IO32(0x1000B810, data = core->rsa.readData(4)) // RSA_DATA4
                DEF_IO32(0x1000B814, data = core->rsa.readData(5)) // RSA_DATA5
                DEF_IO32(0x1000B818, data = core->rsa.readData(6)) // RSA_DATA6
                DEF_IO32(0x1000B81C, data = core->rsa.readData(7)) // RSA_DATA7
                DEF_IO32(0x1000B820, data = core->rsa.readData(8)) // RSA_DATA8
                DEF_IO32(0x1000B824, data = core->rsa.readData(9)) // RSA_DATA9
                DEF_IO32(0x1000B828, data = core->rsa.readData(10)) // RSA_DATA10
                DEF_IO32(0x1000B82C, data = core->rsa.readData(11)) // RSA_DATA11
                DEF_IO32(0x1000B830, data = core->rsa.readData(12)) // RSA_DATA12
                DEF_IO32(0x1000B834, data = core->rsa.readData(13)) // RSA_DATA13
                DEF_IO32(0x1000B838, data = core->rsa.readData(14)) // RSA_DATA14
                DEF_IO32(0x1000B83C, data = core->rsa.readData(15)) // RSA_DATA15
                DEF_IO32(0x1000B840, data = core->rsa.readData(16)) // RSA_DATA16
                DEF_IO32(0x1000B844, data = core->rsa.readData(17)) // RSA_DATA17
                DEF_IO32(0x1000B848, data = core->rsa.readData(18)) // RSA_DATA18
                DEF_IO32(0x1000B84C, data = core->rsa.readData(19)) // RSA_DATA19
                DEF_IO32(0x1000B850, data = core->rsa.readData(20)) // RSA_DATA20
                DEF_IO32(0x1000B854, data = core->rsa.readData(21)) // RSA_DATA21
                DEF_IO32(0x1000B858, data = core->rsa.readData(22)) // RSA_DATA22
                DEF_IO32(0x1000B85C, data = core->rsa.readData(23)) // RSA_DATA23
                DEF_IO32(0x1000B860, data = core->rsa.readData(24)) // RSA_DATA24
                DEF_IO32(0x1000B864, data = core->rsa.readData(25)) // RSA_DATA25
                DEF_IO32(0x1000B868, data = core->rsa.readData(26)) // RSA_DATA26
                DEF_IO32(0x1000B86C, data = core->rsa.readData(27)) // RSA_DATA27
                DEF_IO32(0x1000B870, data = core->rsa.readData(28)) // RSA_DATA28
                DEF_IO32(0x1000B874, data = core->rsa.readData(29)) // RSA_DATA29
                DEF_IO32(0x1000B878, data = core->rsa.readData(30)) // RSA_DATA30
                DEF_IO32(0x1000B87C, data = core->rsa.readData(31)) // RSA_DATA31
                DEF_IO32(0x1000B880, data = core->rsa.readData(32)) // RSA_DATA32
                DEF_IO32(0x1000B884, data = core->rsa.readData(33)) // RSA_DATA33
                DEF_IO32(0x1000B888, data = core->rsa.readData(34)) // RSA_DATA34
                DEF_IO32(0x1000B88C, data = core->rsa.readData(35)) // RSA_DATA35
                DEF_IO32(0x1000B890, data = core->rsa.readData(36)) // RSA_DATA36
                DEF_IO32(0x1000B894, data = core->rsa.readData(37)) // RSA_DATA37
                DEF_IO32(0x1000B898, data = core->rsa.readData(38)) // RSA_DATA38
                DEF_IO32(0x1000B89C, data = core->rsa.readData(39)) // RSA_DATA39
                DEF_IO32(0x1000B8A0, data = core->rsa.readData(40)) // RSA_DATA40
                DEF_IO32(0x1000B8A4, data = core->rsa.readData(41)) // RSA_DATA41
                DEF_IO32(0x1000B8A8, data = core->rsa.readData(42)) // RSA_DATA42
                DEF_IO32(0x1000B8AC, data = core->rsa.readData(43)) // RSA_DATA43
                DEF_IO32(0x1000B8B0, data = core->rsa.readData(44)) // RSA_DATA44
                DEF_IO32(0x1000B8B4, data = core->rsa.readData(45)) // RSA_DATA45
                DEF_IO32(0x1000B8B8, data = core->rsa.readData(46)) // RSA_DATA46
                DEF_IO32(0x1000B8BC, data = core->rsa.readData(47)) // RSA_DATA47
                DEF_IO32(0x1000B8C0, data = core->rsa.readData(48)) // RSA_DATA48
                DEF_IO32(0x1000B8C4, data = core->rsa.readData(49)) // RSA_DATA49
                DEF_IO32(0x1000B8C8, data = core->rsa.readData(50)) // RSA_DATA50
                DEF_IO32(0x1000B8CC, data = core->rsa.readData(51)) // RSA_DATA51
                DEF_IO32(0x1000B8D0, data = core->rsa.readData(52)) // RSA_DATA52
                DEF_IO32(0x1000B8D4, data = core->rsa.readData(53)) // RSA_DATA53
                DEF_IO32(0x1000B8D8, data = core->rsa.readData(54)) // RSA_DATA54
                DEF_IO32(0x1000B8DC, data = core->rsa.readData(55)) // RSA_DATA55
                DEF_IO32(0x1000B8E0, data = core->rsa.readData(56)) // RSA_DATA56
                DEF_IO32(0x1000B8E4, data = core->rsa.readData(57)) // RSA_DATA57
                DEF_IO32(0x1000B8E8, data = core->rsa.readData(58)) // RSA_DATA58
                DEF_IO32(0x1000B8EC, data = core->rsa.readData(59)) // RSA_DATA59
                DEF_IO32(0x1000B8F0, data = core->rsa.readData(60)) // RSA_DATA60
                DEF_IO32(0x1000B8F4, data = core->rsa.readData(61)) // RSA_DATA61
                DEF_IO32(0x1000B8F8, data = core->rsa.readData(62)) // RSA_DATA62
                DEF_IO32(0x1000B8FC, data = core->rsa.readData(63)) // RSA_DATA63
                DEF_IO32(0x1000C000, data = core->cdmas[XDMA].readCsr(8)) // XDMA_DSR
                DEF_IO32(0x1000C004, data = core->cdmas[XDMA].readCpc(8)) // XDMA_DPC
                DEF_IO32(0x1000C020, data = core->cdmas[XDMA].readInten()) // XDMA_INTEN
                DEF_IO32(0x1000C024, data = core->cdmas[XDMA].readIntEventRis()) // XDMA_INT_EVENT_RIS
                DEF_IO32(0x1000C028, data = core->cdmas[XDMA].readIntmis()) // XDMA_INTMIS
                DEF_IO32(0x1000C030, data = core->cdmas[XDMA].readFsrd()) // XDMA_FSRD
                DEF_IO32(0x1000C034, data = core->cdmas[XDMA].readFsrc()) // XDMA_FSRC
                DEF_IO32(0x1000C038, data = core->cdmas[XDMA].readFtr(8)) // XDMA_FTRD
                DEF_IO32(0x1000C040, data = core->cdmas[XDMA].readFtr(0)) // XDMA_FTR0
                DEF_IO32(0x1000C044, data = core->cdmas[XDMA].readFtr(1)) // XDMA_FTR1
                DEF_IO32(0x1000C048, data = core->cdmas[XDMA].readFtr(2)) // XDMA_FTR2
                DEF_IO32(0x1000C04C, data = core->cdmas[XDMA].readFtr(3)) // XDMA_FTR3
                DEF_IO32(0x1000C100, data = core->cdmas[XDMA].readCsr(0)) // XDMA_CSR0
                DEF_IO32(0x1000C104, data = core->cdmas[XDMA].readCpc(0)) // XDMA_CPC0
                DEF_IO32(0x1000C108, data = core->cdmas[XDMA].readCsr(2)) // XDMA_CSR1
                DEF_IO32(0x1000C10C, data = core->cdmas[XDMA].readCpc(1)) // XDMA_CPC1
                DEF_IO32(0x1000C110, data = core->cdmas[XDMA].readCsr(2)) // XDMA_CSR2
                DEF_IO32(0x1000C114, data = core->cdmas[XDMA].readCpc(2)) // XDMA_CPC2
                DEF_IO32(0x1000C118, data = core->cdmas[XDMA].readCsr(3)) // XDMA_CSR3
                DEF_IO32(0x1000C11C, data = core->cdmas[XDMA].readCpc(3)) // XDMA_CPC3
                DEF_IO32(0x1000C400, data = core->cdmas[XDMA].readSar(0)) // XDMA_SAR0
                DEF_IO32(0x1000C404, data = core->cdmas[XDMA].readDar(0)) // XDMA_DAR0
                DEF_IO32(0x1000C408, data = core->cdmas[XDMA].readCcr(0)) // XDMA_CCR0
                DEF_IO32(0x1000C40C, data = core->cdmas[XDMA].readLc0(0)) // XDMA_LC0_0
                DEF_IO32(0x1000C410, data = core->cdmas[XDMA].readLc1(0)) // XDMA_LC1_0
                DEF_IO32(0x1000C420, data = core->cdmas[XDMA].readSar(1)) // XDMA_SAR1
                DEF_IO32(0x1000C424, data = core->cdmas[XDMA].readDar(1)) // XDMA_DAR1
                DEF_IO32(0x1000C428, data = core->cdmas[XDMA].readCcr(1)) // XDMA_CCR1
                DEF_IO32(0x1000C42C, data = core->cdmas[XDMA].readLc0(1)) // XDMA_LC0_1
                DEF_IO32(0x1000C430, data = core->cdmas[XDMA].readLc1(1)) // XDMA_LC1_1
                DEF_IO32(0x1000C440, data = core->cdmas[XDMA].readSar(2)) // XDMA_SAR2
                DEF_IO32(0x1000C444, data = core->cdmas[XDMA].readDar(2)) // XDMA_DAR2
                DEF_IO32(0x1000C448, data = core->cdmas[XDMA].readCcr(2)) // XDMA_CCR2
                DEF_IO32(0x1000C44C, data = core->cdmas[XDMA].readLc0(2)) // XDMA_LC0_2
                DEF_IO32(0x1000C450, data = core->cdmas[XDMA].readLc1(2)) // XDMA_LC1_2
                DEF_IO32(0x1000C460, data = core->cdmas[XDMA].readSar(3)) // XDMA_SAR3
                DEF_IO32(0x1000C464, data = core->cdmas[XDMA].readDar(3)) // XDMA_DAR3
                DEF_IO32(0x1000C468, data = core->cdmas[XDMA].readCcr(3)) // XDMA_CCR3
                DEF_IO32(0x1000C46C, data = core->cdmas[XDMA].readLc0(3)) // XDMA_LC0_3
                DEF_IO32(0x1000C470, data = core->cdmas[XDMA].readLc1(3)) // XDMA_LC1_3
                DEF_IO32(0x1000CD00, data = core->cdmas[XDMA].readDbgstatus()) // XDMA_DBGSTATUS
                DEF_IO32(0x1000CD08, data = core->cdmas[XDMA].readDbginst0()) // XDMA_DBGINST0
                DEF_IO32(0x1000CD0C, data = core->cdmas[XDMA].readDbginst1()) // XDMA_DBGINST1
                DEF_IO32(0x1000D800, data = core->cartridge.readSpiFifoCnt()) // SPICARD_FIFO_CNT
                DEF_IO32(0x1000D804, data = core->cartridge.readSpiFifoSelect()) // SPICARD_FIFO_SELECT
                DEF_IO32(0x1000D808, data = core->cartridge.readSpiFifoBlklen()) // SPICARD_FIFO_BLKLEN
                DEF_IO32(0x1000D80C, data = core->cartridge.readSpiFifoData()) // SPICARD_FIFO_DATA
                DEF_IO32(0x1000D818, data = core->cartridge.readSpiFifoIntMask()) // SPICARD_FIFO_INT_MASK
                DEF_IO32(0x1000D81C, data = core->cartridge.readSpiFifoIntStat()) // SPICARD_FIFO_INT_STAT
                DEF_IO32(0x10012000, data = readOtpEncrypted(0)) // OTP_ENCRYPTED0
                DEF_IO32(0x10012004, data = readOtpEncrypted(1)) // OTP_ENCRYPTED1
                DEF_IO32(0x10012008, data = readOtpEncrypted(2)) // OTP_ENCRYPTED2
                DEF_IO32(0x1001200C, data = readOtpEncrypted(3)) // OTP_ENCRYPTED3
                DEF_IO32(0x10012010, data = readOtpEncrypted(4)) // OTP_ENCRYPTED4
                DEF_IO32(0x10012014, data = readOtpEncrypted(5)) // OTP_ENCRYPTED5
                DEF_IO32(0x10012018, data = readOtpEncrypted(6)) // OTP_ENCRYPTED6
                DEF_IO32(0x1001201C, data = readOtpEncrypted(7)) // OTP_ENCRYPTED7
                DEF_IO32(0x10012020, data = readOtpEncrypted(8)) // OTP_ENCRYPTED8
                DEF_IO32(0x10012024, data = readOtpEncrypted(9)) // OTP_ENCRYPTED9
                DEF_IO32(0x10012028, data = readOtpEncrypted(10)) // OTP_ENCRYPTED10
                DEF_IO32(0x1001202C, data = readOtpEncrypted(11)) // OTP_ENCRYPTED11
                DEF_IO32(0x10012030, data = readOtpEncrypted(12)) // OTP_ENCRYPTED12
                DEF_IO32(0x10012034, data = readOtpEncrypted(13)) // OTP_ENCRYPTED13
                DEF_IO32(0x10012038, data = readOtpEncrypted(14)) // OTP_ENCRYPTED14
                DEF_IO32(0x1001203C, data = readOtpEncrypted(15)) // OTP_ENCRYPTED15
                DEF_IO32(0x10012040, data = readOtpEncrypted(16)) // OTP_ENCRYPTED16
                DEF_IO32(0x10012044, data = readOtpEncrypted(17)) // OTP_ENCRYPTED17
                DEF_IO32(0x10012048, data = readOtpEncrypted(18)) // OTP_ENCRYPTED18
                DEF_IO32(0x1001204C, data = readOtpEncrypted(19)) // OTP_ENCRYPTED19
                DEF_IO32(0x10012050, data = readOtpEncrypted(20)) // OTP_ENCRYPTED20
                DEF_IO32(0x10012054, data = readOtpEncrypted(21)) // OTP_ENCRYPTED21
                DEF_IO32(0x10012058, data = readOtpEncrypted(22)) // OTP_ENCRYPTED22
                DEF_IO32(0x1001205C, data = readOtpEncrypted(23)) // OTP_ENCRYPTED23
                DEF_IO32(0x10012060, data = readOtpEncrypted(24)) // OTP_ENCRYPTED24
                DEF_IO32(0x10012064, data = readOtpEncrypted(25)) // OTP_ENCRYPTED25
                DEF_IO32(0x10012068, data = readOtpEncrypted(26)) // OTP_ENCRYPTED26
                DEF_IO32(0x1001206C, data = readOtpEncrypted(27)) // OTP_ENCRYPTED27
                DEF_IO32(0x10012070, data = readOtpEncrypted(28)) // OTP_ENCRYPTED28
                DEF_IO32(0x10012074, data = readOtpEncrypted(29)) // OTP_ENCRYPTED29
                DEF_IO32(0x10012078, data = readOtpEncrypted(30)) // OTP_ENCRYPTED30
                DEF_IO32(0x1001207C, data = readOtpEncrypted(31)) // OTP_ENCRYPTED31
                DEF_IO32(0x10012080, data = readOtpEncrypted(32)) // OTP_ENCRYPTED32
                DEF_IO32(0x10012084, data = readOtpEncrypted(33)) // OTP_ENCRYPTED33
                DEF_IO32(0x10012088, data = readOtpEncrypted(34)) // OTP_ENCRYPTED34
                DEF_IO32(0x1001208C, data = readOtpEncrypted(35)) // OTP_ENCRYPTED35
                DEF_IO32(0x10012090, data = readOtpEncrypted(36)) // OTP_ENCRYPTED36
                DEF_IO32(0x10012094, data = readOtpEncrypted(37)) // OTP_ENCRYPTED37
                DEF_IO32(0x10012098, data = readOtpEncrypted(38)) // OTP_ENCRYPTED38
                DEF_IO32(0x1001209C, data = readOtpEncrypted(39)) // OTP_ENCRYPTED39
                DEF_IO32(0x100120A0, data = readOtpEncrypted(40)) // OTP_ENCRYPTED40
                DEF_IO32(0x100120A4, data = readOtpEncrypted(41)) // OTP_ENCRYPTED41
                DEF_IO32(0x100120A8, data = readOtpEncrypted(42)) // OTP_ENCRYPTED42
                DEF_IO32(0x100120AC, data = readOtpEncrypted(43)) // OTP_ENCRYPTED43
                DEF_IO32(0x100120B0, data = readOtpEncrypted(44)) // OTP_ENCRYPTED44
                DEF_IO32(0x100120B4, data = readOtpEncrypted(45)) // OTP_ENCRYPTED45
                DEF_IO32(0x100120B8, data = readOtpEncrypted(46)) // OTP_ENCRYPTED46
                DEF_IO32(0x100120BC, data = readOtpEncrypted(47)) // OTP_ENCRYPTED47
                DEF_IO32(0x100120C0, data = readOtpEncrypted(48)) // OTP_ENCRYPTED48
                DEF_IO32(0x100120C4, data = readOtpEncrypted(49)) // OTP_ENCRYPTED49
                DEF_IO32(0x100120C8, data = readOtpEncrypted(50)) // OTP_ENCRYPTED50
                DEF_IO32(0x100120CC, data = readOtpEncrypted(51)) // OTP_ENCRYPTED51
                DEF_IO32(0x100120D0, data = readOtpEncrypted(52)) // OTP_ENCRYPTED52
                DEF_IO32(0x100120D4, data = readOtpEncrypted(53)) // OTP_ENCRYPTED53
                DEF_IO32(0x100120D8, data = readOtpEncrypted(54)) // OTP_ENCRYPTED54
                DEF_IO32(0x100120DC, data = readOtpEncrypted(55)) // OTP_ENCRYPTED55
                DEF_IO32(0x100120E0, data = readOtpEncrypted(56)) // OTP_ENCRYPTED56
                DEF_IO32(0x100120E4, data = readOtpEncrypted(57)) // OTP_ENCRYPTED57
                DEF_IO32(0x100120E8, data = readOtpEncrypted(58)) // OTP_ENCRYPTED58
                DEF_IO32(0x100120EC, data = readOtpEncrypted(59)) // OTP_ENCRYPTED59
                DEF_IO32(0x100120F0, data = readOtpEncrypted(60)) // OTP_ENCRYPTED60
                DEF_IO32(0x100120F4, data = readOtpEncrypted(61)) // OTP_ENCRYPTED61
                DEF_IO32(0x100120F8, data = readOtpEncrypted(62)) // OTP_ENCRYPTED62
                DEF_IO32(0x100120FC, data = readOtpEncrypted(63)) // OTP_ENCRYPTED63
            }
        }

        // Catch reads from unknown I/O registers
        if (id == ARM9)
            LOG_WARN("Unknown ARM9 I/O read: 0x%X\n", base);
        else
            LOG_WARN("Unknown ARM11 core %d I/O read: 0x%X\n", id, base);
        return value;

    next:
        // Process the data and loop until a full value is formed
        value |= (data >> (base << 3)) << (i << 3);
        i += size - base;
    }
    return value;
}

template <typename T> void Memory::ioWrite(CpuId id, uint32_t address, T value) {
    // Mirror the ARM11 DSP register area
    if (id != ARM9 && (address >> 12) == 0x10203)
        address &= 0xFFFFF03F;

    // Write a value to one or more I/O registers
    for (uint32_t i = 0; i < sizeof(T);) {
        uint32_t base = address + i, size;
        uint32_t mask = (1ULL << ((sizeof(T) - i) << 3)) - 1;
        uint32_t data = value >> (i << 3);

        // Check registers that are shared between CPUs
        switch (base) {
            DEF_IO32(0x10101000, core->shas[0].writeCnt(IO_PARAMS)) // SHA_CNT11
            DEF_IO32(0x10101004, core->shas[0].writeBlkcnt(IO_PARAMS)) // SHA_BLKCNT11
            DEF_IO32(0x10101040, core->shas[0].writeHash(0, IO_PARAMS)) // SHA_HASH0_11
            DEF_IO32(0x10101044, core->shas[0].writeHash(1, IO_PARAMS)) // SHA_HASH1_11
            DEF_IO32(0x10101048, core->shas[0].writeHash(2, IO_PARAMS)) // SHA_HASH2_11
            DEF_IO32(0x1010104C, core->shas[0].writeHash(3, IO_PARAMS)) // SHA_HASH3_11
            DEF_IO32(0x10101050, core->shas[0].writeHash(4, IO_PARAMS)) // SHA_HASH4_11
            DEF_IO32(0x10101054, core->shas[0].writeHash(5, IO_PARAMS)) // SHA_HASH5_11
            DEF_IO32(0x10101058, core->shas[0].writeHash(6, IO_PARAMS)) // SHA_HASH6_11
            DEF_IO32(0x1010105C, core->shas[0].writeHash(7, IO_PARAMS)) // SHA_HASH7_11
            DEF_IO16(0x10103000, core->csnd.writeMainVol(IO_PARAMS)) // CSND_MAIN_VOL
            DEF_IO16(0x10103002, core->csnd.writeMainCnt(IO_PARAMS)) // CSND_MAIN_CNT
            DEF_IO16(0x10103400, core->csnd.writeChanCnt(0, IO_PARAMS)) // CSND_CHAN0_CNT
            DEF_IO16(0x10103402, core->csnd.writeChanRate(0, IO_PARAMS)) // CSND_CHAN0_RATE
            DEF_IO16(0x10103404, core->csnd.writeChanRvol(0, IO_PARAMS)) // CSND_CHAN0_RVOL
            DEF_IO16(0x10103406, core->csnd.writeChanLvol(0, IO_PARAMS)) // CSND_CHAN0_LVOL
            DEF_IO32(0x1010340C, core->csnd.writeChanStart(0, IO_PARAMS)) // CSND_CHAN0_START
            DEF_IO32(0x10103410, core->csnd.writeChanSize(0, IO_PARAMS)) // CSND_CHAN0_SIZE
            DEF_IO32(0x10103414, core->csnd.writeChanLoop(0, IO_PARAMS)) // CSND_CHAN0_LOOP
            DEF_IO32(0x10103418, core->csnd.writeAdpcmStart(0, IO_PARAMS)) // CSND_ADPCM0_START
            DEF_IO32(0x1010341C, core->csnd.writeAdpcmLoop(0, IO_PARAMS)) // CSND_ADPCM0_LOOP
            DEF_IO16(0x10103420, core->csnd.writeChanCnt(1, IO_PARAMS)) // CSND_CHAN1_CNT
            DEF_IO16(0x10103422, core->csnd.writeChanRate(1, IO_PARAMS)) // CSND_CHAN1_RATE
            DEF_IO16(0x10103424, core->csnd.writeChanRvol(1, IO_PARAMS)) // CSND_CHAN1_RVOL
            DEF_IO16(0x10103426, core->csnd.writeChanLvol(1, IO_PARAMS)) // CSND_CHAN1_LVOL
            DEF_IO32(0x1010342C, core->csnd.writeChanStart(1, IO_PARAMS)) // CSND_CHAN1_START
            DEF_IO32(0x10103430, core->csnd.writeChanSize(1, IO_PARAMS)) // CSND_CHAN1_SIZE
            DEF_IO32(0x10103434, core->csnd.writeChanLoop(1, IO_PARAMS)) // CSND_CHAN1_LOOP
            DEF_IO32(0x10103438, core->csnd.writeAdpcmStart(1, IO_PARAMS)) // CSND_ADPCM1_START
            DEF_IO32(0x1010343C, core->csnd.writeAdpcmLoop(1, IO_PARAMS)) // CSND_ADPCM1_LOOP
            DEF_IO16(0x10103440, core->csnd.writeChanCnt(2, IO_PARAMS)) // CSND_CHAN2_CNT
            DEF_IO16(0x10103442, core->csnd.writeChanRate(2, IO_PARAMS)) // CSND_CHAN2_RATE
            DEF_IO16(0x10103444, core->csnd.writeChanRvol(2, IO_PARAMS)) // CSND_CHAN2_RVOL
            DEF_IO16(0x10103446, core->csnd.writeChanLvol(2, IO_PARAMS)) // CSND_CHAN2_LVOL
            DEF_IO32(0x1010344C, core->csnd.writeChanStart(2, IO_PARAMS)) // CSND_CHAN2_START
            DEF_IO32(0x10103450, core->csnd.writeChanSize(2, IO_PARAMS)) // CSND_CHAN2_SIZE
            DEF_IO32(0x10103454, core->csnd.writeChanLoop(2, IO_PARAMS)) // CSND_CHAN2_LOOP
            DEF_IO32(0x10103458, core->csnd.writeAdpcmStart(2, IO_PARAMS)) // CSND_ADPCM2_START
            DEF_IO32(0x1010345C, core->csnd.writeAdpcmLoop(2, IO_PARAMS)) // CSND_ADPCM2_LOOP
            DEF_IO16(0x10103460, core->csnd.writeChanCnt(3, IO_PARAMS)) // CSND_CHAN3_CNT
            DEF_IO16(0x10103462, core->csnd.writeChanRate(3, IO_PARAMS)) // CSND_CHAN3_RATE
            DEF_IO16(0x10103464, core->csnd.writeChanRvol(3, IO_PARAMS)) // CSND_CHAN3_RVOL
            DEF_IO16(0x10103466, core->csnd.writeChanLvol(3, IO_PARAMS)) // CSND_CHAN3_LVOL
            DEF_IO32(0x1010346C, core->csnd.writeChanStart(3, IO_PARAMS)) // CSND_CHAN3_START
            DEF_IO32(0x10103470, core->csnd.writeChanSize(3, IO_PARAMS)) // CSND_CHAN3_SIZE
            DEF_IO32(0x10103474, core->csnd.writeChanLoop(3, IO_PARAMS)) // CSND_CHAN3_LOOP
            DEF_IO32(0x10103478, core->csnd.writeAdpcmStart(3, IO_PARAMS)) // CSND_ADPCM3_START
            DEF_IO32(0x1010347C, core->csnd.writeAdpcmLoop(3, IO_PARAMS)) // CSND_ADPCM3_LOOP
            DEF_IO16(0x10103480, core->csnd.writeChanCnt(4, IO_PARAMS)) // CSND_CHAN4_CNT
            DEF_IO16(0x10103482, core->csnd.writeChanRate(4, IO_PARAMS)) // CSND_CHAN4_RATE
            DEF_IO16(0x10103484, core->csnd.writeChanRvol(4, IO_PARAMS)) // CSND_CHAN4_RVOL
            DEF_IO16(0x10103486, core->csnd.writeChanLvol(4, IO_PARAMS)) // CSND_CHAN4_LVOL
            DEF_IO32(0x1010348C, core->csnd.writeChanStart(4, IO_PARAMS)) // CSND_CHAN4_START
            DEF_IO32(0x10103490, core->csnd.writeChanSize(4, IO_PARAMS)) // CSND_CHAN4_SIZE
            DEF_IO32(0x10103494, core->csnd.writeChanLoop(4, IO_PARAMS)) // CSND_CHAN4_LOOP
            DEF_IO32(0x10103498, core->csnd.writeAdpcmStart(4, IO_PARAMS)) // CSND_ADPCM4_START
            DEF_IO32(0x1010349C, core->csnd.writeAdpcmLoop(4, IO_PARAMS)) // CSND_ADPCM4_LOOP
            DEF_IO16(0x101034A0, core->csnd.writeChanCnt(5, IO_PARAMS)) // CSND_CHAN5_CNT
            DEF_IO16(0x101034A2, core->csnd.writeChanRate(5, IO_PARAMS)) // CSND_CHAN5_RATE
            DEF_IO16(0x101034A4, core->csnd.writeChanRvol(5, IO_PARAMS)) // CSND_CHAN5_RVOL
            DEF_IO16(0x101034A6, core->csnd.writeChanLvol(5, IO_PARAMS)) // CSND_CHAN5_LVOL
            DEF_IO32(0x101034AC, core->csnd.writeChanStart(5, IO_PARAMS)) // CSND_CHAN5_START
            DEF_IO32(0x101034B0, core->csnd.writeChanSize(5, IO_PARAMS)) // CSND_CHAN5_SIZE
            DEF_IO32(0x101034B4, core->csnd.writeChanLoop(5, IO_PARAMS)) // CSND_CHAN5_LOOP
            DEF_IO32(0x101034B8, core->csnd.writeAdpcmStart(5, IO_PARAMS)) // CSND_ADPCM5_START
            DEF_IO32(0x101034BC, core->csnd.writeAdpcmLoop(5, IO_PARAMS)) // CSND_ADPCM5_LOOP
            DEF_IO16(0x101034C0, core->csnd.writeChanCnt(6, IO_PARAMS)) // CSND_CHAN6_CNT
            DEF_IO16(0x101034C2, core->csnd.writeChanRate(6, IO_PARAMS)) // CSND_CHAN6_RATE
            DEF_IO16(0x101034C4, core->csnd.writeChanRvol(6, IO_PARAMS)) // CSND_CHAN6_RVOL
            DEF_IO16(0x101034C6, core->csnd.writeChanLvol(6, IO_PARAMS)) // CSND_CHAN6_LVOL
            DEF_IO32(0x101034CC, core->csnd.writeChanStart(6, IO_PARAMS)) // CSND_CHAN6_START
            DEF_IO32(0x101034D0, core->csnd.writeChanSize(6, IO_PARAMS)) // CSND_CHAN6_SIZE
            DEF_IO32(0x101034D4, core->csnd.writeChanLoop(6, IO_PARAMS)) // CSND_CHAN6_LOOP
            DEF_IO32(0x101034D8, core->csnd.writeAdpcmStart(6, IO_PARAMS)) // CSND_ADPCM6_START
            DEF_IO32(0x101034DC, core->csnd.writeAdpcmLoop(6, IO_PARAMS)) // CSND_ADPCM6_LOOP
            DEF_IO16(0x101034E0, core->csnd.writeChanCnt(7, IO_PARAMS)) // CSND_CHAN7_CNT
            DEF_IO16(0x101034E2, core->csnd.writeChanRate(7, IO_PARAMS)) // CSND_CHAN7_RATE
            DEF_IO16(0x101034E4, core->csnd.writeChanRvol(7, IO_PARAMS)) // CSND_CHAN7_RVOL
            DEF_IO16(0x101034E6, core->csnd.writeChanLvol(7, IO_PARAMS)) // CSND_CHAN7_LVOL
            DEF_IO32(0x101034EC, core->csnd.writeChanStart(7, IO_PARAMS)) // CSND_CHAN7_START
            DEF_IO32(0x101034F0, core->csnd.writeChanSize(7, IO_PARAMS)) // CSND_CHAN7_SIZE
            DEF_IO32(0x101034F4, core->csnd.writeChanLoop(7, IO_PARAMS)) // CSND_CHAN7_LOOP
            DEF_IO32(0x101034F8, core->csnd.writeAdpcmStart(7, IO_PARAMS)) // CSND_ADPCM7_START
            DEF_IO32(0x101034FC, core->csnd.writeAdpcmLoop(7, IO_PARAMS)) // CSND_ADPCM7_LOOP
            DEF_IO16(0x10103500, core->csnd.writeChanCnt(8, IO_PARAMS)) // CSND_CHAN8_CNT
            DEF_IO16(0x10103502, core->csnd.writeChanRate(8, IO_PARAMS)) // CSND_CHAN8_RATE
            DEF_IO16(0x10103504, core->csnd.writeChanRvol(8, IO_PARAMS)) // CSND_CHAN8_RVOL
            DEF_IO16(0x10103506, core->csnd.writeChanLvol(8, IO_PARAMS)) // CSND_CHAN8_LVOL
            DEF_IO32(0x1010350C, core->csnd.writeChanStart(8, IO_PARAMS)) // CSND_CHAN8_START
            DEF_IO32(0x10103510, core->csnd.writeChanSize(8, IO_PARAMS)) // CSND_CHAN8_SIZE
            DEF_IO32(0x10103514, core->csnd.writeChanLoop(8, IO_PARAMS)) // CSND_CHAN8_LOOP
            DEF_IO32(0x10103518, core->csnd.writeAdpcmStart(8, IO_PARAMS)) // CSND_ADPCM8_START
            DEF_IO32(0x1010351C, core->csnd.writeAdpcmLoop(8, IO_PARAMS)) // CSND_ADPCM8_LOOP
            DEF_IO16(0x10103520, core->csnd.writeChanCnt(9, IO_PARAMS)) // CSND_CHAN9_CNT
            DEF_IO16(0x10103522, core->csnd.writeChanRate(9, IO_PARAMS)) // CSND_CHAN9_RATE
            DEF_IO16(0x10103524, core->csnd.writeChanRvol(9, IO_PARAMS)) // CSND_CHAN9_RVOL
            DEF_IO16(0x10103526, core->csnd.writeChanLvol(9, IO_PARAMS)) // CSND_CHAN9_LVOL
            DEF_IO32(0x1010352C, core->csnd.writeChanStart(9, IO_PARAMS)) // CSND_CHAN9_START
            DEF_IO32(0x10103530, core->csnd.writeChanSize(9, IO_PARAMS)) // CSND_CHAN9_SIZE
            DEF_IO32(0x10103534, core->csnd.writeChanLoop(9, IO_PARAMS)) // CSND_CHAN9_LOOP
            DEF_IO32(0x10103538, core->csnd.writeAdpcmStart(9, IO_PARAMS)) // CSND_ADPCM9_START
            DEF_IO32(0x1010353C, core->csnd.writeAdpcmLoop(9, IO_PARAMS)) // CSND_ADPCM9_LOOP
            DEF_IO16(0x10103540, core->csnd.writeChanCnt(10, IO_PARAMS)) // CSND_CHAN10_CNT
            DEF_IO16(0x10103542, core->csnd.writeChanRate(10, IO_PARAMS)) // CSND_CHAN10_RATE
            DEF_IO16(0x10103544, core->csnd.writeChanRvol(10, IO_PARAMS)) // CSND_CHAN10_RVOL
            DEF_IO16(0x10103546, core->csnd.writeChanLvol(10, IO_PARAMS)) // CSND_CHAN10_LVOL
            DEF_IO32(0x1010354C, core->csnd.writeChanStart(10, IO_PARAMS)) // CSND_CHAN10_START
            DEF_IO32(0x10103550, core->csnd.writeChanSize(10, IO_PARAMS)) // CSND_CHAN10_SIZE
            DEF_IO32(0x10103554, core->csnd.writeChanLoop(10, IO_PARAMS)) // CSND_CHAN10_LOOP
            DEF_IO32(0x10103558, core->csnd.writeAdpcmStart(10, IO_PARAMS)) // CSND_ADPCM10_START
            DEF_IO32(0x1010355C, core->csnd.writeAdpcmLoop(10, IO_PARAMS)) // CSND_ADPCM10_LOOP
            DEF_IO16(0x10103560, core->csnd.writeChanCnt(11, IO_PARAMS)) // CSND_CHAN11_CNT
            DEF_IO16(0x10103562, core->csnd.writeChanRate(11, IO_PARAMS)) // CSND_CHAN11_RATE
            DEF_IO16(0x10103564, core->csnd.writeChanRvol(11, IO_PARAMS)) // CSND_CHAN11_RVOL
            DEF_IO16(0x10103566, core->csnd.writeChanLvol(11, IO_PARAMS)) // CSND_CHAN11_LVOL
            DEF_IO32(0x1010356C, core->csnd.writeChanStart(11, IO_PARAMS)) // CSND_CHAN11_START
            DEF_IO32(0x10103570, core->csnd.writeChanSize(11, IO_PARAMS)) // CSND_CHAN11_SIZE
            DEF_IO32(0x10103574, core->csnd.writeChanLoop(11, IO_PARAMS)) // CSND_CHAN11_LOOP
            DEF_IO32(0x10103578, core->csnd.writeAdpcmStart(11, IO_PARAMS)) // CSND_ADPCM11_START
            DEF_IO32(0x1010357C, core->csnd.writeAdpcmLoop(11, IO_PARAMS)) // CSND_ADPCM11_LOOP
            DEF_IO16(0x10103580, core->csnd.writeChanCnt(12, IO_PARAMS)) // CSND_CHAN12_CNT
            DEF_IO16(0x10103582, core->csnd.writeChanRate(12, IO_PARAMS)) // CSND_CHAN12_RATE
            DEF_IO16(0x10103584, core->csnd.writeChanRvol(12, IO_PARAMS)) // CSND_CHAN12_RVOL
            DEF_IO16(0x10103586, core->csnd.writeChanLvol(12, IO_PARAMS)) // CSND_CHAN12_LVOL
            DEF_IO32(0x1010358C, core->csnd.writeChanStart(12, IO_PARAMS)) // CSND_CHAN12_START
            DEF_IO32(0x10103590, core->csnd.writeChanSize(12, IO_PARAMS)) // CSND_CHAN12_SIZE
            DEF_IO32(0x10103594, core->csnd.writeChanLoop(12, IO_PARAMS)) // CSND_CHAN12_LOOP
            DEF_IO32(0x10103598, core->csnd.writeAdpcmStart(12, IO_PARAMS)) // CSND_ADPCM12_START
            DEF_IO32(0x1010359C, core->csnd.writeAdpcmLoop(12, IO_PARAMS)) // CSND_ADPCM12_LOOP
            DEF_IO16(0x101035A0, core->csnd.writeChanCnt(13, IO_PARAMS)) // CSND_CHAN13_CNT
            DEF_IO16(0x101035A2, core->csnd.writeChanRate(13, IO_PARAMS)) // CSND_CHAN13_RATE
            DEF_IO16(0x101035A4, core->csnd.writeChanRvol(13, IO_PARAMS)) // CSND_CHAN13_RVOL
            DEF_IO16(0x101035A6, core->csnd.writeChanLvol(13, IO_PARAMS)) // CSND_CHAN13_LVOL
            DEF_IO32(0x101035AC, core->csnd.writeChanStart(13, IO_PARAMS)) // CSND_CHAN13_START
            DEF_IO32(0x101035B0, core->csnd.writeChanSize(13, IO_PARAMS)) // CSND_CHAN13_SIZE
            DEF_IO32(0x101035B4, core->csnd.writeChanLoop(13, IO_PARAMS)) // CSND_CHAN13_LOOP
            DEF_IO32(0x101035B8, core->csnd.writeAdpcmStart(13, IO_PARAMS)) // CSND_ADPCM13_START
            DEF_IO32(0x101035BC, core->csnd.writeAdpcmLoop(13, IO_PARAMS)) // CSND_ADPCM13_LOOP
            DEF_IO16(0x101035C0, core->csnd.writeChanCnt(14, IO_PARAMS)) // CSND_CHAN14_CNT
            DEF_IO16(0x101035C2, core->csnd.writeChanRate(14, IO_PARAMS)) // CSND_CHAN14_RATE
            DEF_IO16(0x101035C4, core->csnd.writeChanRvol(14, IO_PARAMS)) // CSND_CHAN14_RVOL
            DEF_IO16(0x101035C6, core->csnd.writeChanLvol(14, IO_PARAMS)) // CSND_CHAN14_LVOL
            DEF_IO32(0x101035CC, core->csnd.writeChanStart(14, IO_PARAMS)) // CSND_CHAN14_START
            DEF_IO32(0x101035D0, core->csnd.writeChanSize(14, IO_PARAMS)) // CSND_CHAN14_SIZE
            DEF_IO32(0x101035D4, core->csnd.writeChanLoop(14, IO_PARAMS)) // CSND_CHAN14_LOOP
            DEF_IO32(0x101035D8, core->csnd.writeAdpcmStart(14, IO_PARAMS)) // CSND_ADPCM14_START
            DEF_IO32(0x101035DC, core->csnd.writeAdpcmLoop(14, IO_PARAMS)) // CSND_ADPCM14_LOOP
            DEF_IO16(0x101035E0, core->csnd.writeChanCnt(15, IO_PARAMS)) // CSND_CHAN15_CNT
            DEF_IO16(0x101035E2, core->csnd.writeChanRate(15, IO_PARAMS)) // CSND_CHAN15_RATE
            DEF_IO16(0x101035E4, core->csnd.writeChanRvol(15, IO_PARAMS)) // CSND_CHAN15_RVOL
            DEF_IO16(0x101035E6, core->csnd.writeChanLvol(15, IO_PARAMS)) // CSND_CHAN15_LVOL
            DEF_IO32(0x101035EC, core->csnd.writeChanStart(15, IO_PARAMS)) // CSND_CHAN15_START
            DEF_IO32(0x101035F0, core->csnd.writeChanSize(15, IO_PARAMS)) // CSND_CHAN15_SIZE
            DEF_IO32(0x101035F4, core->csnd.writeChanLoop(15, IO_PARAMS)) // CSND_CHAN15_LOOP
            DEF_IO32(0x101035F8, core->csnd.writeAdpcmStart(15, IO_PARAMS)) // CSND_ADPCM15_START
            DEF_IO32(0x101035FC, core->csnd.writeAdpcmLoop(15, IO_PARAMS)) // CSND_ADPCM15_LOOP
            DEF_IO16(0x10103600, core->csnd.writeChanCnt(16, IO_PARAMS)) // CSND_CHAN16_CNT
            DEF_IO16(0x10103602, core->csnd.writeChanRate(16, IO_PARAMS)) // CSND_CHAN16_RATE
            DEF_IO16(0x10103604, core->csnd.writeChanRvol(16, IO_PARAMS)) // CSND_CHAN16_RVOL
            DEF_IO16(0x10103606, core->csnd.writeChanLvol(16, IO_PARAMS)) // CSND_CHAN16_LVOL
            DEF_IO32(0x1010360C, core->csnd.writeChanStart(16, IO_PARAMS)) // CSND_CHAN16_START
            DEF_IO32(0x10103610, core->csnd.writeChanSize(16, IO_PARAMS)) // CSND_CHAN16_SIZE
            DEF_IO32(0x10103614, core->csnd.writeChanLoop(16, IO_PARAMS)) // CSND_CHAN16_LOOP
            DEF_IO32(0x10103618, core->csnd.writeAdpcmStart(16, IO_PARAMS)) // CSND_ADPCM16_START
            DEF_IO32(0x1010361C, core->csnd.writeAdpcmLoop(16, IO_PARAMS)) // CSND_ADPCM16_LOOP
            DEF_IO16(0x10103620, core->csnd.writeChanCnt(17, IO_PARAMS)) // CSND_CHAN17_CNT
            DEF_IO16(0x10103622, core->csnd.writeChanRate(17, IO_PARAMS)) // CSND_CHAN17_RATE
            DEF_IO16(0x10103624, core->csnd.writeChanRvol(17, IO_PARAMS)) // CSND_CHAN17_RVOL
            DEF_IO16(0x10103626, core->csnd.writeChanLvol(17, IO_PARAMS)) // CSND_CHAN17_LVOL
            DEF_IO32(0x1010362C, core->csnd.writeChanStart(17, IO_PARAMS)) // CSND_CHAN17_START
            DEF_IO32(0x10103630, core->csnd.writeChanSize(17, IO_PARAMS)) // CSND_CHAN17_SIZE
            DEF_IO32(0x10103634, core->csnd.writeChanLoop(17, IO_PARAMS)) // CSND_CHAN17_LOOP
            DEF_IO32(0x10103638, core->csnd.writeAdpcmStart(17, IO_PARAMS)) // CSND_ADPCM17_START
            DEF_IO32(0x1010363C, core->csnd.writeAdpcmLoop(17, IO_PARAMS)) // CSND_ADPCM17_LOOP
            DEF_IO16(0x10103640, core->csnd.writeChanCnt(18, IO_PARAMS)) // CSND_CHAN18_CNT
            DEF_IO16(0x10103642, core->csnd.writeChanRate(18, IO_PARAMS)) // CSND_CHAN18_RATE
            DEF_IO16(0x10103644, core->csnd.writeChanRvol(18, IO_PARAMS)) // CSND_CHAN18_RVOL
            DEF_IO16(0x10103646, core->csnd.writeChanLvol(18, IO_PARAMS)) // CSND_CHAN18_LVOL
            DEF_IO32(0x1010364C, core->csnd.writeChanStart(18, IO_PARAMS)) // CSND_CHAN18_START
            DEF_IO32(0x10103650, core->csnd.writeChanSize(18, IO_PARAMS)) // CSND_CHAN18_SIZE
            DEF_IO32(0x10103654, core->csnd.writeChanLoop(18, IO_PARAMS)) // CSND_CHAN18_LOOP
            DEF_IO32(0x10103658, core->csnd.writeAdpcmStart(18, IO_PARAMS)) // CSND_ADPCM18_START
            DEF_IO32(0x1010365C, core->csnd.writeAdpcmLoop(18, IO_PARAMS)) // CSND_ADPCM18_LOOP
            DEF_IO16(0x10103660, core->csnd.writeChanCnt(19, IO_PARAMS)) // CSND_CHAN19_CNT
            DEF_IO16(0x10103662, core->csnd.writeChanRate(19, IO_PARAMS)) // CSND_CHAN19_RATE
            DEF_IO16(0x10103664, core->csnd.writeChanRvol(19, IO_PARAMS)) // CSND_CHAN19_RVOL
            DEF_IO16(0x10103666, core->csnd.writeChanLvol(19, IO_PARAMS)) // CSND_CHAN19_LVOL
            DEF_IO32(0x1010366C, core->csnd.writeChanStart(19, IO_PARAMS)) // CSND_CHAN19_START
            DEF_IO32(0x10103670, core->csnd.writeChanSize(19, IO_PARAMS)) // CSND_CHAN19_SIZE
            DEF_IO32(0x10103674, core->csnd.writeChanLoop(19, IO_PARAMS)) // CSND_CHAN19_LOOP
            DEF_IO32(0x10103678, core->csnd.writeAdpcmStart(19, IO_PARAMS)) // CSND_ADPCM19_START
            DEF_IO32(0x1010367C, core->csnd.writeAdpcmLoop(19, IO_PARAMS)) // CSND_ADPCM19_LOOP
            DEF_IO16(0x10103680, core->csnd.writeChanCnt(20, IO_PARAMS)) // CSND_CHAN20_CNT
            DEF_IO16(0x10103682, core->csnd.writeChanRate(20, IO_PARAMS)) // CSND_CHAN20_RATE
            DEF_IO16(0x10103684, core->csnd.writeChanRvol(20, IO_PARAMS)) // CSND_CHAN20_RVOL
            DEF_IO16(0x10103686, core->csnd.writeChanLvol(20, IO_PARAMS)) // CSND_CHAN20_LVOL
            DEF_IO32(0x1010368C, core->csnd.writeChanStart(20, IO_PARAMS)) // CSND_CHAN20_START
            DEF_IO32(0x10103690, core->csnd.writeChanSize(20, IO_PARAMS)) // CSND_CHAN20_SIZE
            DEF_IO32(0x10103694, core->csnd.writeChanLoop(20, IO_PARAMS)) // CSND_CHAN20_LOOP
            DEF_IO32(0x10103698, core->csnd.writeAdpcmStart(20, IO_PARAMS)) // CSND_ADPCM20_START
            DEF_IO32(0x1010369C, core->csnd.writeAdpcmLoop(20, IO_PARAMS)) // CSND_ADPCM20_LOOP
            DEF_IO16(0x101036A0, core->csnd.writeChanCnt(21, IO_PARAMS)) // CSND_CHAN21_CNT
            DEF_IO16(0x101036A2, core->csnd.writeChanRate(21, IO_PARAMS)) // CSND_CHAN21_RATE
            DEF_IO16(0x101036A4, core->csnd.writeChanRvol(21, IO_PARAMS)) // CSND_CHAN21_RVOL
            DEF_IO16(0x101036A6, core->csnd.writeChanLvol(21, IO_PARAMS)) // CSND_CHAN21_LVOL
            DEF_IO32(0x101036AC, core->csnd.writeChanStart(21, IO_PARAMS)) // CSND_CHAN21_START
            DEF_IO32(0x101036B0, core->csnd.writeChanSize(21, IO_PARAMS)) // CSND_CHAN21_SIZE
            DEF_IO32(0x101036B4, core->csnd.writeChanLoop(21, IO_PARAMS)) // CSND_CHAN21_LOOP
            DEF_IO32(0x101036B8, core->csnd.writeAdpcmStart(21, IO_PARAMS)) // CSND_ADPCM21_START
            DEF_IO32(0x101036BC, core->csnd.writeAdpcmLoop(21, IO_PARAMS)) // CSND_ADPCM21_LOOP
            DEF_IO16(0x101036C0, core->csnd.writeChanCnt(22, IO_PARAMS)) // CSND_CHAN22_CNT
            DEF_IO16(0x101036C2, core->csnd.writeChanRate(22, IO_PARAMS)) // CSND_CHAN22_RATE
            DEF_IO16(0x101036C4, core->csnd.writeChanRvol(22, IO_PARAMS)) // CSND_CHAN22_RVOL
            DEF_IO16(0x101036C6, core->csnd.writeChanLvol(22, IO_PARAMS)) // CSND_CHAN22_LVOL
            DEF_IO32(0x101036CC, core->csnd.writeChanStart(22, IO_PARAMS)) // CSND_CHAN22_START
            DEF_IO32(0x101036D0, core->csnd.writeChanSize(22, IO_PARAMS)) // CSND_CHAN22_SIZE
            DEF_IO32(0x101036D4, core->csnd.writeChanLoop(22, IO_PARAMS)) // CSND_CHAN22_LOOP
            DEF_IO32(0x101036D8, core->csnd.writeAdpcmStart(22, IO_PARAMS)) // CSND_ADPCM22_START
            DEF_IO32(0x101036DC, core->csnd.writeAdpcmLoop(22, IO_PARAMS)) // CSND_ADPCM22_LOOP
            DEF_IO16(0x101036E0, core->csnd.writeChanCnt(23, IO_PARAMS)) // CSND_CHAN23_CNT
            DEF_IO16(0x101036E2, core->csnd.writeChanRate(23, IO_PARAMS)) // CSND_CHAN23_RATE
            DEF_IO16(0x101036E4, core->csnd.writeChanRvol(23, IO_PARAMS)) // CSND_CHAN23_RVOL
            DEF_IO16(0x101036E6, core->csnd.writeChanLvol(23, IO_PARAMS)) // CSND_CHAN23_LVOL
            DEF_IO32(0x101036EC, core->csnd.writeChanStart(23, IO_PARAMS)) // CSND_CHAN23_START
            DEF_IO32(0x101036F0, core->csnd.writeChanSize(23, IO_PARAMS)) // CSND_CHAN23_SIZE
            DEF_IO32(0x101036F4, core->csnd.writeChanLoop(23, IO_PARAMS)) // CSND_CHAN23_LOOP
            DEF_IO32(0x101036F8, core->csnd.writeAdpcmStart(23, IO_PARAMS)) // CSND_ADPCM23_START
            DEF_IO32(0x101036FC, core->csnd.writeAdpcmLoop(23, IO_PARAMS)) // CSND_ADPCM23_LOOP
            DEF_IO16(0x10103700, core->csnd.writeChanCnt(24, IO_PARAMS)) // CSND_CHAN24_CNT
            DEF_IO16(0x10103702, core->csnd.writeChanRate(24, IO_PARAMS)) // CSND_CHAN24_RATE
            DEF_IO16(0x10103704, core->csnd.writeChanRvol(24, IO_PARAMS)) // CSND_CHAN24_RVOL
            DEF_IO16(0x10103706, core->csnd.writeChanLvol(24, IO_PARAMS)) // CSND_CHAN24_LVOL
            DEF_IO32(0x1010370C, core->csnd.writeChanStart(24, IO_PARAMS)) // CSND_CHAN24_START
            DEF_IO32(0x10103710, core->csnd.writeChanSize(24, IO_PARAMS)) // CSND_CHAN24_SIZE
            DEF_IO32(0x10103714, core->csnd.writeChanLoop(24, IO_PARAMS)) // CSND_CHAN24_LOOP
            DEF_IO32(0x10103718, core->csnd.writeAdpcmStart(24, IO_PARAMS)) // CSND_ADPCM24_START
            DEF_IO32(0x1010371C, core->csnd.writeAdpcmLoop(24, IO_PARAMS)) // CSND_ADPCM24_LOOP
            DEF_IO16(0x10103720, core->csnd.writeChanCnt(25, IO_PARAMS)) // CSND_CHAN25_CNT
            DEF_IO16(0x10103722, core->csnd.writeChanRate(25, IO_PARAMS)) // CSND_CHAN25_RATE
            DEF_IO16(0x10103724, core->csnd.writeChanRvol(25, IO_PARAMS)) // CSND_CHAN25_RVOL
            DEF_IO16(0x10103726, core->csnd.writeChanLvol(25, IO_PARAMS)) // CSND_CHAN25_LVOL
            DEF_IO32(0x1010372C, core->csnd.writeChanStart(25, IO_PARAMS)) // CSND_CHAN25_START
            DEF_IO32(0x10103730, core->csnd.writeChanSize(25, IO_PARAMS)) // CSND_CHAN25_SIZE
            DEF_IO32(0x10103734, core->csnd.writeChanLoop(25, IO_PARAMS)) // CSND_CHAN25_LOOP
            DEF_IO32(0x10103738, core->csnd.writeAdpcmStart(25, IO_PARAMS)) // CSND_ADPCM25_START
            DEF_IO32(0x1010373C, core->csnd.writeAdpcmLoop(25, IO_PARAMS)) // CSND_ADPCM25_LOOP
            DEF_IO16(0x10103740, core->csnd.writeChanCnt(26, IO_PARAMS)) // CSND_CHAN26_CNT
            DEF_IO16(0x10103742, core->csnd.writeChanRate(26, IO_PARAMS)) // CSND_CHAN26_RATE
            DEF_IO16(0x10103744, core->csnd.writeChanRvol(26, IO_PARAMS)) // CSND_CHAN26_RVOL
            DEF_IO16(0x10103746, core->csnd.writeChanLvol(26, IO_PARAMS)) // CSND_CHAN26_LVOL
            DEF_IO32(0x1010374C, core->csnd.writeChanStart(26, IO_PARAMS)) // CSND_CHAN26_START
            DEF_IO32(0x10103750, core->csnd.writeChanSize(26, IO_PARAMS)) // CSND_CHAN26_SIZE
            DEF_IO32(0x10103754, core->csnd.writeChanLoop(26, IO_PARAMS)) // CSND_CHAN26_LOOP
            DEF_IO32(0x10103758, core->csnd.writeAdpcmStart(26, IO_PARAMS)) // CSND_ADPCM26_START
            DEF_IO32(0x1010375C, core->csnd.writeAdpcmLoop(26, IO_PARAMS)) // CSND_ADPCM26_LOOP
            DEF_IO16(0x10103760, core->csnd.writeChanCnt(27, IO_PARAMS)) // CSND_CHAN27_CNT
            DEF_IO16(0x10103762, core->csnd.writeChanRate(27, IO_PARAMS)) // CSND_CHAN27_RATE
            DEF_IO16(0x10103764, core->csnd.writeChanRvol(27, IO_PARAMS)) // CSND_CHAN27_RVOL
            DEF_IO16(0x10103766, core->csnd.writeChanLvol(27, IO_PARAMS)) // CSND_CHAN27_LVOL
            DEF_IO32(0x1010376C, core->csnd.writeChanStart(27, IO_PARAMS)) // CSND_CHAN27_START
            DEF_IO32(0x10103770, core->csnd.writeChanSize(27, IO_PARAMS)) // CSND_CHAN27_SIZE
            DEF_IO32(0x10103774, core->csnd.writeChanLoop(27, IO_PARAMS)) // CSND_CHAN27_LOOP
            DEF_IO32(0x10103778, core->csnd.writeAdpcmStart(27, IO_PARAMS)) // CSND_ADPCM27_START
            DEF_IO32(0x1010377C, core->csnd.writeAdpcmLoop(27, IO_PARAMS)) // CSND_ADPCM27_LOOP
            DEF_IO16(0x10103780, core->csnd.writeChanCnt(28, IO_PARAMS)) // CSND_CHAN28_CNT
            DEF_IO16(0x10103782, core->csnd.writeChanRate(28, IO_PARAMS)) // CSND_CHAN28_RATE
            DEF_IO16(0x10103784, core->csnd.writeChanRvol(28, IO_PARAMS)) // CSND_CHAN28_RVOL
            DEF_IO16(0x10103786, core->csnd.writeChanLvol(28, IO_PARAMS)) // CSND_CHAN28_LVOL
            DEF_IO32(0x1010378C, core->csnd.writeChanStart(28, IO_PARAMS)) // CSND_CHAN28_START
            DEF_IO32(0x10103790, core->csnd.writeChanSize(28, IO_PARAMS)) // CSND_CHAN28_SIZE
            DEF_IO32(0x10103794, core->csnd.writeChanLoop(28, IO_PARAMS)) // CSND_CHAN28_LOOP
            DEF_IO32(0x10103798, core->csnd.writeAdpcmStart(28, IO_PARAMS)) // CSND_ADPCM28_START
            DEF_IO32(0x1010379C, core->csnd.writeAdpcmLoop(28, IO_PARAMS)) // CSND_ADPCM28_LOOP
            DEF_IO16(0x101037A0, core->csnd.writeChanCnt(29, IO_PARAMS)) // CSND_CHAN29_CNT
            DEF_IO16(0x101037A2, core->csnd.writeChanRate(29, IO_PARAMS)) // CSND_CHAN29_RATE
            DEF_IO16(0x101037A4, core->csnd.writeChanRvol(29, IO_PARAMS)) // CSND_CHAN29_RVOL
            DEF_IO16(0x101037A6, core->csnd.writeChanLvol(29, IO_PARAMS)) // CSND_CHAN29_LVOL
            DEF_IO32(0x101037AC, core->csnd.writeChanStart(29, IO_PARAMS)) // CSND_CHAN29_START
            DEF_IO32(0x101037B0, core->csnd.writeChanSize(29, IO_PARAMS)) // CSND_CHAN29_SIZE
            DEF_IO32(0x101037B4, core->csnd.writeChanLoop(29, IO_PARAMS)) // CSND_CHAN29_LOOP
            DEF_IO32(0x101037B8, core->csnd.writeAdpcmStart(29, IO_PARAMS)) // CSND_ADPCM29_START
            DEF_IO32(0x101037BC, core->csnd.writeAdpcmLoop(29, IO_PARAMS)) // CSND_ADPCM29_LOOP
            DEF_IO16(0x101037C0, core->csnd.writeChanCnt(30, IO_PARAMS)) // CSND_CHAN30_CNT
            DEF_IO16(0x101037C2, core->csnd.writeChanRate(30, IO_PARAMS)) // CSND_CHAN30_RATE
            DEF_IO16(0x101037C4, core->csnd.writeChanRvol(30, IO_PARAMS)) // CSND_CHAN30_RVOL
            DEF_IO16(0x101037C6, core->csnd.writeChanLvol(30, IO_PARAMS)) // CSND_CHAN30_LVOL
            DEF_IO32(0x101037CC, core->csnd.writeChanStart(30, IO_PARAMS)) // CSND_CHAN30_START
            DEF_IO32(0x101037D0, core->csnd.writeChanSize(30, IO_PARAMS)) // CSND_CHAN30_SIZE
            DEF_IO32(0x101037D4, core->csnd.writeChanLoop(30, IO_PARAMS)) // CSND_CHAN30_LOOP
            DEF_IO32(0x101037D8, core->csnd.writeAdpcmStart(30, IO_PARAMS)) // CSND_ADPCM30_START
            DEF_IO32(0x101037DC, core->csnd.writeAdpcmLoop(30, IO_PARAMS)) // CSND_ADPCM30_LOOP
            DEF_IO16(0x101037E0, core->csnd.writeChanCnt(31, IO_PARAMS)) // CSND_CHAN31_CNT
            DEF_IO16(0x101037E2, core->csnd.writeChanRate(31, IO_PARAMS)) // CSND_CHAN31_RATE
            DEF_IO16(0x101037E4, core->csnd.writeChanRvol(31, IO_PARAMS)) // CSND_CHAN31_RVOL
            DEF_IO16(0x101037E6, core->csnd.writeChanLvol(31, IO_PARAMS)) // CSND_CHAN31_LVOL
            DEF_IO32(0x101037EC, core->csnd.writeChanStart(31, IO_PARAMS)) // CSND_CHAN31_START
            DEF_IO32(0x101037F0, core->csnd.writeChanSize(31, IO_PARAMS)) // CSND_CHAN31_SIZE
            DEF_IO32(0x101037F4, core->csnd.writeChanLoop(31, IO_PARAMS)) // CSND_CHAN31_LOOP
            DEF_IO32(0x101037F8, core->csnd.writeAdpcmStart(31, IO_PARAMS)) // CSND_ADPCM31_START
            DEF_IO32(0x101037FC, core->csnd.writeAdpcmLoop(31, IO_PARAMS)) // CSND_ADPCM31_LOOP
            DEF_IO16(0x10122000, core->wifi.writeCmd(IO_PARAMS)) // WIFI_CMD
            DEF_IO32(0x10122004, core->wifi.writeCmdParam(IO_PARAMS)) // WIFI_CMD_PARAM
            DEF_IO32(0x1012201C, core->wifi.writeIrqStatus(IO_PARAMS)) // WIFI_IRQ_STATUS
            DEF_IO32(0x10122020, core->wifi.writeIrqMask(IO_PARAMS)) // WIFI_IRQ_MASK
            DEF_IO16(0x10122026, core->wifi.writeData16Blklen(IO_PARAMS)) // WIFI_DATA16_BLKLEN
            DEF_IO16(0x10122030, core->wifi.writeData16Fifo(IO_PARAMS)) // WIFI_DATA16_FIFO
            DEF_IO16(0x101220D8, core->wifi.writeDataCtl(IO_PARAMS)) // WIFI_DATA_CTL
            DEF_IO16(0x10122100, core->wifi.writeData32Irq(IO_PARAMS)) // WIFI_DATA32_IRQ
            DEF_IO16(0x10122104, core->wifi.writeData32Blklen(IO_PARAMS)) // WIFI_DATA32_BLKLEN
            DEF_IO32(0x10140420, writeCfg11BrOverlayCnt(IO_PARAMS)) // CFG11_BR_OVERLAY_CNT
            DEF_IO32(0x10140424, writeCfg11BrOverlayVal(IO_PARAMS)) // CFG11_BR_OVERLAY_VAL
            DEF_IO32(0x10141200, core->gpu.writeCfg11GpuCnt(IO_PARAMS)) // CFG11_GPU_CNT
            DEF_IO32(0x10141300, core->interrupts.writeCfg11MpClkcnt(IO_PARAMS)) // CFG11_MP_CLKCNT
            DEF_IO32(0x10141304, writeCfg11MpCnt(IO_PARAMS)) // CFG11_MP_CNT
            DEF_IO08(0x10141312, core->interrupts.writeCfg11MpBootcnt(2, IO_PARAMS8)) // CFG11_MP_BOOTCNT2
            DEF_IO08(0x10141313, core->interrupts.writeCfg11MpBootcnt(3, IO_PARAMS8)) // CFG11_MP_BOOTCNT3
            DEF_IO08(0x10144000, core->i2c.writeBusData(1, IO_PARAMS8)) // I2C_BUS1_DATA
            DEF_IO08(0x10144001, core->i2c.writeBusCnt(1, IO_PARAMS8)) // I2C_BUS1_CNT
            DEF_IO08(0x10148000, core->i2c.writeBusData(2, IO_PARAMS8)) // I2C_BUS2_DATA
            DEF_IO08(0x10148001, core->i2c.writeBusCnt(2, IO_PARAMS8)) // I2C_BUS2_CNT
            DEF_IO08(0x10161000, core->i2c.writeBusData(0, IO_PARAMS8)) // I2C_BUS0_DATA
            DEF_IO08(0x10161001, core->i2c.writeBusCnt(0, IO_PARAMS8)) // I2C_BUS0_CNT
            DEF_IO32(0x10163000, core->pxi.writeSync(0, IO_PARAMS)) // PXI_SYNC11
            DEF_IO32(0x10163004, core->pxi.writeCnt(0, IO_PARAMS)) // PXI_CNT11
            DEF_IO32(0x10163008, core->pxi.writeSend(0, IO_PARAMS)) // PXI_SEND11
            DEF_IO16(0x10164000, core->cartridge.writeNtrMcnt(IO_PARAMS)) // NTRCARD_MCNT
            DEF_IO32(0x10164004, core->cartridge.writeNtrRomcnt(IO_PARAMS)) // NTRCARD_ROMCNT
            DEF_IO32(0x10164008, core->cartridge.writeNtrCmd(0, IO_PARAMS)) // NTRCARD_CMD0
            DEF_IO32(0x1016400C, core->cartridge.writeNtrCmd(1, IO_PARAMS)) // NTRCARD_CMD1
        }

        // Check registers that are exclusive to one CPU
        if (id != ARM9) { // ARM11
            switch (base) {
                DEF_IO32(0x10200D04, core->cdmas[CDMA0].writeDbgcmd(IO_PARAMS)) // CDMA0_DBGCMD
                DEF_IO32(0x10200D08, core->cdmas[CDMA0].writeDbginst0(IO_PARAMS)) // CDMA0_DBGINST0
                DEF_IO32(0x10200D0C, core->cdmas[CDMA0].writeDbginst1(IO_PARAMS)) // CDMA0_DBGINST1
                DEF_IO32(0x10200020, core->cdmas[CDMA0].writeInten(IO_PARAMS)) // CDMA0_INTEN
                DEF_IO32(0x1020002C, core->cdmas[CDMA0].writeIntclr(IO_PARAMS)) // CDMA0_INTCLR
                DEF_IO16(0x10203000, core->dsp.writePdata(IO_PARAMS)) // DSP_PDATA
                DEF_IO16(0x10203004, core->dsp.writePadr(IO_PARAMS)) // DSP_PADR
                DEF_IO16(0x10203008, core->dsp.writePcfg(IO_PARAMS)) // DSP_PCFG
                DEF_IO16(0x10203010, core->dsp.writePsem(IO_PARAMS)) // DSP_PSEM
                DEF_IO16(0x10203014, core->dsp.writePmask(IO_PARAMS)) // DSP_PMASK
                DEF_IO16(0x10203018, core->dsp.writePclear(IO_PARAMS)) // DSP_PCLEAR
                DEF_IO16(0x10203020, core->dsp.writeCmd(0, IO_PARAMS)) // DSP_CMD0
                DEF_IO16(0x10203028, core->dsp.writeCmd(1, IO_PARAMS)) // DSP_CMD1
                DEF_IO16(0x10203030, core->dsp.writeCmd(2, IO_PARAMS)) // DSP_CMD2
                DEF_IO32(0x10206D04, core->cdmas[CDMA1].writeDbgcmd(IO_PARAMS)) // CDMA1_DBGCMD
                DEF_IO32(0x10206D08, core->cdmas[CDMA1].writeDbginst0(IO_PARAMS)) // CDMA1_DBGINST0
                DEF_IO32(0x10206D0C, core->cdmas[CDMA1].writeDbginst1(IO_PARAMS)) // CDMA1_DBGINST1
                DEF_IO32(0x10206020, core->cdmas[CDMA1].writeInten(IO_PARAMS)) // CDMA1_INTEN
                DEF_IO32(0x1020602C, core->cdmas[CDMA1].writeIntclr(IO_PARAMS)) // CDMA1_INTCLR
                DEF_IO32(0x10301000, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301004, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301008, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030100C, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301010, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301014, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301018, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030101C, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301020, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301024, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301028, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030102C, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301030, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301034, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301038, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030103C, core->shas[0].writeFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10322000, core->wifi.writeData32Fifo(IO_PARAMS)) // WIFI_DATA32_FIFO
                DEF_IO32(0x10400010, core->gpu.writeMemsetDstAddr(0, IO_PARAMS)) // GPU_MEMSET_DST_ADDR0
                DEF_IO32(0x10400014, core->gpu.writeMemsetDstEnd(0, IO_PARAMS)) // GPU_MEMSET_DST_END0
                DEF_IO32(0x10400018, core->gpu.writeMemsetData(0, IO_PARAMS)) // GPU_MEMSET_DATA0
                DEF_IO32(0x1040001C, core->gpu.writeMemsetCnt(0, IO_PARAMS)) // GPU_MEMSET_CNT0
                DEF_IO32(0x10400020, core->gpu.writeMemsetDstAddr(1, IO_PARAMS)) // GPU_MEMSET_DST_ADDR1
                DEF_IO32(0x10400024, core->gpu.writeMemsetDstEnd(1, IO_PARAMS)) // GPU_MEMSET_DST_END1
                DEF_IO32(0x10400028, core->gpu.writeMemsetData(1, IO_PARAMS)) // GPU_MEMSET_DATA1
                DEF_IO32(0x1040002C, core->gpu.writeMemsetCnt(1, IO_PARAMS)) // GPU_MEMSET_CNT1
                DEF_IO32(0x10400468, core->pdc.writeFramebufLt0(0, IO_PARAMS)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x1040046C, core->pdc.writeFramebufLt1(0, IO_PARAMS)) // PDC0_FRAMEBUF_LT1
                DEF_IO32(0x10400470, core->pdc.writeFramebufFormat(0, IO_PARAMS)) // PDC0_FRAMEBUF_FORMAT
                DEF_IO32(0x10400474, core->pdc.writeInterruptType(0, IO_PARAMS)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400478, core->pdc.writeFramebufSelAck(0, IO_PARAMS)) // PDC0_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400568, core->pdc.writeFramebufLt0(1, IO_PARAMS)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x1040056C, core->pdc.writeFramebufLt1(1, IO_PARAMS)) // PDC1_FRAMEBUF_LT1
                DEF_IO32(0x10400570, core->pdc.writeFramebufFormat(1, IO_PARAMS)) // PDC1_FRAMEBUF_FORMAT
                DEF_IO32(0x10400574, core->pdc.writeInterruptType(1, IO_PARAMS)) // PDC1_INTERRUPT_TYPE
                DEF_IO32(0x10400578, core->pdc.writeFramebufSelAck(1, IO_PARAMS)) // PDC1_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400C00, core->gpu.writeMemcpySrcAddr(IO_PARAMS)) // GPU_MEMCPY_SRC_ADDR
                DEF_IO32(0x10400C04, core->gpu.writeMemcpyDstAddr(IO_PARAMS)) // GPU_MEMCPY_DST_ADDR
                DEF_IO32(0x10400C08, core->gpu.writeMemcpyDispDstSize(IO_PARAMS)) // GPU_MEMCPY_DISP_DST_SIZE
                DEF_IO32(0x10400C0C, core->gpu.writeMemcpyDispSrcSize(IO_PARAMS)) // GPU_MEMCPY_DISP_SRC_SIZE
                DEF_IO32(0x10400C10, core->gpu.writeMemcpyFlags(IO_PARAMS)) // GPU_MEMCPY_FLAGS
                DEF_IO32(0x10400C18, core->gpu.writeMemcpyCnt(IO_PARAMS)) // GPU_MEMCPY_CNT
                DEF_IO32(0x10400C20, core->gpu.writeMemcpyTexSize(IO_PARAMS)) // GPU_MEMCPY_TEX_SIZE
                DEF_IO32(0x10400C24, core->gpu.writeMemcpyTexSrcWidth(IO_PARAMS)) // GPU_MEMCPY_TEX_SRC_WIDTH
                DEF_IO32(0x10400C28, core->gpu.writeMemcpyTexDstWidth(IO_PARAMS)) // GPU_MEMCPY_TEX_DST_WIDTH
                DEF_IO32(0x10401000, core->gpu.writeIrqAck(0, IO_PARAMS)) // GPU_IRQ_ACK0
                DEF_IO32(0x10401004, core->gpu.writeIrqAck(1, IO_PARAMS)) // GPU_IRQ_ACK1
                DEF_IO32(0x10401008, core->gpu.writeIrqAck(2, IO_PARAMS)) // GPU_IRQ_ACK2
                DEF_IO32(0x1040100C, core->gpu.writeIrqAck(3, IO_PARAMS)) // GPU_IRQ_ACK3
                DEF_IO32(0x10401010, core->gpu.writeIrqAck(4, IO_PARAMS)) // GPU_IRQ_ACK4
                DEF_IO32(0x10401014, core->gpu.writeIrqAck(5, IO_PARAMS)) // GPU_IRQ_ACK5
                DEF_IO32(0x10401018, core->gpu.writeIrqAck(6, IO_PARAMS)) // GPU_IRQ_ACK6
                DEF_IO32(0x1040101C, core->gpu.writeIrqAck(7, IO_PARAMS)) // GPU_IRQ_ACK7
                DEF_IO32(0x10401020, core->gpu.writeIrqAck(8, IO_PARAMS)) // GPU_IRQ_ACK8
                DEF_IO32(0x10401024, core->gpu.writeIrqAck(9, IO_PARAMS)) // GPU_IRQ_ACK9
                DEF_IO32(0x10401028, core->gpu.writeIrqAck(10, IO_PARAMS)) // GPU_IRQ_ACK10
                DEF_IO32(0x1040102C, core->gpu.writeIrqAck(11, IO_PARAMS)) // GPU_IRQ_ACK11
                DEF_IO32(0x10401030, core->gpu.writeIrqAck(12, IO_PARAMS)) // GPU_IRQ_ACK12
                DEF_IO32(0x10401034, core->gpu.writeIrqAck(13, IO_PARAMS)) // GPU_IRQ_ACK13
                DEF_IO32(0x10401038, core->gpu.writeIrqAck(14, IO_PARAMS)) // GPU_IRQ_ACK14
                DEF_IO32(0x1040103C, core->gpu.writeIrqAck(15, IO_PARAMS)) // GPU_IRQ_ACK15
                DEF_IO32(0x10401040, core->gpu.writeIrqReq<0>(IO_PARAMS)) // GPU_IRQ_REQ0
                DEF_IO32(0x10401044, core->gpu.writeIrqReq<1>(IO_PARAMS)) // GPU_IRQ_REQ1
                DEF_IO32(0x10401048, core->gpu.writeIrqReq<2>(IO_PARAMS)) // GPU_IRQ_REQ2
                DEF_IO32(0x1040104C, core->gpu.writeIrqReq<3>(IO_PARAMS)) // GPU_IRQ_REQ3
                DEF_IO32(0x10401050, core->gpu.writeIrqReq<4>(IO_PARAMS)) // GPU_IRQ_REQ4
                DEF_IO32(0x10401054, core->gpu.writeIrqReq<5>(IO_PARAMS)) // GPU_IRQ_REQ5
                DEF_IO32(0x10401058, core->gpu.writeIrqReq<6>(IO_PARAMS)) // GPU_IRQ_REQ6
                DEF_IO32(0x1040105C, core->gpu.writeIrqReq<7>(IO_PARAMS)) // GPU_IRQ_REQ7
                DEF_IO32(0x10401060, core->gpu.writeIrqReq<8>(IO_PARAMS)) // GPU_IRQ_REQ8
                DEF_IO32(0x10401064, core->gpu.writeIrqReq<9>(IO_PARAMS)) // GPU_IRQ_REQ9
                DEF_IO32(0x10401068, core->gpu.writeIrqReq<10>(IO_PARAMS)) // GPU_IRQ_REQ10
                DEF_IO32(0x1040106C, core->gpu.writeIrqReq<11>(IO_PARAMS)) // GPU_IRQ_REQ11
                DEF_IO32(0x10401070, core->gpu.writeIrqReq<12>(IO_PARAMS)) // GPU_IRQ_REQ12
                DEF_IO32(0x10401074, core->gpu.writeIrqReq<13>(IO_PARAMS)) // GPU_IRQ_REQ13
                DEF_IO32(0x10401078, core->gpu.writeIrqReq<14>(IO_PARAMS)) // GPU_IRQ_REQ14
                DEF_IO32(0x1040107C, core->gpu.writeIrqReq<15>(IO_PARAMS)) // GPU_IRQ_REQ15
                DEF_IO32(0x10401080, core->gpu.writeIrqCmp(0, IO_PARAMS)) // GPU_IRQ_CMP0
                DEF_IO32(0x10401084, core->gpu.writeIrqCmp(1, IO_PARAMS)) // GPU_IRQ_CMP1
                DEF_IO32(0x10401088, core->gpu.writeIrqCmp(2, IO_PARAMS)) // GPU_IRQ_CMP2
                DEF_IO32(0x1040108C, core->gpu.writeIrqCmp(3, IO_PARAMS)) // GPU_IRQ_CMP3
                DEF_IO32(0x10401090, core->gpu.writeIrqCmp(4, IO_PARAMS)) // GPU_IRQ_CMP4
                DEF_IO32(0x10401094, core->gpu.writeIrqCmp(5, IO_PARAMS)) // GPU_IRQ_CMP5
                DEF_IO32(0x10401098, core->gpu.writeIrqCmp(6, IO_PARAMS)) // GPU_IRQ_CMP6
                DEF_IO32(0x1040109C, core->gpu.writeIrqCmp(7, IO_PARAMS)) // GPU_IRQ_CMP7
                DEF_IO32(0x104010A0, core->gpu.writeIrqCmp(8, IO_PARAMS)) // GPU_IRQ_CMP8
                DEF_IO32(0x104010A4, core->gpu.writeIrqCmp(9, IO_PARAMS)) // GPU_IRQ_CMP9
                DEF_IO32(0x104010A8, core->gpu.writeIrqCmp(10, IO_PARAMS)) // GPU_IRQ_CMP10
                DEF_IO32(0x104010AC, core->gpu.writeIrqCmp(11, IO_PARAMS)) // GPU_IRQ_CMP11
                DEF_IO32(0x104010B0, core->gpu.writeIrqCmp(12, IO_PARAMS)) // GPU_IRQ_CMP12
                DEF_IO32(0x104010B4, core->gpu.writeIrqCmp(13, IO_PARAMS)) // GPU_IRQ_CMP13
                DEF_IO32(0x104010B8, core->gpu.writeIrqCmp(14, IO_PARAMS)) // GPU_IRQ_CMP14
                DEF_IO32(0x104010BC, core->gpu.writeIrqCmp(15, IO_PARAMS)) // GPU_IRQ_CMP15
                DEF_IO32(0x104010C0, core->gpu.writeIrqMaskL(IO_PARAMS)) // GPU_IRQ_MASK_L
                DEF_IO32(0x104010C4, core->gpu.writeIrqMaskH(IO_PARAMS)) // GPU_IRQ_MASK_H
                DEF_IO32(0x104010D0, core->gpu.writeIrqAutostop(IO_PARAMS)) // GPU_IRQ_AUTOSTOP
                DEF_IO32(0x10401100, core->gpu.writeFaceCulling(IO_PARAMS)) // GPU_FACE_CULLING
                DEF_IO32(0x10401104, core->gpu.writeViewScaleH(IO_PARAMS)) // GPU_VIEW_SCALE_H
                DEF_IO32(0x10401108, core->gpu.writeViewStepH(IO_PARAMS)) // GPU_VIEW_STEP_H
                DEF_IO32(0x1040110C, core->gpu.writeViewScaleV(IO_PARAMS)) // GPU_VIEW_SCALE_V
                DEF_IO32(0x10401110, core->gpu.writeViewStepV(IO_PARAMS)) // GPU_VIEW_STEP_V
                DEF_IO32(0x1040113C, core->gpu.writeShdOutTotal(IO_PARAMS)) // GPU_SHD_OUT_TOTAL
                DEF_IO32(0x10401140, core->gpu.writeShdOutMap<0>(IO_PARAMS)) // GPU_SHD_OUT_MAP0
                DEF_IO32(0x10401144, core->gpu.writeShdOutMap<1>(IO_PARAMS)) // GPU_SHD_OUT_MAP1
                DEF_IO32(0x10401148, core->gpu.writeShdOutMap<2>(IO_PARAMS)) // GPU_SHD_OUT_MAP2
                DEF_IO32(0x1040114C, core->gpu.writeShdOutMap<3>(IO_PARAMS)) // GPU_SHD_OUT_MAP3
                DEF_IO32(0x10401150, core->gpu.writeShdOutMap<4>(IO_PARAMS)) // GPU_SHD_OUT_MAP4
                DEF_IO32(0x10401154, core->gpu.writeShdOutMap<5>(IO_PARAMS)) // GPU_SHD_OUT_MAP5
                DEF_IO32(0x10401158, core->gpu.writeShdOutMap<6>(IO_PARAMS)) // GPU_SHD_OUT_MAP6
                DEF_IO32(0x10401208, core->gpu.writeTexDim<0>(IO_PARAMS)) // GPU_TEX0_DIM
                DEF_IO32(0x10401214, core->gpu.writeTexAddr1<0>(IO_PARAMS)) // GPU_TEX0_ADDR1
                DEF_IO32(0x10401238, core->gpu.writeTexType<0>(IO_PARAMS)) // GPU_TEX0_TYPE
                DEF_IO32(0x10401248, core->gpu.writeTexDim<1>(IO_PARAMS)) // GPU_TEX1_DIM
                DEF_IO32(0x10401254, core->gpu.writeTexAddr1<1>(IO_PARAMS)) // GPU_TEX1_ADDR
                DEF_IO32(0x10401258, core->gpu.writeTexType<1>(IO_PARAMS)) // GPU_TEX1_TYPE
                DEF_IO32(0x10401268, core->gpu.writeTexDim<2>(IO_PARAMS)) // GPU_TEX2_DIM
                DEF_IO32(0x10401274, core->gpu.writeTexAddr1<2>(IO_PARAMS)) // GPU_TEX2_ADDR
                DEF_IO32(0x10401278, core->gpu.writeTexType<2>(IO_PARAMS)) // GPU_TEX2_TYPE
                DEF_IO32(0x10401300, core->gpu.writeCombSrc<0>(IO_PARAMS)) // GPU_COMB0_SRC
                DEF_IO32(0x10401304, core->gpu.writeCombOper<0>(IO_PARAMS)) // GPU_COMB0_OPER
                DEF_IO32(0x10401308, core->gpu.writeCombMode<0>(IO_PARAMS)) // GPU_COMB0_MODE
                DEF_IO32(0x1040130C, core->gpu.writeCombColor<0>(IO_PARAMS)) // GPU_COMB0_COLOR
                DEF_IO32(0x10401320, core->gpu.writeCombSrc<1>(IO_PARAMS)) // GPU_COMB1_SRC
                DEF_IO32(0x10401324, core->gpu.writeCombOper<1>(IO_PARAMS)) // GPU_COMB1_OPER
                DEF_IO32(0x10401328, core->gpu.writeCombMode<1>(IO_PARAMS)) // GPU_COMB1_MODE
                DEF_IO32(0x1040132C, core->gpu.writeCombColor<1>(IO_PARAMS)) // GPU_COMB1_COLOR
                DEF_IO32(0x10401340, core->gpu.writeCombSrc<2>(IO_PARAMS)) // GPU_COMB2_SRC
                DEF_IO32(0x10401344, core->gpu.writeCombOper<2>(IO_PARAMS)) // GPU_COMB2_OPER
                DEF_IO32(0x10401348, core->gpu.writeCombMode<2>(IO_PARAMS)) // GPU_COMB2_MODE
                DEF_IO32(0x1040134C, core->gpu.writeCombColor<2>(IO_PARAMS)) // GPU_COMB2_COLOR
                DEF_IO32(0x10401360, core->gpu.writeCombSrc<3>(IO_PARAMS)) // GPU_COMB3_SRC
                DEF_IO32(0x10401364, core->gpu.writeCombOper<3>(IO_PARAMS)) // GPU_COMB3_OPER
                DEF_IO32(0x10401368, core->gpu.writeCombMode<3>(IO_PARAMS)) // GPU_COMB3_MODE
                DEF_IO32(0x1040136C, core->gpu.writeCombColor<3>(IO_PARAMS)) // GPU_COMB3_COLOR
                DEF_IO32(0x104013C0, core->gpu.writeCombSrc<4>(IO_PARAMS)) // GPU_COMB4_SRC
                DEF_IO32(0x104013C4, core->gpu.writeCombOper<4>(IO_PARAMS)) // GPU_COMB4_OPER
                DEF_IO32(0x104013C8, core->gpu.writeCombMode<4>(IO_PARAMS)) // GPU_COMB4_MODE
                DEF_IO32(0x104013CC, core->gpu.writeCombColor<4>(IO_PARAMS)) // GPU_COMB4_COLOR
                DEF_IO32(0x104013E0, core->gpu.writeCombSrc<5>(IO_PARAMS)) // GPU_COMB5_SRC
                DEF_IO32(0x104013E4, core->gpu.writeCombOper<5>(IO_PARAMS)) // GPU_COMB5_OPER
                DEF_IO32(0x104013E8, core->gpu.writeCombMode<5>(IO_PARAMS)) // GPU_COMB5_MODE
                DEF_IO32(0x104013EC, core->gpu.writeCombColor<5>(IO_PARAMS)) // GPU_COMB5_COLOR
                DEF_IO32(0x1040141C, core->gpu.writeDepcolMask(IO_PARAMS)) // GPU_DEPCOL_MASK
                DEF_IO32(0x1040144C, core->gpu.writeColbufWrite(IO_PARAMS)) // GPU_COLBUF_WRITE
                DEF_IO32(0x10401454, core->gpu.writeDepbufWrite(IO_PARAMS)) // GPU_DEPBUF_WRITE
                DEF_IO32(0x10401458, core->gpu.writeDepbufFmt(IO_PARAMS)) // GPU_DEPBUF_FMT
                DEF_IO32(0x1040145C, core->gpu.writeColbufFmt(IO_PARAMS)) // GPU_COLBUF_FMT
                DEF_IO32(0x10401470, core->gpu.writeDepbufLoc(IO_PARAMS)) // GPU_DEPBUF_LOC
                DEF_IO32(0x10401474, core->gpu.writeColbufLoc(IO_PARAMS)) // GPU_COLBUF_LOC
                DEF_IO32(0x10401478, core->gpu.writeBufferDim(IO_PARAMS)) // GPU_BUFFER_DIM
                DEF_IO32(0x10401800, core->gpu.writeAttrBase(IO_PARAMS)) // GPU_ATTR_BASE
                DEF_IO32(0x10401804, core->gpu.writeAttrFmtL(IO_PARAMS)) // GPU_ATTR_FMT_L
                DEF_IO32(0x10401808, core->gpu.writeAttrFmtH(IO_PARAMS)) // GPU_ATTR_FMT_H
                DEF_IO32(0x1040180C, core->gpu.writeAttrOfs<0>(IO_PARAMS)) // GPU_ATTR_OFS0
                DEF_IO32(0x10401810, core->gpu.writeAttrCfgL<0>(IO_PARAMS)) // GPU_ATTR_CFG0_L
                DEF_IO32(0x10401814, core->gpu.writeAttrCfgH<0>(IO_PARAMS)) // GPU_ATTR_CFG0_H
                DEF_IO32(0x10401818, core->gpu.writeAttrOfs<1>(IO_PARAMS)) // GPU_ATTR_OFS1
                DEF_IO32(0x1040181C, core->gpu.writeAttrCfgL<1>(IO_PARAMS)) // GPU_ATTR_CFG1_L
                DEF_IO32(0x10401820, core->gpu.writeAttrCfgH<1>(IO_PARAMS)) // GPU_ATTR_CFG1_H
                DEF_IO32(0x10401824, core->gpu.writeAttrOfs<2>(IO_PARAMS)) // GPU_ATTR_OFS2
                DEF_IO32(0x10401828, core->gpu.writeAttrCfgL<2>(IO_PARAMS)) // GPU_ATTR_CFG2_L
                DEF_IO32(0x1040182C, core->gpu.writeAttrCfgH<2>(IO_PARAMS)) // GPU_ATTR_CFG2_H
                DEF_IO32(0x10401830, core->gpu.writeAttrOfs<3>(IO_PARAMS)) // GPU_ATTR_OFS3
                DEF_IO32(0x10401834, core->gpu.writeAttrCfgL<3>(IO_PARAMS)) // GPU_ATTR_CFG3_L
                DEF_IO32(0x10401838, core->gpu.writeAttrCfgH<3>(IO_PARAMS)) // GPU_ATTR_CFG3_H
                DEF_IO32(0x1040183C, core->gpu.writeAttrOfs<4>(IO_PARAMS)) // GPU_ATTR_OFS4
                DEF_IO32(0x10401840, core->gpu.writeAttrCfgL<4>(IO_PARAMS)) // GPU_ATTR_CFG4_L
                DEF_IO32(0x10401844, core->gpu.writeAttrCfgH<4>(IO_PARAMS)) // GPU_ATTR_CFG4_H
                DEF_IO32(0x10401848, core->gpu.writeAttrOfs<5>(IO_PARAMS)) // GPU_ATTR_OFS5
                DEF_IO32(0x1040184C, core->gpu.writeAttrCfgL<5>(IO_PARAMS)) // GPU_ATTR_CFG5_L
                DEF_IO32(0x10401850, core->gpu.writeAttrCfgH<5>(IO_PARAMS)) // GPU_ATTR_CFG5_H
                DEF_IO32(0x10401854, core->gpu.writeAttrOfs<6>(IO_PARAMS)) // GPU_ATTR_OFS6
                DEF_IO32(0x10401858, core->gpu.writeAttrCfgL<6>(IO_PARAMS)) // GPU_ATTR_CFG6_L
                DEF_IO32(0x1040185C, core->gpu.writeAttrCfgH<6>(IO_PARAMS)) // GPU_ATTR_CFG6_H
                DEF_IO32(0x10401860, core->gpu.writeAttrOfs<7>(IO_PARAMS)) // GPU_ATTR_OFS7
                DEF_IO32(0x10401864, core->gpu.writeAttrCfgL<7>(IO_PARAMS)) // GPU_ATTR_CFG7_L
                DEF_IO32(0x10401868, core->gpu.writeAttrCfgH<7>(IO_PARAMS)) // GPU_ATTR_CFG7_H
                DEF_IO32(0x1040186C, core->gpu.writeAttrOfs<8>(IO_PARAMS)) // GPU_ATTR_OFS8
                DEF_IO32(0x10401870, core->gpu.writeAttrCfgL<8>(IO_PARAMS)) // GPU_ATTR_CFG8_L
                DEF_IO32(0x10401874, core->gpu.writeAttrCfgH<8>(IO_PARAMS)) // GPU_ATTR_CFG8_H
                DEF_IO32(0x10401878, core->gpu.writeAttrOfs<9>(IO_PARAMS)) // GPU_ATTR_OFS9
                DEF_IO32(0x1040187C, core->gpu.writeAttrCfgL<9>(IO_PARAMS)) // GPU_ATTR_CFG9_L
                DEF_IO32(0x10401880, core->gpu.writeAttrCfgH<9>(IO_PARAMS)) // GPU_ATTR_CFG9_H
                DEF_IO32(0x10401884, core->gpu.writeAttrOfs<10>(IO_PARAMS)) // GPU_ATTR_OFS10
                DEF_IO32(0x10401888, core->gpu.writeAttrCfgL<10>(IO_PARAMS)) // GPU_ATTR_CFG10_L
                DEF_IO32(0x1040188C, core->gpu.writeAttrCfgH<10>(IO_PARAMS)) // GPU_ATTR_CFG10_H
                DEF_IO32(0x10401890, core->gpu.writeAttrOfs<11>(IO_PARAMS)) // GPU_ATTR_OFS11
                DEF_IO32(0x10401894, core->gpu.writeAttrCfgL<11>(IO_PARAMS)) // GPU_ATTR_CFG11_L
                DEF_IO32(0x10401898, core->gpu.writeAttrCfgH<11>(IO_PARAMS)) // GPU_ATTR_CFG11_H
                DEF_IO32(0x1040189C, core->gpu.writeAttrIdxList(IO_PARAMS)) // GPU_ATTR_IDX_LIST
                DEF_IO32(0x104018A0, core->gpu.writeAttrNumVerts(IO_PARAMS)) // GPU_ATTR_NUM_VERTS
                DEF_IO32(0x104018A8, core->gpu.writeAttrFirstIdx(IO_PARAMS)) // GPU_ATTR_FIRST_IDX
                DEF_IO32(0x104018B8, core->gpu.writeAttrDrawArrays(IO_PARAMS)) // GPU_ATTR_DRAW_ARRAYS
                DEF_IO32(0x104018BC, core->gpu.writeAttrDrawElems(IO_PARAMS)) // GPU_ATTR_DRAW_ELEMS
                DEF_IO32(0x104018C8, core->gpu.writeAttrFixedIdx(IO_PARAMS)) // GPU_ATTR_FIXED_IDX
                DEF_IO32(0x104018CC, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018D0, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018D4, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018E0, core->gpu.writeCmdSize<0>(IO_PARAMS)) // GPU_CMD_SIZE0
                DEF_IO32(0x104018E4, core->gpu.writeCmdSize<1>(IO_PARAMS)) // GPU_CMD_SIZE1
                DEF_IO32(0x104018E8, core->gpu.writeCmdAddr<0>(IO_PARAMS)) // GPU_CMD_ADDR0
                DEF_IO32(0x104018EC, core->gpu.writeCmdAddr<1>(IO_PARAMS)) // GPU_CMD_ADDR1
                DEF_IO32(0x104018F0, core->gpu.writeCmdJump<0>(IO_PARAMS)) // GPU_CMD_JUMP0
                DEF_IO32(0x104018F4, core->gpu.writeCmdJump<1>(IO_PARAMS)) // GPU_CMD_JUMP1
                DEF_IO32(0x10401978, core->gpu.writePrimConfig(IO_PARAMS)) // GPU_PRIM_CONFIG
                DEF_IO32(0x1040197C, core->gpu.writePrimRestart(IO_PARAMS)) // GPU_PRIM_RESTART
                DEF_IO32(0x10401AC0, core->gpu.writeVshBools(IO_PARAMS)) // GPU_VSH_BOOLS
                DEF_IO32(0x10401AC4, core->gpu.writeVshInts<0>(IO_PARAMS)) // GPU_VSH_INTS0
                DEF_IO32(0x10401AC8, core->gpu.writeVshInts<1>(IO_PARAMS)) // GPU_VSH_INTS1
                DEF_IO32(0x10401ACC, core->gpu.writeVshInts<2>(IO_PARAMS)) // GPU_VSH_INTS2
                DEF_IO32(0x10401AD0, core->gpu.writeVshInts<3>(IO_PARAMS)) // GPU_VSH_INTS3
                DEF_IO32(0x10401AE8, core->gpu.writeVshEntry(IO_PARAMS)) // GPU_VSH_ENTRY
                DEF_IO32(0x10401AEC, core->gpu.writeVshAttrIdsL(IO_PARAMS)) // GPU_VSH_ATTR_IDS_L
                DEF_IO32(0x10401AF0, core->gpu.writeVshAttrIdsH(IO_PARAMS)) // GPU_VSH_ATTR_IDS_H
                DEF_IO32(0x10401AF4, core->gpu.writeVshOutMask(IO_PARAMS)) // GPU_VSH_OUT_MASK
                DEF_IO32(0x10401B00, core->gpu.writeVshFloatIdx(IO_PARAMS)) // GPU_VSH_FLOAT_IDX
                DEF_IO32(0x10401B04, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B08, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B0C, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B10, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B14, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B18, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B1C, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B20, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B2C, core->gpu.writeVshCodeIdx(IO_PARAMS)) // GPU_VSH_CODE_IDX
                DEF_IO32(0x10401B30, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B34, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B38, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B3C, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B40, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B44, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B48, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B4C, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B54, core->gpu.writeVshDescIdx(IO_PARAMS)) // GPU_VSH_DESC_IDX
                DEF_IO32(0x10401B58, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B5C, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B60, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B64, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B68, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B6C, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B70, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B74, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x17E00100, core->interrupts.writeMpIle(id, IO_PARAMS)) // MP_ILE
                DEF_IO32(0x17E00110, core->interrupts.writeMpEoi(id, IO_PARAMS)) // MP_EOI
                DEF_IO32(0x17E00600, core->timers.writeMpReload(id, 0, IO_PARAMS)) // MP_RELOAD0
                DEF_IO32(0x17E00604, core->timers.writeMpCounter(id, 0, IO_PARAMS)) // MP_COUNTER0
                DEF_IO32(0x17E00608, core->timers.writeMpTmcnt(id, 0, IO_PARAMS)) // MP_TMCNT0
                DEF_IO32(0x17E0060C, core->timers.writeMpTmirq(id, 0, IO_PARAMS)) // MP_TMIRQ0
                DEF_IO32(0x17E00620, core->timers.writeMpReload(id, 1, IO_PARAMS)) // MP_RELOAD1
                DEF_IO32(0x17E00624, core->timers.writeMpCounter(id, 1, IO_PARAMS)) // MP_COUNTER1
                DEF_IO32(0x17E00628, core->timers.writeMpTmcnt(id, 1, IO_PARAMS)) // MP_TMCNT1
                DEF_IO32(0x17E0062C, core->timers.writeMpTmirq(id, 1, IO_PARAMS)) // MP_TMIRQ1
                DEF_IO32(0x17E00700, core->timers.writeMpReload(ARM11A, 0, IO_PARAMS)) // MP0_RELOAD0
                DEF_IO32(0x17E00704, core->timers.writeMpCounter(ARM11A, 0, IO_PARAMS)) // MP0_COUNTER0
                DEF_IO32(0x17E00708, core->timers.writeMpTmcnt(ARM11A, 0, IO_PARAMS)) // MP0_TMCNT0
                DEF_IO32(0x17E0070C, core->timers.writeMpTmirq(ARM11A, 0, IO_PARAMS)) // MP0_TMIRQ0
                DEF_IO32(0x17E00720, core->timers.writeMpReload(ARM11A, 1, IO_PARAMS)) // MP0_RELOAD1
                DEF_IO32(0x17E00724, core->timers.writeMpCounter(ARM11A, 1, IO_PARAMS)) // MP0_COUNTER1
                DEF_IO32(0x17E00728, core->timers.writeMpTmcnt(ARM11A, 1, IO_PARAMS)) // MP0_TMCNT1
                DEF_IO32(0x17E0072C, core->timers.writeMpTmirq(ARM11A, 1, IO_PARAMS)) // MP0_TMIRQ1
                DEF_IO32(0x17E00800, core->timers.writeMpReload(ARM11B, 0, IO_PARAMS)) // MP1_RELOAD0
                DEF_IO32(0x17E00804, core->timers.writeMpCounter(ARM11B, 0, IO_PARAMS)) // MP1_COUNTER0
                DEF_IO32(0x17E00808, core->timers.writeMpTmcnt(ARM11B, 0, IO_PARAMS)) // MP1_TMCNT0
                DEF_IO32(0x17E0080C, core->timers.writeMpTmirq(ARM11B, 0, IO_PARAMS)) // MP1_TMIRQ0
                DEF_IO32(0x17E00820, core->timers.writeMpReload(ARM11B, 1, IO_PARAMS)) // MP1_RELOAD1
                DEF_IO32(0x17E00824, core->timers.writeMpCounter(ARM11B, 1, IO_PARAMS)) // MP1_COUNTER1
                DEF_IO32(0x17E00828, core->timers.writeMpTmcnt(ARM11B, 1, IO_PARAMS)) // MP1_TMCNT1
                DEF_IO32(0x17E0082C, core->timers.writeMpTmirq(ARM11B, 1, IO_PARAMS)) // MP1_TMIRQ1
                DEF_IO32(0x17E00900, core->timers.writeMpReload(ARM11C, 0, IO_PARAMS)) // MP2_RELOAD0
                DEF_IO32(0x17E00904, core->timers.writeMpCounter(ARM11C, 0, IO_PARAMS)) // MP2_COUNTER0
                DEF_IO32(0x17E00908, core->timers.writeMpTmcnt(ARM11C, 0, IO_PARAMS)) // MP2_TMCNT0
                DEF_IO32(0x17E0090C, core->timers.writeMpTmirq(ARM11C, 0, IO_PARAMS)) // MP2_TMIRQ0
                DEF_IO32(0x17E00920, core->timers.writeMpReload(ARM11C, 1, IO_PARAMS)) // MP2_RELOAD1
                DEF_IO32(0x17E00924, core->timers.writeMpCounter(ARM11C, 1, IO_PARAMS)) // MP2_COUNTER1
                DEF_IO32(0x17E00928, core->timers.writeMpTmcnt(ARM11C, 1, IO_PARAMS)) // MP2_TMCNT1
                DEF_IO32(0x17E0092C, core->timers.writeMpTmirq(ARM11C, 1, IO_PARAMS)) // MP2_TMIRQ1
                DEF_IO32(0x17E00A00, core->timers.writeMpReload(ARM11D, 0, IO_PARAMS)) // MP3_RELOAD0
                DEF_IO32(0x17E00A04, core->timers.writeMpCounter(ARM11D, 0, IO_PARAMS)) // MP3_COUNTER0
                DEF_IO32(0x17E00A08, core->timers.writeMpTmcnt(ARM11D, 0, IO_PARAMS)) // MP3_TMCNT0
                DEF_IO32(0x17E00A0C, core->timers.writeMpTmirq(ARM11D, 0, IO_PARAMS)) // MP3_TMIRQ0
                DEF_IO32(0x17E00A20, core->timers.writeMpReload(ARM11D, 1, IO_PARAMS)) // MP3_RELOAD1
                DEF_IO32(0x17E00A24, core->timers.writeMpCounter(ARM11D, 1, IO_PARAMS)) // MP3_COUNTER1
                DEF_IO32(0x17E00A28, core->timers.writeMpTmcnt(ARM11D, 1, IO_PARAMS)) // MP3_TMCNT1
                DEF_IO32(0x17E00A2C, core->timers.writeMpTmirq(ARM11D, 1, IO_PARAMS)) // MP3_TMIRQ1
                DEF_IO32(0x17E01000, core->interrupts.writeMpIge(IO_PARAMS)) // MP_IGE
                DEF_IO32(0x17E01100, core->interrupts.writeMpIeSet(0, IO_PARAMS)) // MP_IE0_SET
                DEF_IO32(0x17E01104, core->interrupts.writeMpIeSet(1, IO_PARAMS)) // MP_IE1_SET
                DEF_IO32(0x17E01108, core->interrupts.writeMpIeSet(2, IO_PARAMS)) // MP_IE2_SET
                DEF_IO32(0x17E0110C, core->interrupts.writeMpIeSet(3, IO_PARAMS)) // MP_IE3_SET
                DEF_IO32(0x17E01180, core->interrupts.writeMpIeClear(0, IO_PARAMS)) // MP_IE0_CLEAR
                DEF_IO32(0x17E01184, core->interrupts.writeMpIeClear(1, IO_PARAMS)) // MP_IE1_CLEAR
                DEF_IO32(0x17E01188, core->interrupts.writeMpIeClear(2, IO_PARAMS)) // MP_IE2_CLEAR
                DEF_IO32(0x17E0118C, core->interrupts.writeMpIeClear(3, IO_PARAMS)) // MP_IE3_CLEAR
                DEF_IO32(0x17E01200, core->interrupts.writeMpIpSet(0, IO_PARAMS)) // MP_IP0_SET
                DEF_IO32(0x17E01204, core->interrupts.writeMpIpSet(1, IO_PARAMS)) // MP_IP1_SET
                DEF_IO32(0x17E01208, core->interrupts.writeMpIpSet(2, IO_PARAMS)) // MP_IP2_SET
                DEF_IO32(0x17E0120C, core->interrupts.writeMpIpSet(3, IO_PARAMS)) // MP_IP3_SET
                DEF_IO32(0x17E01280, core->interrupts.writeMpIpClear(0, IO_PARAMS)) // MP_IP0_CLEAR
                DEF_IO32(0x17E01284, core->interrupts.writeMpIpClear(1, IO_PARAMS)) // MP_IP1_CLEAR
                DEF_IO32(0x17E01288, core->interrupts.writeMpIpClear(2, IO_PARAMS)) // MP_IP2_CLEAR
                DEF_IO32(0x17E0128C, core->interrupts.writeMpIpClear(3, IO_PARAMS)) // MP_IP3_CLEAR
                DEF_IO08(0x17E01800, core->interrupts.writeMpTarget(0, IO_PARAMS8)) // MP_TARGET0
                DEF_IO08(0x17E01801, core->interrupts.writeMpTarget(1, IO_PARAMS8)) // MP_TARGET1
                DEF_IO08(0x17E01802, core->interrupts.writeMpTarget(2, IO_PARAMS8)) // MP_TARGET2
                DEF_IO08(0x17E01803, core->interrupts.writeMpTarget(3, IO_PARAMS8)) // MP_TARGET3
                DEF_IO08(0x17E01804, core->interrupts.writeMpTarget(4, IO_PARAMS8)) // MP_TARGET4
                DEF_IO08(0x17E01805, core->interrupts.writeMpTarget(5, IO_PARAMS8)) // MP_TARGET5
                DEF_IO08(0x17E01806, core->interrupts.writeMpTarget(6, IO_PARAMS8)) // MP_TARGET6
                DEF_IO08(0x17E01807, core->interrupts.writeMpTarget(7, IO_PARAMS8)) // MP_TARGET7
                DEF_IO08(0x17E01808, core->interrupts.writeMpTarget(8, IO_PARAMS8)) // MP_TARGET8
                DEF_IO08(0x17E01809, core->interrupts.writeMpTarget(9, IO_PARAMS8)) // MP_TARGET9
                DEF_IO08(0x17E0180A, core->interrupts.writeMpTarget(10, IO_PARAMS8)) // MP_TARGET10
                DEF_IO08(0x17E0180B, core->interrupts.writeMpTarget(11, IO_PARAMS8)) // MP_TARGET11
                DEF_IO08(0x17E0180C, core->interrupts.writeMpTarget(12, IO_PARAMS8)) // MP_TARGET12
                DEF_IO08(0x17E0180D, core->interrupts.writeMpTarget(13, IO_PARAMS8)) // MP_TARGET13
                DEF_IO08(0x17E0180E, core->interrupts.writeMpTarget(14, IO_PARAMS8)) // MP_TARGET14
                DEF_IO08(0x17E0180F, core->interrupts.writeMpTarget(15, IO_PARAMS8)) // MP_TARGET15
                DEF_IO08(0x17E01810, core->interrupts.writeMpTarget(16, IO_PARAMS8)) // MP_TARGET16
                DEF_IO08(0x17E01811, core->interrupts.writeMpTarget(17, IO_PARAMS8)) // MP_TARGET17
                DEF_IO08(0x17E01812, core->interrupts.writeMpTarget(18, IO_PARAMS8)) // MP_TARGET18
                DEF_IO08(0x17E01813, core->interrupts.writeMpTarget(19, IO_PARAMS8)) // MP_TARGET19
                DEF_IO08(0x17E01814, core->interrupts.writeMpTarget(20, IO_PARAMS8)) // MP_TARGET20
                DEF_IO08(0x17E01815, core->interrupts.writeMpTarget(21, IO_PARAMS8)) // MP_TARGET21
                DEF_IO08(0x17E01816, core->interrupts.writeMpTarget(22, IO_PARAMS8)) // MP_TARGET22
                DEF_IO08(0x17E01817, core->interrupts.writeMpTarget(23, IO_PARAMS8)) // MP_TARGET23
                DEF_IO08(0x17E01818, core->interrupts.writeMpTarget(24, IO_PARAMS8)) // MP_TARGET24
                DEF_IO08(0x17E01819, core->interrupts.writeMpTarget(25, IO_PARAMS8)) // MP_TARGET25
                DEF_IO08(0x17E0181A, core->interrupts.writeMpTarget(26, IO_PARAMS8)) // MP_TARGET26
                DEF_IO08(0x17E0181B, core->interrupts.writeMpTarget(27, IO_PARAMS8)) // MP_TARGET27
                DEF_IO08(0x17E0181C, core->interrupts.writeMpTarget(28, IO_PARAMS8)) // MP_TARGET28
                DEF_IO08(0x17E0181D, core->interrupts.writeMpTarget(29, IO_PARAMS8)) // MP_TARGET29
                DEF_IO08(0x17E0181E, core->interrupts.writeMpTarget(30, IO_PARAMS8)) // MP_TARGET30
                DEF_IO08(0x17E0181F, core->interrupts.writeMpTarget(31, IO_PARAMS8)) // MP_TARGET31
                DEF_IO08(0x17E01820, core->interrupts.writeMpTarget(32, IO_PARAMS8)) // MP_TARGET32
                DEF_IO08(0x17E01821, core->interrupts.writeMpTarget(33, IO_PARAMS8)) // MP_TARGET33
                DEF_IO08(0x17E01822, core->interrupts.writeMpTarget(34, IO_PARAMS8)) // MP_TARGET34
                DEF_IO08(0x17E01823, core->interrupts.writeMpTarget(35, IO_PARAMS8)) // MP_TARGET35
                DEF_IO08(0x17E01824, core->interrupts.writeMpTarget(36, IO_PARAMS8)) // MP_TARGET36
                DEF_IO08(0x17E01825, core->interrupts.writeMpTarget(37, IO_PARAMS8)) // MP_TARGET37
                DEF_IO08(0x17E01826, core->interrupts.writeMpTarget(38, IO_PARAMS8)) // MP_TARGET38
                DEF_IO08(0x17E01827, core->interrupts.writeMpTarget(39, IO_PARAMS8)) // MP_TARGET39
                DEF_IO08(0x17E01828, core->interrupts.writeMpTarget(40, IO_PARAMS8)) // MP_TARGET40
                DEF_IO08(0x17E01829, core->interrupts.writeMpTarget(41, IO_PARAMS8)) // MP_TARGET41
                DEF_IO08(0x17E0182A, core->interrupts.writeMpTarget(42, IO_PARAMS8)) // MP_TARGET42
                DEF_IO08(0x17E0182B, core->interrupts.writeMpTarget(43, IO_PARAMS8)) // MP_TARGET43
                DEF_IO08(0x17E0182C, core->interrupts.writeMpTarget(44, IO_PARAMS8)) // MP_TARGET44
                DEF_IO08(0x17E0182D, core->interrupts.writeMpTarget(45, IO_PARAMS8)) // MP_TARGET45
                DEF_IO08(0x17E0182E, core->interrupts.writeMpTarget(46, IO_PARAMS8)) // MP_TARGET46
                DEF_IO08(0x17E0182F, core->interrupts.writeMpTarget(47, IO_PARAMS8)) // MP_TARGET47
                DEF_IO08(0x17E01830, core->interrupts.writeMpTarget(48, IO_PARAMS8)) // MP_TARGET48
                DEF_IO08(0x17E01831, core->interrupts.writeMpTarget(49, IO_PARAMS8)) // MP_TARGET49
                DEF_IO08(0x17E01832, core->interrupts.writeMpTarget(50, IO_PARAMS8)) // MP_TARGET50
                DEF_IO08(0x17E01833, core->interrupts.writeMpTarget(51, IO_PARAMS8)) // MP_TARGET51
                DEF_IO08(0x17E01834, core->interrupts.writeMpTarget(52, IO_PARAMS8)) // MP_TARGET52
                DEF_IO08(0x17E01835, core->interrupts.writeMpTarget(53, IO_PARAMS8)) // MP_TARGET53
                DEF_IO08(0x17E01836, core->interrupts.writeMpTarget(54, IO_PARAMS8)) // MP_TARGET54
                DEF_IO08(0x17E01837, core->interrupts.writeMpTarget(55, IO_PARAMS8)) // MP_TARGET55
                DEF_IO08(0x17E01838, core->interrupts.writeMpTarget(56, IO_PARAMS8)) // MP_TARGET56
                DEF_IO08(0x17E01839, core->interrupts.writeMpTarget(57, IO_PARAMS8)) // MP_TARGET57
                DEF_IO08(0x17E0183A, core->interrupts.writeMpTarget(58, IO_PARAMS8)) // MP_TARGET58
                DEF_IO08(0x17E0183B, core->interrupts.writeMpTarget(59, IO_PARAMS8)) // MP_TARGET59
                DEF_IO08(0x17E0183C, core->interrupts.writeMpTarget(60, IO_PARAMS8)) // MP_TARGET60
                DEF_IO08(0x17E0183D, core->interrupts.writeMpTarget(61, IO_PARAMS8)) // MP_TARGET61
                DEF_IO08(0x17E0183E, core->interrupts.writeMpTarget(62, IO_PARAMS8)) // MP_TARGET62
                DEF_IO08(0x17E0183F, core->interrupts.writeMpTarget(63, IO_PARAMS8)) // MP_TARGET63
                DEF_IO08(0x17E01840, core->interrupts.writeMpTarget(64, IO_PARAMS8)) // MP_TARGET64
                DEF_IO08(0x17E01841, core->interrupts.writeMpTarget(65, IO_PARAMS8)) // MP_TARGET65
                DEF_IO08(0x17E01842, core->interrupts.writeMpTarget(66, IO_PARAMS8)) // MP_TARGET66
                DEF_IO08(0x17E01843, core->interrupts.writeMpTarget(67, IO_PARAMS8)) // MP_TARGET67
                DEF_IO08(0x17E01844, core->interrupts.writeMpTarget(68, IO_PARAMS8)) // MP_TARGET68
                DEF_IO08(0x17E01845, core->interrupts.writeMpTarget(69, IO_PARAMS8)) // MP_TARGET69
                DEF_IO08(0x17E01846, core->interrupts.writeMpTarget(70, IO_PARAMS8)) // MP_TARGET70
                DEF_IO08(0x17E01847, core->interrupts.writeMpTarget(71, IO_PARAMS8)) // MP_TARGET71
                DEF_IO08(0x17E01848, core->interrupts.writeMpTarget(72, IO_PARAMS8)) // MP_TARGET72
                DEF_IO08(0x17E01849, core->interrupts.writeMpTarget(73, IO_PARAMS8)) // MP_TARGET73
                DEF_IO08(0x17E0184A, core->interrupts.writeMpTarget(74, IO_PARAMS8)) // MP_TARGET74
                DEF_IO08(0x17E0184B, core->interrupts.writeMpTarget(75, IO_PARAMS8)) // MP_TARGET75
                DEF_IO08(0x17E0184C, core->interrupts.writeMpTarget(76, IO_PARAMS8)) // MP_TARGET76
                DEF_IO08(0x17E0184D, core->interrupts.writeMpTarget(77, IO_PARAMS8)) // MP_TARGET77
                DEF_IO08(0x17E0184E, core->interrupts.writeMpTarget(78, IO_PARAMS8)) // MP_TARGET78
                DEF_IO08(0x17E0184F, core->interrupts.writeMpTarget(79, IO_PARAMS8)) // MP_TARGET79
                DEF_IO08(0x17E01850, core->interrupts.writeMpTarget(80, IO_PARAMS8)) // MP_TARGET80
                DEF_IO08(0x17E01851, core->interrupts.writeMpTarget(81, IO_PARAMS8)) // MP_TARGET81
                DEF_IO08(0x17E01852, core->interrupts.writeMpTarget(82, IO_PARAMS8)) // MP_TARGET82
                DEF_IO08(0x17E01853, core->interrupts.writeMpTarget(83, IO_PARAMS8)) // MP_TARGET83
                DEF_IO08(0x17E01854, core->interrupts.writeMpTarget(84, IO_PARAMS8)) // MP_TARGET84
                DEF_IO08(0x17E01855, core->interrupts.writeMpTarget(85, IO_PARAMS8)) // MP_TARGET85
                DEF_IO08(0x17E01856, core->interrupts.writeMpTarget(86, IO_PARAMS8)) // MP_TARGET86
                DEF_IO08(0x17E01857, core->interrupts.writeMpTarget(87, IO_PARAMS8)) // MP_TARGET87
                DEF_IO08(0x17E01858, core->interrupts.writeMpTarget(88, IO_PARAMS8)) // MP_TARGET88
                DEF_IO08(0x17E01859, core->interrupts.writeMpTarget(89, IO_PARAMS8)) // MP_TARGET89
                DEF_IO08(0x17E0185A, core->interrupts.writeMpTarget(90, IO_PARAMS8)) // MP_TARGET90
                DEF_IO08(0x17E0185B, core->interrupts.writeMpTarget(91, IO_PARAMS8)) // MP_TARGET91
                DEF_IO08(0x17E0185C, core->interrupts.writeMpTarget(92, IO_PARAMS8)) // MP_TARGET92
                DEF_IO08(0x17E0185D, core->interrupts.writeMpTarget(93, IO_PARAMS8)) // MP_TARGET93
                DEF_IO08(0x17E0185E, core->interrupts.writeMpTarget(94, IO_PARAMS8)) // MP_TARGET94
                DEF_IO08(0x17E0185F, core->interrupts.writeMpTarget(95, IO_PARAMS8)) // MP_TARGET95
                DEF_IO08(0x17E01860, core->interrupts.writeMpTarget(96, IO_PARAMS8)) // MP_TARGET96
                DEF_IO08(0x17E01861, core->interrupts.writeMpTarget(97, IO_PARAMS8)) // MP_TARGET97
                DEF_IO08(0x17E01862, core->interrupts.writeMpTarget(98, IO_PARAMS8)) // MP_TARGET98
                DEF_IO08(0x17E01863, core->interrupts.writeMpTarget(99, IO_PARAMS8)) // MP_TARGET99
                DEF_IO08(0x17E01864, core->interrupts.writeMpTarget(100, IO_PARAMS8)) // MP_TARGET100
                DEF_IO08(0x17E01865, core->interrupts.writeMpTarget(101, IO_PARAMS8)) // MP_TARGET101
                DEF_IO08(0x17E01866, core->interrupts.writeMpTarget(102, IO_PARAMS8)) // MP_TARGET102
                DEF_IO08(0x17E01867, core->interrupts.writeMpTarget(103, IO_PARAMS8)) // MP_TARGET103
                DEF_IO08(0x17E01868, core->interrupts.writeMpTarget(104, IO_PARAMS8)) // MP_TARGET104
                DEF_IO08(0x17E01869, core->interrupts.writeMpTarget(105, IO_PARAMS8)) // MP_TARGET105
                DEF_IO08(0x17E0186A, core->interrupts.writeMpTarget(106, IO_PARAMS8)) // MP_TARGET106
                DEF_IO08(0x17E0186B, core->interrupts.writeMpTarget(107, IO_PARAMS8)) // MP_TARGET107
                DEF_IO08(0x17E0186C, core->interrupts.writeMpTarget(108, IO_PARAMS8)) // MP_TARGET108
                DEF_IO08(0x17E0186D, core->interrupts.writeMpTarget(109, IO_PARAMS8)) // MP_TARGET109
                DEF_IO08(0x17E0186E, core->interrupts.writeMpTarget(110, IO_PARAMS8)) // MP_TARGET110
                DEF_IO08(0x17E0186F, core->interrupts.writeMpTarget(111, IO_PARAMS8)) // MP_TARGET111
                DEF_IO08(0x17E01870, core->interrupts.writeMpTarget(112, IO_PARAMS8)) // MP_TARGET112
                DEF_IO08(0x17E01871, core->interrupts.writeMpTarget(113, IO_PARAMS8)) // MP_TARGET113
                DEF_IO08(0x17E01872, core->interrupts.writeMpTarget(114, IO_PARAMS8)) // MP_TARGET114
                DEF_IO08(0x17E01873, core->interrupts.writeMpTarget(115, IO_PARAMS8)) // MP_TARGET115
                DEF_IO08(0x17E01874, core->interrupts.writeMpTarget(116, IO_PARAMS8)) // MP_TARGET116
                DEF_IO08(0x17E01875, core->interrupts.writeMpTarget(117, IO_PARAMS8)) // MP_TARGET117
                DEF_IO08(0x17E01876, core->interrupts.writeMpTarget(118, IO_PARAMS8)) // MP_TARGET118
                DEF_IO08(0x17E01877, core->interrupts.writeMpTarget(119, IO_PARAMS8)) // MP_TARGET119
                DEF_IO08(0x17E01878, core->interrupts.writeMpTarget(120, IO_PARAMS8)) // MP_TARGET120
                DEF_IO08(0x17E01879, core->interrupts.writeMpTarget(121, IO_PARAMS8)) // MP_TARGET121
                DEF_IO08(0x17E0187A, core->interrupts.writeMpTarget(122, IO_PARAMS8)) // MP_TARGET122
                DEF_IO08(0x17E0187B, core->interrupts.writeMpTarget(123, IO_PARAMS8)) // MP_TARGET123
                DEF_IO08(0x17E0187C, core->interrupts.writeMpTarget(124, IO_PARAMS8)) // MP_TARGET124
                DEF_IO08(0x17E0187D, core->interrupts.writeMpTarget(125, IO_PARAMS8)) // MP_TARGET125
                DEF_IO08(0x17E0187E, core->interrupts.writeMpTarget(126, IO_PARAMS8)) // MP_TARGET126
                DEF_IO08(0x17E0187F, core->interrupts.writeMpTarget(127, IO_PARAMS8)) // MP_TARGET127
                DEF_IO32(0x17E01F00, core->interrupts.writeMpSoftIrq(id, IO_PARAMS)) // MP_SOFT_IRQ
            }
        }
        else { // ARM9
            switch (base) {
                DEF_IO08(0x10000000, writeCfg9Sysprot9(IO_PARAMS8)) // CFG9_SYSPROT9
                DEF_IO08(0x10000001, writeCfg9Sysprot11(IO_PARAMS8)) // CFG9_SYSPROT11
                DEF_IO16(0x10000010, core->cartridge.writeCfg9CardPower(IO_PARAMS)) // CFG9_CARD_POWER
                DEF_IO32(0x10000200, writeCfg9Extmemcnt9(IO_PARAMS)) // CFG9_EXTMEMCNT9
                DEF_IO32(0x10001000, core->interrupts.writeIrqIe(IO_PARAMS)) // IRQ_IE
                DEF_IO32(0x10001004, core->interrupts.writeIrqIf(IO_PARAMS)) // IRQ_IF
                DEF_IO32(0x10002004, core->ndma.writeSad(0, IO_PARAMS)) // NDMA0SAD
                DEF_IO32(0x10002008, core->ndma.writeDad(0, IO_PARAMS)) // NDMA0DAD
                DEF_IO32(0x1000200C, core->ndma.writeTcnt(0, IO_PARAMS)) // NDMA0TCNT
                DEF_IO32(0x10002010, core->ndma.writeWcnt(0, IO_PARAMS)) // NDMA0WCNT
                DEF_IO32(0x10002018, core->ndma.writeFdata(0, IO_PARAMS)) // NDMA0FDATA
                DEF_IO32(0x1000201C, core->ndma.writeCnt(0, IO_PARAMS)) // NDMA0CNT
                DEF_IO32(0x10002020, core->ndma.writeSad(1, IO_PARAMS)) // NDMA1SAD
                DEF_IO32(0x10002024, core->ndma.writeDad(1, IO_PARAMS)) // NDMA1DAD
                DEF_IO32(0x10002028, core->ndma.writeTcnt(1, IO_PARAMS)) // NDMA1TCNT
                DEF_IO32(0x1000202C, core->ndma.writeWcnt(1, IO_PARAMS)) // NDMA1WCNT
                DEF_IO32(0x10002034, core->ndma.writeFdata(1, IO_PARAMS)) // NDMA1FDATA
                DEF_IO32(0x10002038, core->ndma.writeCnt(1, IO_PARAMS)) // NDMA1CNT
                DEF_IO32(0x1000203C, core->ndma.writeSad(2, IO_PARAMS)) // NDMA2SAD
                DEF_IO32(0x10002040, core->ndma.writeDad(2, IO_PARAMS)) // NDMA2DAD
                DEF_IO32(0x10002044, core->ndma.writeTcnt(2, IO_PARAMS)) // NDMA2TCNT
                DEF_IO32(0x10002048, core->ndma.writeWcnt(2, IO_PARAMS)) // NDMA2WCNT
                DEF_IO32(0x10002050, core->ndma.writeFdata(2, IO_PARAMS)) // NDMA2FDATA
                DEF_IO32(0x10002054, core->ndma.writeCnt(2, IO_PARAMS)) // NDMA2CNT
                DEF_IO32(0x10002058, core->ndma.writeSad(3, IO_PARAMS)) // NDMA3SAD
                DEF_IO32(0x1000205C, core->ndma.writeDad(3, IO_PARAMS)) // NDMA3DAD
                DEF_IO32(0x10002060, core->ndma.writeTcnt(3, IO_PARAMS)) // NDMA3TCNT
                DEF_IO32(0x10002064, core->ndma.writeWcnt(3, IO_PARAMS)) // NDMA3WCNT
                DEF_IO32(0x1000206C, core->ndma.writeFdata(3, IO_PARAMS)) // NDMA3FDATA
                DEF_IO32(0x10002070, core->ndma.writeCnt(3, IO_PARAMS)) // NDMA3CNT
                DEF_IO32(0x10002074, core->ndma.writeSad(4, IO_PARAMS)) // NDMA4SAD
                DEF_IO32(0x10002078, core->ndma.writeDad(4, IO_PARAMS)) // NDMA4DAD
                DEF_IO32(0x1000207C, core->ndma.writeTcnt(4, IO_PARAMS)) // NDMA4TCNT
                DEF_IO32(0x10002080, core->ndma.writeWcnt(4, IO_PARAMS)) // NDMA4WCNT
                DEF_IO32(0x10002088, core->ndma.writeFdata(4, IO_PARAMS)) // NDMA4FDATA
                DEF_IO32(0x1000208C, core->ndma.writeCnt(4, IO_PARAMS)) // NDMA4CNT
                DEF_IO32(0x10002090, core->ndma.writeSad(5, IO_PARAMS)) // NDMA5SAD
                DEF_IO32(0x10002094, core->ndma.writeDad(5, IO_PARAMS)) // NDMA5DAD
                DEF_IO32(0x10002098, core->ndma.writeTcnt(5, IO_PARAMS)) // NDMA5TCNT
                DEF_IO32(0x1000209C, core->ndma.writeWcnt(5, IO_PARAMS)) // NDMA5WCNT
                DEF_IO32(0x100020A4, core->ndma.writeFdata(5, IO_PARAMS)) // NDMA5FDATA
                DEF_IO32(0x100020A8, core->ndma.writeCnt(5, IO_PARAMS)) // NDMA5CNT
                DEF_IO32(0x100020AC, core->ndma.writeSad(6, IO_PARAMS)) // NDMA6SAD
                DEF_IO32(0x100020B0, core->ndma.writeDad(6, IO_PARAMS)) // NDMA6DAD
                DEF_IO32(0x100020B4, core->ndma.writeTcnt(6, IO_PARAMS)) // NDMA6TCNT
                DEF_IO32(0x100020B8, core->ndma.writeWcnt(6, IO_PARAMS)) // NDMA6WCNT
                DEF_IO32(0x100020C0, core->ndma.writeFdata(6, IO_PARAMS)) // NDMA6FDATA
                DEF_IO32(0x100020C4, core->ndma.writeCnt(6, IO_PARAMS)) // NDMA6CNT
                DEF_IO32(0x100020C8, core->ndma.writeSad(7, IO_PARAMS)) // NDMA7SAD
                DEF_IO32(0x100020CC, core->ndma.writeDad(7, IO_PARAMS)) // NDMA7DAD
                DEF_IO32(0x100020D0, core->ndma.writeTcnt(7, IO_PARAMS)) // NDMA7TCNT
                DEF_IO32(0x100020D4, core->ndma.writeWcnt(7, IO_PARAMS)) // NDMA7WCNT
                DEF_IO32(0x100020DC, core->ndma.writeFdata(7, IO_PARAMS)) // NDMA7FDATA
                DEF_IO32(0x100020E0, core->ndma.writeCnt(7, IO_PARAMS)) // NDMA7CNT
                DEF_IO16(0x10003000, core->timers.writeTmCntL(0, IO_PARAMS)) // TM0CNT_L
                DEF_IO16(0x10003002, core->timers.writeTmCntH(0, IO_PARAMS)) // TM0CNT_H
                DEF_IO16(0x10003004, core->timers.writeTmCntL(1, IO_PARAMS)) // TM1CNT_L
                DEF_IO16(0x10003006, core->timers.writeTmCntH(1, IO_PARAMS)) // TM1CNT_H
                DEF_IO16(0x10003008, core->timers.writeTmCntL(2, IO_PARAMS)) // TM2CNT_L
                DEF_IO16(0x1000300A, core->timers.writeTmCntH(2, IO_PARAMS)) // TM2CNT_H
                DEF_IO16(0x1000300C, core->timers.writeTmCntL(3, IO_PARAMS)) // TM3CNT_L
                DEF_IO16(0x1000300E, core->timers.writeTmCntH(3, IO_PARAMS)) // TM3CNT_H
                DEF_IO32(0x10004000, core->cartridge.writeCtrCnt(IO_PARAMS)) // CTRCARD_CNT
                DEF_IO32(0x10004004, core->cartridge.writeCtrBlkcnt(IO_PARAMS)) // CTRCARD_BLKCNT
                DEF_IO32(0x10004008, core->cartridge.writeCtrSeccnt(IO_PARAMS)) // CTRCARD_SECCNT
                DEF_IO32(0x10004020, core->cartridge.writeCtrCmd(0, IO_PARAMS)) // CTRCARD_CMD0
                DEF_IO32(0x10004024, core->cartridge.writeCtrCmd(1, IO_PARAMS)) // CTRCARD_CMD1
                DEF_IO32(0x10004028, core->cartridge.writeCtrCmd(2, IO_PARAMS)) // CTRCARD_CMD2
                DEF_IO32(0x1000402C, core->cartridge.writeCtrCmd(3, IO_PARAMS)) // CTRCARD_CMD3
                DEF_IO16(0x10006000, core->sdMmcs[0].writeCmd(IO_PARAMS)) // SD0_CMD
                DEF_IO16(0x10006002, core->sdMmcs[0].writePortSelect(IO_PARAMS)) // SD0_PORT_SELECT
                DEF_IO32(0x10006004, core->sdMmcs[0].writeCmdParam(IO_PARAMS)) // SD0_CMD_PARAM
                DEF_IO16(0x1000600A, core->sdMmcs[0].writeData16Blkcnt(IO_PARAMS)) // SD0_DATA16_BLKCNT
                DEF_IO32(0x1000601C, core->sdMmcs[0].writeIrqStatus(IO_PARAMS)) // SD0_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmcs[0].writeIrqMask(IO_PARAMS)) // SD0_IRQ_MASK
                DEF_IO16(0x10006026, core->sdMmcs[0].writeData16Blklen(IO_PARAMS)) // SD0_DATA16_BLKLEN
                DEF_IO16(0x10006030, core->sdMmcs[0].writeData16Fifo(IO_PARAMS)) // SD0_DATA16_FIFO
                DEF_IO16(0x100060D8, core->sdMmcs[0].writeDataCtl(IO_PARAMS)) // SD0_DATA_CTL
                DEF_IO16(0x10006100, core->sdMmcs[0].writeData32Irq(IO_PARAMS)) // SD0_DATA32_IRQ
                DEF_IO16(0x10006104, core->sdMmcs[0].writeData32Blklen(IO_PARAMS)) // SD0_DATA32_BLKLEN
                DEF_IO32(0x1000610C, core->sdMmcs[0].writeData32Fifo(IO_PARAMS)) // SD0_DATA32_FIFO
                DEF_IO16(0x10007000, core->sdMmcs[1].writeCmd(IO_PARAMS)) // SD1_CMD
                DEF_IO16(0x10007002, core->sdMmcs[1].writePortSelect(IO_PARAMS)) // SD1_PORT_SELECT
                DEF_IO32(0x10007004, core->sdMmcs[1].writeCmdParam(IO_PARAMS)) // SD1_CMD_PARAM
                DEF_IO16(0x1000700A, core->sdMmcs[1].writeData16Blkcnt(IO_PARAMS)) // SD1_DATA16_BLKCNT
                DEF_IO32(0x1000701C, core->sdMmcs[1].writeIrqStatus(IO_PARAMS)) // SD1_IRQ_STATUS
                DEF_IO32(0x10007020, core->sdMmcs[1].writeIrqMask(IO_PARAMS)) // SD1_IRQ_MASK
                DEF_IO16(0x10007026, core->sdMmcs[1].writeData16Blklen(IO_PARAMS)) // SD1_DATA16_BLKLEN
                DEF_IO16(0x10007030, core->sdMmcs[1].writeData16Fifo(IO_PARAMS)) // SD1_DATA16_FIFO
                DEF_IO16(0x100070D8, core->sdMmcs[1].writeDataCtl(IO_PARAMS)) // SD1_DATA_CTL
                DEF_IO16(0x10007100, core->sdMmcs[1].writeData32Irq(IO_PARAMS)) // SD1_DATA32_IRQ
                DEF_IO16(0x10007104, core->sdMmcs[1].writeData32Blklen(IO_PARAMS)) // SD1_DATA32_BLKLEN
                DEF_IO32(0x1000710C, core->sdMmcs[1].writeData32Fifo(IO_PARAMS)) // SD1_DATA32_FIFO
                DEF_IO32(0x10008000, core->pxi.writeSync(1, IO_PARAMS)) // PXI_SYNC9
                DEF_IO32(0x10008004, core->pxi.writeCnt(1, IO_PARAMS)) // PXI_CNT9
                DEF_IO32(0x10008008, core->pxi.writeSend(1, IO_PARAMS)) // PXI_SEND9
                DEF_IO32(0x10009000, core->aes.writeCnt(IO_PARAMS)) // AES_CNT
                DEF_IO32(0x10009004, core->aes.writeBlkcnt(IO_PARAMS)) // AES_BLKCNT
                DEF_IO32(0x10009008, core->aes.writeWrfifo(IO_PARAMS)) // AES_WRFIFO
                DEF_IO08(0x10009010, core->aes.writeKeysel(IO_PARAMS8)) // AES_KEYSEL
                DEF_IO08(0x10009011, core->aes.writeKeycnt(IO_PARAMS8)) // AES_KEYCNT
                DEF_IO32(0x10009020, core->aes.writeIv(0, IO_PARAMS)) // AES_IV0
                DEF_IO32(0x10009024, core->aes.writeIv(1, IO_PARAMS)) // AES_IV1
                DEF_IO32(0x10009028, core->aes.writeIv(2, IO_PARAMS)) // AES_IV2
                DEF_IO32(0x1000902C, core->aes.writeIv(3, IO_PARAMS)) // AES_IV3
                DEF_IO32(0x10009030, core->aes.writeMac(0, IO_PARAMS)) // AES_MAC0
                DEF_IO32(0x10009034, core->aes.writeMac(1, IO_PARAMS)) // AES_MAC1
                DEF_IO32(0x10009038, core->aes.writeMac(2, IO_PARAMS)) // AES_MAC2
                DEF_IO32(0x1000903C, core->aes.writeMac(3, IO_PARAMS)) // AES_MAC3
                DEF_IO32(0x10009040, core->aes.writeKey(0, 0, IO_PARAMS)) // AES_KEY0_0
                DEF_IO32(0x10009044, core->aes.writeKey(0, 1, IO_PARAMS)) // AES_KEY0_1
                DEF_IO32(0x10009048, core->aes.writeKey(0, 2, IO_PARAMS)) // AES_KEY0_2
                DEF_IO32(0x1000904C, core->aes.writeKey(0, 3, IO_PARAMS)) // AES_KEY0_3
                DEF_IO32(0x10009050, core->aes.writeKeyx(0, 0, IO_PARAMS)) // AES_KEYX0_0
                DEF_IO32(0x10009054, core->aes.writeKeyx(0, 1, IO_PARAMS)) // AES_KEYX0_1
                DEF_IO32(0x10009058, core->aes.writeKeyx(0, 2, IO_PARAMS)) // AES_KEYX0_2
                DEF_IO32(0x1000905C, core->aes.writeKeyx(0, 3, IO_PARAMS)) // AES_KEYX0_3
                DEF_IO32(0x10009060, core->aes.writeKeyy(0, 0, IO_PARAMS)) // AES_KEYY0_0
                DEF_IO32(0x10009064, core->aes.writeKeyy(0, 1, IO_PARAMS)) // AES_KEYY0_1
                DEF_IO32(0x10009068, core->aes.writeKeyy(0, 2, IO_PARAMS)) // AES_KEYY0_2
                DEF_IO32(0x1000906C, core->aes.writeKeyy(0, 3, IO_PARAMS)) // AES_KEYY0_3
                DEF_IO32(0x10009070, core->aes.writeKey(1, 0, IO_PARAMS)) // AES_KEY1_0
                DEF_IO32(0x10009074, core->aes.writeKey(1, 1, IO_PARAMS)) // AES_KEY1_1
                DEF_IO32(0x10009078, core->aes.writeKey(1, 2, IO_PARAMS)) // AES_KEY1_2
                DEF_IO32(0x1000907C, core->aes.writeKey(1, 3, IO_PARAMS)) // AES_KEY1_3
                DEF_IO32(0x10009080, core->aes.writeKeyx(1, 0, IO_PARAMS)) // AES_KEYX1_0
                DEF_IO32(0x10009084, core->aes.writeKeyx(1, 1, IO_PARAMS)) // AES_KEYX1_1
                DEF_IO32(0x10009088, core->aes.writeKeyx(1, 2, IO_PARAMS)) // AES_KEYX1_2
                DEF_IO32(0x1000908C, core->aes.writeKeyx(1, 3, IO_PARAMS)) // AES_KEYX1_3
                DEF_IO32(0x10009090, core->aes.writeKeyy(1, 0, IO_PARAMS)) // AES_KEYY1_0
                DEF_IO32(0x10009094, core->aes.writeKeyy(1, 1, IO_PARAMS)) // AES_KEYY1_1
                DEF_IO32(0x10009098, core->aes.writeKeyy(1, 2, IO_PARAMS)) // AES_KEYY1_2
                DEF_IO32(0x1000909C, core->aes.writeKeyy(1, 3, IO_PARAMS)) // AES_KEYY1_3
                DEF_IO32(0x100090A0, core->aes.writeKey(2, 0, IO_PARAMS)) // AES_KEY2_0
                DEF_IO32(0x100090A4, core->aes.writeKey(2, 1, IO_PARAMS)) // AES_KEY2_1
                DEF_IO32(0x100090A8, core->aes.writeKey(2, 2, IO_PARAMS)) // AES_KEY2_2
                DEF_IO32(0x100090AC, core->aes.writeKey(2, 3, IO_PARAMS)) // AES_KEY2_3
                DEF_IO32(0x100090B0, core->aes.writeKeyx(2, 0, IO_PARAMS)) // AES_KEYX2_0
                DEF_IO32(0x100090B4, core->aes.writeKeyx(2, 1, IO_PARAMS)) // AES_KEYX2_1
                DEF_IO32(0x100090B8, core->aes.writeKeyx(2, 2, IO_PARAMS)) // AES_KEYX2_2
                DEF_IO32(0x100090BC, core->aes.writeKeyx(2, 3, IO_PARAMS)) // AES_KEYX2_3
                DEF_IO32(0x100090C0, core->aes.writeKeyy(2, 0, IO_PARAMS)) // AES_KEYY2_0
                DEF_IO32(0x100090C4, core->aes.writeKeyy(2, 1, IO_PARAMS)) // AES_KEYY2_1
                DEF_IO32(0x100090C8, core->aes.writeKeyy(2, 2, IO_PARAMS)) // AES_KEYY2_2
                DEF_IO32(0x100090CC, core->aes.writeKeyy(2, 3, IO_PARAMS)) // AES_KEYY2_3
                DEF_IO32(0x100090D0, core->aes.writeKey(3, 0, IO_PARAMS)) // AES_KEY3_0
                DEF_IO32(0x100090D4, core->aes.writeKey(3, 1, IO_PARAMS)) // AES_KEY3_1
                DEF_IO32(0x100090D8, core->aes.writeKey(3, 2, IO_PARAMS)) // AES_KEY3_2
                DEF_IO32(0x100090DC, core->aes.writeKey(3, 3, IO_PARAMS)) // AES_KEY3_3
                DEF_IO32(0x100090E0, core->aes.writeKeyx(3, 0, IO_PARAMS)) // AES_KEYX3_0
                DEF_IO32(0x100090E4, core->aes.writeKeyx(3, 1, IO_PARAMS)) // AES_KEYX3_1
                DEF_IO32(0x100090E8, core->aes.writeKeyx(3, 2, IO_PARAMS)) // AES_KEYX3_2
                DEF_IO32(0x100090EC, core->aes.writeKeyx(3, 3, IO_PARAMS)) // AES_KEYX3_3
                DEF_IO32(0x100090F0, core->aes.writeKeyy(3, 0, IO_PARAMS)) // AES_KEYY3_0
                DEF_IO32(0x100090F4, core->aes.writeKeyy(3, 1, IO_PARAMS)) // AES_KEYY3_1
                DEF_IO32(0x100090F8, core->aes.writeKeyy(3, 2, IO_PARAMS)) // AES_KEYY3_2
                DEF_IO32(0x100090FC, core->aes.writeKeyy(3, 3, IO_PARAMS)) // AES_KEYY3_3
                DEF_IO32(0x10009100, core->aes.writeKeyfifo(IO_PARAMS)) // AES_KEYFIFO
                DEF_IO32(0x10009104, core->aes.writeKeyxfifo(IO_PARAMS)) // AES_KEYXFIFO
                DEF_IO32(0x10009108, core->aes.writeKeyyfifo(IO_PARAMS)) // AES_KEYYFIFO
                DEF_IO32(0x1000A000, core->shas[1].writeCnt(IO_PARAMS)) // SHA_CNT9
                DEF_IO32(0x1000A004, core->shas[1].writeBlkcnt(IO_PARAMS)) // SHA_BLKCNT9
                DEF_IO32(0x1000A040, core->shas[1].writeHash(0, IO_PARAMS)) // SHA_HASH0_9
                DEF_IO32(0x1000A044, core->shas[1].writeHash(1, IO_PARAMS)) // SHA_HASH1_9
                DEF_IO32(0x1000A048, core->shas[1].writeHash(2, IO_PARAMS)) // SHA_HASH2_9
                DEF_IO32(0x1000A04C, core->shas[1].writeHash(3, IO_PARAMS)) // SHA_HASH3_9
                DEF_IO32(0x1000A050, core->shas[1].writeHash(4, IO_PARAMS)) // SHA_HASH4_9
                DEF_IO32(0x1000A054, core->shas[1].writeHash(5, IO_PARAMS)) // SHA_HASH5_9
                DEF_IO32(0x1000A058, core->shas[1].writeHash(6, IO_PARAMS)) // SHA_HASH6_9
                DEF_IO32(0x1000A05C, core->shas[1].writeHash(7, IO_PARAMS)) // SHA_HASH7_9
                DEF_IO32(0x1000A080, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A084, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A088, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A08C, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A090, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A094, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A098, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A09C, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A0, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A4, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A8, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0AC, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B0, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B4, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B8, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0BC, core->shas[1].writeFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000B000, core->rsa.writeCnt(IO_PARAMS)) // RSA_CNT
                DEF_IO32(0x1000B100, core->rsa.writeSlotcnt(0, IO_PARAMS)) // RSA_SLOTCNT0
                DEF_IO32(0x1000B110, core->rsa.writeSlotcnt(1, IO_PARAMS)) // RSA_SLOTCNT1
                DEF_IO32(0x1000B120, core->rsa.writeSlotcnt(2, IO_PARAMS)) // RSA_SLOTCNT2
                DEF_IO32(0x1000B130, core->rsa.writeSlotcnt(3, IO_PARAMS)) // RSA_SLOTCNT3
                DEF_IO32(0x1000B200, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B204, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B208, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B20C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B210, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B214, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B218, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B21C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B220, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B224, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B228, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B22C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B230, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B234, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B238, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B23C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B240, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B244, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B248, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B24C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B250, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B254, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B258, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B25C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B260, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B264, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B268, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B26C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B270, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B274, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B278, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B27C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B280, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B284, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B288, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B28C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B290, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B294, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B298, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B29C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2AC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2BC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2CC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2DC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2EC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2FC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B400, core->rsa.writeMod(0, IO_PARAMS)) // RSA_MOD0
                DEF_IO32(0x1000B404, core->rsa.writeMod(1, IO_PARAMS)) // RSA_MOD1
                DEF_IO32(0x1000B408, core->rsa.writeMod(2, IO_PARAMS)) // RSA_MOD2
                DEF_IO32(0x1000B40C, core->rsa.writeMod(3, IO_PARAMS)) // RSA_MOD3
                DEF_IO32(0x1000B410, core->rsa.writeMod(4, IO_PARAMS)) // RSA_MOD4
                DEF_IO32(0x1000B414, core->rsa.writeMod(5, IO_PARAMS)) // RSA_MOD5
                DEF_IO32(0x1000B418, core->rsa.writeMod(6, IO_PARAMS)) // RSA_MOD6
                DEF_IO32(0x1000B41C, core->rsa.writeMod(7, IO_PARAMS)) // RSA_MOD7
                DEF_IO32(0x1000B420, core->rsa.writeMod(8, IO_PARAMS)) // RSA_MOD8
                DEF_IO32(0x1000B424, core->rsa.writeMod(9, IO_PARAMS)) // RSA_MOD9
                DEF_IO32(0x1000B428, core->rsa.writeMod(10, IO_PARAMS)) // RSA_MOD10
                DEF_IO32(0x1000B42C, core->rsa.writeMod(11, IO_PARAMS)) // RSA_MOD11
                DEF_IO32(0x1000B430, core->rsa.writeMod(12, IO_PARAMS)) // RSA_MOD12
                DEF_IO32(0x1000B434, core->rsa.writeMod(13, IO_PARAMS)) // RSA_MOD13
                DEF_IO32(0x1000B438, core->rsa.writeMod(14, IO_PARAMS)) // RSA_MOD14
                DEF_IO32(0x1000B43C, core->rsa.writeMod(15, IO_PARAMS)) // RSA_MOD15
                DEF_IO32(0x1000B440, core->rsa.writeMod(16, IO_PARAMS)) // RSA_MOD16
                DEF_IO32(0x1000B444, core->rsa.writeMod(17, IO_PARAMS)) // RSA_MOD17
                DEF_IO32(0x1000B448, core->rsa.writeMod(18, IO_PARAMS)) // RSA_MOD18
                DEF_IO32(0x1000B44C, core->rsa.writeMod(19, IO_PARAMS)) // RSA_MOD19
                DEF_IO32(0x1000B450, core->rsa.writeMod(20, IO_PARAMS)) // RSA_MOD20
                DEF_IO32(0x1000B454, core->rsa.writeMod(21, IO_PARAMS)) // RSA_MOD21
                DEF_IO32(0x1000B458, core->rsa.writeMod(22, IO_PARAMS)) // RSA_MOD22
                DEF_IO32(0x1000B45C, core->rsa.writeMod(23, IO_PARAMS)) // RSA_MOD23
                DEF_IO32(0x1000B460, core->rsa.writeMod(24, IO_PARAMS)) // RSA_MOD24
                DEF_IO32(0x1000B464, core->rsa.writeMod(25, IO_PARAMS)) // RSA_MOD25
                DEF_IO32(0x1000B468, core->rsa.writeMod(26, IO_PARAMS)) // RSA_MOD26
                DEF_IO32(0x1000B46C, core->rsa.writeMod(27, IO_PARAMS)) // RSA_MOD27
                DEF_IO32(0x1000B470, core->rsa.writeMod(28, IO_PARAMS)) // RSA_MOD28
                DEF_IO32(0x1000B474, core->rsa.writeMod(29, IO_PARAMS)) // RSA_MOD29
                DEF_IO32(0x1000B478, core->rsa.writeMod(30, IO_PARAMS)) // RSA_MOD30
                DEF_IO32(0x1000B47C, core->rsa.writeMod(31, IO_PARAMS)) // RSA_MOD31
                DEF_IO32(0x1000B480, core->rsa.writeMod(32, IO_PARAMS)) // RSA_MOD32
                DEF_IO32(0x1000B484, core->rsa.writeMod(33, IO_PARAMS)) // RSA_MOD33
                DEF_IO32(0x1000B488, core->rsa.writeMod(34, IO_PARAMS)) // RSA_MOD34
                DEF_IO32(0x1000B48C, core->rsa.writeMod(35, IO_PARAMS)) // RSA_MOD35
                DEF_IO32(0x1000B490, core->rsa.writeMod(36, IO_PARAMS)) // RSA_MOD36
                DEF_IO32(0x1000B494, core->rsa.writeMod(37, IO_PARAMS)) // RSA_MOD37
                DEF_IO32(0x1000B498, core->rsa.writeMod(38, IO_PARAMS)) // RSA_MOD38
                DEF_IO32(0x1000B49C, core->rsa.writeMod(39, IO_PARAMS)) // RSA_MOD39
                DEF_IO32(0x1000B4A0, core->rsa.writeMod(40, IO_PARAMS)) // RSA_MOD40
                DEF_IO32(0x1000B4A4, core->rsa.writeMod(41, IO_PARAMS)) // RSA_MOD41
                DEF_IO32(0x1000B4A8, core->rsa.writeMod(42, IO_PARAMS)) // RSA_MOD42
                DEF_IO32(0x1000B4AC, core->rsa.writeMod(43, IO_PARAMS)) // RSA_MOD43
                DEF_IO32(0x1000B4B0, core->rsa.writeMod(44, IO_PARAMS)) // RSA_MOD44
                DEF_IO32(0x1000B4B4, core->rsa.writeMod(45, IO_PARAMS)) // RSA_MOD45
                DEF_IO32(0x1000B4B8, core->rsa.writeMod(46, IO_PARAMS)) // RSA_MOD46
                DEF_IO32(0x1000B4BC, core->rsa.writeMod(47, IO_PARAMS)) // RSA_MOD47
                DEF_IO32(0x1000B4C0, core->rsa.writeMod(48, IO_PARAMS)) // RSA_MOD48
                DEF_IO32(0x1000B4C4, core->rsa.writeMod(49, IO_PARAMS)) // RSA_MOD49
                DEF_IO32(0x1000B4C8, core->rsa.writeMod(50, IO_PARAMS)) // RSA_MOD50
                DEF_IO32(0x1000B4CC, core->rsa.writeMod(51, IO_PARAMS)) // RSA_MOD51
                DEF_IO32(0x1000B4D0, core->rsa.writeMod(52, IO_PARAMS)) // RSA_MOD52
                DEF_IO32(0x1000B4D4, core->rsa.writeMod(53, IO_PARAMS)) // RSA_MOD53
                DEF_IO32(0x1000B4D8, core->rsa.writeMod(54, IO_PARAMS)) // RSA_MOD54
                DEF_IO32(0x1000B4DC, core->rsa.writeMod(55, IO_PARAMS)) // RSA_MOD55
                DEF_IO32(0x1000B4E0, core->rsa.writeMod(56, IO_PARAMS)) // RSA_MOD56
                DEF_IO32(0x1000B4E4, core->rsa.writeMod(57, IO_PARAMS)) // RSA_MOD57
                DEF_IO32(0x1000B4E8, core->rsa.writeMod(58, IO_PARAMS)) // RSA_MOD58
                DEF_IO32(0x1000B4EC, core->rsa.writeMod(59, IO_PARAMS)) // RSA_MOD59
                DEF_IO32(0x1000B4F0, core->rsa.writeMod(60, IO_PARAMS)) // RSA_MOD60
                DEF_IO32(0x1000B4F4, core->rsa.writeMod(61, IO_PARAMS)) // RSA_MOD61
                DEF_IO32(0x1000B4F8, core->rsa.writeMod(62, IO_PARAMS)) // RSA_MOD62
                DEF_IO32(0x1000B4FC, core->rsa.writeMod(63, IO_PARAMS)) // RSA_MOD63
                DEF_IO32(0x1000B800, core->rsa.writeData(0, IO_PARAMS)) // RSA_DATA0
                DEF_IO32(0x1000B804, core->rsa.writeData(1, IO_PARAMS)) // RSA_DATA1
                DEF_IO32(0x1000B808, core->rsa.writeData(2, IO_PARAMS)) // RSA_DATA2
                DEF_IO32(0x1000B80C, core->rsa.writeData(3, IO_PARAMS)) // RSA_DATA3
                DEF_IO32(0x1000B810, core->rsa.writeData(4, IO_PARAMS)) // RSA_DATA4
                DEF_IO32(0x1000B814, core->rsa.writeData(5, IO_PARAMS)) // RSA_DATA5
                DEF_IO32(0x1000B818, core->rsa.writeData(6, IO_PARAMS)) // RSA_DATA6
                DEF_IO32(0x1000B81C, core->rsa.writeData(7, IO_PARAMS)) // RSA_DATA7
                DEF_IO32(0x1000B820, core->rsa.writeData(8, IO_PARAMS)) // RSA_DATA8
                DEF_IO32(0x1000B824, core->rsa.writeData(9, IO_PARAMS)) // RSA_DATA9
                DEF_IO32(0x1000B828, core->rsa.writeData(10, IO_PARAMS)) // RSA_DATA10
                DEF_IO32(0x1000B82C, core->rsa.writeData(11, IO_PARAMS)) // RSA_DATA11
                DEF_IO32(0x1000B830, core->rsa.writeData(12, IO_PARAMS)) // RSA_DATA12
                DEF_IO32(0x1000B834, core->rsa.writeData(13, IO_PARAMS)) // RSA_DATA13
                DEF_IO32(0x1000B838, core->rsa.writeData(14, IO_PARAMS)) // RSA_DATA14
                DEF_IO32(0x1000B83C, core->rsa.writeData(15, IO_PARAMS)) // RSA_DATA15
                DEF_IO32(0x1000B840, core->rsa.writeData(16, IO_PARAMS)) // RSA_DATA16
                DEF_IO32(0x1000B844, core->rsa.writeData(17, IO_PARAMS)) // RSA_DATA17
                DEF_IO32(0x1000B848, core->rsa.writeData(18, IO_PARAMS)) // RSA_DATA18
                DEF_IO32(0x1000B84C, core->rsa.writeData(19, IO_PARAMS)) // RSA_DATA19
                DEF_IO32(0x1000B850, core->rsa.writeData(20, IO_PARAMS)) // RSA_DATA20
                DEF_IO32(0x1000B854, core->rsa.writeData(21, IO_PARAMS)) // RSA_DATA21
                DEF_IO32(0x1000B858, core->rsa.writeData(22, IO_PARAMS)) // RSA_DATA22
                DEF_IO32(0x1000B85C, core->rsa.writeData(23, IO_PARAMS)) // RSA_DATA23
                DEF_IO32(0x1000B860, core->rsa.writeData(24, IO_PARAMS)) // RSA_DATA24
                DEF_IO32(0x1000B864, core->rsa.writeData(25, IO_PARAMS)) // RSA_DATA25
                DEF_IO32(0x1000B868, core->rsa.writeData(26, IO_PARAMS)) // RSA_DATA26
                DEF_IO32(0x1000B86C, core->rsa.writeData(27, IO_PARAMS)) // RSA_DATA27
                DEF_IO32(0x1000B870, core->rsa.writeData(28, IO_PARAMS)) // RSA_DATA28
                DEF_IO32(0x1000B874, core->rsa.writeData(29, IO_PARAMS)) // RSA_DATA29
                DEF_IO32(0x1000B878, core->rsa.writeData(30, IO_PARAMS)) // RSA_DATA30
                DEF_IO32(0x1000B87C, core->rsa.writeData(31, IO_PARAMS)) // RSA_DATA31
                DEF_IO32(0x1000B880, core->rsa.writeData(32, IO_PARAMS)) // RSA_DATA32
                DEF_IO32(0x1000B884, core->rsa.writeData(33, IO_PARAMS)) // RSA_DATA33
                DEF_IO32(0x1000B888, core->rsa.writeData(34, IO_PARAMS)) // RSA_DATA34
                DEF_IO32(0x1000B88C, core->rsa.writeData(35, IO_PARAMS)) // RSA_DATA35
                DEF_IO32(0x1000B890, core->rsa.writeData(36, IO_PARAMS)) // RSA_DATA36
                DEF_IO32(0x1000B894, core->rsa.writeData(37, IO_PARAMS)) // RSA_DATA37
                DEF_IO32(0x1000B898, core->rsa.writeData(38, IO_PARAMS)) // RSA_DATA38
                DEF_IO32(0x1000B89C, core->rsa.writeData(39, IO_PARAMS)) // RSA_DATA39
                DEF_IO32(0x1000B8A0, core->rsa.writeData(40, IO_PARAMS)) // RSA_DATA40
                DEF_IO32(0x1000B8A4, core->rsa.writeData(41, IO_PARAMS)) // RSA_DATA41
                DEF_IO32(0x1000B8A8, core->rsa.writeData(42, IO_PARAMS)) // RSA_DATA42
                DEF_IO32(0x1000B8AC, core->rsa.writeData(43, IO_PARAMS)) // RSA_DATA43
                DEF_IO32(0x1000B8B0, core->rsa.writeData(44, IO_PARAMS)) // RSA_DATA44
                DEF_IO32(0x1000B8B4, core->rsa.writeData(45, IO_PARAMS)) // RSA_DATA45
                DEF_IO32(0x1000B8B8, core->rsa.writeData(46, IO_PARAMS)) // RSA_DATA46
                DEF_IO32(0x1000B8BC, core->rsa.writeData(47, IO_PARAMS)) // RSA_DATA47
                DEF_IO32(0x1000B8C0, core->rsa.writeData(48, IO_PARAMS)) // RSA_DATA48
                DEF_IO32(0x1000B8C4, core->rsa.writeData(49, IO_PARAMS)) // RSA_DATA49
                DEF_IO32(0x1000B8C8, core->rsa.writeData(50, IO_PARAMS)) // RSA_DATA50
                DEF_IO32(0x1000B8CC, core->rsa.writeData(51, IO_PARAMS)) // RSA_DATA51
                DEF_IO32(0x1000B8D0, core->rsa.writeData(52, IO_PARAMS)) // RSA_DATA52
                DEF_IO32(0x1000B8D4, core->rsa.writeData(53, IO_PARAMS)) // RSA_DATA53
                DEF_IO32(0x1000B8D8, core->rsa.writeData(54, IO_PARAMS)) // RSA_DATA54
                DEF_IO32(0x1000B8DC, core->rsa.writeData(55, IO_PARAMS)) // RSA_DATA55
                DEF_IO32(0x1000B8E0, core->rsa.writeData(56, IO_PARAMS)) // RSA_DATA56
                DEF_IO32(0x1000B8E4, core->rsa.writeData(57, IO_PARAMS)) // RSA_DATA57
                DEF_IO32(0x1000B8E8, core->rsa.writeData(58, IO_PARAMS)) // RSA_DATA58
                DEF_IO32(0x1000B8EC, core->rsa.writeData(59, IO_PARAMS)) // RSA_DATA59
                DEF_IO32(0x1000B8F0, core->rsa.writeData(60, IO_PARAMS)) // RSA_DATA60
                DEF_IO32(0x1000B8F4, core->rsa.writeData(61, IO_PARAMS)) // RSA_DATA61
                DEF_IO32(0x1000B8F8, core->rsa.writeData(62, IO_PARAMS)) // RSA_DATA62
                DEF_IO32(0x1000B8FC, core->rsa.writeData(63, IO_PARAMS)) // RSA_DATA63
                DEF_IO32(0x1000C020, core->cdmas[XDMA].writeInten(IO_PARAMS)) // XDMA_INTEN
                DEF_IO32(0x1000C02C, core->cdmas[XDMA].writeIntclr(IO_PARAMS)) // XDMA_INTCLR
                DEF_IO32(0x1000CD04, core->cdmas[XDMA].writeDbgcmd(IO_PARAMS)) // XDMA_DBGCMD
                DEF_IO32(0x1000CD08, core->cdmas[XDMA].writeDbginst0(IO_PARAMS)) // XDMA_DBGINST0
                DEF_IO32(0x1000CD0C, core->cdmas[XDMA].writeDbginst1(IO_PARAMS)) // XDMA_DBGINST1
                DEF_IO32(0x1000D800, core->cartridge.writeSpiFifoCnt(IO_PARAMS)) // SPICARD_FIFO_CNT
                DEF_IO32(0x1000D804, core->cartridge.writeSpiFifoSelect(IO_PARAMS)) // SPICARD_FIFO_SELECT
                DEF_IO32(0x1000D808, core->cartridge.writeSpiFifoBlklen(IO_PARAMS)) // SPICARD_FIFO_BLKLEN
                DEF_IO32(0x1000D80C, core->cartridge.writeSpiFifoData(IO_PARAMS)) // SPICARD_FIFO_DATA
                DEF_IO32(0x1000D818, core->cartridge.writeSpiFifoIntMask(IO_PARAMS)) // SPICARD_FIFO_INT_MASK
                DEF_IO32(0x1000D81C, core->cartridge.writeSpiFifoIntStat(IO_PARAMS)) // SPICARD_FIFO_INT_STAT
            }
        }

        // Catch writes to unknown I/O registers
        if (id == ARM9)
            LOG_WARN("Unknown ARM9 I/O write: 0x%X\n", base);
        else
            LOG_WARN("Unknown ARM11 core %d I/O write: 0x%X\n", id, base);
        return;

    next:
        // Loop until the full value has been written
        i += size - base;
    }
}

void Memory::writeCfg11BrOverlayCnt(uint32_t mask, uint32_t value){
    // Write to the CFG11_BR_OVERLAY_CNT register
    if (!core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x1;
    cfg11BrOverlayCnt = (cfg11BrOverlayCnt & ~mask) | (value & mask);

    // Update the ARM11 memory map in affected regions
    updateMap(false, 0x0, 0xFFF);
    updateMap(false, 0x10000, 0x10FFF);
    updateMap(false, 0xFFFF0000, 0xFFFF0FFF);
}

void Memory::writeCfg11BrOverlayVal(uint32_t mask, uint32_t value){
    // Write to the CFG11_BR_OVERLAY_VAL register
    if (!core->n3dsMode) return; // N3DS-exclusive
    cfg11BrOverlayVal = (cfg11BrOverlayVal & ~mask) | (value & mask);
}

void Memory::writeCfg11MpCnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_MP_CNT register
    if (!core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x101;
    cfg11MpCnt = (cfg11MpCnt & ~mask) | (value & mask);

    // Update the ARM11 memory map in affected regions
    updateMap(false, 0x1F000000, 0x1F3FFFFF);
}

void Memory::writeCfg9Sysprot9(uint8_t value) {
    // Set bits in the CFG9_SYSPROT9 register and disable sensitive data
    cfg9Sysprot9 |= (value & 0x3);
    if (value & BIT(0)) memset(&boot9[0x8000], 0, 0x8000);
    if (value & BIT(1)) memset(otpEncrypted, 0, sizeof(otpEncrypted));
}

void Memory::writeCfg9Sysprot11(uint8_t value) {
    // Set bits in the CFG9_SYSPROT11 register and disable sensitive data
    cfg9Sysprot11 |= (value & 0x1);
    if (value & BIT(0)) memset(&boot11[0x8000], 0, 0x8000);
}

void Memory::writeCfg9Extmemcnt9(uint32_t mask, uint32_t value) {
    // Write to the CFG9_EXTMEMCNT9 register
    if (!core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x1;
    cfg9Extmemcnt9 = (cfg9Extmemcnt9 & ~mask) | (value & mask);

    // Update the ARM9 memory map in affected regions
    updateMap(true, 0x8100000, 0x817FFFF);
}
