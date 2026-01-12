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

#pragma once

#include <cstdint>
#include <deque>
#include <queue>

class Core;

class Wifi {
public:
    Wifi(Core &core): core(core) {}

    void readBlock();
    void writeBlock();

    uint16_t readCmd() { return wifiCmd; }
    uint32_t readCmdParam() { return wifiCmdParam; }
    uint32_t readResponse(int i) { return wifiResponse[i]; }
    uint32_t readIrqStatus() { return wifiIrqStatus; }
    uint32_t readIrqMask() { return wifiIrqMask; }
    uint16_t readData16Blklen() { return wifiData16Blklen; }
    uint32_t readErrDetail() { return wifiErrDetail; }
    uint16_t readData16Fifo();
    uint16_t readCardIrqStat() { return wifiCardIrqStat; }
    uint16_t readCardIrqMask() { return wifiCardIrqMask; }
    uint16_t readDataCtl() { return wifiDataCtl; }
    uint16_t readData32Irq() { return wifiData32Irq; }
    uint16_t readData32Blklen() { return wifiData32Blklen; }
    uint32_t readData32Fifo();

    void writeCmd(uint16_t mask, uint16_t value);
    void writeCmdParam(uint32_t mask, uint32_t value);
    void writeIrqStatus(uint32_t mask, uint32_t value);
    void writeIrqMask(uint32_t mask, uint32_t value);
    void writeData16Blklen(uint16_t mask, uint16_t value);
    void writeData16Fifo(uint16_t mask, uint16_t value);
    void writeCardIrqStat(uint16_t mask, uint16_t value);
    void writeCardIrqMask(uint16_t mask, uint16_t value);
    void writeDataCtl(uint16_t mask, uint16_t value);
    void writeData32Irq(uint16_t mask, uint16_t value);
    void writeData32Blklen(uint16_t mask, uint16_t value);
    void writeData32Fifo(uint32_t mask, uint32_t value);

private:
    Core &core;

    static const uint8_t func0Reg0[0x17];
    static const uint8_t func0Reg1[0x12];
    static const uint8_t func0Cis0[0x45];
    static const uint8_t func0Cis1[0x42];

    uint32_t cardStatus = 0;
    uint32_t blockLen = 0;
    uint32_t curAddress = 0;
    uint16_t curBlock = 0;
    uint8_t curFunc = 0;
    bool curInc = false;

    uint32_t xtensaRam[0xC0000] = {};
    uint8_t bootStage = 0;

    std::queue<uint16_t> dataFifo16;
    std::queue<uint32_t> dataFifo32;
    std::deque<uint8_t> mboxes[8];

    uint8_t f0IntStat = 0;
    uint8_t f0IntMask = 0;
    uint8_t rxLookValid = 0;
    uint32_t f1IntStat = 0;
    uint32_t f1IntMask = 0;
    uint32_t winData = 0;
    uint32_t winWriteAddr = 0;
    uint32_t winReadAddr = 0;

    uint16_t wifiCmd = 0;
    uint32_t wifiCmdParam = 0;
    uint32_t wifiResponse[4] = {};
    uint32_t wifiIrqStatus = 0x20;
    uint32_t wifiIrqMask = 0;
    uint16_t wifiData16Blklen = 0;
    uint32_t wifiErrDetail = 0;
    uint16_t wifiData16Fifo = 0;
    uint16_t wifiCardIrqStat = 0;
    uint16_t wifiCardIrqMask = 0;
    uint32_t wifiDataCtl = 0x1010;
    uint16_t wifiData32Irq = 0;
    uint32_t wifiData32Blklen = 0;
    uint32_t wifiData32Fifo = 0;

    void extInterrupt(int bit);
    void cardInterrupt(int bit);
    void f0Interrupt(int bit);
    void f1Interrupt(int bit);

    void startWrite();
    uint32_t popFifo();
    void pushFifo(uint32_t value);
    void pushResponse(uint32_t value);

    uint8_t ioRead(uint8_t func, uint32_t address);
    void ioWrite(uint8_t func, uint32_t address, uint8_t value);
    uint32_t readXtensa(uint32_t address);
    void writeXtensa(uint32_t address, uint32_t value);

    void bmiDone(int i);
    void bmiReadMemory(int i);
    void bmiWriteMemory(int i);
    void bmiExecute(int i);
    void bmiReadRegister(int i);
    void bmiWriteRegister(int i);
    void bmiTargetInfo(int i);
    void htcConnectService(int i);
    void htcSetupComplete(int i);
    void wmiGetChanList(int i);

    template <typename T> T readMbox(int i);
    uint8_t readRxLookahead(int i, int j);

    void writeF0IntMask(uint8_t value);
    template <typename T> void writeMbox(int i, T value, bool last = false);
    void writeF1IntMask(int i, uint8_t value);
    void writeWinData(int i, uint8_t value);
    void writeWinWriteAddr(int i, uint8_t value);
    void writeWinReadAddr(int i, uint8_t value);

    void ioRwDirect();
    void ioRwExtended();
};
