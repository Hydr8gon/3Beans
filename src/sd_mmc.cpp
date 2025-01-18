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

SdMmc::~SdMmc() {
    // Close the NAND file
    if (nand) fclose(nand);
}

bool SdMmc::loadFiles() {
    // Try to load encrypted OTP data from the NAND
    nand = fopen("nand.bin", "rb");
    if (!nand) return false;
    fseek(nand, 0xE00, SEEK_SET);
    fread(otpEncrypted, sizeof(uint32_t), 0x40, nand);
    return true;
}

void SdMmc::sendInterrupt(int bit) {
    // Set the interrupt's request bit
    sdIrqStatus |= BIT(bit);

    // Send an interrupt to the ARM9 if conditions are met
    if (sdIrqStatus & ~sdIrqMask)
        core->interrupts.sendInterrupt(true, 16);
}

void SdMmc::pushFifo(uint32_t value) {
    // Handle 32-bit FIFO mode
    if (sdDataCtl & BIT(1)) {
        // Push a value to the 32-bit read FIFO and check if full
        readFifo32.push(value);
        if ((readFifo32.size() << 2) < sdData16Blklen)
            return;

        // Trigger a 32-bit FIFO full interrupt if enabled
        sdData32Irq |= BIT(8);
        if (sdData32Irq & BIT(11))
            core->interrupts.sendInterrupt(true, 16);

        // Trigger a data end interrupt after the last block
        if (--curBlock == 0)
            sendInterrupt(2);
        return;
    }

    // Push a value to the 16-bit read FIFO and check if full
    readFifo16.push(value);
    readFifo16.push(value >> 16);
    if ((readFifo16.size() << 1) < sdData16Blklen)
        return;

    // Trigger a 16-bit FIFO full interrupt, plus data end after the last block
    sendInterrupt(24);
    if (--curBlock == 0)
        sendInterrupt(2);
}

void SdMmc::pushResponse(uint32_t value) {
    // Push a value to the response registers, shifting old data over
    sdResponse[3] = sdResponse[2];
    sdResponse[2] = sdResponse[1];
    sdResponse[1] = sdResponse[0];
    sdResponse[0] = value;
}

void SdMmc::readBlock() {
    // Read a block of data and adjust the address
    if (!nand) return;
    uint32_t data[0x200];
    fseek(nand, curAddress, SEEK_SET);
    fread(data, sizeof(uint32_t), blockLen, nand);
    LOG_INFO("Read MMC block from 0x%08X with size 0x%X\n", curAddress, blockLen << 2);
    curAddress += (blockLen << 2);

    // Push data to the read FIFO and change state based on blocks left
    for (int i = 0; i < blockLen; i++)
        pushFifo(data[i]);
    cardStatus = (cardStatus & ~0x1E00) | ((curBlock ? 0x5 : 0x4) << 9);
}

void SdMmc::runCommand() {
    // Ensure the command is being run in normal mode
    if (cardStatus & BIT(5)) {
        LOG_WARN("Tried to run normal SD/MMC command in app mode\n");
        cardStatus &= ~BIT(5);
        return;
    }

    // Execute a normal SD/MMC command
    switch (uint8_t cmd = sdCmd & 0x3F) {
        case 03: return setRelativeAddr(); // SET_RELATIVE_ADDR
        case 13: return getStatus(); // GET_STATUS
        case 16: return setBlocklen(); // SET_BLOCKLEN
        case 17: return readSingleBlock(); // READ_SINGLE_BLOCK
        case 18: return readMultiBlock(); // READ_MULTIPLE_BLOCK
        case 55: return appCmd(); // APP_CMD

    default:
        LOG_WARN("Unknown SD/MMC command: CMD%d\n", cmd);
        pushResponse(cardStatus);
        return;
    }
}

void SdMmc::runAppCommand() {
    // Ensure the command is being run in app mode
    if (~cardStatus & BIT(5)) {
        LOG_WARN("Tried to run app SD/MMC command in normal mode\n");
        return;
    }

    // Return to normal mode for the next command
    cardStatus &= ~BIT(5);

    // Execute an app SD/MMC command
    switch (uint8_t cmd = sdCmd & 0x3F) {
        case 13: return sdStatus(); // SD_STATUS
        case 41: return sendOpCond(); // SEND_OP_COND
        case 51: return getScr(); // GET_SCR

    default:
        LOG_WARN("Unknown SD/MMC command: ACMD%d\n", cmd);
        pushResponse(cardStatus);
        return;
    }
}

void SdMmc::setRelativeAddr() {
    // Stub CMD3 to get through the ARM9 boot ROM
    LOG_WARN("Stubbed SD/MMC command: CMD3\n");
    pushResponse(0x80000000);
}

void SdMmc::getStatus() {
    // Get the status register without doing anything else
    pushResponse(cardStatus);
}

void SdMmc::setBlocklen() {
    // Set the word length for multi-block reads and writes
    blockLen = std::min(0x200U, sdCmdParam + 3) >> 2;
    pushResponse(cardStatus);
}

void SdMmc::readSingleBlock() {
    // Read a single block of data
    curAddress = sdCmdParam;
    curBlock = 1;
    pushResponse(cardStatus);
    readBlock();
}

void SdMmc::readMultiBlock() {
    // Read the first of multiple blocks of data
    curAddress = sdCmdParam;
    curBlock = sdData16Blkcnt;
    pushResponse(cardStatus);
    readBlock();
}

void SdMmc::appCmd() {
    // Switch to app command mode
    cardStatus |= BIT(5);
    pushResponse(cardStatus);
}

void SdMmc::sdStatus() {
    // Pretend to read the 512-bit SD status register
    curBlock = 1;
    for (int i = 0; i < 16; i++)
        pushFifo(0);
    pushResponse(cardStatus);
}

void SdMmc::sendOpCond() {
    // Stub ACMD41 to get through the ARM9 boot ROM
    LOG_WARN("Stubbed SD/MMC command: ACMD41\n");
    pushResponse(0x80000000);
}

void SdMmc::getScr() {
    // Pretend to read the 64-bit SD configuration register
    curBlock = 1;
    pushFifo(0);
    pushFifo(0);
    pushResponse(cardStatus);
}

uint16_t SdMmc::readSdData16Fifo() {
    // Pop a value from the 16-bit read FIFO
    if (readFifo16.empty()) return sdData16Fifo;
    sdData16Fifo = readFifo16.front();
    readFifo16.pop();

    // Clear the FIFO full bit and trigger another multi-block read if necessary
    sdIrqStatus &= ~BIT(24);
    if (readFifo16.empty() && (cardStatus & 0x1E00) == (0x5 << 9))
        readBlock();
    return sdData16Fifo;
}

uint32_t SdMmc::readSdData32Fifo() {
    // Pop a value from the 32-bit read FIFO
    if (readFifo32.empty()) return sdData32Fifo;
    sdData32Fifo = readFifo32.front();
    readFifo32.pop();

    // Clear the FIFO full bit and trigger another multi-block read if necessary
    sdData32Irq &= ~BIT(8);
    if (readFifo32.empty() && (cardStatus & 0x1E00) == (0x5 << 9))
        readBlock();
    return sdData32Fifo;
}

void SdMmc::writeSdCmd(uint16_t mask, uint16_t value) {
    // Write to the SD_CMD register
    sdCmd = (sdCmd & ~mask) | (value & mask);

    // Send a response end interrupt to indicate command completion
    sendInterrupt(0);

    // Ignore commands sent to the SD card for now
    if (~sdPortSelect & BIT(0)) {
        LOG_WARN("Unhandled SD/MMC command with SD card selected\n");
        return;
    }

    // Auto response type is assumed, so warn when that isn't the case
    if (uint8_t resp = (sdCmd >> 8) & 0x7)
        LOG_WARN("Running SD/MMC command with non-auto response type: %d\n", resp);

    // Run the written command
    switch (uint8_t type = (sdCmd >> 6) & 0x3) {
        case 0: return runCommand();
        case 1: return runAppCommand();

    default:
        LOG_WARN("Unknown SD/MMC command type: %d\n", type);
        return;
    }
}

void SdMmc::writeSdPortSelect(uint16_t mask, uint16_t value) {
    // Write to the SD_PORT_SELECT register
    mask &= 0xF;
    sdPortSelect = (sdPortSelect & ~mask) | (value & mask);
}

void SdMmc::writeSdCmdParam(uint32_t mask, uint32_t value) {
    // Write to the SD_CMD_PARAM parameter
    sdCmdParam = (sdCmdParam & ~mask) | (value & mask);
}

void SdMmc::writeSdData16Blkcnt(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA16_BLKCNT block count
    sdData16Blkcnt = (sdData16Blkcnt & ~mask) | (value & mask);
}

void SdMmc::writeSdIrqStatus(uint32_t mask, uint32_t value) {
    // Acknowledge bits in the SD_IRQ_STATUS register
    sdIrqStatus = (sdIrqStatus & ~mask) | (sdIrqStatus & value & mask);
}

void SdMmc::writeSdIrqMask(uint32_t mask, uint32_t value) {
    // Write to the SD_IRQ_MASK register
    mask &= 0x8B7F031D;
    sdIrqMask = (sdIrqMask & ~mask) | (value & mask);
}

void SdMmc::writeSdData16Blklen(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA16_BLKLEN block length, clipping past 0x200
    mask &= 0x3FF;
    sdData16Blklen = std::min(0x200, (sdData16Blklen & ~mask) | (value & mask));
}

void SdMmc::writeSdDataCtl(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA_CTL register
    mask &= 0x22;
    sdDataCtl = (sdDataCtl & ~mask) | (value & mask);
}

void SdMmc::writeSdData32Irq(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA32_IRQ register
    mask &= 0x1802;
    sdData32Irq = (sdData32Irq & ~mask) | (value & mask);
}
