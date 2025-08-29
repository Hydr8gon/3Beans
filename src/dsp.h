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
#include <queue>

class Core;

class Dsp {
public:
    Dsp(Core *core): core(core) {}
    void resetCycles();

    uint16_t readData(uint16_t address);
    void writeData(uint16_t address, uint16_t value);

    void underflowTmr(int i);
    void unsignalTmr(int i);
    void sendAudio();
    void setAudClock(DspClock clock);
    uint32_t getIcuVector();
    void updateIcuState();

    uint16_t readPdata();
    uint16_t readPcfg() { return dspPcfg; }
    uint16_t readPsts() { return dspPsts; }
    uint16_t readPsem() { return dspPsem; }
    uint16_t readPmask() { return dspPmask; }
    uint16_t readSem() { return dspSem; }
    uint16_t readCmd(int i) { return dspCmd[i]; }
    uint16_t readRep(int i);

    void writePdata(uint16_t mask, uint16_t value);
    void writePadr(uint16_t mask, uint16_t value);
    void writePcfg(uint16_t mask, uint16_t value);
    void writePsem(uint16_t mask, uint16_t value);
    void writePmask(uint16_t mask, uint16_t value);
    void writePclear(uint16_t mask, uint16_t value);
    void writeCmd(int i, uint16_t mask, uint16_t value);

private:
    Core *core;

    std::queue<uint16_t> audOutFifo;
    std::queue<uint16_t> readFifo;
    uint8_t readLength = 0;

    uint64_t tmrCycles[2] = { -1UL, -1UL };
    uint32_t tmrLatches[2] = {};
    bool tmrSignals[2] = {};
    uint32_t miuBoundX = 0x8000;
    uint32_t miuBoundY = 0x10000;
    bool dmaSignals[3] = {};
    uint16_t icuState = 0;
    uint32_t audCycles = 0;
    bool audScheduled = false;

    uint16_t dspPadr = 0;
    uint16_t dspPcfg = 0x1;
    uint16_t dspPsts = 0x100;
    uint16_t dspPsem = 0;
    uint16_t dspPmask = 0;
    uint16_t dspSem = 0;
    uint16_t dspCmd[3] = {};
    uint16_t dspRep[3] = {};

    uint16_t tmrCtrl[2] = {};
    uint32_t tmrReload[2] = {};
    uint32_t tmrCount[2] = {};
    uint16_t hpiMask = 0;
    uint16_t hpiCfg = 0;
    uint16_t hpiSts = 0;
    uint16_t miuPageX = 0;
    uint16_t miuPageY = 0;
    uint16_t miuPageZ = 0;
    uint16_t miuSize0 = 0x2020;
    uint16_t miuMisc = 0x14;
    uint16_t miuIoBase = 0x8000;
    uint16_t dmaStart = 0x1;
    uint16_t dmaEnd[3] = {};
    uint16_t dmaSelect = 0;
    uint32_t dmaSrcAddr[8] = {};
    uint32_t dmaDstAddr[8] = {};
    uint16_t dmaSize[3][8] = { {1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1} };
    uint16_t dmaAreaCfg[8] = {};
    uint16_t dmaCtrl[8] = {};
    uint16_t icuPending = 0;
    uint16_t icuTrigger = 0;
    uint16_t icuEnable[4] = {};
    uint16_t icuMode = 0;
    uint32_t icuVector[16] = { 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00,
        0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00, 0x3FC00 };
    uint16_t icuDisable = 0;
    uint16_t audOutCtrl = 0;
    uint16_t audOutEnable = 0;
    uint16_t audOutStatus = 0;
    uint16_t audOutFlush = 0;

    uint32_t getMiuAddr(uint16_t address);
    void scheduleTmr(int i);
    void flushAudOut();

    uint16_t dmaRead(uint8_t area, uint32_t address);
    void dmaWrite(uint8_t area, uint32_t address, uint16_t value);
    void dmaTransfer(int i);

    void updateArmSemIrq();
    void updateDspSemIrq();
    void updateReadFifo();

    uint32_t readTmrCount(int i);
    uint16_t readHpiCmd(int i);
    uint16_t readDmaEnd(int i);

    void writeTmrCtrl(int i, uint16_t value);
    void writeTmrEvent(int i, uint16_t value);
    void writeTmrReloadL(int i, uint16_t value);
    void writeTmrReloadH(int i, uint16_t value);
    void writeHpiRep(int i, uint16_t value);
    void writeHpiSem(uint16_t value);
    void writeHpiMask(uint16_t value);
    void writeHpiClear(uint16_t value);
    void writeHpiCfg(uint16_t value);
    void writeMiuPageX(uint16_t value);
    void writeMiuPageY(uint16_t value);
    void writeMiuPageZ(uint16_t value);
    void writeMiuSize0(uint16_t value);
    void writeMiuMisc(uint16_t value);
    void writeMiuIoBase(uint16_t value);
    void writeDmaStart(uint16_t value);
    void writeDmaSelect(uint16_t value);
    void writeDmaSrcAddrL(uint16_t value);
    void writeDmaSrcAddrH(uint16_t value);
    void writeDmaDstAddrL(uint16_t value);
    void writeDmaDstAddrH(uint16_t value);
    void writeDmaSize(int i, uint16_t value);
    void writeDmaAreaCfg(uint16_t value);
    void writeDmaCtrl(uint16_t value);
    void writeIcuAck(uint16_t value);
    void writeIcuTrigger(uint16_t value);
    void writeIcuEnable(int i, uint16_t value);
    void writeIcuMode(uint16_t value);
    void writeIcuVectorL(int i, uint16_t value);
    void writeIcuVectorH(int i, uint16_t value);
    void writeIcuDisable(uint16_t value);
    void writeAudOutCtrl(uint16_t value);
    void writeAudOutEnable(uint16_t value);
    void writeAudOutFifo(uint16_t value);
    void writeAudOutFlush(uint16_t value);
};
