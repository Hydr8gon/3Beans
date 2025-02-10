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

template uint8_t Memory::readFallback(CpuId id, uint32_t address);
template uint16_t Memory::readFallback(CpuId id, uint32_t address);
template uint32_t Memory::readFallback(CpuId id, uint32_t address);
template <typename T> T Memory::readFallback(CpuId id, uint32_t address) {
    // Forward a read to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioRead<T>(id, address);

    // Handle the ARM11 bootrom overlay if reads to it have fallen through
    if (id != ARM9 && (address < 0x20000 || address >= 0xFFFF0000))
        return (address == *core->arms[id].registers[15]) ? 0xE59FF018 : cfg11BrOverlayVal;

    // Catch reads from unmapped memory
    if (id == ARM9)
        LOG_CRIT("Unmapped ARM9 memory read: 0x%08X\n", address);
    else
        LOG_CRIT("Unmapped ARM11 core %d memory read: 0x%08X\n", id, address);
    return 0;
}

template void Memory::writeFallback(CpuId id, uint32_t address, uint8_t value);
template void Memory::writeFallback(CpuId id, uint32_t address, uint16_t value);
template void Memory::writeFallback(CpuId id, uint32_t address, uint32_t value);
template <typename T> void Memory::writeFallback(CpuId id, uint32_t address, T value) {
    // Forward a write to I/O registers if within range
    if (address >= 0x10000000 && address < 0x18000000)
        return ioWrite<T>(id, address, value);

    // Catch writes to unmapped memory
    if (id == ARM9)
        LOG_CRIT("Unmapped ARM9 memory write: 0x%08X\n", address);
    else
        LOG_CRIT("Unmapped ARM11 core %d memory write: 0x%08X\n", id, address);
}

template <typename T> T Memory::ioRead(CpuId id, uint32_t address) {
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
        }

        // Check registers that are exclusive to one CPU
        if (id != ARM9) { // ARM11
            switch (base) {
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
                DEF_IO32(0x10400010, data = core->gpu.readMemfillDstAddr(0)) // GPU_MEMFILL_DST_ADDR0
                DEF_IO32(0x10400014, data = core->gpu.readMemfillDstEnd(0)) // GPU_MEMFILL_DST_END0
                DEF_IO32(0x10400018, data = core->gpu.readMemfillData(0)) // GPU_MEMFILL_DATA0
                DEF_IO32(0x1040001C, data = core->gpu.readMemfillCnt(0)) // GPU_MEMFILL_CNT0
                DEF_IO32(0x10400020, data = core->gpu.readMemfillDstAddr(1)) // GPU_MEMFILL_DST_ADDR1
                DEF_IO32(0x10400024, data = core->gpu.readMemfillDstEnd(1)) // GPU_MEMFILL_DST_END1
                DEF_IO32(0x10400028, data = core->gpu.readMemfillData(1)) // GPU_MEMFILL_DATA1
                DEF_IO32(0x1040002C, data = core->gpu.readMemfillCnt(1)) // GPU_MEMFILL_CNT1
                DEF_IO32(0x10400468, data = core->pdc.readFramebufLt0(0)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400470, data = core->pdc.readFramebufFormat(0)) // PDC0_FRAMEBUF_FORMAT
                DEF_IO32(0x10400474, data = core->pdc.readInterruptType(0)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400568, data = core->pdc.readFramebufLt0(1)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x10400570, data = core->pdc.readFramebufFormat(1)) // PDC1_FRAMEBUF_FORMAT
                DEF_IO32(0x10400574, data = core->pdc.readInterruptType(1)) // PDC1_INTERRUPT_TYPE
                DEF_IO32(0x17E00004, data = core->interrupts.readMpScuConfig()) // MP_SCU_CONFIG
                DEF_IO32(0x17E00100, data = core->interrupts.readMpIle(id)) // MP_ILE
                DEF_IO32(0x17E0010C, data = core->interrupts.readMpAck(id)) // MP_ACK
                DEF_IO32(0x17E00118, data = core->interrupts.readMpPending(id)) // MP_PENDING
                DEF_IO32(0x17E01000, data = core->interrupts.readMpIge()) // MP_IGE
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
            }
        }
        else { // ARM9
            switch (base) {
                DEF_IO08(0x10000000, data = readCfg9Sysprot9()) // CFG9_SYSPROT9
                DEF_IO08(0x10000001, data = readCfg9Sysprot11()) // CFG9_SYSPROT11
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
                DEF_IO16(0x10006000, data = core->sdMmc.readCmd()) // SD_CMD
                DEF_IO16(0x10006002, data = core->sdMmc.readPortSelect()) // SD_PORT_SELECT
                DEF_IO32(0x10006004, data = core->sdMmc.readCmdParam()) // SD_CMD_PARAM
                DEF_IO16(0x1000600A, data = core->sdMmc.readData16Blkcnt()) // SD_DATA16_BLKCNT
                DEF_IO32(0x1000600C, data = core->sdMmc.readResponse(0)) // SD_RESPONSE0
                DEF_IO32(0x10006010, data = core->sdMmc.readResponse(1)) // SD_RESPONSE1
                DEF_IO32(0x10006014, data = core->sdMmc.readResponse(2)) // SD_RESPONSE2
                DEF_IO32(0x10006018, data = core->sdMmc.readResponse(3)) // SD_RESPONSE3
                DEF_IO32(0x1000601C, data = core->sdMmc.readIrqStatus()) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, data = core->sdMmc.readIrqMask()) // SD_IRQ_MASK
                DEF_IO16(0x10006026, data = core->sdMmc.readData16Blklen()) // SD_DATA16_BLKLEN
                DEF_IO16(0x10006030, data = core->sdMmc.readData16Fifo()) // SD_DATA16_FIFO
                DEF_IO16(0x100060D8, data = core->sdMmc.readDataCtl()) // SD_DATA_CTL
                DEF_IO16(0x10006100, data = core->sdMmc.readData32Irq()) // SD_DATA32_IRQ
                DEF_IO16(0x10006104, data = core->sdMmc.readData32Blklen()) // SD_DATA32_BLKLEN
                DEF_IO32(0x1000610C, data = core->sdMmc.readData32Fifo()) // SD_DATA32_FIFO
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
            LOG_WARN("Unknown ARM9 I/O read: 0x%08X\n", base);
        else
            LOG_WARN("Unknown ARM11 core %d memory read: 0x%08X\n", id, base);
        return value;

    next:
        // Process the data and loop until a full value is formed
        value |= (data >> (base << 3)) << (i << 3);
        i += size - base;
    }
    return value;
}

template <typename T> void Memory::ioWrite(CpuId id, uint32_t address, T value) {
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
        }

        // Check registers that are exclusive to one CPU
        if (id != ARM9) { // ARM11
            switch (base) {
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
                DEF_IO32(0x10400010, core->gpu.writeMemfillDstAddr(0, IO_PARAMS)) // GPU_MEMFILL_DST_ADDR0
                DEF_IO32(0x10400014, core->gpu.writeMemfillDstEnd(0, IO_PARAMS)) // GPU_MEMFILL_DST_END0
                DEF_IO32(0x10400018, core->gpu.writeMemfillData(0, IO_PARAMS)) // GPU_MEMFILL_DATA0
                DEF_IO32(0x1040001C, core->gpu.writeMemfillCnt(0, IO_PARAMS)) // GPU_MEMFILL_CNT0
                DEF_IO32(0x10400020, core->gpu.writeMemfillDstAddr(1, IO_PARAMS)) // GPU_MEMFILL_DST_ADDR1
                DEF_IO32(0x10400024, core->gpu.writeMemfillDstEnd(1, IO_PARAMS)) // GPU_MEMFILL_DST_END1
                DEF_IO32(0x10400028, core->gpu.writeMemfillData(1, IO_PARAMS)) // GPU_MEMFILL_DATA1
                DEF_IO32(0x1040002C, core->gpu.writeMemfillCnt(1, IO_PARAMS)) // GPU_MEMFILL_CNT1
                DEF_IO32(0x10400468, core->pdc.writeFramebufLt0(0, IO_PARAMS)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400470, core->pdc.writeFramebufFormat(0, IO_PARAMS)) // PDC0_FRAMEBUF_FORMAT
                DEF_IO32(0x10400474, core->pdc.writeInterruptType(0, IO_PARAMS)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400568, core->pdc.writeFramebufLt0(1, IO_PARAMS)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x10400570, core->pdc.writeFramebufFormat(1, IO_PARAMS)) // PDC1_FRAMEBUF_FORMAT
                DEF_IO32(0x10400574, core->pdc.writeInterruptType(1, IO_PARAMS)) // PDC1_INTERRUPT_TYPE
                DEF_IO32(0x17E00100, core->interrupts.writeMpIle(id, IO_PARAMS)) // MP_ILE
                DEF_IO32(0x17E00110, core->interrupts.writeMpEoi(id, IO_PARAMS)) // MP_EOI
                DEF_IO32(0x17E01000, core->interrupts.writeMpIge(IO_PARAMS)) // MP_IGE
                DEF_IO32(0x17E01100, core->interrupts.writeMpIeSet(0, IO_PARAMS)) // MP_IE0_SET
                DEF_IO32(0x17E01104, core->interrupts.writeMpIeSet(1, IO_PARAMS)) // MP_IE1_SET
                DEF_IO32(0x17E01108, core->interrupts.writeMpIeSet(2, IO_PARAMS)) // MP_IE2_SET
                DEF_IO32(0x17E0110C, core->interrupts.writeMpIeSet(3, IO_PARAMS)) // MP_IE3_SET
                DEF_IO32(0x17E01180, core->interrupts.writeMpIeClear(0, IO_PARAMS)) // MP_IE0_CLEAR
                DEF_IO32(0x17E01184, core->interrupts.writeMpIeClear(1, IO_PARAMS)) // MP_IE1_CLEAR
                DEF_IO32(0x17E01188, core->interrupts.writeMpIeClear(2, IO_PARAMS)) // MP_IE2_CLEAR
                DEF_IO32(0x17E0118C, core->interrupts.writeMpIeClear(3, IO_PARAMS)) // MP_IE3_CLEAR
                DEF_IO32(0x17E01F00, core->interrupts.writeMpSoftIrq(id, IO_PARAMS)) // MP_SOFT_IRQ
            }
        }
        else { // ARM9
            switch (base) {
                DEF_IO08(0x10000000, writeCfg9Sysprot9(IO_PARAMS8)) // CFG9_SYSPROT9
                DEF_IO08(0x10000001, writeCfg9Sysprot11(IO_PARAMS8)) // CFG9_SYSPROT11
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
                DEF_IO16(0x10006000, core->sdMmc.writeCmd(IO_PARAMS)) // SD_CMD
                DEF_IO16(0x10006002, core->sdMmc.writePortSelect(IO_PARAMS)) // SD_PORT_SELECT
                DEF_IO32(0x10006004, core->sdMmc.writeCmdParam(IO_PARAMS)) // SD_CMD_PARAM
                DEF_IO16(0x1000600A, core->sdMmc.writeData16Blkcnt(IO_PARAMS)) // SD_DATA16_BLKCNT
                DEF_IO32(0x1000601C, core->sdMmc.writeIrqStatus(IO_PARAMS)) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmc.writeIrqMask(IO_PARAMS)) // SD_IRQ_MASK
                DEF_IO16(0x10006026, core->sdMmc.writeData16Blklen(IO_PARAMS)) // SD_DATA16_BLKLEN
                DEF_IO16(0x100060D8, core->sdMmc.writeDataCtl(IO_PARAMS)) // SD_DATA_CTL
                DEF_IO16(0x10006100, core->sdMmc.writeData32Irq(IO_PARAMS)) // SD_DATA32_IRQ
                DEF_IO16(0x10006104, core->sdMmc.writeData32Blklen(IO_PARAMS)) // SD_DATA32_BLKLEN
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
            }
        }

        // Catch writes to unknown I/O registers
        if (id == ARM9)
            LOG_WARN("Unknown ARM9 I/O write: 0x%08X\n", base);
        else
            LOG_WARN("Unknown ARM11 core %d I/O write: 0x%08X\n", id, base);
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
