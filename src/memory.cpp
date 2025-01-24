/*
    Copyright 2023-2024 Hydr8gon

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

bool Memory::loadFiles() {
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
template <typename T> T Memory::read(CpuId id, uint32_t address) {
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
    if (id == ARM9) {
        Cp15 &cp15 = core->cpus[ARM9].cp15;
        if (address < cp15.itcmSize && cp15.itcmRead)
            data = &itcm[address & 0x7FFF];
        else if (address >= cp15.dtcmAddr && address < cp15.dtcmAddr + cp15.dtcmSize && cp15.dtcmRead)
            data = &dtcm[(address - cp15.dtcmAddr) & 0x3FFF];
    }

    // Form an LSB-first value from the data at the pointer
    if (data) {
        T value = 0;
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

template void Memory::write(CpuId id, uint32_t address, uint8_t  value);
template void Memory::write(CpuId id, uint32_t address, uint16_t value);
template void Memory::write(CpuId id, uint32_t address, uint32_t value);
template <typename T> void Memory::write(CpuId id, uint32_t address, T value) {
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
    if (id == ARM9) {
        Cp15 &cp15 = core->cpus[ARM9].cp15;
        if (address < cp15.itcmSize && cp15.itcmWrite)
            data = &itcm[address & 0x7FFF];
        else if (address >= cp15.dtcmAddr && address < cp15.dtcmAddr + cp15.dtcmSize && cp15.dtcmWrite)
            data = &dtcm[(address - cp15.dtcmAddr) & 0x3FFF];
    }

    // Write an LSB-first value to the data at the pointer
    if (data) {
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

template <typename T> T Memory::ioRead(CpuId id, uint32_t address) {
    // Read a value from one or more I/O registers
    T value = 0;
    for (uint32_t i = 0; i < sizeof(T);) {
        uint32_t base = address + i, size, data;
        if (id != ARM9) {
            // Check registers that are exclusive to the ARM11
            switch (base) {
                DEF_IO32(0x10101000, data = core->shas[0].readShaCnt()) // SHA_CNT11
                DEF_IO32(0x10101004, data = core->shas[0].readShaBlkcnt()) // SHA_BLKCNT11
                DEF_IO32(0x10101040, data = core->shas[0].readShaHash(0)) // SHA_HASH0_11
                DEF_IO32(0x10101044, data = core->shas[0].readShaHash(1)) // SHA_HASH1_11
                DEF_IO32(0x10101048, data = core->shas[0].readShaHash(2)) // SHA_HASH2_11
                DEF_IO32(0x1010104C, data = core->shas[0].readShaHash(3)) // SHA_HASH3_11
                DEF_IO32(0x10101050, data = core->shas[0].readShaHash(4)) // SHA_HASH4_11
                DEF_IO32(0x10101054, data = core->shas[0].readShaHash(5)) // SHA_HASH5_11
                DEF_IO32(0x10101058, data = core->shas[0].readShaHash(6)) // SHA_HASH6_11
                DEF_IO32(0x1010105C, data = core->shas[0].readShaHash(7)) // SHA_HASH7_11
                DEF_IO32(0x10163000, data = core->pxi.readPxiSync(false)) // PXI_SYNC11
                DEF_IO32(0x10163004, data = core->pxi.readPxiCnt(false)) // PXI_CNT11
                DEF_IO32(0x1016300C, data = core->pxi.readPxiRecv(false)) // PXI_RECV11
                DEF_IO32(0x10301000, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301004, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301008, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x1030100C, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301010, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301014, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301018, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x1030101C, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301020, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301024, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301028, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x1030102C, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301030, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301034, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10301038, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x1030103C, data = core->shas[0].readShaFifo()) // SHA_FIFO11
                DEF_IO32(0x10400468, data = core->gpu.readPdcFramebufLt0(false)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400474, data = core->gpu.readPdcInterruptType(false)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400568, data = core->gpu.readPdcFramebufLt0(true)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x10400574, data = core->gpu.readPdcInterruptType(true)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x17E00100, data = core->interrupts.readMpIle(id)) // MP_ILE
                DEF_IO32(0x17E0010C, data = core->interrupts.readMpAck(id)) // MP_ACK
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
        else {
            // Check registers that are exclusive to the ARM9
            switch (base) {
                DEF_IO16(0x10000000, data = 0x101) // CFG9_SYSPROT (stub)
                DEF_IO32(0x10001000, data = core->interrupts.readIrqIe()) // IRQ_IE
                DEF_IO32(0x10001004, data = core->interrupts.readIrqIf()) // IRQ_IF
                DEF_IO16(0x10003000, data = core->timers.readTmCntL(0)) // TM0CNT_L
                DEF_IO16(0x10003002, data = core->timers.readTmCntH(0)) // TM0CNT_H
                DEF_IO16(0x10003004, data = core->timers.readTmCntL(1)) // TM1CNT_L
                DEF_IO16(0x10003006, data = core->timers.readTmCntH(1)) // TM1CNT_H
                DEF_IO16(0x10003008, data = core->timers.readTmCntL(2)) // TM2CNT_L
                DEF_IO16(0x1000300A, data = core->timers.readTmCntH(2)) // TM2CNT_H
                DEF_IO16(0x1000300C, data = core->timers.readTmCntL(3)) // TM3CNT_L
                DEF_IO16(0x1000300E, data = core->timers.readTmCntH(3)) // TM3CNT_H
                DEF_IO32(0x1000B000, data = core->rsa.readRsaCnt()) // RSA_CNT
                DEF_IO32(0x1000B100, data = core->rsa.readRsaSlotcnt(0)) // RSA_SLOTCNT0
                DEF_IO32(0x1000B104, data = core->rsa.readRsaSlotsize(0)) // RSA_SLOTSIZE0
                DEF_IO32(0x1000B110, data = core->rsa.readRsaSlotcnt(1)) // RSA_SLOTCNT1
                DEF_IO32(0x1000B114, data = core->rsa.readRsaSlotsize(1)) // RSA_SLOTSIZE1
                DEF_IO32(0x1000B120, data = core->rsa.readRsaSlotcnt(2)) // RSA_SLOTCNT2
                DEF_IO32(0x1000B124, data = core->rsa.readRsaSlotsize(2)) // RSA_SLOTSIZE2
                DEF_IO32(0x1000B130, data = core->rsa.readRsaSlotcnt(3)) // RSA_SLOTCNT3
                DEF_IO32(0x1000B134, data = core->rsa.readRsaSlotsize(3)) // RSA_SLOTSIZE3
                DEF_IO32(0x1000B400, data = core->rsa.readRsaMod(0)) // RSA_MOD0
                DEF_IO32(0x1000B404, data = core->rsa.readRsaMod(1)) // RSA_MOD1
                DEF_IO32(0x1000B408, data = core->rsa.readRsaMod(2)) // RSA_MOD2
                DEF_IO32(0x1000B40C, data = core->rsa.readRsaMod(3)) // RSA_MOD3
                DEF_IO32(0x1000B410, data = core->rsa.readRsaMod(4)) // RSA_MOD4
                DEF_IO32(0x1000B414, data = core->rsa.readRsaMod(5)) // RSA_MOD5
                DEF_IO32(0x1000B418, data = core->rsa.readRsaMod(6)) // RSA_MOD6
                DEF_IO32(0x1000B41C, data = core->rsa.readRsaMod(7)) // RSA_MOD7
                DEF_IO32(0x1000B420, data = core->rsa.readRsaMod(8)) // RSA_MOD8
                DEF_IO32(0x1000B424, data = core->rsa.readRsaMod(9)) // RSA_MOD9
                DEF_IO32(0x1000B428, data = core->rsa.readRsaMod(10)) // RSA_MOD10
                DEF_IO32(0x1000B42C, data = core->rsa.readRsaMod(11)) // RSA_MOD11
                DEF_IO32(0x1000B430, data = core->rsa.readRsaMod(12)) // RSA_MOD12
                DEF_IO32(0x1000B434, data = core->rsa.readRsaMod(13)) // RSA_MOD13
                DEF_IO32(0x1000B438, data = core->rsa.readRsaMod(14)) // RSA_MOD14
                DEF_IO32(0x1000B43C, data = core->rsa.readRsaMod(15)) // RSA_MOD15
                DEF_IO32(0x1000B440, data = core->rsa.readRsaMod(16)) // RSA_MOD16
                DEF_IO32(0x1000B444, data = core->rsa.readRsaMod(17)) // RSA_MOD17
                DEF_IO32(0x1000B448, data = core->rsa.readRsaMod(18)) // RSA_MOD18
                DEF_IO32(0x1000B44C, data = core->rsa.readRsaMod(19)) // RSA_MOD19
                DEF_IO32(0x1000B450, data = core->rsa.readRsaMod(20)) // RSA_MOD20
                DEF_IO32(0x1000B454, data = core->rsa.readRsaMod(21)) // RSA_MOD21
                DEF_IO32(0x1000B458, data = core->rsa.readRsaMod(22)) // RSA_MOD22
                DEF_IO32(0x1000B45C, data = core->rsa.readRsaMod(23)) // RSA_MOD23
                DEF_IO32(0x1000B460, data = core->rsa.readRsaMod(24)) // RSA_MOD24
                DEF_IO32(0x1000B464, data = core->rsa.readRsaMod(25)) // RSA_MOD25
                DEF_IO32(0x1000B468, data = core->rsa.readRsaMod(26)) // RSA_MOD26
                DEF_IO32(0x1000B46C, data = core->rsa.readRsaMod(27)) // RSA_MOD27
                DEF_IO32(0x1000B470, data = core->rsa.readRsaMod(28)) // RSA_MOD28
                DEF_IO32(0x1000B474, data = core->rsa.readRsaMod(29)) // RSA_MOD29
                DEF_IO32(0x1000B478, data = core->rsa.readRsaMod(30)) // RSA_MOD30
                DEF_IO32(0x1000B47C, data = core->rsa.readRsaMod(31)) // RSA_MOD31
                DEF_IO32(0x1000B480, data = core->rsa.readRsaMod(32)) // RSA_MOD32
                DEF_IO32(0x1000B484, data = core->rsa.readRsaMod(33)) // RSA_MOD33
                DEF_IO32(0x1000B488, data = core->rsa.readRsaMod(34)) // RSA_MOD34
                DEF_IO32(0x1000B48C, data = core->rsa.readRsaMod(35)) // RSA_MOD35
                DEF_IO32(0x1000B490, data = core->rsa.readRsaMod(36)) // RSA_MOD36
                DEF_IO32(0x1000B494, data = core->rsa.readRsaMod(37)) // RSA_MOD37
                DEF_IO32(0x1000B498, data = core->rsa.readRsaMod(38)) // RSA_MOD38
                DEF_IO32(0x1000B49C, data = core->rsa.readRsaMod(39)) // RSA_MOD39
                DEF_IO32(0x1000B4A0, data = core->rsa.readRsaMod(40)) // RSA_MOD40
                DEF_IO32(0x1000B4A4, data = core->rsa.readRsaMod(41)) // RSA_MOD41
                DEF_IO32(0x1000B4A8, data = core->rsa.readRsaMod(42)) // RSA_MOD42
                DEF_IO32(0x1000B4AC, data = core->rsa.readRsaMod(43)) // RSA_MOD43
                DEF_IO32(0x1000B4B0, data = core->rsa.readRsaMod(44)) // RSA_MOD44
                DEF_IO32(0x1000B4B4, data = core->rsa.readRsaMod(45)) // RSA_MOD45
                DEF_IO32(0x1000B4B8, data = core->rsa.readRsaMod(46)) // RSA_MOD46
                DEF_IO32(0x1000B4BC, data = core->rsa.readRsaMod(47)) // RSA_MOD47
                DEF_IO32(0x1000B4C0, data = core->rsa.readRsaMod(48)) // RSA_MOD48
                DEF_IO32(0x1000B4C4, data = core->rsa.readRsaMod(49)) // RSA_MOD49
                DEF_IO32(0x1000B4C8, data = core->rsa.readRsaMod(50)) // RSA_MOD50
                DEF_IO32(0x1000B4CC, data = core->rsa.readRsaMod(51)) // RSA_MOD51
                DEF_IO32(0x1000B4D0, data = core->rsa.readRsaMod(52)) // RSA_MOD52
                DEF_IO32(0x1000B4D4, data = core->rsa.readRsaMod(53)) // RSA_MOD53
                DEF_IO32(0x1000B4D8, data = core->rsa.readRsaMod(54)) // RSA_MOD54
                DEF_IO32(0x1000B4DC, data = core->rsa.readRsaMod(55)) // RSA_MOD55
                DEF_IO32(0x1000B4E0, data = core->rsa.readRsaMod(56)) // RSA_MOD56
                DEF_IO32(0x1000B4E4, data = core->rsa.readRsaMod(57)) // RSA_MOD57
                DEF_IO32(0x1000B4E8, data = core->rsa.readRsaMod(58)) // RSA_MOD58
                DEF_IO32(0x1000B4EC, data = core->rsa.readRsaMod(59)) // RSA_MOD59
                DEF_IO32(0x1000B4F0, data = core->rsa.readRsaMod(60)) // RSA_MOD60
                DEF_IO32(0x1000B4F4, data = core->rsa.readRsaMod(61)) // RSA_MOD61
                DEF_IO32(0x1000B4F8, data = core->rsa.readRsaMod(62)) // RSA_MOD62
                DEF_IO32(0x1000B4FC, data = core->rsa.readRsaMod(63)) // RSA_MOD63
                DEF_IO32(0x1000B800, data = core->rsa.readRsaData(0)) // RSA_DATA0
                DEF_IO32(0x1000B804, data = core->rsa.readRsaData(1)) // RSA_DATA1
                DEF_IO32(0x1000B808, data = core->rsa.readRsaData(2)) // RSA_DATA2
                DEF_IO32(0x1000B80C, data = core->rsa.readRsaData(3)) // RSA_DATA3
                DEF_IO32(0x1000B810, data = core->rsa.readRsaData(4)) // RSA_DATA4
                DEF_IO32(0x1000B814, data = core->rsa.readRsaData(5)) // RSA_DATA5
                DEF_IO32(0x1000B818, data = core->rsa.readRsaData(6)) // RSA_DATA6
                DEF_IO32(0x1000B81C, data = core->rsa.readRsaData(7)) // RSA_DATA7
                DEF_IO32(0x1000B820, data = core->rsa.readRsaData(8)) // RSA_DATA8
                DEF_IO32(0x1000B824, data = core->rsa.readRsaData(9)) // RSA_DATA9
                DEF_IO32(0x1000B828, data = core->rsa.readRsaData(10)) // RSA_DATA10
                DEF_IO32(0x1000B82C, data = core->rsa.readRsaData(11)) // RSA_DATA11
                DEF_IO32(0x1000B830, data = core->rsa.readRsaData(12)) // RSA_DATA12
                DEF_IO32(0x1000B834, data = core->rsa.readRsaData(13)) // RSA_DATA13
                DEF_IO32(0x1000B838, data = core->rsa.readRsaData(14)) // RSA_DATA14
                DEF_IO32(0x1000B83C, data = core->rsa.readRsaData(15)) // RSA_DATA15
                DEF_IO32(0x1000B840, data = core->rsa.readRsaData(16)) // RSA_DATA16
                DEF_IO32(0x1000B844, data = core->rsa.readRsaData(17)) // RSA_DATA17
                DEF_IO32(0x1000B848, data = core->rsa.readRsaData(18)) // RSA_DATA18
                DEF_IO32(0x1000B84C, data = core->rsa.readRsaData(19)) // RSA_DATA19
                DEF_IO32(0x1000B850, data = core->rsa.readRsaData(20)) // RSA_DATA20
                DEF_IO32(0x1000B854, data = core->rsa.readRsaData(21)) // RSA_DATA21
                DEF_IO32(0x1000B858, data = core->rsa.readRsaData(22)) // RSA_DATA22
                DEF_IO32(0x1000B85C, data = core->rsa.readRsaData(23)) // RSA_DATA23
                DEF_IO32(0x1000B860, data = core->rsa.readRsaData(24)) // RSA_DATA24
                DEF_IO32(0x1000B864, data = core->rsa.readRsaData(25)) // RSA_DATA25
                DEF_IO32(0x1000B868, data = core->rsa.readRsaData(26)) // RSA_DATA26
                DEF_IO32(0x1000B86C, data = core->rsa.readRsaData(27)) // RSA_DATA27
                DEF_IO32(0x1000B870, data = core->rsa.readRsaData(28)) // RSA_DATA28
                DEF_IO32(0x1000B874, data = core->rsa.readRsaData(29)) // RSA_DATA29
                DEF_IO32(0x1000B878, data = core->rsa.readRsaData(30)) // RSA_DATA30
                DEF_IO32(0x1000B87C, data = core->rsa.readRsaData(31)) // RSA_DATA31
                DEF_IO32(0x1000B880, data = core->rsa.readRsaData(32)) // RSA_DATA32
                DEF_IO32(0x1000B884, data = core->rsa.readRsaData(33)) // RSA_DATA33
                DEF_IO32(0x1000B888, data = core->rsa.readRsaData(34)) // RSA_DATA34
                DEF_IO32(0x1000B88C, data = core->rsa.readRsaData(35)) // RSA_DATA35
                DEF_IO32(0x1000B890, data = core->rsa.readRsaData(36)) // RSA_DATA36
                DEF_IO32(0x1000B894, data = core->rsa.readRsaData(37)) // RSA_DATA37
                DEF_IO32(0x1000B898, data = core->rsa.readRsaData(38)) // RSA_DATA38
                DEF_IO32(0x1000B89C, data = core->rsa.readRsaData(39)) // RSA_DATA39
                DEF_IO32(0x1000B8A0, data = core->rsa.readRsaData(40)) // RSA_DATA40
                DEF_IO32(0x1000B8A4, data = core->rsa.readRsaData(41)) // RSA_DATA41
                DEF_IO32(0x1000B8A8, data = core->rsa.readRsaData(42)) // RSA_DATA42
                DEF_IO32(0x1000B8AC, data = core->rsa.readRsaData(43)) // RSA_DATA43
                DEF_IO32(0x1000B8B0, data = core->rsa.readRsaData(44)) // RSA_DATA44
                DEF_IO32(0x1000B8B4, data = core->rsa.readRsaData(45)) // RSA_DATA45
                DEF_IO32(0x1000B8B8, data = core->rsa.readRsaData(46)) // RSA_DATA46
                DEF_IO32(0x1000B8BC, data = core->rsa.readRsaData(47)) // RSA_DATA47
                DEF_IO32(0x1000B8C0, data = core->rsa.readRsaData(48)) // RSA_DATA48
                DEF_IO32(0x1000B8C4, data = core->rsa.readRsaData(49)) // RSA_DATA49
                DEF_IO32(0x1000B8C8, data = core->rsa.readRsaData(50)) // RSA_DATA50
                DEF_IO32(0x1000B8CC, data = core->rsa.readRsaData(51)) // RSA_DATA51
                DEF_IO32(0x1000B8D0, data = core->rsa.readRsaData(52)) // RSA_DATA52
                DEF_IO32(0x1000B8D4, data = core->rsa.readRsaData(53)) // RSA_DATA53
                DEF_IO32(0x1000B8D8, data = core->rsa.readRsaData(54)) // RSA_DATA54
                DEF_IO32(0x1000B8DC, data = core->rsa.readRsaData(55)) // RSA_DATA55
                DEF_IO32(0x1000B8E0, data = core->rsa.readRsaData(56)) // RSA_DATA56
                DEF_IO32(0x1000B8E4, data = core->rsa.readRsaData(57)) // RSA_DATA57
                DEF_IO32(0x1000B8E8, data = core->rsa.readRsaData(58)) // RSA_DATA58
                DEF_IO32(0x1000B8EC, data = core->rsa.readRsaData(59)) // RSA_DATA59
                DEF_IO32(0x1000B8F0, data = core->rsa.readRsaData(60)) // RSA_DATA60
                DEF_IO32(0x1000B8F4, data = core->rsa.readRsaData(61)) // RSA_DATA61
                DEF_IO32(0x1000B8F8, data = core->rsa.readRsaData(62)) // RSA_DATA62
                DEF_IO32(0x1000B8FC, data = core->rsa.readRsaData(63)) // RSA_DATA63
                DEF_IO32(0x10012000, data = core->sdMmc.readOtpEncrypted(0)) // OTP_ENCRYPTED0
                DEF_IO32(0x10012004, data = core->sdMmc.readOtpEncrypted(1)) // OTP_ENCRYPTED1
                DEF_IO32(0x10012008, data = core->sdMmc.readOtpEncrypted(2)) // OTP_ENCRYPTED2
                DEF_IO32(0x1001200C, data = core->sdMmc.readOtpEncrypted(3)) // OTP_ENCRYPTED3
                DEF_IO32(0x10012010, data = core->sdMmc.readOtpEncrypted(4)) // OTP_ENCRYPTED4
                DEF_IO32(0x10012014, data = core->sdMmc.readOtpEncrypted(5)) // OTP_ENCRYPTED5
                DEF_IO32(0x10012018, data = core->sdMmc.readOtpEncrypted(6)) // OTP_ENCRYPTED6
                DEF_IO32(0x1001201C, data = core->sdMmc.readOtpEncrypted(7)) // OTP_ENCRYPTED7
                DEF_IO32(0x10012020, data = core->sdMmc.readOtpEncrypted(8)) // OTP_ENCRYPTED8
                DEF_IO32(0x10012024, data = core->sdMmc.readOtpEncrypted(9)) // OTP_ENCRYPTED9
                DEF_IO32(0x10012028, data = core->sdMmc.readOtpEncrypted(10)) // OTP_ENCRYPTED10
                DEF_IO32(0x1001202C, data = core->sdMmc.readOtpEncrypted(11)) // OTP_ENCRYPTED11
                DEF_IO32(0x10012030, data = core->sdMmc.readOtpEncrypted(12)) // OTP_ENCRYPTED12
                DEF_IO32(0x10012034, data = core->sdMmc.readOtpEncrypted(13)) // OTP_ENCRYPTED13
                DEF_IO32(0x10012038, data = core->sdMmc.readOtpEncrypted(14)) // OTP_ENCRYPTED14
                DEF_IO32(0x1001203C, data = core->sdMmc.readOtpEncrypted(15)) // OTP_ENCRYPTED15
                DEF_IO32(0x10012040, data = core->sdMmc.readOtpEncrypted(16)) // OTP_ENCRYPTED16
                DEF_IO32(0x10012044, data = core->sdMmc.readOtpEncrypted(17)) // OTP_ENCRYPTED17
                DEF_IO32(0x10012048, data = core->sdMmc.readOtpEncrypted(18)) // OTP_ENCRYPTED18
                DEF_IO32(0x1001204C, data = core->sdMmc.readOtpEncrypted(19)) // OTP_ENCRYPTED19
                DEF_IO32(0x10012050, data = core->sdMmc.readOtpEncrypted(20)) // OTP_ENCRYPTED20
                DEF_IO32(0x10012054, data = core->sdMmc.readOtpEncrypted(21)) // OTP_ENCRYPTED21
                DEF_IO32(0x10012058, data = core->sdMmc.readOtpEncrypted(22)) // OTP_ENCRYPTED22
                DEF_IO32(0x1001205C, data = core->sdMmc.readOtpEncrypted(23)) // OTP_ENCRYPTED23
                DEF_IO32(0x10012060, data = core->sdMmc.readOtpEncrypted(24)) // OTP_ENCRYPTED24
                DEF_IO32(0x10012064, data = core->sdMmc.readOtpEncrypted(25)) // OTP_ENCRYPTED25
                DEF_IO32(0x10012068, data = core->sdMmc.readOtpEncrypted(26)) // OTP_ENCRYPTED26
                DEF_IO32(0x1001206C, data = core->sdMmc.readOtpEncrypted(27)) // OTP_ENCRYPTED27
                DEF_IO32(0x10012070, data = core->sdMmc.readOtpEncrypted(28)) // OTP_ENCRYPTED28
                DEF_IO32(0x10012074, data = core->sdMmc.readOtpEncrypted(29)) // OTP_ENCRYPTED29
                DEF_IO32(0x10012078, data = core->sdMmc.readOtpEncrypted(30)) // OTP_ENCRYPTED30
                DEF_IO32(0x1001207C, data = core->sdMmc.readOtpEncrypted(31)) // OTP_ENCRYPTED31
                DEF_IO32(0x10012080, data = core->sdMmc.readOtpEncrypted(32)) // OTP_ENCRYPTED32
                DEF_IO32(0x10012084, data = core->sdMmc.readOtpEncrypted(33)) // OTP_ENCRYPTED33
                DEF_IO32(0x10012088, data = core->sdMmc.readOtpEncrypted(34)) // OTP_ENCRYPTED34
                DEF_IO32(0x1001208C, data = core->sdMmc.readOtpEncrypted(35)) // OTP_ENCRYPTED35
                DEF_IO32(0x10012090, data = core->sdMmc.readOtpEncrypted(36)) // OTP_ENCRYPTED36
                DEF_IO32(0x10012094, data = core->sdMmc.readOtpEncrypted(37)) // OTP_ENCRYPTED37
                DEF_IO32(0x10012098, data = core->sdMmc.readOtpEncrypted(38)) // OTP_ENCRYPTED38
                DEF_IO32(0x1001209C, data = core->sdMmc.readOtpEncrypted(39)) // OTP_ENCRYPTED39
                DEF_IO32(0x100120A0, data = core->sdMmc.readOtpEncrypted(40)) // OTP_ENCRYPTED40
                DEF_IO32(0x100120A4, data = core->sdMmc.readOtpEncrypted(41)) // OTP_ENCRYPTED41
                DEF_IO32(0x100120A8, data = core->sdMmc.readOtpEncrypted(42)) // OTP_ENCRYPTED42
                DEF_IO32(0x100120AC, data = core->sdMmc.readOtpEncrypted(43)) // OTP_ENCRYPTED43
                DEF_IO32(0x100120B0, data = core->sdMmc.readOtpEncrypted(44)) // OTP_ENCRYPTED44
                DEF_IO32(0x100120B4, data = core->sdMmc.readOtpEncrypted(45)) // OTP_ENCRYPTED45
                DEF_IO32(0x100120B8, data = core->sdMmc.readOtpEncrypted(46)) // OTP_ENCRYPTED46
                DEF_IO32(0x100120BC, data = core->sdMmc.readOtpEncrypted(47)) // OTP_ENCRYPTED47
                DEF_IO32(0x100120C0, data = core->sdMmc.readOtpEncrypted(48)) // OTP_ENCRYPTED48
                DEF_IO32(0x100120C4, data = core->sdMmc.readOtpEncrypted(49)) // OTP_ENCRYPTED49
                DEF_IO32(0x100120C8, data = core->sdMmc.readOtpEncrypted(50)) // OTP_ENCRYPTED50
                DEF_IO32(0x100120CC, data = core->sdMmc.readOtpEncrypted(51)) // OTP_ENCRYPTED51
                DEF_IO32(0x100120D0, data = core->sdMmc.readOtpEncrypted(52)) // OTP_ENCRYPTED52
                DEF_IO32(0x100120D4, data = core->sdMmc.readOtpEncrypted(53)) // OTP_ENCRYPTED53
                DEF_IO32(0x100120D8, data = core->sdMmc.readOtpEncrypted(54)) // OTP_ENCRYPTED54
                DEF_IO32(0x100120DC, data = core->sdMmc.readOtpEncrypted(55)) // OTP_ENCRYPTED55
                DEF_IO32(0x100120E0, data = core->sdMmc.readOtpEncrypted(56)) // OTP_ENCRYPTED56
                DEF_IO32(0x100120E4, data = core->sdMmc.readOtpEncrypted(57)) // OTP_ENCRYPTED57
                DEF_IO32(0x100120E8, data = core->sdMmc.readOtpEncrypted(58)) // OTP_ENCRYPTED58
                DEF_IO32(0x100120EC, data = core->sdMmc.readOtpEncrypted(59)) // OTP_ENCRYPTED59
                DEF_IO32(0x100120F0, data = core->sdMmc.readOtpEncrypted(60)) // OTP_ENCRYPTED60
                DEF_IO32(0x100120F4, data = core->sdMmc.readOtpEncrypted(61)) // OTP_ENCRYPTED61
                DEF_IO32(0x100120F8, data = core->sdMmc.readOtpEncrypted(62)) // OTP_ENCRYPTED62
                DEF_IO32(0x100120FC, data = core->sdMmc.readOtpEncrypted(63)) // OTP_ENCRYPTED63
                DEF_IO16(0x10006000, data = core->sdMmc.readSdCmd()) // SD_CMD
                DEF_IO16(0x10006002, data = core->sdMmc.readSdPortSelect()) // SD_PORT_SELECT
                DEF_IO32(0x10006004, data = core->sdMmc.readSdCmdParam()) // SD_CMD_PARAM
                DEF_IO16(0x1000600A, data = core->sdMmc.readSdData16Blkcnt()) // SD_DATA16_BLKCNT
                DEF_IO32(0x1000600C, data = core->sdMmc.readSdResponse(0)) // SD_RESPONSE0
                DEF_IO32(0x10006010, data = core->sdMmc.readSdResponse(1)) // SD_RESPONSE1
                DEF_IO32(0x10006014, data = core->sdMmc.readSdResponse(2)) // SD_RESPONSE2
                DEF_IO32(0x10006018, data = core->sdMmc.readSdResponse(3)) // SD_RESPONSE3
                DEF_IO32(0x1000601C, data = core->sdMmc.readSdIrqStatus()) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, data = core->sdMmc.readSdIrqMask()) // SD_IRQ_MASK
                DEF_IO16(0x10006026, data = core->sdMmc.readSdData16Blklen()) // SD_DATA16_BLKLEN
                DEF_IO16(0x10006030, data = core->sdMmc.readSdData16Fifo()) // SD_DATA16_FIFO
                DEF_IO16(0x100060D8, data = core->sdMmc.readSdDataCtl()) // SD_DATA_CTL
                DEF_IO16(0x10006100, data = core->sdMmc.readSdData32Irq()) // SD_DATA32_IRQ
                DEF_IO32(0x1000610C, data = core->sdMmc.readSdData32Fifo()) // SD_DATA32_FIFO
                DEF_IO32(0x10008000, data = core->pxi.readPxiSync(true)) // PXI_SYNC9
                DEF_IO32(0x10008004, data = core->pxi.readPxiCnt(true)) // PXI_CNT9
                DEF_IO32(0x1000800C, data = core->pxi.readPxiRecv(true)) // PXI_RECV9
                DEF_IO32(0x10009000, data = core->aes.readAesCnt()) // AES_CNT
                DEF_IO32(0x1000900C, data = core->aes.readAesRdfifo()) // AES_RDFIFO
                DEF_IO08(0x10009010, data = core->aes.readAesKeysel()) // AES_KEYSEL
                DEF_IO08(0x10009011, data = core->aes.readAesKeycnt()) // AES_KEYCNT
                DEF_IO32(0x1000A000, data = core->shas[1].readShaCnt()) // SHA_CNT9
                DEF_IO32(0x1000A004, data = core->shas[1].readShaBlkcnt()) // SHA_BLKCNT9
                DEF_IO32(0x1000A040, data = core->shas[1].readShaHash(0)) // SHA_HASH0_9
                DEF_IO32(0x1000A044, data = core->shas[1].readShaHash(1)) // SHA_HASH1_9
                DEF_IO32(0x1000A048, data = core->shas[1].readShaHash(2)) // SHA_HASH2_9
                DEF_IO32(0x1000A04C, data = core->shas[1].readShaHash(3)) // SHA_HASH3_9
                DEF_IO32(0x1000A050, data = core->shas[1].readShaHash(4)) // SHA_HASH4_9
                DEF_IO32(0x1000A054, data = core->shas[1].readShaHash(5)) // SHA_HASH5_9
                DEF_IO32(0x1000A058, data = core->shas[1].readShaHash(6)) // SHA_HASH6_9
                DEF_IO32(0x1000A05C, data = core->shas[1].readShaHash(7)) // SHA_HASH7_9
                DEF_IO32(0x1000A080, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A084, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A088, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A08C, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A090, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A094, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A098, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A09C, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A0, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A4, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0A8, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0AC, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B0, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B4, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0B8, data = core->shas[1].readShaFifo()) // SHA_FIFO9
                DEF_IO32(0x1000A0BC, data = core->shas[1].readShaFifo()) // SHA_FIFO9
            }
        }

        // Catch reads from unknown I/O registers
        LOG_WARN("Unknown ARM%d I/O read: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
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
        if (id != ARM9) {
            // Check registers that are exclusive to the ARM11
            switch (base) {
                DEF_IO32(0x10101000, core->shas[0].writeShaCnt(IO_PARAMS)) // SHA_CNT11
                DEF_IO32(0x10101004, core->shas[0].writeShaBlkcnt(IO_PARAMS)) // SHA_BLKCNT11
                DEF_IO32(0x10101040, core->shas[0].writeShaHash(0, IO_PARAMS)) // SHA_HASH0_11
                DEF_IO32(0x10101044, core->shas[0].writeShaHash(1, IO_PARAMS)) // SHA_HASH1_11
                DEF_IO32(0x10101048, core->shas[0].writeShaHash(2, IO_PARAMS)) // SHA_HASH2_11
                DEF_IO32(0x1010104C, core->shas[0].writeShaHash(3, IO_PARAMS)) // SHA_HASH3_11
                DEF_IO32(0x10101050, core->shas[0].writeShaHash(4, IO_PARAMS)) // SHA_HASH4_11
                DEF_IO32(0x10101054, core->shas[0].writeShaHash(5, IO_PARAMS)) // SHA_HASH5_11
                DEF_IO32(0x10101058, core->shas[0].writeShaHash(6, IO_PARAMS)) // SHA_HASH6_11
                DEF_IO32(0x1010105C, core->shas[0].writeShaHash(7, IO_PARAMS)) // SHA_HASH7_11
                DEF_IO32(0x10163000, core->pxi.writePxiSync(false, IO_PARAMS)) // PXI_SYNC11
                DEF_IO32(0x10163004, core->pxi.writePxiCnt(false, IO_PARAMS)) // PXI_CNT11
                DEF_IO32(0x10163008, core->pxi.writePxiSend(false, IO_PARAMS)) // PXI_SEND11
                DEF_IO32(0x10301000, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301004, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301008, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030100C, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301010, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301014, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301018, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030101C, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301020, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301024, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301028, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030102C, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301030, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301034, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10301038, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x1030103C, core->shas[0].writeShaFifo(IO_PARAMS)) // SHA_FIFO11
                DEF_IO32(0x10400468, core->gpu.writePdcFramebufLt0(false, IO_PARAMS)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400474, core->gpu.writePdcInterruptType(false, IO_PARAMS)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400568, core->gpu.writePdcFramebufLt0(true, IO_PARAMS)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x10400574, core->gpu.writePdcInterruptType(true, IO_PARAMS)) // PDC1_INTERRUPT_TYPE
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
            }
        }
        else {
            // Check registers that are exclusive to the ARM9
            switch (base) {
                DEF_IO32(0x10001000, core->interrupts.writeIrqIe(IO_PARAMS)) // IRQ_IE
                DEF_IO32(0x10001004, core->interrupts.writeIrqIf(IO_PARAMS)) // IRQ_IF
                DEF_IO16(0x10003000, core->timers.writeTmCntL(0, IO_PARAMS)) // TM0CNT_L
                DEF_IO16(0x10003002, core->timers.writeTmCntH(0, IO_PARAMS)) // TM0CNT_H
                DEF_IO16(0x10003004, core->timers.writeTmCntL(1, IO_PARAMS)) // TM1CNT_L
                DEF_IO16(0x10003006, core->timers.writeTmCntH(1, IO_PARAMS)) // TM1CNT_H
                DEF_IO16(0x10003008, core->timers.writeTmCntL(2, IO_PARAMS)) // TM2CNT_L
                DEF_IO16(0x1000300A, core->timers.writeTmCntH(2, IO_PARAMS)) // TM2CNT_H
                DEF_IO16(0x1000300C, core->timers.writeTmCntL(3, IO_PARAMS)) // TM3CNT_L
                DEF_IO16(0x1000300E, core->timers.writeTmCntH(3, IO_PARAMS)) // TM3CNT_H
                DEF_IO16(0x10006000, core->sdMmc.writeSdCmd(IO_PARAMS)) // SD_CMD
                DEF_IO16(0x10006002, core->sdMmc.writeSdPortSelect(IO_PARAMS)) // SD_PORT_SELECT
                DEF_IO32(0x10006004, core->sdMmc.writeSdCmdParam(IO_PARAMS)) // SD_CMD_PARAM
                DEF_IO16(0x1000600A, core->sdMmc.writeSdData16Blkcnt(IO_PARAMS)) // SD_DATA16_BLKCNT
                DEF_IO32(0x1000601C, core->sdMmc.writeSdIrqStatus(IO_PARAMS)) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmc.writeSdIrqMask(IO_PARAMS)) // SD_IRQ_MASK
                DEF_IO16(0x10006026, core->sdMmc.writeSdData16Blklen(IO_PARAMS)) // SD_DATA16_BLKLEN
                DEF_IO16(0x100060D8, core->sdMmc.writeSdDataCtl(IO_PARAMS)) // SD_DATA_CTL
                DEF_IO16(0x10006100, core->sdMmc.writeSdData32Irq(IO_PARAMS)) // SD_DATA32_IRQ
                DEF_IO32(0x10008000, core->pxi.writePxiSync(true, IO_PARAMS)) // PXI_SYNC9
                DEF_IO32(0x10008004, core->pxi.writePxiCnt(true, IO_PARAMS)) // PXI_CNT9
                DEF_IO32(0x10008008, core->pxi.writePxiSend(true, IO_PARAMS)) // PXI_SEND9
                DEF_IO32(0x10009000, core->aes.writeAesCnt(IO_PARAMS)) // AES_CNT
                DEF_IO16(0x10009006, core->aes.writeAesBlkcnt(IO_PARAMS)) // AES_BLKCNT
                DEF_IO32(0x10009008, core->aes.writeAesWrfifo(IO_PARAMS)) // AES_WRFIFO
                DEF_IO08(0x10009010, core->aes.writeAesKeysel(IO_PARAMS8)) // AES_KEYSEL
                DEF_IO08(0x10009011, core->aes.writeAesKeycnt(IO_PARAMS8)) // AES_KEYCNT
                DEF_IO32(0x10009020, core->aes.writeAesIv(0, IO_PARAMS)) // AES_IV0
                DEF_IO32(0x10009024, core->aes.writeAesIv(1, IO_PARAMS)) // AES_IV1
                DEF_IO32(0x10009028, core->aes.writeAesIv(2, IO_PARAMS)) // AES_IV2
                DEF_IO32(0x1000902C, core->aes.writeAesIv(3, IO_PARAMS)) // AES_IV3
                DEF_IO32(0x10009100, core->aes.writeAesKeyfifo(IO_PARAMS)) // AES_KEYFIFO
                DEF_IO32(0x10009104, core->aes.writeAesKeyxfifo(IO_PARAMS)) // AES_KEYXFIFO
                DEF_IO32(0x10009108, core->aes.writeAesKeyyfifo(IO_PARAMS)) // AES_KEYYFIFO
                DEF_IO32(0x1000A000, core->shas[1].writeShaCnt(IO_PARAMS)) // SHA_CNT9
                DEF_IO32(0x1000A004, core->shas[1].writeShaBlkcnt(IO_PARAMS)) // SHA_BLKCNT9
                DEF_IO32(0x1000A040, core->shas[1].writeShaHash(0, IO_PARAMS)) // SHA_HASH0_9
                DEF_IO32(0x1000A044, core->shas[1].writeShaHash(1, IO_PARAMS)) // SHA_HASH1_9
                DEF_IO32(0x1000A048, core->shas[1].writeShaHash(2, IO_PARAMS)) // SHA_HASH2_9
                DEF_IO32(0x1000A04C, core->shas[1].writeShaHash(3, IO_PARAMS)) // SHA_HASH3_9
                DEF_IO32(0x1000A050, core->shas[1].writeShaHash(4, IO_PARAMS)) // SHA_HASH4_9
                DEF_IO32(0x1000A054, core->shas[1].writeShaHash(5, IO_PARAMS)) // SHA_HASH5_9
                DEF_IO32(0x1000A058, core->shas[1].writeShaHash(6, IO_PARAMS)) // SHA_HASH6_9
                DEF_IO32(0x1000A05C, core->shas[1].writeShaHash(7, IO_PARAMS)) // SHA_HASH7_9
                DEF_IO32(0x1000A080, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A084, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A088, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A08C, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A090, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A094, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A098, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A09C, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A0, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A4, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0A8, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0AC, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B0, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B4, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0B8, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000A0BC, core->shas[1].writeShaFifo(IO_PARAMS)) // SHA_FIFO9
                DEF_IO32(0x1000B000, core->rsa.writeRsaCnt(IO_PARAMS)) // RSA_CNT
                DEF_IO32(0x1000B100, core->rsa.writeRsaSlotcnt(0, IO_PARAMS)) // RSA_SLOTCNT0
                DEF_IO32(0x1000B110, core->rsa.writeRsaSlotcnt(1, IO_PARAMS)) // RSA_SLOTCNT1
                DEF_IO32(0x1000B120, core->rsa.writeRsaSlotcnt(2, IO_PARAMS)) // RSA_SLOTCNT2
                DEF_IO32(0x1000B130, core->rsa.writeRsaSlotcnt(3, IO_PARAMS)) // RSA_SLOTCNT3
                DEF_IO32(0x1000B200, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B204, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B208, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B20C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B210, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B214, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B218, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B21C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B220, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B224, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B228, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B22C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B230, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B234, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B238, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B23C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B240, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B244, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B248, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B24C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B250, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B254, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B258, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B25C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B260, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B264, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B268, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B26C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B270, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B274, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B278, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B27C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B280, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B284, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B288, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B28C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B290, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B294, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B298, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B29C, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2AC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2BC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2CC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2DC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2EC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F0, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F4, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F8, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2FC, core->rsa.writeRsaExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B400, core->rsa.writeRsaMod(0, IO_PARAMS)) // RSA_MOD0
                DEF_IO32(0x1000B404, core->rsa.writeRsaMod(1, IO_PARAMS)) // RSA_MOD1
                DEF_IO32(0x1000B408, core->rsa.writeRsaMod(2, IO_PARAMS)) // RSA_MOD2
                DEF_IO32(0x1000B40C, core->rsa.writeRsaMod(3, IO_PARAMS)) // RSA_MOD3
                DEF_IO32(0x1000B410, core->rsa.writeRsaMod(4, IO_PARAMS)) // RSA_MOD4
                DEF_IO32(0x1000B414, core->rsa.writeRsaMod(5, IO_PARAMS)) // RSA_MOD5
                DEF_IO32(0x1000B418, core->rsa.writeRsaMod(6, IO_PARAMS)) // RSA_MOD6
                DEF_IO32(0x1000B41C, core->rsa.writeRsaMod(7, IO_PARAMS)) // RSA_MOD7
                DEF_IO32(0x1000B420, core->rsa.writeRsaMod(8, IO_PARAMS)) // RSA_MOD8
                DEF_IO32(0x1000B424, core->rsa.writeRsaMod(9, IO_PARAMS)) // RSA_MOD9
                DEF_IO32(0x1000B428, core->rsa.writeRsaMod(10, IO_PARAMS)) // RSA_MOD10
                DEF_IO32(0x1000B42C, core->rsa.writeRsaMod(11, IO_PARAMS)) // RSA_MOD11
                DEF_IO32(0x1000B430, core->rsa.writeRsaMod(12, IO_PARAMS)) // RSA_MOD12
                DEF_IO32(0x1000B434, core->rsa.writeRsaMod(13, IO_PARAMS)) // RSA_MOD13
                DEF_IO32(0x1000B438, core->rsa.writeRsaMod(14, IO_PARAMS)) // RSA_MOD14
                DEF_IO32(0x1000B43C, core->rsa.writeRsaMod(15, IO_PARAMS)) // RSA_MOD15
                DEF_IO32(0x1000B440, core->rsa.writeRsaMod(16, IO_PARAMS)) // RSA_MOD16
                DEF_IO32(0x1000B444, core->rsa.writeRsaMod(17, IO_PARAMS)) // RSA_MOD17
                DEF_IO32(0x1000B448, core->rsa.writeRsaMod(18, IO_PARAMS)) // RSA_MOD18
                DEF_IO32(0x1000B44C, core->rsa.writeRsaMod(19, IO_PARAMS)) // RSA_MOD19
                DEF_IO32(0x1000B450, core->rsa.writeRsaMod(20, IO_PARAMS)) // RSA_MOD20
                DEF_IO32(0x1000B454, core->rsa.writeRsaMod(21, IO_PARAMS)) // RSA_MOD21
                DEF_IO32(0x1000B458, core->rsa.writeRsaMod(22, IO_PARAMS)) // RSA_MOD22
                DEF_IO32(0x1000B45C, core->rsa.writeRsaMod(23, IO_PARAMS)) // RSA_MOD23
                DEF_IO32(0x1000B460, core->rsa.writeRsaMod(24, IO_PARAMS)) // RSA_MOD24
                DEF_IO32(0x1000B464, core->rsa.writeRsaMod(25, IO_PARAMS)) // RSA_MOD25
                DEF_IO32(0x1000B468, core->rsa.writeRsaMod(26, IO_PARAMS)) // RSA_MOD26
                DEF_IO32(0x1000B46C, core->rsa.writeRsaMod(27, IO_PARAMS)) // RSA_MOD27
                DEF_IO32(0x1000B470, core->rsa.writeRsaMod(28, IO_PARAMS)) // RSA_MOD28
                DEF_IO32(0x1000B474, core->rsa.writeRsaMod(29, IO_PARAMS)) // RSA_MOD29
                DEF_IO32(0x1000B478, core->rsa.writeRsaMod(30, IO_PARAMS)) // RSA_MOD30
                DEF_IO32(0x1000B47C, core->rsa.writeRsaMod(31, IO_PARAMS)) // RSA_MOD31
                DEF_IO32(0x1000B480, core->rsa.writeRsaMod(32, IO_PARAMS)) // RSA_MOD32
                DEF_IO32(0x1000B484, core->rsa.writeRsaMod(33, IO_PARAMS)) // RSA_MOD33
                DEF_IO32(0x1000B488, core->rsa.writeRsaMod(34, IO_PARAMS)) // RSA_MOD34
                DEF_IO32(0x1000B48C, core->rsa.writeRsaMod(35, IO_PARAMS)) // RSA_MOD35
                DEF_IO32(0x1000B490, core->rsa.writeRsaMod(36, IO_PARAMS)) // RSA_MOD36
                DEF_IO32(0x1000B494, core->rsa.writeRsaMod(37, IO_PARAMS)) // RSA_MOD37
                DEF_IO32(0x1000B498, core->rsa.writeRsaMod(38, IO_PARAMS)) // RSA_MOD38
                DEF_IO32(0x1000B49C, core->rsa.writeRsaMod(39, IO_PARAMS)) // RSA_MOD39
                DEF_IO32(0x1000B4A0, core->rsa.writeRsaMod(40, IO_PARAMS)) // RSA_MOD40
                DEF_IO32(0x1000B4A4, core->rsa.writeRsaMod(41, IO_PARAMS)) // RSA_MOD41
                DEF_IO32(0x1000B4A8, core->rsa.writeRsaMod(42, IO_PARAMS)) // RSA_MOD42
                DEF_IO32(0x1000B4AC, core->rsa.writeRsaMod(43, IO_PARAMS)) // RSA_MOD43
                DEF_IO32(0x1000B4B0, core->rsa.writeRsaMod(44, IO_PARAMS)) // RSA_MOD44
                DEF_IO32(0x1000B4B4, core->rsa.writeRsaMod(45, IO_PARAMS)) // RSA_MOD45
                DEF_IO32(0x1000B4B8, core->rsa.writeRsaMod(46, IO_PARAMS)) // RSA_MOD46
                DEF_IO32(0x1000B4BC, core->rsa.writeRsaMod(47, IO_PARAMS)) // RSA_MOD47
                DEF_IO32(0x1000B4C0, core->rsa.writeRsaMod(48, IO_PARAMS)) // RSA_MOD48
                DEF_IO32(0x1000B4C4, core->rsa.writeRsaMod(49, IO_PARAMS)) // RSA_MOD49
                DEF_IO32(0x1000B4C8, core->rsa.writeRsaMod(50, IO_PARAMS)) // RSA_MOD50
                DEF_IO32(0x1000B4CC, core->rsa.writeRsaMod(51, IO_PARAMS)) // RSA_MOD51
                DEF_IO32(0x1000B4D0, core->rsa.writeRsaMod(52, IO_PARAMS)) // RSA_MOD52
                DEF_IO32(0x1000B4D4, core->rsa.writeRsaMod(53, IO_PARAMS)) // RSA_MOD53
                DEF_IO32(0x1000B4D8, core->rsa.writeRsaMod(54, IO_PARAMS)) // RSA_MOD54
                DEF_IO32(0x1000B4DC, core->rsa.writeRsaMod(55, IO_PARAMS)) // RSA_MOD55
                DEF_IO32(0x1000B4E0, core->rsa.writeRsaMod(56, IO_PARAMS)) // RSA_MOD56
                DEF_IO32(0x1000B4E4, core->rsa.writeRsaMod(57, IO_PARAMS)) // RSA_MOD57
                DEF_IO32(0x1000B4E8, core->rsa.writeRsaMod(58, IO_PARAMS)) // RSA_MOD58
                DEF_IO32(0x1000B4EC, core->rsa.writeRsaMod(59, IO_PARAMS)) // RSA_MOD59
                DEF_IO32(0x1000B4F0, core->rsa.writeRsaMod(60, IO_PARAMS)) // RSA_MOD60
                DEF_IO32(0x1000B4F4, core->rsa.writeRsaMod(61, IO_PARAMS)) // RSA_MOD61
                DEF_IO32(0x1000B4F8, core->rsa.writeRsaMod(62, IO_PARAMS)) // RSA_MOD62
                DEF_IO32(0x1000B4FC, core->rsa.writeRsaMod(63, IO_PARAMS)) // RSA_MOD63
                DEF_IO32(0x1000B800, core->rsa.writeRsaData(0, IO_PARAMS)) // RSA_DATA0
                DEF_IO32(0x1000B804, core->rsa.writeRsaData(1, IO_PARAMS)) // RSA_DATA1
                DEF_IO32(0x1000B808, core->rsa.writeRsaData(2, IO_PARAMS)) // RSA_DATA2
                DEF_IO32(0x1000B80C, core->rsa.writeRsaData(3, IO_PARAMS)) // RSA_DATA3
                DEF_IO32(0x1000B810, core->rsa.writeRsaData(4, IO_PARAMS)) // RSA_DATA4
                DEF_IO32(0x1000B814, core->rsa.writeRsaData(5, IO_PARAMS)) // RSA_DATA5
                DEF_IO32(0x1000B818, core->rsa.writeRsaData(6, IO_PARAMS)) // RSA_DATA6
                DEF_IO32(0x1000B81C, core->rsa.writeRsaData(7, IO_PARAMS)) // RSA_DATA7
                DEF_IO32(0x1000B820, core->rsa.writeRsaData(8, IO_PARAMS)) // RSA_DATA8
                DEF_IO32(0x1000B824, core->rsa.writeRsaData(9, IO_PARAMS)) // RSA_DATA9
                DEF_IO32(0x1000B828, core->rsa.writeRsaData(10, IO_PARAMS)) // RSA_DATA10
                DEF_IO32(0x1000B82C, core->rsa.writeRsaData(11, IO_PARAMS)) // RSA_DATA11
                DEF_IO32(0x1000B830, core->rsa.writeRsaData(12, IO_PARAMS)) // RSA_DATA12
                DEF_IO32(0x1000B834, core->rsa.writeRsaData(13, IO_PARAMS)) // RSA_DATA13
                DEF_IO32(0x1000B838, core->rsa.writeRsaData(14, IO_PARAMS)) // RSA_DATA14
                DEF_IO32(0x1000B83C, core->rsa.writeRsaData(15, IO_PARAMS)) // RSA_DATA15
                DEF_IO32(0x1000B840, core->rsa.writeRsaData(16, IO_PARAMS)) // RSA_DATA16
                DEF_IO32(0x1000B844, core->rsa.writeRsaData(17, IO_PARAMS)) // RSA_DATA17
                DEF_IO32(0x1000B848, core->rsa.writeRsaData(18, IO_PARAMS)) // RSA_DATA18
                DEF_IO32(0x1000B84C, core->rsa.writeRsaData(19, IO_PARAMS)) // RSA_DATA19
                DEF_IO32(0x1000B850, core->rsa.writeRsaData(20, IO_PARAMS)) // RSA_DATA20
                DEF_IO32(0x1000B854, core->rsa.writeRsaData(21, IO_PARAMS)) // RSA_DATA21
                DEF_IO32(0x1000B858, core->rsa.writeRsaData(22, IO_PARAMS)) // RSA_DATA22
                DEF_IO32(0x1000B85C, core->rsa.writeRsaData(23, IO_PARAMS)) // RSA_DATA23
                DEF_IO32(0x1000B860, core->rsa.writeRsaData(24, IO_PARAMS)) // RSA_DATA24
                DEF_IO32(0x1000B864, core->rsa.writeRsaData(25, IO_PARAMS)) // RSA_DATA25
                DEF_IO32(0x1000B868, core->rsa.writeRsaData(26, IO_PARAMS)) // RSA_DATA26
                DEF_IO32(0x1000B86C, core->rsa.writeRsaData(27, IO_PARAMS)) // RSA_DATA27
                DEF_IO32(0x1000B870, core->rsa.writeRsaData(28, IO_PARAMS)) // RSA_DATA28
                DEF_IO32(0x1000B874, core->rsa.writeRsaData(29, IO_PARAMS)) // RSA_DATA29
                DEF_IO32(0x1000B878, core->rsa.writeRsaData(30, IO_PARAMS)) // RSA_DATA30
                DEF_IO32(0x1000B87C, core->rsa.writeRsaData(31, IO_PARAMS)) // RSA_DATA31
                DEF_IO32(0x1000B880, core->rsa.writeRsaData(32, IO_PARAMS)) // RSA_DATA32
                DEF_IO32(0x1000B884, core->rsa.writeRsaData(33, IO_PARAMS)) // RSA_DATA33
                DEF_IO32(0x1000B888, core->rsa.writeRsaData(34, IO_PARAMS)) // RSA_DATA34
                DEF_IO32(0x1000B88C, core->rsa.writeRsaData(35, IO_PARAMS)) // RSA_DATA35
                DEF_IO32(0x1000B890, core->rsa.writeRsaData(36, IO_PARAMS)) // RSA_DATA36
                DEF_IO32(0x1000B894, core->rsa.writeRsaData(37, IO_PARAMS)) // RSA_DATA37
                DEF_IO32(0x1000B898, core->rsa.writeRsaData(38, IO_PARAMS)) // RSA_DATA38
                DEF_IO32(0x1000B89C, core->rsa.writeRsaData(39, IO_PARAMS)) // RSA_DATA39
                DEF_IO32(0x1000B8A0, core->rsa.writeRsaData(40, IO_PARAMS)) // RSA_DATA40
                DEF_IO32(0x1000B8A4, core->rsa.writeRsaData(41, IO_PARAMS)) // RSA_DATA41
                DEF_IO32(0x1000B8A8, core->rsa.writeRsaData(42, IO_PARAMS)) // RSA_DATA42
                DEF_IO32(0x1000B8AC, core->rsa.writeRsaData(43, IO_PARAMS)) // RSA_DATA43
                DEF_IO32(0x1000B8B0, core->rsa.writeRsaData(44, IO_PARAMS)) // RSA_DATA44
                DEF_IO32(0x1000B8B4, core->rsa.writeRsaData(45, IO_PARAMS)) // RSA_DATA45
                DEF_IO32(0x1000B8B8, core->rsa.writeRsaData(46, IO_PARAMS)) // RSA_DATA46
                DEF_IO32(0x1000B8BC, core->rsa.writeRsaData(47, IO_PARAMS)) // RSA_DATA47
                DEF_IO32(0x1000B8C0, core->rsa.writeRsaData(48, IO_PARAMS)) // RSA_DATA48
                DEF_IO32(0x1000B8C4, core->rsa.writeRsaData(49, IO_PARAMS)) // RSA_DATA49
                DEF_IO32(0x1000B8C8, core->rsa.writeRsaData(50, IO_PARAMS)) // RSA_DATA50
                DEF_IO32(0x1000B8CC, core->rsa.writeRsaData(51, IO_PARAMS)) // RSA_DATA51
                DEF_IO32(0x1000B8D0, core->rsa.writeRsaData(52, IO_PARAMS)) // RSA_DATA52
                DEF_IO32(0x1000B8D4, core->rsa.writeRsaData(53, IO_PARAMS)) // RSA_DATA53
                DEF_IO32(0x1000B8D8, core->rsa.writeRsaData(54, IO_PARAMS)) // RSA_DATA54
                DEF_IO32(0x1000B8DC, core->rsa.writeRsaData(55, IO_PARAMS)) // RSA_DATA55
                DEF_IO32(0x1000B8E0, core->rsa.writeRsaData(56, IO_PARAMS)) // RSA_DATA56
                DEF_IO32(0x1000B8E4, core->rsa.writeRsaData(57, IO_PARAMS)) // RSA_DATA57
                DEF_IO32(0x1000B8E8, core->rsa.writeRsaData(58, IO_PARAMS)) // RSA_DATA58
                DEF_IO32(0x1000B8EC, core->rsa.writeRsaData(59, IO_PARAMS)) // RSA_DATA59
                DEF_IO32(0x1000B8F0, core->rsa.writeRsaData(60, IO_PARAMS)) // RSA_DATA60
                DEF_IO32(0x1000B8F4, core->rsa.writeRsaData(61, IO_PARAMS)) // RSA_DATA61
                DEF_IO32(0x1000B8F8, core->rsa.writeRsaData(62, IO_PARAMS)) // RSA_DATA62
                DEF_IO32(0x1000B8FC, core->rsa.writeRsaData(63, IO_PARAMS)) // RSA_DATA63
            }
        }

        // Catch writes to unknown I/O registers
        LOG_WARN("Unknown ARM%d I/O write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
        return;

    next:
        // Loop until the full value has been written
        i += size - base;
    }
}
