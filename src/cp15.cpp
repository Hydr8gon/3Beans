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

template uint8_t Cp15::read(CpuId, uint32_t);
template uint16_t Cp15::read(CpuId, uint32_t);
template uint32_t Cp15::read(CpuId, uint32_t);
template void Cp15::write(CpuId, uint32_t, uint8_t);
template void Cp15::write(CpuId, uint32_t, uint16_t);
template void Cp15::write(CpuId, uint32_t, uint32_t);

uint8_t *Cp15::getReadPtr(CpuId id, uint32_t address) {
    // Get a readable memory pointer to use for caching
    if (id == ARM9) return readMap9[address >> 12];
    if (!mmuEnables[id]) return core->memory.readMap11[address >> 12];
    MmuMap &map = mmuMaps[id][address >> 12];
    if (map.tag != mmuTags[id]) updateEntry(id, address);
    return map.read;
}

uint32_t Cp15::mmuTranslate(CpuId id, uint32_t address) {
    // Check control value X to determine the table base address
    uint32_t base;
    if (tlbCtrlRegs[id]) {
        // Use base 1 if any upper X bits are set, or use base 0 with X extra bits
        uint32_t mask = ((1 << tlbCtrlRegs[id]) - 1) << (32 - tlbCtrlRegs[id]);
        base = (address & mask) ? (tlbBase1Regs[id] & 0xFFFFC000) : (tlbBase0Regs[id] & (0xFFFFC000 | (mask >> 18)));
    }
    else {
        // Always use base 0 if X is zero
        base = (tlbBase0Regs[id] & 0xFFFFC000);
    }

    // Translate a virtual address to physical using MMU translation tables
    // TODO: handle all the extra bits
    uint32_t entry = core->memory.read<uint32_t>(id, base + ((address >> 18) & 0x3FFC));
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
    LOG_CRIT("Unhandled ARM11 core %d MMU translation fault at 0x%X\n", id, address);
    return address;
}

void Cp15::mmuInvalidate(CpuId id) {
    // Increment the MMU tag to invalidate maps and reset on overflow to avoid false positives
    core->arms[id].invalidatePc();
    if (++mmuTags[id]) return;
    memset(mmuMaps[id], 0, sizeof(mmuMaps[id]));
    mmuTags[id] = 1;
}

void Cp15::updateEntry(CpuId id, uint32_t address) {
    // Cache an MMU read/write mapping with the current tag
    MmuMap &map = mmuMaps[id][address >> 12];
    address = mmuTranslate(id, address);
    map.read = core->memory.readMap11[address >> 12];
    map.write = core->memory.writeMap11[address >> 12];
    map.addr = (address & ~0xFFF);
    map.tag = mmuTags[id];
}

void Cp15::updateMap9(uint32_t start, uint32_t end) {
    // Use the ARM9 physical read/write maps as a base
    uint32_t size = ((uint64_t(end) - start + 0xFFF) >> 12) * sizeof(uint8_t*);
    memcpy(&readMap9[start >> 12], &core->memory.readMap9[start >> 12], size);
    memcpy(&writeMap9[start >> 12], &core->memory.writeMap9[start >> 12], size);
    core->arms[ARM9].invalidatePc();

    // Overlay TCM mappings if enabled for read/write
    for (uint64_t address = start; address <= end; address += 0x1000) {
        if (address < itcmSize) {
            if (itcmRead) readMap9[address >> 12] = &itcm[address & 0x7FFF];
            if (itcmWrite) writeMap9[address >> 12] = &itcm[address & 0x7FFF];
        }
        else if (address >= dtcmAddr && address < dtcmAddr + dtcmSize) {
            if (dtcmRead) readMap9[address >> 12] = &dtcm[(address - dtcmAddr) & 0x3FFF];
            if (dtcmWrite) writeMap9[address >> 12] = &dtcm[(address - dtcmAddr) & 0x3FFF];
        }
    }
}

template <typename T> T Cp15::read(CpuId id, uint32_t address) {
    // Get a pointer to mapped readable memory if it exists
    uint8_t *data;
    if (id == ARM9) {
        // Align the address and read from ARM9 memory with TCM
        address &= ~(sizeof(T) - 1);
        data = readMap9[address >> 12];
    }
    else if (mmuEnables[id]) {
        // Read from ARM11 virtual memory, updating the cache if necessary
        MmuMap &map = mmuMaps[id][address >> 12];
        if (map.tag != mmuTags[id]) updateEntry(id, address);
        if (!(data = map.read)) address = map.addr | (address & 0xFFF);
    }
    else {
        // Read from ARM11 physical memory
        data = core->memory.readMap11[address >> 12];
    }

    // Fall back to read handlers for special cases
    if (!data)
        return core->memory.readFallback<T>(id, address);

    // Load an LSB-first value from a direct memory pointer
    T value = 0;
    data += (address & 0xFFF);
    for (uint32_t i = 0; i < sizeof(T); i++)
        value |= data[i] << (i << 3);
    return value;
}

template <typename T> void Cp15::write(CpuId id, uint32_t address, T value) {
    // Get a pointer to mapped writable memory if it exists
    uint8_t *data;
    if (id == ARM9) {
        // Align the address and write to ARM9 memory with TCM
        address &= ~(sizeof(T) - 1);
        data = writeMap9[address >> 12];
    }
    else if (mmuEnables[id]) {
        // Write to ARM11 virtual memory, updating the cache if necessary
        MmuMap &map = mmuMaps[id][address >> 12];
        if (map.tag != mmuTags[id]) updateEntry(id, address);
        if (!(data = map.write)) address = map.addr | (address & 0xFFF);

        #if LOG_LEVEL > 3
        // Catch writes to special memory used by the 3DS OS
        if (address == 0xFFFF9004 && value) {
            // Log process names when the active 3DS kernel process struct changes
            uint32_t kCodeSet = read<uint32_t>(id, value + 0xB0 + core->n3dsMode * 8);
            uint8_t procName[9] = {};
            for (int i = 0; i < 8; i++)
                procName[i] = read<uint8_t>(id, kCodeSet + 0x50 + i);
            LOG_OS("ARM11 core %d kernel switching to process '%s'\n", id, procName);
        }
        else if (address >= threadIdRegs[id][1] + 0x80 && address < threadIdRegs[id][1] + 0x180) {
            // Log IPC commands when the TLS buffer is written to
            LOG_OS("ARM11 core %d writing IPC command: 0x%X @ 0x%X\n", id, value, address - threadIdRegs[id][1] - 0x80);
        }
        #endif
    }
    else {
        // Write to ARM11 physical memory
        data = core->memory.writeMap11[address >> 12];
    }

    // Fall back to write handlers for special cases
    if (!data)
        return core->memory.writeFallback<T>(id, address, value);

    // Write an LSB-first value to a direct memory pointer
    data += (address & 0xFFF);
    for (uint32_t i = 0; i < sizeof(T); i++)
        data[i] = value >> (i << 3);
}

uint32_t Cp15::readReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Read a value from a CP15 register
    if (id != ARM9) { // ARM11
        switch ((cn << 16) | (cm << 8) | cp) {
            case 0x000005: return id; // CPU ID
            case 0x010000: return ctrlRegs[id]; // Control
            case 0x020000: return tlbBase0Regs[id]; // TLB base 0
            case 0x020001: return tlbBase1Regs[id]; // TLB base 1
            case 0x020002: return tlbCtrlRegs[id]; // TLB control
            case 0x070400: return physAddrRegs[id]; // Physical address
            case 0x0D0002: return threadIdRegs[id][0]; // Thread ID 0
            case 0x0D0003: return threadIdRegs[id][1]; // Thread ID 1
            case 0x0D0004: return threadIdRegs[id][2]; // Thread ID 2
        }
    }
    else { // ARM9
        switch ((cn << 16) | (cm << 8) | cp) {
            case 0x000000: return 0x41059461; // Main ID
            case 0x000001: return 0x0F0D2112; // Cache type
            case 0x010000: return ctrlRegs[id]; // Control
            case 0x090100: return dtcmReg; // DTCM base/size
            case 0x090101: return itcmReg; // ITCM size
        }
    }

    // Catch reads from unknown CP15 registers
    if (id == ARM9)
        LOG_WARN("Unknown ARM9 CP15 register read: C%d,C%d,%d\n", cn, cm, cp);
    else
        LOG_WARN("Unknown ARM11 core %d CP15 register read: C%d,C%d,%d\n", id, cn, cm, cp);
    return 0;
}

void Cp15::writeReg(CpuId id, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value) {
    // Write a value to a CP15 register
    if (id != ARM9) { // ARM11
        switch ((cn << 16) | (cm << 8) | cp) {
            case 0x010000: return writeCtrl11(id, value); // Control
            case 0x020000: return writeTlbBase0(id, value); // TLB base 0
            case 0x020001: return writeTlbBase1(id, value); // TLB base 1
            case 0x020002: return writeTlbCtrl(id, value); // TLB control
            case 0x070004: return writeWfi(id, value); // Wait for interrupt
            case 0x070501: return; // Invalidate i-cache line (stub)
            case 0x070601: return; // Invalidate d-cache line (stub)
            case 0x070800: return writeAddrTrans(id, value); // Privileged read address translation
            case 0x070801: return writeAddrTrans(id, value); // Privileged write address translation
            case 0x070802: return writeAddrTrans(id, value); // User read address translation
            case 0x070803: return writeAddrTrans(id, value); // User write address translation
            case 0x070A01: return; // Clean d-cache line (stub)
            case 0x070A04: return; // Data sync barrier (stub)
            case 0x070A05: return; // Data memory barrier (stub)
            case 0x070E01: return; // Clean+invalidate d-cache line (stub)
            case 0x080500: return mmuInvalidate(id); // Invalidate i-TLB (stub)
            case 0x080501: return mmuInvalidate(id); // Invalidate i-TLB by MVA+ASID (stub)
            case 0x080502: return mmuInvalidate(id); // Invalidate i-TLB by ASID (stub)
            case 0x080503: return mmuInvalidate(id); // Invalidate i-TLB by MVA (stub)
            case 0x080600: return mmuInvalidate(id); // Invalidate d-TLB (stub)
            case 0x080601: return mmuInvalidate(id); // Invalidate d-TLB by MVA+ASID (stub)
            case 0x080602: return mmuInvalidate(id); // Invalidate d-TLB by ASID (stub)
            case 0x080603: return mmuInvalidate(id); // Invalidate d-TLB by MVA (stub)
            case 0x080700: return mmuInvalidate(id); // Invalidate TLB (stub)
            case 0x080701: return mmuInvalidate(id); // Invalidate TLB by MVA+ASID (stub)
            case 0x080702: return mmuInvalidate(id); // Invalidate TLB by ASID (stub)
            case 0x080703: return mmuInvalidate(id); // Invalidate TLB by MVA (stub)
            case 0x0D0002: return writeThreadId(id, 0, value); // Thread ID 0
            case 0x0D0003: return writeThreadId(id, 1, value); // Thread ID 1
            case 0x0D0004: return writeThreadId(id, 2, value); // Thread ID 2
        }
    }
    else { // ARM9
        switch ((cn << 16) | (cm << 8) | cp) {
            case 0x010000: return writeCtrl9(id, value); // Control
            case 0x070004: return writeWfi(id, value); // Wait for interrupt
            case 0x070501: return; // Invalidate i-cache line (stub)
            case 0x070601: return; // Invalidate d-cache line (stub)
            case 0x070802: return writeWfi(id, value); // Wait for interrupt
            case 0x070A01: return; // Clean d-cache line (stub)
            case 0x070A04: return; // Drain write buffer (stub)
            case 0x070E01: return; // Clean+invalidate d-cache line (stub)
            case 0x090100: return writeDtcm(id, value); // DTCM base/size
            case 0x090101: return writeItcm(id, value); // ITCM size
        }
    }

    // Catch writes to unknown CP15 registers
    if (id == ARM9)
        LOG_WARN("Unknown ARM9 CP15 register write: C%d,C%d,%d\n", cn, cm, cp);
    else
        LOG_WARN("Unknown ARM11 core %d CP15 register write: C%d,C%d,%d\n", id, cn, cm, cp);
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

    // Update the memory map at the current TCM locations
    updateMap9(dtcmAddr, dtcmAddr + dtcmSize);
    updateMap9(0, itcmSize);
}

void Cp15::writeTlbBase0(CpuId id, uint32_t value) {
    // Set a core's translation table base 0 register
    tlbBase0Regs[id] = value;
    LOG_INFO("Changing ARM11 core %d translation table base 0 to 0x%X\n", id, tlbBase0Regs[id] & 0xFFFFFF80);
    mmuInvalidate(id);
}

void Cp15::writeTlbBase1(CpuId id, uint32_t value) {
    // Set a core's translation table base 1 register
    tlbBase1Regs[id] = value;
    LOG_INFO("Changing ARM11 core %d translation table base 1 to 0x%X\n", id, tlbBase1Regs[id] & 0xFFFFC000);
    mmuInvalidate(id);
}

void Cp15::writeTlbCtrl(CpuId id, uint32_t value) {
    // Set a core's translation table control register
    tlbCtrlRegs[id] = value & 0x7;
    LOG_INFO("Changing ARM11 core %d translation table control to %d\n", id, tlbCtrlRegs[id]);
    mmuInvalidate(id);
}

void Cp15::writeAddrTrans(CpuId id, uint32_t value) {
    // Translate a virtual address and store it in the physical address register
    // TODO: handle read/write and privileged/user variants
    physAddrRegs[id] = mmuTranslate(id, value);
}

void Cp15::writeThreadId(CpuId id, int i, uint32_t value) {
    // Set one of a core's thread ID registers
    // TODO: enforce access permissions
    threadIdRegs[id][i] = value;
}

void Cp15::writeWfi(CpuId id, uint32_t value) {
    // Halt the CPU
    core->interrupts.halt(id);
}

void Cp15::writeDtcm(CpuId id, uint32_t value) {
    // Set the DTCM address and size with a minimum of 4KB
    dtcmReg = value;
    uint32_t oldAddr = dtcmAddr, oldSize = dtcmSize;
    dtcmAddr = dtcmReg & 0xFFFFF000;
    dtcmSize = std::max(0x1000, 0x200 << ((dtcmReg >> 1) & 0x1F));
    LOG_INFO("Remapping ARM9 DTCM to 0x%X with size 0x%X\n", dtcmAddr, dtcmSize);

    // Update the memory map at the old and new DTCM areas
    updateMap9(oldAddr, oldAddr + oldSize);
    updateMap9(dtcmAddr, dtcmAddr + dtcmSize);
}

void Cp15::writeItcm(CpuId id, uint32_t value) {
    // Set the ITCM size with a minimum of 4KB
    itcmReg = value;
    uint32_t oldSize = itcmSize;
    itcmSize = std::max(0x1000, 0x200 << ((itcmReg >> 1) & 0x1F));
    LOG_INFO("Remapping ARM9 ITCM with size 0x%X\n", itcmSize);

    // Update the memory map at the old and new ITCM areas
    updateMap9(0, std::max(oldSize, itcmSize));
}
