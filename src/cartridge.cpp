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

void Cartridge::wordReady() {
    // Indicate that a word is ready
    ntrRomcnt |= BIT(23);
}

uint32_t Cartridge::readNtrData() {
    // Wait until a word is ready and then clear the ready bit
    if (~ntrRomcnt & BIT(23)) return 0xFFFFFFFF;
    ntrRomcnt &= ~BIT(23);

    // Increment the read counter and check if finished
    if ((readCount += 4) == blockSize) {
        // End the transfer and trigger interrupts if enabled
        ntrRomcnt &= ~BIT(31); // Busy
        if (ntrMcnt & BIT(14)) {
            core->interrupts.sendInterrupt(ARM9, 27);
            core->interrupts.sendInterrupt(ARM11, 0x44);
        }
    }
    else {
        // Schedule the next word at either 4.2MHz or 6.7MHz
        core->schedule(CART_WORD_READY, 32 * ((ntrRomcnt & BIT(27)) ? 8 : 5));
    }

    // Return a value based on the current command
    switch (cartCmd) {
        case CMD_CHIP: return 0x9000FEC2;
        case CMD_NONE: return 0xFFFFFFFF;
    }
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
        case 0: blockSize = 0; break;
        case 7: blockSize = 4; break;
        default: blockSize = 0x100 << size; break;
    }

    // Byteswap the command and reset reply state
    uint64_t cmd = BSWAP64(ntrCmd);
    cartCmd = CMD_NONE;

    // Interpret the NTRCARD command
    if (cartFile && !ctrMode) {
        if (cmd == 0x9000000000000000 || cmd == 0xA000000000000000) { // Chip ID
            // Switch to chip ID reply state
            cartCmd = CMD_CHIP;
        }
        else if (cmd == 0x3E00000000000000) { // CTRCARD mode
            // Switch to CTRCARD mode
            LOG_INFO("Cartridge switching to CTRCARD mode\n");
            ctrMode = true;
        } else if (cmd != 0x9F00000000000000 && cmd != 0x71C93FE9BB0A3B18) { // Not reset or signal
            // Catch unknown NTRCARD commands
            LOG_CRIT("Unknown NTRCARD command: 0x%lX\n", cmd);
        }
    }

    // End the transfer and trigger interrupts instantly if the size is zero
    if (blockSize == 0) {
        ntrRomcnt &= ~0x80800000; // Busy, word ready
        if (~ntrMcnt & BIT(14)) return;
        core->interrupts.sendInterrupt(ARM9, 27);
        core->interrupts.sendInterrupt(ARM11, 0x44);
        return;
    }

    // Schedule the first word at either 4.2MHz or 6.7MHz
    LOG_INFO("Starting NTRCARD transfer with command 0x%lX and size 0x%X\n", cmd, blockSize);
    core->schedule(CART_WORD_READY, 32 * ((ntrRomcnt & BIT(27)) ? 8 : 5));
    readCount = 0;
}

void Cartridge::writeNtrCmdL(uint32_t mask, uint32_t value) {
    // Write to the low part of the NTRCARD_CMD register
    ntrCmd = (ntrCmd & ~uint64_t(mask)) | (value & mask);
}

void Cartridge::writeNtrCmdH(uint32_t mask, uint32_t value) {
    // Write to the high part of the NTRCARD_CMD register
    ntrCmd = (ntrCmd & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
}
