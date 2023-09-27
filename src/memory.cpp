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

// Define an 8-bit register in an I/O switch statement
#define DEF_IO08(addr, func) \
    case addr + 0:           \
        base -= addr;        \
        size = 1;            \
        func;                \
        goto next;

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
#define IO_PARAMS8 data << (base << 3)

bool Memory::loadFiles()
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
    LOG_WARN("Unmapped ARM%d memory read: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
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
    LOG_WARN("Unmapped ARM%d memory write: 0x%08X\n", (id == ARM9) ? 9 : 11, address);
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
                DEF_IO32(0x10400468, data = core->gpu.readPdcFramebufLt0(false)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400568, data = core->gpu.readPdcFramebufLt0(true)) // PDC1_FRAMEBUF_LT0
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
        else
        {
            // Check registers that are exclusive to the ARM9
            switch (base)
            {
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
                DEF_IO32(0x1000600C, data = core->sdMmc.readSdResponse(0)) // SD_RESPONSE0
                DEF_IO32(0x10006010, data = core->sdMmc.readSdResponse(1)) // SD_RESPONSE1
                DEF_IO32(0x10006014, data = core->sdMmc.readSdResponse(2)) // SD_RESPONSE2
                DEF_IO32(0x10006018, data = core->sdMmc.readSdResponse(3)) // SD_RESPONSE3
                DEF_IO32(0x1000601C, data = core->sdMmc.readSdIrqStatus()) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, data = core->sdMmc.readSdIrqMask()) // SD_IRQ_MASK
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
                DEF_IO32(0x10163000, core->pxi.writePxiSync(false, IO_PARAMS)) // PXI_SYNC11
                DEF_IO32(0x10163004, core->pxi.writePxiCnt(false, IO_PARAMS)) // PXI_CNT11
                DEF_IO32(0x10163008, core->pxi.writePxiSend(false, IO_PARAMS)) // PXI_SEND11
                DEF_IO32(0x10400468, core->gpu.writePdcFramebufLt0(false, IO_PARAMS)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x10400568, core->gpu.writePdcFramebufLt0(true, IO_PARAMS)) // PDC1_FRAMEBUF_LT0
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
        else
        {
            // Check registers that are exclusive to the ARM9
            switch (base)
            {
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
                DEF_IO32(0x1000601C, core->sdMmc.writeSdIrqStatus(IO_PARAMS)) // SD_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmc.writeSdIrqMask(IO_PARAMS)) // SD_IRQ_MASK
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
