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

const uint16_t Cartridge::ctrClocks[] = { 64, 80, 96, 128, 160, 256, 256, 256 };

Cartridge::Cartridge(Core *core, std::string &cartPath): core(core) {
    // Open a cartridge file if a path was provided
    if (cartPath.empty()) return;
    cartFile = fopen(cartPath.c_str(), "rb");
    cfg9CardPower &= ~BIT(0); // Inserted
}

Cartridge::~Cartridge() {
    // Close the cartridge file if it was opened
    if (cartFile)
        fclose(cartFile);
}

uint32_t Cartridge::readCart(uint32_t address) {
    // Read a value from cartridge ROM, loading new blocks from file as necessary
    if (((address ^ cartBase) >> 11) != 0) {
        cartBase = (address & ~0x7FF);
        fseek(cartFile, cartBase, SEEK_SET);
        fread(cartBlock, 0x200, sizeof(uint32_t), cartFile);
    }
    return cartBlock[(address >> 2) & 0x1FF];
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
    case REPLY_CHIP:
        // Reply with a standard 3DS cartridge chip ID
        return ctrFifo.push(0x9000FEC2);

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
    case REPLY_CHIP:
        // Reply with a standard 3DS cartridge chip ID
        return 0x9000FEC2;

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
        case 0x90: case 0xA0: // Chip ID
            // Switch to chip ID reply state
            ntrReply = REPLY_CHIP;
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
    LOG_INFO("Starting NTRCARD transfer with command 0x%lX and size 0x%X\n", cmd, ntrCount);
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
        case 0xA2: case 0xA3: // Chip ID
            // Switch to chip ID reply state
            ctrReply = REPLY_CHIP;
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
    LOG_INFO("Starting CTRCARD transfer with command 0x%lX%016lX and size 0x%X\n", cmdH, cmdL, ctrCount);
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
