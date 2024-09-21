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
    bool loadFiles();

    uint32_t readOtpEncrypted(int i) { return otpEncrypted[i]; }
    uint16_t readSdCmd() { return sdCmd; }
    uint16_t readSdPortSelect() { return sdPortSelect; }
    uint32_t readSdCmdParam() { return sdCmdParam; }
    uint16_t readSdData16Blkcnt() { return sdData16Blkcnt; }
    uint32_t readSdResponse(int i) { return sdResponse[i]; }
    uint32_t readSdIrqStatus() { return sdIrqStatus; }
    uint32_t readSdIrqMask() { return sdIrqMask; }
    uint16_t readSdData16Blklen() { return sdData16Blklen; }
    uint16_t readSdData16Fifo();
    uint16_t readSdDataCtl() { return sdDataCtl; }
    uint16_t readSdData32Irq() { return sdData32Irq; }
    uint32_t readSdData32Fifo();

    void writeSdCmd(uint16_t mask, uint16_t value);
    void writeSdPortSelect(uint16_t mask, uint16_t value);
    void writeSdCmdParam(uint32_t mask, uint32_t value);
    void writeSdData16Blkcnt(uint16_t mask, uint16_t value);
    void writeSdIrqStatus(uint32_t mask, uint32_t value);
    void writeSdIrqMask(uint32_t mask, uint32_t value);
    void writeSdData16Blklen(uint16_t mask, uint16_t value);
    void writeSdDataCtl(uint16_t mask, uint16_t value);
    void writeSdData32Irq(uint16_t mask, uint16_t value);

private:
    Core *core;
    FILE *nand = nullptr;

    uint32_t cardStatus = 0;
    uint32_t blockLen = 0;
    uint32_t curAddress = 0;
    uint16_t curBlock = 0;

    std::queue<uint16_t> readFifo16;
    std::queue<uint32_t> readFifo32;

    uint32_t otpEncrypted[0x40] = {};
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

    void setRelativeAddr();
    void setBlocklen();
    void readSingleBlock();
    void readMultiBlock();
    void appCmd();
    void sdStatus();
    void sendOpCond();
    void getScr();
};
