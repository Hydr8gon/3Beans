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

SdMmc::~SdMmc() {
    // Close the NAND and SD files
    if (nand) fclose(nand);
    if (sd) fclose(sd);
}

bool SdMmc::init() {
    // Try to open an SD image
    sd = fopen("sd.img", "rb");

    // Try to open a GM9 NAND dump and load CID and OTP data
    if (!(nand = fopen("nand.bin", "rb"))) return false;
    fseek(nand, 0xC00, SEEK_SET);
    fread(mmcCid, sizeof(uint32_t), 4, nand);
    fseek(nand, 0xE00, SEEK_SET);
    core->memory.loadOtp(nand);

    // Determine if this is a new 3DS NAND by checking for data in sector 0x96
    uint32_t sect96[8];
    fseek(nand, 0x12C00, SEEK_SET);
    fread(sect96, sizeof(uint32_t), 8, nand);
    for (int i = 0; i < 8; i++)
        if (sect96[i] != 0xFFFFFFFF) return true;
    return false;
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
        uint32_t size = (readFifo32.size() << 2);
        if (size >= 0x200) return;
        readFifo32.push(value);
        if (size + 4 < sdData32Blklen) return;

        // Trigger a 32-bit FIFO full interrupt if enabled and finish a block
        sdData32Irq |= BIT(8);
        if (sdData32Irq & BIT(11))
            core->interrupts.sendInterrupt(true, 16);
        curBlock--;
        return;
    }

    // Push a value to the 16-bit read FIFO and check if full
    uint32_t size = (readFifo16.size() << 1);
    if (size >= 0x200) return;
    readFifo16.push(value);
    readFifo16.push(value >> 16);
    if (size + 4 < sdData16Blklen) return;

    // Trigger a 16-bit FIFO full interrupt and finish a block
    sendInterrupt(24);
    curBlock--;
}

void SdMmc::pushResponse(uint32_t value) {
    // Push a value to the response registers, shifting old data over
    sdResponse[3] = sdResponse[2];
    sdResponse[2] = sdResponse[1];
    sdResponse[1] = sdResponse[0];
    sdResponse[0] = value;
}

void SdMmc::readBlock() {
    // Read a block of data from the SD or MMC if present
    uint32_t data[0x80];
    if (FILE *src = (sdPortSelect & BIT(0)) ? nand : sd) {
        fseek(src, curAddress, SEEK_SET);
        fread(data, sizeof(uint32_t), blockLen, src);
    }
    else {
        memset(data, 0, sizeof(data));
    }

    // Push data to the read FIFO and change state based on blocks left
    LOG_INFO("Read %s block from 0x%08X with size 0x%X\n",
        (sdPortSelect & BIT(0)) ? "MMC" : "SD", curAddress, blockLen << 2);
    for (int i = 0; i < blockLen; i++) pushFifo(data[i]);
    cardStatus = (cardStatus & ~0x1E00) | ((curBlock ? 0x5 : 0x4) << 9);
    curAddress += (blockLen << 2);
    core->ndma.triggerMode(0x6);
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
        case 2: return getCid(); // ALL_GET_CID (stub)
        case 10: return getCid(); // GET_CID
        case 13: return getStatus(); // GET_STATUS
        case 16: return setBlocklen(); // SET_BLOCKLEN
        case 17: return readSingleBlock(); // READ_SINGLE_BLOCK
        case 18: return readMultiBlock(); // READ_MULTIPLE_BLOCK
        case 55: return appCmd(); // APP_CMD

    case 1: // SEND_OP_COND
    case 3: // SET_RELATIVE_ADDR
        // Stub commands that want response bit 31 to be set
        LOG_WARN("Stubbed SD/MMC command: CMD%d\n", cmd);
        pushResponse(0x80000000);
        return;

    default:
        // Assume response type 1 for unknown commands
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
        case 51: return getScr(); // GET_SCR

    case 41: // SD_SEND_OP_COND
        // Stub commands that want response bit 31 to be set
        LOG_WARN("Stubbed SD/MMC command: ACMD%d\n", cmd);
        pushResponse(0x80000000);
        return;

    default:
        // Assume response type 1 for unknown commands
        LOG_WARN("Unknown SD/MMC command: ACMD%d\n", cmd);
        pushResponse(cardStatus);
        return;
    }
}

void SdMmc::getCid() {
    // Get the card ID value as a response
    for (int i = 0; i < 4; i++)
        pushResponse(mmcCid[3 - i]);
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
    core->schedule(SDMMC_READ_BLOCK, 1);
}

void SdMmc::readMultiBlock() {
    // Read the first of multiple blocks of data
    curAddress = sdCmdParam;
    curBlock = sdData16Blkcnt;
    pushResponse(cardStatus);
    core->schedule(SDMMC_READ_BLOCK, 1);
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

void SdMmc::getScr() {
    // Pretend to read the 64-bit SD configuration register
    curBlock = 1;
    pushFifo(0);
    pushFifo(0);
    pushResponse(cardStatus);
}

uint32_t SdMmc::readIrqStatus() {
    // Read from the SD_IRQ_STATUS register with the insert bit set for SD cards
    return sdIrqStatus | ((~sdPortSelect & BIT(0)) << 5);
}

uint16_t SdMmc::readData16Fifo() {
    // Pop a value from the 16-bit read FIFO
    if (readFifo16.empty()) return sdData16Fifo;
    sdData16Fifo = readFifo16.front();
    readFifo16.pop();

    // Clear the FIFO full bit and read another block or end with an interrupt if empty
    sdIrqStatus &= ~BIT(24);
    if (readFifo16.empty())
        (cardStatus & 0x1E00) == (0x5 << 9) ? core->schedule(SDMMC_READ_BLOCK, 1) : sendInterrupt(2);
    return sdData16Fifo;
}

uint32_t SdMmc::readData32Fifo() {
    // Pop a value from the 32-bit read FIFO
    if (readFifo32.empty()) return sdData32Fifo;
    sdData32Fifo = readFifo32.front();
    readFifo32.pop();

    // Clear the FIFO full bit and read another block or end with an interrupt if empty
    sdData32Irq &= ~BIT(8);
    if (readFifo32.empty())
        (cardStatus & 0x1E00) == (0x5 << 9) ? core->schedule(SDMMC_READ_BLOCK, 1) : sendInterrupt(2);
    return sdData32Fifo;
}

void SdMmc::writeCmd(uint16_t mask, uint16_t value) {
    // Write to the SD_CMD register
    sdCmd = (sdCmd & ~mask) | (value & mask);

    // Send a response end interrupt to indicate command completion
    sendInterrupt(0);

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

void SdMmc::writePortSelect(uint16_t mask, uint16_t value) {
    // Write to the SD_PORT_SELECT register
    mask &= 0xF;
    sdPortSelect = (sdPortSelect & ~mask) | (value & mask);
}

void SdMmc::writeCmdParam(uint32_t mask, uint32_t value) {
    // Write to the SD_CMD_PARAM parameter
    sdCmdParam = (sdCmdParam & ~mask) | (value & mask);
}

void SdMmc::writeData16Blkcnt(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA16_BLKCNT block count
    sdData16Blkcnt = (sdData16Blkcnt & ~mask) | (value & mask);
}

void SdMmc::writeIrqStatus(uint32_t mask, uint32_t value) {
    // Acknowledge bits in the SD_IRQ_STATUS register
    sdIrqStatus = (sdIrqStatus & ~mask) | (sdIrqStatus & value & mask);
}

void SdMmc::writeIrqMask(uint32_t mask, uint32_t value) {
    // Write to the SD_IRQ_MASK register
    mask &= 0x8B7F031D;
    sdIrqMask = (sdIrqMask & ~mask) | (value & mask);
}

void SdMmc::writeData16Blklen(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA16_BLKLEN block length, clipping past 0x200
    mask &= 0x3FF;
    sdData16Blklen = std::min(0x200, (sdData16Blklen & ~mask) | (value & mask));
}

void SdMmc::writeDataCtl(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA_CTL register
    mask &= 0x22;
    sdDataCtl = (sdDataCtl & ~mask) | (value & mask);
}

void SdMmc::writeData32Irq(uint16_t mask, uint16_t value) {
    // Empty the 32-bit FIFO if requested
    if (value & mask & BIT(10)) {
        sdData32Irq &= ~BIT(8); // Not full
        readFifo32 = {};
    }

    // Write to the SD_DATA32_IRQ register
    mask &= 0x1802;
    sdData32Irq = (sdData32Irq & ~mask) | (value & mask);
}

void SdMmc::writeData32Blklen(uint16_t mask, uint16_t value) {
    // Write to the SD_DATA32_BLKLEN block length
    mask &= 0x3FF;
    sdData32Blklen = (sdData32Blklen & ~mask) | (value & mask);
}
