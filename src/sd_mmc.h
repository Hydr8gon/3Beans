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

#pragma once

#include <cstdint>
#include <queue>

class Core;

class SdMmc {
public:
    SdMmc(Core *core): core(core) {}
    ~SdMmc();
    void loadFiles();

    uint16_t readCmd() { return sdCmd; }
    uint16_t readPortSelect() { return sdPortSelect; }
    uint32_t readCmdParam() { return sdCmdParam; }
    uint16_t readData16Blkcnt() { return sdData16Blkcnt; }
    uint32_t readResponse(int i) { return sdResponse[i]; }
    uint32_t readIrqStatus() { return sdIrqStatus; }
    uint32_t readIrqMask() { return sdIrqMask; }
    uint16_t readData16Blklen() { return sdData16Blklen; }
    uint16_t readData16Fifo();
    uint16_t readDataCtl() { return sdDataCtl; }
    uint16_t readData32Irq() { return sdData32Irq; }
    uint32_t readData32Fifo();

    void writeCmd(uint16_t mask, uint16_t value);
    void writePortSelect(uint16_t mask, uint16_t value);
    void writeCmdParam(uint32_t mask, uint32_t value);
    void writeData16Blkcnt(uint16_t mask, uint16_t value);
    void writeIrqStatus(uint32_t mask, uint32_t value);
    void writeIrqMask(uint32_t mask, uint32_t value);
    void writeData16Blklen(uint16_t mask, uint16_t value);
    void writeDataCtl(uint16_t mask, uint16_t value);
    void writeData32Irq(uint16_t mask, uint16_t value);

private:
    Core *core;
    FILE *nand = nullptr;
    FILE *sd = nullptr;

    uint32_t cardStatus = 0;
    uint32_t blockLen = 0;
    uint32_t curAddress = 0;
    uint16_t curBlock = 0;

    std::queue<uint16_t> readFifo16;
    std::queue<uint32_t> readFifo32;

    uint32_t mmcCid[0x4] = {};
    uint16_t sdCmd = 0;
    uint16_t sdPortSelect = 0;
    uint32_t sdCmdParam = 0;
    uint16_t sdData16Blkcnt = 0;
    uint32_t sdResponse[4] = {};
    uint32_t sdIrqStatus = 0;
    uint32_t sdIrqMask = 0;
    uint16_t sdData16Blklen = 0;
    uint16_t sdData16Fifo = 0;
    uint32_t sdDataCtl = 0x1010;
    uint16_t sdData32Irq = 0;
    uint32_t sdData32Fifo = 0;

    void sendInterrupt(int bit);
    void pushFifo(uint32_t value);
    void pushResponse(uint32_t value);
    void readBlock();
    void runCommand();
    void runAppCommand();

    void getCid();
    void getStatus();
    void setBlocklen();
    void readSingleBlock();
    void readMultiBlock();
    void appCmd();
    void sdStatus();
    void getScr();
};
