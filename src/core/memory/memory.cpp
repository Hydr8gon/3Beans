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
#include "../core.h"

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

    // Use OTP data to seed the PRNG registers
    for (int i = 0; i < 0x10; i++) {
        prngSource[0] ^= otpEncrypted[0x00 + i];
        prngSource[1] ^= otpEncrypted[0x10 + i];
        prngSource[2] ^= otpEncrypted[0x20 + i];
    }
}

void Memory::updateMap(bool arm9, uint32_t start, uint32_t end) {
    // Update the ARM9 or ARM11 physical memory maps with 4KB pages
    bool extend = (core->interrupts.readCfg11MpClkcnt() & 0x70000);
    for (uint64_t address = start; address <= end; address += 0x1000) {
        // Look up and reset pointers for the current address
        uint8_t *&read = (arm9 ? readMap9 : readMap11)[address >> 12];
        uint8_t *&write = (arm9 ? writeMap9 : writeMap11)[address >> 12];
        read = write = nullptr;

        // Map DSP WRAM based on the code and data 32KB block registers
        if (address >= 0x1FF00000 && address < 0x1FF80000) { // 512KB area
            uint8_t slot = (address >> 15) & 0x7;
            if (address < 0x1FF40000) { // 256KB code
                for (int i = 0; i < 8; i++) {
                    if (!(cfg11Wram32kCode[i] & BIT(7)) || ((cfg11Wram32kCode[i] >> 2) & 0x7) != slot) continue;
                    read = write = &dspWram[(i << 15) | (address & 0x7FFF)];
                    break;
                }
            }
            else { // 256KB data
                for (int i = 0; i < 8; i++) {
                    if (!(cfg11Wram32kData[i] & BIT(7)) || ((cfg11Wram32kData[i] >> 2) & 0x7) != slot) continue;
                    read = write = &dspWram[(i << 15) | (address & 0x47FFF)];
                    break;
                }
            }
            continue;
        }

        // Set a pointer to readable memory if it exists at the current address
        if (arm9 && address >= 0x8000000 && address < (cfg9Extmemcnt9 ? 0x8180000 : 0x8100000))
            read = &arm9Ram[address & 0x1FFFFF]; // 1.5MB ARM9 internal RAM
        else if (address >= 0x18000000 && address < 0x18600000)
            read = &vram[address & 0x7FFFFF]; // 6MB VRAM
        else if (!arm9 && (cfg11MpCnt & BIT(0)) && address >= 0x1F000000 && address < 0x1F400000)
            read = &vramExt[address & 0x3FFFFF]; // 4MB extended VRAM
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
        else if (address >= 0x1FF80000 && address < 0x20000000)
            write = &axiWram[address & 0x7FFFF]; // 512KB AXI WRAM
        else if (address >= 0x20000000 && address < 0x28000000)
            write = &fcram[address & 0x7FFFFFF]; // 128MB FCRAM
        else if (extend && address >= 0x28000000 && address < 0x30000000)
            write = &fcramExt[address & 0x7FFFFFF]; // 128MB extended FCRAM

        // Use a hack to work around boot9strap 1.4's uninitialized stack
        // TODO: fix this properly (it works on hardware)
        else if (!arm9 && address >= 0xFFFFE000)
            write = &boot11[address & 0xFFFF];
    }

    // Update the virtual memory maps as well
    if (arm9) return core->cp15.updateMap9(start, end);
    for (int i = 0; i < MAX_CPUS - 1; i++)
        core->cp15.mmuInvalidate(CpuId(i));
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

uint32_t Memory::readPrngSource(int i) {
    // Use a made-up pseudo-random formula to adjust PRNG registers in a reproducible way
    // TODO: figure out what formula they actually use
    if (i < 2) prngSource[i] = (((prngSource[i] >> 2) + 0x800000) ^ ((prngSource[i] << 3) - (i + 1))) * 3;
    return prngSource[i];
}

void Memory::writeCfg11Wram32kCode(int i, uint8_t value) {
    // Write to one of the CFG11_WRAM_32K_CODE registers
    // TODO: handle the ARM-only bit
    cfg11Wram32kCode[i] = (value & 0x9D);
    if (value & BIT(7))
        LOG_INFO("Remapping DSP WRAM code block %d to slot %d\n", i, (value >> 2) & 0x7);
    else
        LOG_INFO("Unmapping DSP WRAM code block %d\n", i);

    // Update the ARM11 and ARM9 memory maps in DSP code regions
    updateMap(false, 0x1FF00000, 0x1FF3FFFF);
    updateMap(true, 0x1FF00000, 0x1FF3FFFF);
}

void Memory::writeCfg11Wram32kData(int i, uint8_t value) {
    // Write to one of the CFG11_WRAM_32K_DATA registers
    // TODO: handle the ARM-only bit
    cfg11Wram32kData[i] = (value & 0x9D);
    if (value & BIT(7))
        LOG_INFO("Remapping DSP WRAM data block %d to slot %d\n", i, (value >> 2) & 0x7);
    else
        LOG_INFO("Unmapping DSP WRAM data block %d\n", i);

    // Update the ARM11 and ARM9 memory maps in DSP data regions
    updateMap(false, 0x1FF40000, 0x1FF7FFFF);
    updateMap(true, 0x1FF40000, 0x1FF7FFFF);
}

void Memory::writeCfg11BrOverlayCnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_BR_OVERLAY_CNT register
    if (!core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x1;
    cfg11BrOverlayCnt = (cfg11BrOverlayCnt & ~mask) | (value & mask);

    // Update the ARM11 memory map in affected regions
    updateMap(false, 0x0, 0xFFF);
    updateMap(false, 0x10000, 0x10FFF);
    updateMap(false, 0xFFFF0000, 0xFFFF0FFF);
}

void Memory::writeCfg11BrOverlayVal(uint32_t mask, uint32_t value) {
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
