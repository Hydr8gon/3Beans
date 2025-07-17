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

const uint16_t Cartridge::ctrClocks[] = { 64, 80, 96, 128, 160, 256, 256, 256 };

Cartridge::Cartridge(Core *core, std::string &cartPath): core(core) {
    // Open a cartridge file if a path was provided
    if (cartPath.empty() || !(cartFile = fopen(cartPath.c_str(), "rb"))) return;
    cfg9CardPower &= ~BIT(0); // Inserted

    // Determine a 3DS cartridge ID based on ROM size, from 128MB to 4GB
    fseek(cartFile, 0, SEEK_END);
    cartSize = ftell(cartFile);
    const uint8_t ids[] = { 0x7F, 0xFF, 0xFE, 0xFA, 0xF8, 0xF0 };
    uint8_t idx = 0;
    while ((0x8000000 << idx) < cartSize && idx < 5) idx++;
    cartId = 0x900000C2 | (ids[idx] << 8);

    // Read the cartridge media type and handle saving based on that
    uint8_t type = readCart(0x18C) >> 8;
    if (type == 2) cartId |= BIT(27);
    LOG_INFO("Cartridge is type %d, and its ID is 0x%X\n", type, cartId);
    if (type != 1) return;

    // Open a save file associated with the ROM if it exists
    savePath = cartPath.substr(0, cartPath.rfind('.')) + ".sav";
    if (FILE *saveFile = fopen(savePath.c_str(), "rb")) {
        // Determine a CARD1 save ID based on size, up to 8MB
        fseek(saveFile, 0, SEEK_END);
        saveSize = ftell(saveFile);
        uint8_t id = 0;
        while ((1 << id) < saveSize && id < 0x17) id++;
        saveId = (id << 16) | 0x22C2;

        // Allocate save data and fill it with the file contents
        saveData = new uint8_t[1 << id];
        memset(saveData, 0xFF, saveSize);
        fseek(saveFile, 0, SEEK_SET);
        fread(saveData, sizeof(uint8_t), std::min(saveSize, 1U << id), saveFile);
        saveSize = (1 << id);
        fclose(saveFile);
    }
    else {
        // Create a new CARD1 save and assume 512KB size
        saveId = 0x1322C2;
        saveData = new uint8_t[saveSize = 0x80000];
        memset(saveData, 0xFF, saveSize);
    }
}

Cartridge::~Cartridge() {
    // Clean up the cartridge file and save data
    if (cartFile) fclose(cartFile);
    if (saveData) delete[] saveData;
}

uint32_t Cartridge::readCart(uint32_t address) {
    // Read a value from cartridge ROM, loading new blocks from file as necessary
    if (address >= cartSize) return 0xFFFFFFFF;
    if (((address ^ cartBase) >> 11) != 0) {
        cartBase = (address & ~0x7FF);
        fseek(cartFile, cartBase, SEEK_SET);
        fread(cartBlock, sizeof(uint32_t), 0x200, cartFile);
    }
    return cartBlock[(address >> 2) & 0x1FF];
}

void Cartridge::updateSave() {
    // Update the save file if its data changed
    if (!saveDirty) return;
    if (FILE *saveFile = fopen(savePath.c_str(), "wb")) {
        LOG_INFO("Writing updated save file to disk\n");
        fwrite(saveData, sizeof(uint8_t), saveSize, saveFile);
        fclose(saveFile);
        saveDirty = false;
    }
}

void Cartridge::ntrWordReady() {
    // Indicate that a NTRCARD word is ready and trigger DMAs
    ntrRomcnt |= BIT(23);
    core->ndma.triggerMode(0xC);
}

void Cartridge::ctrWordReady() {
    // Clear the busy bit if finished, or schedule the next word if not full and running
    if ((ctrCount -= 4) == 0)
        ctrCnt &= ~BIT(31);
    else if (ctrFifo.size() < 7 && (ctrCnt & BIT(31)))
        core->schedule(CTR_WORD_READY, ctrClocks[(ctrCnt >> 24) & 0x7]);

    // Set the ready bit and trigger DMAs every 8 words
    if ((ctrCount & 0x1F) == 0) {
        ctrCnt |= BIT(27);
        core->ndma.triggerMode(0x4);
    }

    // Push a value to the FIFO based on the current CTRCARD reply state
    switch (ctrReply) {
    case REPLY_CHIP1:
        // Reply with the primary 3DS cartridge chip ID
        return ctrFifo.push(cartId);

    case REPLY_CHIP2:
        // Reply with the secondary 3DS cartridge chip ID (empty?)
        return ctrFifo.push(0);

    case REPLY_HEADER:
        // Reply with data from the initial cartridge header
        return ctrFifo.push(readCart(0x1000 + (((ctrAddress += 4) - 4) & 0x1FF)));

    case REPLY_ROM:
        // Reply with data from any address in the cartridge
        return ctrFifo.push(readCart((ctrAddress += 4) - 4));

    case REPLY_PROM:
        // Reply with a 16-byte unique ID (stubbed to zero), followed by 0x30 high bytes
        return ctrFifo.push((((ctrAddress += 4) & 0x3F) > 0x10) ? 0xFFFFFFFF : 0);

    default:
        // Reply with all high bits when there's no data
        return ctrFifo.push(0xFFFFFFFF);
    }
}

uint8_t Cartridge::spiTransfer(uint8_t value) {
    // End an SPICARD transfer and trigger an interrupt when its length is reached
    if (--spiCount == 0) {
        spiFifoCnt &= ~BIT(15);
        if (spiFifoIntMask & BIT(0)) {
            spiFifoIntStat |= BIT(0);
            core->interrupts.sendInterrupt(ARM9, 23);
        }
    }

    // Set the command byte on the first write
    if (++spiTotal == 1) {
        spiCommand = value;
        spiAddress = 0;
    }

    // Handle SPICARD accesses based on the command byte
    switch (spiCommand) {
    case 0x02: // Write byte data
        // Set the 3-byte address to write to and handle dummy bytes
        if (spiTotal <= 4) {
            if (spiTotal >= 2)
                spiAddress |= value << ((4 - spiTotal) * 8);
            return 0;
        }

        // Write save data and increment the address if enabled
        if ((spiStatus & BIT(1)) && spiAddress < saveSize)
            saveData[spiAddress++] = value, saveDirty = true;
        return 0;

    case 0x03: // Read byte data
    case 0xEB: // Read quad data
        // Set the 3-byte address to read from and handle dummy bytes
        if (spiTotal <= (spiCommand == 0xEB ? 7 : 4)) {
            if (spiTotal >= 2 && spiTotal <= 4)
                spiAddress |= value << ((4 - spiTotal) * 8);
            return 0;
        }

        // Read save data and increment the address once it's set
        return (spiAddress < saveSize) ? saveData[spiAddress++] : 0xFF;

    case 0x04: // Disable writes
        // Clear the write enable bit in the status register
        spiStatus &= ~BIT(1);
        return 0;

    case 0x05: // Read status
        // Read the FLASH status register
        return spiStatus;

    case 0x06: // Enable writes
        // Set the write enable bit in the status register
        spiStatus |= BIT(1);
        return 0;

    case 0x20: // Erase 4KB
        // Set the 3-byte address to erase
        if (spiTotal <= 4 && spiTotal >= 2)
            spiAddress |= value << ((4 - spiTotal) * 8);

        // On the last write, erase a 4KB block if enabled
        if (spiTotal != 4 || !(spiStatus & BIT(1))) return 0;
        for (int i = 0; i < 0x1000; i++)
            if (spiAddress + i < saveSize)
                saveData[spiAddress + i] = 0xFF, saveDirty = true;
        return 0;

    case 0x9F: // Read ID
        // Read the 3DS cartridge save ID
        if (spiTotal <= 4 && spiTotal >= 2)
            return saveId >> ((spiTotal - 2) * 8);
        return 0;

    default:
        // Catch SPICARD accesses with unknown commands
        LOG_CRIT("Accessing SPICARD with unknown command: 0x%X\n", spiCommand);
        return 0;
    }
}

uint32_t Cartridge::readNtrData() {
    // Wait until a word is ready and then clear the ready bit
    if (~ntrRomcnt & BIT(23)) return 0xFFFFFFFF;
    ntrRomcnt &= ~BIT(23);

    // Decrement the read counter and check if finished
    if ((ntrCount -= 4) == 0) {
        // End the transfer and trigger interrupts if enabled
        ntrRomcnt &= ~BIT(31); // Busy
        if (ntrMcnt & BIT(14)) {
            core->interrupts.sendInterrupt(ARM9, 27);
            core->interrupts.sendInterrupt(ARM11, 0x44);
        }
    }
    else {
        // Schedule the next word at either 4.2MHz or 6.7MHz
        core->schedule(NTR_WORD_READY, (ntrRomcnt & BIT(27)) ? 256 : 160);
    }

    // Return a value based on the current NTRCARD reply state
    switch (ntrReply) {
    case REPLY_CHIP1:
        // Reply with the primary 3DS cartridge chip ID
        return cartId;

    case REPLY_CHIP2:
        // Reply with the secondary 3DS cartridge chip ID (empty?)
        return 0;

    default:
        // Reply with all high bits when there's no data
        return 0xFFFFFFFF;
    }
}

uint32_t Cartridge::readCtrFifo() {
    // Schedule the next word if full but running, or trigger an end interrupt if done
    if (ctrFifo.size() == 8 && ctrCount > 0 && (ctrCnt & BIT(31)))
        core->schedule(CTR_WORD_READY, ctrClocks[(ctrCnt >> 24) & 0x7]);
    else if (ctrFifo.size() == 1 && ctrCount == 0 && (ctrCnt & BIT(30)))
        core->interrupts.sendInterrupt(ARM9, 23);

    // Pop a word from the CTRCARD FIFO and clear the ready bit if empty
    if (ctrFifo.empty()) return 0xFFFFFFFF;
    uint32_t value = ctrFifo.front();
    ctrFifo.pop();
    ctrCnt &= ~(ctrFifo.empty() << 27);
    return value;
}

uint32_t Cartridge::readSpiFifoData() {
    // Read up to 4 bytes from SPICARD if transferring in the read direction
    uint32_t value = 0;
    if ((spiFifoCnt & 0xA000) == 0x8000 && spiFifoSelect)
        for (int i = 0; i < 32 && spiCount; i += 8)
            value |= (spiTransfer(0) << i);
    return value;
}

void Cartridge::writeCfg9CardPower(uint16_t mask, uint16_t value) {
    // Write to the CFG9_CARD_POWER state bits
    mask &= 0xC;
    cfg9CardPower = (cfg9CardPower & ~mask) | (value & mask);

    // Change to off state automatically if requested
    if ((cfg9CardPower & 0xC) == 0xC)
        cfg9CardPower &= ~0xC;
}

void Cartridge::writeNtrMcnt(uint16_t mask, uint16_t value) {
    // Write to the NTRCARD_MCNT register
    mask &= 0xE043;
    ntrMcnt = (ntrMcnt & ~mask) | (value & mask);
}

void Cartridge::writeNtrRomcnt(uint32_t mask, uint32_t value) {
    // Write to the NTRCARD_ROMCNT register and check if a transfer was just started
    mask &= 0xFF7F7FFF;
    bool transfer = (~ntrRomcnt & value & mask & BIT(31));
    ntrRomcnt = (ntrRomcnt & ~mask) | (value & mask);
    if (!transfer) return;

    // Determine the size of the block to transfer
    switch (uint8_t size = (ntrRomcnt >> 24) & 0x7) {
        case 0: ntrCount = 0; break;
        case 7: ntrCount = 4; break;
        default: ntrCount = 0x100 << size; break;
    }

    // Byteswap the command and reset reply state
    uint64_t cmd = BSWAP64((uint64_t(ntrCmd[1]) << 32) | ntrCmd[0]);
    ntrReply = REPLY_NONE;

    // Interpret the NTRCARD command
    if (cartFile && !ctrMode) {
        switch (cmd >> 56) {
        case 0x90: // Chip ID 1
            // Switch to primary chip ID reply state
            ntrReply = REPLY_CHIP1;
            break;

        case 0xA0: // Chip ID 2
            // Switch to secondary chip ID reply state
            ntrReply = REPLY_CHIP2;
            break;

        case 0x3E: // CTRCARD mode
            // Switch to CTRCARD mode
            LOG_INFO("Cartridge switching to CTRCARD mode\n");
            ctrMode = true;
            break;

        case 0x9F: case 0x71: // Reset or signal
            // Do nothing for these commands
            break;

        default:
            // Catch unknown NTRCARD commands
            LOG_CRIT("Unknown NTRCARD command: 0x%lX\n", cmd);
            break;
        }
    }

    // End the transfer and trigger interrupts instantly if the size is zero
    if (ntrCount == 0) {
        ntrRomcnt &= ~0x80800000; // Busy, word ready
        if (~ntrMcnt & BIT(14)) return;
        core->interrupts.sendInterrupt(ARM9, 27);
        core->interrupts.sendInterrupt(ARM11, 0x44);
        return;
    }

    // Schedule the first word at either 4.2MHz or 6.7MHz per byte
    core->schedule(NTR_WORD_READY, (ntrRomcnt & BIT(27)) ? 256 : 160);
}

void Cartridge::writeNtrCmd(int i, uint32_t mask, uint32_t value) {
    // Write to a 32-bit part of the NTRCARD_CMD register
    ntrCmd[i] = (ntrCmd[i] & ~mask) | (value & mask);
}

void Cartridge::writeCtrCnt(uint32_t mask, uint32_t value) {
    // Write to the CTRCARD_CNT register and check if a transfer was just started
    mask &= 0xF70F825F;
    bool transfer = (~ctrCnt & value & mask & BIT(31));
    ctrCnt = (ctrCnt & ~mask) | (value & mask);
    if (!transfer) return;

    // Determine the total size of data to transfer
    static const uint16_t sizes[9] = { 0x0, 0x4, 0x10, 0x40, 0x200, 0x400, 0x800, 0x1000, 0x2000 };
    uint16_t blkSize = sizes[std::min(8U, (ctrCnt >> 16) & 0xF)];
    ctrCount = blkSize * ((ctrBlkcnt & 0x7FFF) + 1);

    // Get the command in two parts and reset reply state
    uint64_t cmdL = (uint64_t(ctrCmd[1]) << 32) | ctrCmd[0];
    uint64_t cmdH = (uint64_t(ctrCmd[3]) << 32) | ctrCmd[2];
    ctrReply = REPLY_NONE;

    // Interpret the CTRCARD command
    if (cartFile && ctrMode) {
        switch (cmdH >> 56) {
        case 0xA2: // Chip ID 1
            // Switch to primary chip ID reply state
            ctrReply = REPLY_CHIP1;
            break;

        case 0xA3: // Chip ID 2
            // Switch to secondary chip ID reply state
            ctrReply = REPLY_CHIP2;
            break;

        case 0x82: // Header
            // Switch to header reply state and reset the address
            ctrReply = REPLY_HEADER;
            ctrAddress = 0;
            break;

        case 0xBF: // ROM address
            // Switch to ROM reply state and set the address
            ctrReply = REPLY_ROM;
            ctrAddress = cmdH;
            LOG_INFO("Starting CTRCARD read from address 0x%X with size 0x%X\n", ctrAddress, ctrCount);
            break;

        case 0xC6: // Unique ID
            // Switch to PROM reply state and reset the address
            ctrReply = REPLY_PROM;
            ctrAddress = 0;
            break;

        case 0x83: case 0xC5: // Reseed or refresh
            // Do nothing for these commands
            break;

        default:
            // Catch unknown CTRCARD commands
            LOG_CRIT("Unknown CTRCARD command: 0x%lX%016lX\n", cmdH, cmdL);
            break;
        }
    }

    // End the transfer and trigger an interrupt instantly if the size is zero
    if (ctrCount == 0) {
        ctrCnt &= ~0x88000000; // Busy, word ready
        if (ctrCnt & BIT(30))
            core->interrupts.sendInterrupt(ARM9, 23);
        return;
    }

    // Schedule the first word between 4.2MHz to 16.7MHz per byte
    if (ctrFifo.size() < 8)
        core->schedule(CTR_WORD_READY, ctrClocks[(ctrCnt >> 24) & 0x7]);
}

void Cartridge::writeCtrBlkcnt(uint32_t mask, uint32_t value) {
    // Write to the CTRCARD_BLKCNT register
    mask &= 0x1FFF7FFF;
    ctrBlkcnt = (ctrBlkcnt & ~mask) | (value & mask);
}

void Cartridge::writeCtrSeccnt(uint32_t mask, uint32_t value) {
    // Write to the CTRCARD_SECCNT register
    mask &= 0x307;
    ctrSeccnt = (ctrSeccnt & ~mask) | (value & mask);

    // Set the ready bit if crypto is enabled, but don't actually do anything
    ctrSeccnt = (ctrSeccnt & ~BIT(14)) | ((ctrSeccnt & BIT(2)) << 12);
}

void Cartridge::writeCtrCmd(int i, uint32_t mask, uint32_t value) {
    // Write to a 32-bit part of the CTRCARD_CMD register
    ctrCmd[i] = (ctrCmd[i] & ~mask) | (value & mask);
}

void Cartridge::writeSpiFifoCnt(uint32_t mask, uint32_t value) {
    // Write to the SPICARD_FIFO_CNT register
    mask &= 0xB007;
    uint32_t old = spiFifoCnt;
    spiFifoCnt = (spiFifoCnt & ~mask) | (value & mask);

    // Reload SPICARD transfer length and chip select if the start bit was newly set
    if (!(~old & spiFifoCnt & BIT(15))) return;
    spiCount = spiFifoBlklen;
    spiFifoSelect |= BIT(0);
}

void Cartridge::writeSpiFifoSelect(uint32_t mask, uint32_t value) {
    // Write to the SPICARD_FIFO_SELECT register
    mask &= 0x1;
    spiFifoSelect = (spiFifoSelect & ~mask) | (value & mask);

    // Reset SPICARD transfer state when the chip is deselected
    if (!spiFifoSelect)
        spiTotal = 0;
}

void Cartridge::writeSpiFifoBlklen(uint32_t mask, uint32_t value) {
    // Write to the SPICARD_FIFO_BLKLEN register
    mask &= 0x1FFFFF;
    spiFifoBlklen = (spiFifoBlklen & ~mask) | (value & mask);
}

void Cartridge::writeSpiFifoData(uint32_t mask, uint32_t value) {
    // Write up to 4 bytes to SPICARD if transferring in the write direction
    if ((spiFifoCnt & 0xA000) == 0xA000 && spiFifoSelect)
        for (int i = 0; i < 32 && spiCount; i += 8)
            spiTransfer((value & mask) >> i);
}

void Cartridge::writeSpiFifoIntMask(uint32_t mask, uint32_t value) {
    // Write to the SPICARD_FIFO_INT_MASK register
    mask &= 0xF;
    spiFifoIntMask = (spiFifoIntMask & ~mask) | (value & mask);
}

void Cartridge::writeSpiFifoIntStat(uint32_t mask, uint32_t value) {
    // Acknowledge bits in the SPICARD_FIFO_INT_STAT register
    spiFifoIntStat &= ~(value & mask);
}
