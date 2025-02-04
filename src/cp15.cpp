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

#include "core.h"

uint32_t Cp15::mmuTranslate(CpuId id, uint32_t address) {
    // Translate a virtual address to physical using MMU translation tables
    // TODO: handle all the extra bits
    uint32_t entry = core->memory.read<uint32_t>(id, (tlbBase0Regs[id] & 0xFFFFC000) + ((address >> 18) & 0x3FFC));
    switch (entry & 0x3) {
    case 0x1: // Coarse
        entry = core->memory.read<uint32_t>(id, (entry & 0xFFFFFC00) + ((address >> 10) & 0x3FC));
        switch (entry & 0x3) {
        case 0x1: // 64KB large page
            return (entry & 0xFFFF0000) | (address & 0xFFFF);
        case 0x2: case 0x3: // 4KB small page
            return (entry & 0xFFFFF000) | (address & 0xFFF);
        }
        break;

    case 0x2: // Section
        if (entry & BIT(18)) // 16MB supersection
            return (entry & 0xFF000000) | (address & 0xFFFFFF);
        else // 1MB section
            return (entry & 0xFFF00000) | (address & 0xFFFFF);
    }

    // Catch unhandled translation table entries
    LOG_CRIT("Unhandled ARM11 core %d MMU translation fault at 0x%08X\n", id, address);
    return address;
}

template uint8_t Cp15::read(CpuId id, uint32_t address);
template uint16_t Cp15::read(CpuId id, uint32_t address);
template uint32_t Cp15::read(CpuId id, uint32_t address);
template <typename T> T Cp15::read(CpuId id, uint32_t address) {
    // Handle special cases that only apply to CPU memory reads
    if (id == ARM9) {
        // Get a pointer to readable TCM memory on the ARM9 if it exists
        uint8_t *data = nullptr;
        if (address < itcmSize && itcmRead)
            data = &itcm[address & 0x7FFF];
        else if (address >= dtcmAddr && address < dtcmAddr + dtcmSize && dtcmRead)
            data = &dtcm[(address - dtcmAddr) & 0x3FFF];

        // Form an LSB-first value from the data at the pointer
        if (data) {
            T value = 0;
            for (uint32_t i = 0; i < sizeof(T); i++)
                value |= data[i] << (i << 3);
            return value;
        }
    }
    else if (mmuEnables[id]) {
        // Translate the address using an ARM11 MMU if enabled
        address = mmuTranslate(id, address);
    }

    // Fall back to a regular memory read
    return core->memory.read<T>(id, address);
}

template void Cp15::write(CpuId id, uint32_t address, uint8_t value);
template void Cp15::write(CpuId id, uint32_t address, uint16_t value);
template void Cp15::write(CpuId id, uint32_t address, uint32_t value);
template <typename T> void Cp15::write(CpuId id, uint32_t address, T value) {
    // Handle special cases that only apply to CPU memory writes
    if (id == ARM9) {
        // Get a pointer to writable TCM memory on the ARM9 if it exists
        uint8_t *data = nullptr;
        if (address < itcmSize && itcmWrite)
            data = &itcm[address & 0x7FFF];
        else if (address >= dtcmAddr && address < dtcmAddr + dtcmSize && dtcmWrite)
            data = &dtcm[(address - dtcmAddr) & 0x3FFF];

        // Write an LSB-first value to the data at the pointer
        if (data) {
            for (uint32_t i = 0; i < sizeof(T); i++)
                data[i] = value >> (i << 3);
            return;
        }
    }
    else if (mmuEnables[id]) {
        // Translate the address using an ARM11 MMU if enabled
        address = mmuTranslate(id, address);
    }

    // Fall back to a regular memory write
    return core->memory.write<T>(id, address, value);
}

uint32_t Cp15::readReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Read a value from a CP15 register
    if (id != ARM9) { // ARM11
        switch ((cn << 16) | (cm << 8) | (cp << 0)) {
            case 0x000005: return id; // CPU ID
            case 0x010000: return ctrlRegs[id]; // Control
            case 0x020000: return tlbBase0Regs[id]; // TLB base 0
        }
    }
    else { // ARM9
        switch ((cn << 16) | (cm << 8) | (cp << 0)) {
            case 0x000000: return 0x41059461; // Main ID
            case 0x000001: return 0x0F0D2112; // Cache type
            case 0x010000: return ctrlRegs[id]; // Control
            case 0x090100: return dtcmReg; // DTCM base/size
            case 0x090101: return itcmReg; // ITCM size
        }
    }

    // Catch reads from unknown CP15 registers
    LOG_WARN("Unknown ARM%d CP15 register read: C%d,C%d,%d\n", (id == ARM9) ? 9 : 11, cn, cm, cp);
    return 0;
}

void Cp15::writeReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value) {
    // Write a value to a CP15 register
    if (id != ARM9) { // ARM11
        switch ((cn << 16) | (cm << 8) | (cp << 0)) {
            case 0x010000: return writeCtrl11(id, value); // Control
            case 0x020000: return writeTlbBase0(id, value); // TLB base 0
            case 0x070004: return writeWfi(id, value); // Wait for interrupt
            case 0x070802: return writeWfi(id, value); // Wait for interrupt
        }
    }
    else { // ARM9
        switch ((cn << 16) | (cm << 8) | (cp << 0)) {
            case 0x010000: return writeCtrl9(id, value); // Control
            case 0x070004: return writeWfi(id, value); // Wait for interrupt
            case 0x070802: return writeWfi(id, value); // Wait for interrupt
            case 0x090100: return writeDtcm(id, value); // DTCM base/size
            case 0x090101: return writeItcm(id, value); // ITCM size
        }
    }

    // Catch writes to unknown CP15 registers
    LOG_WARN("Unknown ARM%d CP15 register write: C%d,C%d,%d\n", (id == ARM9) ? 9 : 11, cn, cm, cp);
}

void Cp15::writeCtrl11(CpuId id, uint32_t value) {
    // Set writable control bits on the ARM11
    ctrlRegs[id] = (ctrlRegs[id] & ~0x32C0BB07) | (value & 0x32C0BB07);
    mmuEnables[id] = (ctrlRegs[id] & BIT(0));
    exceptAddrs[id] = (ctrlRegs[id] & BIT(13)) ? 0xFFFF0000 : 0x00000000;
}

void Cp15::writeCtrl9(CpuId id, uint32_t value) {
    // Set writable control bits on the ARM9
    ctrlRegs[id] = (ctrlRegs[id] & ~0xFF085) | (value & 0xFF085);
    exceptAddrs[id] = (ctrlRegs[id] & BIT(13)) ? 0xFFFF0000 : 0x00000000;
    dtcmRead = (ctrlRegs[id] & BIT(16)) && !(ctrlRegs[id] & BIT(17));
    dtcmWrite = (ctrlRegs[id] & BIT(16));
    itcmRead = (ctrlRegs[id] & BIT(18)) && !(ctrlRegs[id] & BIT(19));
    itcmWrite = (ctrlRegs[id] & BIT(18));
}

void Cp15::writeTlbBase0(CpuId id, uint32_t value) {
    // Set the translation table base 0 register
    tlbBase0Regs[id] = value;
    LOG_INFO("Changing ARM11 core %d translation table base 0 to 0x%08X\n", id, tlbBase0Regs[id] & 0xFFFFC000);
}

void Cp15::writeWfi(CpuId id, uint32_t value) {
    // Halt the CPU
    core->arms[id].halted = true;
}

void Cp15::writeDtcm(CpuId id, uint32_t value) {
    // Set the DTCM address and size with a minimum of 4KB
    dtcmReg = value;
    dtcmAddr = dtcmReg & 0xFFFFF000;
    dtcmSize = std::max(0x1000, 0x200 << ((dtcmReg >> 1) & 0x1F));
    LOG_INFO("Remapping ARM9 DTCM to 0x%08X with size 0x%X\n", dtcmAddr, dtcmSize);
}

void Cp15::writeItcm(CpuId id, uint32_t value) {
    // Set the ITCM size with a minimum of 4KB
    itcmReg = value;
    itcmSize = std::max(0x1000, 0x200 << ((itcmReg >> 1) & 0x1F));
    LOG_INFO("Remapping ARM9 ITCM with size 0x%X\n", itcmSize);
}
