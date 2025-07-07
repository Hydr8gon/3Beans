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

const uint8_t Wifi::func0Reg0[] = {
    0x11, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x82, // 0x00-0x07
    0x17, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x08-0x0F
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 // 0x10-0x16
};

const uint8_t Wifi::func0Reg1[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x00-0x07
    0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x08-0x0F
    0x80, 0x00 // 0x10-0x11
};

const uint8_t Wifi::func0Cis0[] = {
    0x01, 0x03, 0xD9, 0x01, 0xFF, 0x20, 0x04, 0x71, // 0x00-0x07
    0x02, 0x00, 0x02, 0x21, 0x02, 0x0C, 0x00, 0x22, // 0x08-0x0F
    0x04, 0x00, 0x00, 0x08, 0x32, 0x1A, 0x05, 0x01, // 0x10-0x17
    0x01, 0x00, 0x02, 0x07, 0x1B, 0x08, 0xC1, 0x41, // 0x18-0x1F
    0x30, 0x30, 0xFF, 0xFF, 0x32, 0x00, 0x14, 0x00, // 0x20-0x27
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x28-0x2F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x30-0x37
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x38-0x3F
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF // 0x40-0x44
};

const uint8_t Wifi::func0Cis1[] = {
    0x20, 0x04, 0x71, 0x02, 0x00, 0x02, 0x21, 0x02, // 0x00-0x07
    0x0C, 0x00, 0x22, 0x2A, 0x01, 0x01, 0x11, 0x00, // 0x08-0x0F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x10-0x17
    0x00, 0x08, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, // 0x18-0x1F
    0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, // 0x20-0x27
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // 0x28-0x2F
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x80, 0x01, // 0x30-0x37
    0x06, 0x81, 0x01, 0x07, 0x82, 0x01, 0xDF, 0xFF, // 0x38-0x3F
    0x01, 0x00 // 0x40-0x41
};

void Wifi::sendInterrupt(int bit) {
    // Set an external interrupt request bit
    wifiIrqStatus |= BIT(bit);

    // Send an interrupt to the ARM11 if conditions are met
    if (wifiIrqStatus & ~wifiIrqMask)
        core->interrupts.sendInterrupt(ARM11, 0x40);
}

void Wifi::startWrite() {
    // Change to write state and trigger an initial DRQ
    cardStatus = (cardStatus & ~0x1E00) | (0x6 << 9);
    core->cdmas[CDMA0].sendInterrupt(0x4);
    core->cdmas[CDMA1].sendInterrupt(0x4);

    // Write a block right away if the FIFO is full, or trigger a FIFO empty interrupt
    if (wifiDataCtl & BIT(1)) { // 32-bit
        if ((dataFifo32.size() << 2) >= wifiData32Blklen)
            core->schedule(WIFI_WRITE_BLOCK, 1);
        else if (dataFifo32.empty() && (wifiData32Irq & BIT(12)))
            core->interrupts.sendInterrupt(ARM11, 0x40);
    }
    else { // 16-bit
        if ((dataFifo16.size() << 1) >= wifiData16Blklen)
            core->schedule(WIFI_WRITE_BLOCK, 1);
        else if (dataFifo16.empty())
            sendInterrupt(25);
    }
}

uint32_t Wifi::popFifo() {
    // Handle 32-bit FIFO mode
    if (wifiDataCtl & BIT(1)) {
        // Pop a value from the 32-bit FIFO and check if empty
        if (dataFifo32.empty()) return 0;
        uint32_t value = dataFifo32.front();
        dataFifo32.pop();
        wifiData32Irq &= ~BIT(8); // Not full
        if (!dataFifo32.empty()) return value;

        // Trigger a 32-bit FIFO empty interrupt if enabled and finish a block
        wifiData32Irq &= ~BIT(9); // Empty
        if (wifiData32Irq & BIT(12))
            core->interrupts.sendInterrupt(ARM11, 0x40);
        curBlock--;
        return value;
    }

    // Pop two values from the 16-bit FIFO and check if empty
    if (dataFifo16.empty()) return 0;
    uint32_t value = dataFifo16.front();
    dataFifo16.pop();
    if (!dataFifo16.empty()) {
        value |= dataFifo16.front() << 16;
        dataFifo16.pop();
    }
    if (!dataFifo16.empty()) return value;

    // Trigger a 16-bit FIFO empty interrupt and finish a block
    sendInterrupt(25);
    curBlock--;
    return value;
}

void Wifi::pushFifo(uint32_t value) {
    // Handle 32-bit FIFO mode
    if (wifiDataCtl & BIT(1)) {
        // Push a value to the 32-bit FIFO and check if full
        uint32_t size = (dataFifo32.size() << 2);
        if (size >= 0x200) return;
        dataFifo32.push(value);
        wifiData32Irq |= BIT(9); // Not empty
        if (size + 4 < wifiData32Blklen) return;

        // Trigger a 32-bit FIFO full interrupt if enabled and finish a block
        wifiData32Irq |= BIT(8);
        if (wifiData32Irq & BIT(11))
            core->interrupts.sendInterrupt(ARM11, 0x40);
        curBlock--;
        return;
    }

    // Push two values to the 16-bit FIFO and check if full
    uint32_t size = (dataFifo16.size() << 1);
    if (size >= 0x200) return;
    dataFifo16.push(value);
    dataFifo16.push(value >> 16);
    if (size + 4 < wifiData16Blklen) return;

    // Trigger a 16-bit FIFO full interrupt and finish a block
    sendInterrupt(24);
    curBlock--;
}

void Wifi::pushResponse(uint32_t value) {
    // Push a value to the response registers, shifting old data over
    wifiResponse[3] = wifiResponse[2];
    wifiResponse[2] = wifiResponse[1];
    wifiResponse[1] = wifiResponse[0];
    wifiResponse[0] = value;
}

void Wifi::readBlock() {
    // Read a block of data from the selected WiFi function
    for (int i = 0; i < blockLen; i += 4) {
        uint32_t value = 0;
        for (int j = 0; j < 4 && i + j < blockLen; j++, curAddress += curInc)
            value |= ioRead(curFunc, curAddress) << (j * 8);
        pushFifo(value);
    }

    // Change to read or idle state based on blocks left and trigger a DRQ
    cardStatus = (cardStatus & ~0x1E00) | ((curBlock ? 0x5 : 0x4) << 9);
    curAddress += blockLen;
    core->cdmas[CDMA0].sendInterrupt(0x4);
    core->cdmas[CDMA1].sendInterrupt(0x4);
}

void Wifi::writeBlock() {
    // Write a block of data to the selected WiFi function
    for (int i = 0; i < blockLen; i += 4) {
        uint32_t value = popFifo();
        for (int j = 0; j < 4 && i + j < blockLen; j++, curAddress += curInc)
            ioWrite(curFunc, curAddress, value >> (j * 8));
    }

    // Change state and trigger a DRQ or end interrupt based on blocks left
    cardStatus = (cardStatus & ~0x1E00) | ((curBlock ? 0x6 : 0x4) << 9);
    curAddress += blockLen;
    if (!curBlock) return sendInterrupt(2);
    core->cdmas[CDMA0].sendInterrupt(0x4);
    core->cdmas[CDMA1].sendInterrupt(0x4);
}

uint8_t Wifi::ioRead(uint8_t func, uint32_t address) {
    // Read from an I/O address in a WiFi function
    switch (func) {
    case 0:
        // Read from the function 0 register areas
        if (address < 0x17)
            return func0Reg0[address & 0xFF];
        else if (address >= 0x100 && address < 0x112)
            return func0Reg1[address & 0xFF];

        // Read from the function 0 mirrored CIS areas
        if (address >= 0x1000 && address < 0x3000) {
            address &= 0x1FF;
            if (address < 0x45)
                return func0Cis0[address & 0xFF];
            else if (address >= 0x100 && address < 0x142)
                return func0Cis1[address & 0xFF];
        }
        return 0;

    case 1:
        // Read from the function 1 MBOX receive FIFOs
        if (address < 0x400)
            return readMbox<uint8_t>((address >> 8) + 4);
        else if (address >= 0x800 && address < 0x2800)
            return readMbox<uint8_t>((address >> 11) + 3);
        else if (address >= 0x2800 && address < 0x4000)
            return readMbox<uint8_t>(4);

        // Read from a regular function 1 register
        switch (address) {
        case 0x474: case 0x475: case 0x476: case 0x477:
            return winData >> ((address - 0x474) * 8);
        }
        break;
    }

    // Catch reads from unknown function addresses
    LOG_WARN("Unknown WiFi function %d read: 0x%X\n", func, address);
    return 0;
}

void Wifi::ioWrite(uint8_t func, uint32_t address, uint8_t value) {
    // Write to an I/O address in a WiFi function
    switch (func) {
    case 0:
        // Ignore writes to function 0 for now
        LOG_WARN("Unhandled WiFi function 0 write: 0x%X\n", address);
        return;

    case 1:
        // Write to the function 1 MBOX send FIFOs
        if (address < 0x400)
            return writeMbox(address >> 8, value, (address & 0xFF) == 0xFF);
        else if (address >= 0x800 && address < 0x2800)
            return writeMbox((address >> 11) - 1, value, (address & 0x7FF) == 0x7FF);
        else if (address >= 0x2800 && address < 0x4000)
            return writeMbox(0, value, address == 0x3FFF);

        // Write to a regular function 1 register
        switch (address) {
        case 0x474: case 0x475: case 0x476: case 0x477:
            return writeWinData(address - 0x474, value);
        case 0x478: case 0x479: case 0x47A: case 0x47B:
            return writeWinWriteAddr(address - 0x478, value);
        case 0x47C: case 0x47D: case 0x47E: case 0x47F:
            return writeWinReadAddr(address - 0x47C, value);
        }
        break;
    }

    // Catch writes to unknown function addresses
    LOG_WARN("Unknown WiFi function %d write: 0x%X\n", func, address);
}

uint32_t Wifi::readXtensa(uint32_t address) {
    // Read a value from internal Xtensa memory
    switch (address) {
        case 0x520058: return boardInited;

    default:
        // Catch reads from unknown memory addresses
        LOG_WARN("Unknown Xtensa WiFi memory read: 0x%X\n", address);
        return 0;
    }
}

void Wifi::writeXtensa(uint32_t address, uint32_t value) {
    // Stub writes to internal Xtensa memory for now
    LOG_WARN("Unhandled Xtensa WiFi memory write: 0x%X\n", address);
}

template <typename T> T Wifi::readMbox(int i) {
    // Read an LSB-first value from one of the MBOX FIFOs
    // TODO: handle receive underflow errors
    T value = 0;
    for (int j = 0; j < sizeof(T); j++) {
        if (mboxes[i].empty()) break;
        value |= mboxes[i].front() << (i * 8);
        mboxes[i].pop();
    }
    return value;
}

void Wifi::writeMbox(int i, uint8_t value, bool last) {
    // Push an 8-bit value to an MBOX FIFO and send on the last byte
    // TODO: handle send overflow errors
    mboxes[i].push(value);
    if (!last) return;

    // Stub WiFi command execution when data is sent
    if (boardInited) {
        LOG_WARN("Unhandled WiFi command sent in WMI mode\n");
    }
    else {
        uint32_t cmd = readMbox<uint32_t>(i);
        if (cmd == 0x1) // BMI_DONE
            boardInited = 1;
        else
            LOG_WARN("Unknown WiFi BMI command: 0x%X\n", cmd);
    }

    // Ensure the MBOX is empty after command execution
    mboxes[i] = {};
}

void Wifi::writeWinData(int i, uint8_t value) {
    // Write to part of the window data register
    winData = (winData & ~(0xFF << (i * 8))) | (value << (i * 8));
}

void Wifi::writeWinWriteAddr(int i, uint8_t value) {
    // Write to the window write address and initiate a transfer on the LSB
    winWriteAddr = (winWriteAddr & ~(0xFF << (i * 8))) | (value << (i * 8));
    if (i == 0) writeXtensa(winWriteAddr, winData);
}

void Wifi::writeWinReadAddr(int i, uint8_t value) {
    // Write to the window read address and initiate a transfer on the LSB
    winReadAddr = (winReadAddr & ~(0xFF << (i * 8))) | (value << (i * 8));
    if (i == 0) winData = readXtensa(winReadAddr);
}

void Wifi::ioRwDirect() {
    // Write a single byte to a WiFi function
    if (wifiCmdParam & BIT(31)) // Write
        ioWrite((wifiCmdParam >> 28) & 0x7, (wifiCmdParam >> 9) & 0x1FFFF, wifiCmdParam & 0xFF);

    // Read a single byte from a WiFi function
    if (!(wifiCmdParam & BIT(31)) || (wifiCmdParam & BIT(27))) // Read or read-after-write
        pushResponse(ioRead((wifiCmdParam >> 28) & 0x7, (wifiCmdParam >> 9) & 0x1FFFF));
    else
        pushResponse(wifiCmdParam & 0xFF);
}

void Wifi::ioRwExtended() {
    // Set parameters for a multi-byte function read/write
    blockLen = (wifiCmdParam & BIT(27)) ? 0x200 : ((wifiCmdParam & 0x1FF) ? (wifiCmdParam & 0x1FF) : 0x200);
    curAddress = (wifiCmdParam >> 9) & 0x1FFFF;
    curBlock = (wifiCmdParam & BIT(27)) ? (wifiCmdParam & 0x1FF) : 1;
    curFunc = (wifiCmdParam >> 28) & 0x7;
    curInc = (wifiCmdParam & BIT(26));

    // Initiate the WiFi read/write
    if (wifiCmdParam & BIT(31)) // Write
        startWrite();
    else // Read
        core->schedule(WIFI_READ_BLOCK, 1);
    pushResponse(0);
}

uint16_t Wifi::readData16Fifo() {
    // Pop a value from the 16-bit FIFO and check if empty
    if (dataFifo16.empty()) return wifiData16Fifo;
    wifiData16Fifo = dataFifo16.front();
    dataFifo16.pop();
    if (!dataFifo16.empty()) return wifiData16Fifo;

    // Trigger a 16-bit FIFO empty interrupt and read another block or finish with a data end interrupt
    sendInterrupt(25);
    (cardStatus & 0x1E00) == (0x5 << 9) ? core->schedule(WIFI_READ_BLOCK, 1) : sendInterrupt(2);
    return wifiData16Fifo;
}

uint32_t Wifi::readData32Fifo() {
    // Pop a value from the 32-bit read FIFO and check if empty
    if (dataFifo32.empty()) return wifiData32Fifo;
    wifiData32Fifo = dataFifo32.front();
    dataFifo32.pop();
    wifiData32Irq &= ~BIT(8); // Not full
    if (!dataFifo32.empty()) return wifiData32Fifo;

    // Trigger a 32-bit FIFO empty interrupt if enabled
    wifiData32Irq &= ~BIT(9);
    if (wifiData32Irq & BIT(12))
        core->interrupts.sendInterrupt(ARM11, 0x40);

    // Read another block or finish with a data end interrupt
    (cardStatus & 0x1E00) == (0x5 << 9) ? core->schedule(WIFI_READ_BLOCK, 1) : sendInterrupt(2);
    return wifiData32Fifo;
}

void Wifi::writeCmd(uint16_t mask, uint16_t value) {
    // Write to the WIFI_CMD register
    // TODO: handle response type and other bits
    wifiCmd = (wifiCmd & ~mask) | (value & mask);

    // Run the written command and send an interrupt for completion
    sendInterrupt(0);
    switch (uint8_t cmd = wifiCmd & 0x3F) {
        case 52: return ioRwDirect(); // IO_RW_DIRECT
        case 53: return ioRwExtended(); // IO_RW_EXTENDED

    default:
        // Assume response R1 for unknown commands
        LOG_WARN("Unknown WiFi command: CMD%d\n", cmd);
        return pushResponse(cardStatus);
    }
}

void Wifi::writeCmdParam(uint32_t mask, uint32_t value) {
    // Write to the WIFI_CMD_PARAM parameter
    wifiCmdParam = (wifiCmdParam & ~mask) | (value & mask);
}

void Wifi::writeIrqStatus(uint32_t mask, uint32_t value) {
    // Acknowledge bits in the WIFI_IRQ_STATUS register
    mask &= ~BIT(5); // Always set
    wifiIrqStatus = (wifiIrqStatus & ~mask) | (wifiIrqStatus & value & mask);
}

void Wifi::writeIrqMask(uint32_t mask, uint32_t value) {
    // Write to the WIFI_IRQ_MASK register
    mask &= 0x8B7F031D;
    wifiIrqMask = (wifiIrqMask & ~mask) | (value & mask);
}

void Wifi::writeData16Blklen(uint16_t mask, uint16_t value) {
    // Write to the WIFI_DATA16_BLKLEN block length, clipping past 0x200
    mask &= 0x3FF;
    wifiData16Blklen = std::min(0x200, (wifiData16Blklen & ~mask) | (value & mask));
}

void Wifi::writeData16Fifo(uint16_t mask, uint16_t value) {
    // Push a value to the 16-bit FIFO and check if full
    uint32_t size = (dataFifo16.size() << 1);
    if (size >= 0x200) return;
    dataFifo16.push(value & mask);
    if (size + 2 < wifiData16Blklen) return;

    // Trigger a 16-bit FIFO full interrupt and write a block if in write mode
    sendInterrupt(24);
    if ((cardStatus & 0x1E00) == (0x6 << 9))
        core->schedule(WIFI_WRITE_BLOCK, 1);
}

void Wifi::writeDataCtl(uint16_t mask, uint16_t value) {
    // Write to the WIFI_DATA_CTL register
    mask &= 0x22;
    wifiDataCtl = (wifiDataCtl & ~mask) | (value & mask);
}

void Wifi::writeData32Irq(uint16_t mask, uint16_t value) {
    // Empty the 32-bit FIFO if requested
    if (value & mask & BIT(10)) {
        wifiData32Irq &= ~(BIT(8) | BIT(9)); // Not full, empty
        dataFifo32 = {};
    }

    // Write to the WIFI_DATA32_IRQ register
    mask &= 0x1802;
    wifiData32Irq = (wifiData32Irq & ~mask) | (value & mask);
}

void Wifi::writeData32Blklen(uint16_t mask, uint16_t value) {
    // Write to the WIFI_DATA32_BLKLEN block length
    mask &= 0x3FF;
    wifiData32Blklen = (wifiData32Blklen & ~mask) | (value & mask);
}

void Wifi::writeData32Fifo(uint32_t mask, uint32_t value) {
    // Push a value to the 32-bit FIFO and check if full
    uint32_t size = (dataFifo32.size() << 2);
    if (size >= 0x200) return;
    dataFifo32.push(value & mask);
    wifiData32Irq |= BIT(9); // Not empty
    if (size + 4 < wifiData32Blklen) return;

    // Trigger a 32-bit FIFO full interrupt if enabled
    wifiData32Irq |= BIT(8);
    if (wifiData32Irq & BIT(11))
        core->interrupts.sendInterrupt(ARM11, 0x40);

    // Write a block if in write mode
    if ((cardStatus & 0x1E00) == (0x6 << 9))
        core->schedule(WIFI_WRITE_BLOCK, 1);
}
