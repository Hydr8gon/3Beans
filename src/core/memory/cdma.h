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

enum CdmaId {
    CDMA0,
    CDMA1,
    XDMA
};

class Cdma {
public:
    Cdma(Core *core, CdmaId id);

    void setDrq(uint8_t type);
    void clearDrq(uint8_t type);
    void update();

    uint32_t readInten() { return inten; }
    uint32_t readIntEventRis() { return intEventRis; }
    uint32_t readIntmis() { return intmis; }
    uint32_t readFsrd() { return fsrc >> 8; }
    uint32_t readFsrc() { return fsrc & 0xFF; }
    uint32_t readFtr(int i) { return ftrs[i]; }
    uint32_t readCpc(int i) { return cpcs[i]; }
    uint32_t readCsr(int i) { return csrs[i]; }
    uint32_t readSar(int i) { return sars[i]; }
    uint32_t readDar(int i) { return dars[i]; }
    uint32_t readCcr(int i) { return ccrs[i]; }
    uint32_t readLc0(int i) { return lc0s[i]; }
    uint32_t readLc1(int i) { return lc1s[i]; }
    uint32_t readDbgstatus() { return dbgstatus; }
    uint32_t readDbginst0() { return dbginst0; }
    uint32_t readDbginst1() { return dbginst1; }

    void writeInten(uint32_t mask, uint32_t value);
    void writeIntclr(uint32_t mask, uint32_t value);
    void writeDbgcmd(uint32_t mask, uint32_t value);
    void writeDbginst0(uint32_t mask, uint32_t value);
    void writeDbginst1(uint32_t mask, uint32_t value);

private:
    Core *core;
    CdmaId id;
    CpuId cpu;

    std::queue<uint8_t> fifos[8];
    uint32_t drqMask = 0;
    uint8_t dbgId = 0;
    bool burstReq = false;
    bool scheduled = false;

    uint32_t inten = 0;
    uint32_t intEventRis = 0;
    uint32_t intmis = 0;
    uint32_t fsrc = 0;
    uint32_t ftrs[9] = {};
    uint32_t csrs[9] = {};
    uint32_t cpcs[9] = {};
    uint32_t sars[8] = {};
    uint32_t dars[8] = {};
    uint32_t ccrs[8] = {};
    uint32_t lc0s[8] = {};
    uint32_t lc1s[8] = {};
    uint32_t dbgstatus = 0;
    uint32_t dbginst0 = 0;
    uint32_t dbginst1 = 0;

    void triggerUpdate();
    void runOpcodes(int i);
    void fault(int i, int type);
    void dmaStubC(int i, int inc);

    void dmaEnd(int i);
    void dmaLd(int i);
    void dmaLds(int i);
    void dmaLdb(int i);
    void dmaSt(int i);
    void dmaSts(int i);
    void dmaStb(int i);
    void dmaLp0(int i);
    void dmaLp1(int i);
    void dmaLdps(int i);
    void dmaLdpb(int i);
    void dmaStps(int i);
    void dmaStpb(int i);
    void dmaWfp(int i, uint16_t burst);
    void dmaSev(int i);
    void dmaLpend0(int i);
    void dmaLpend1(int i);
    void dmaAddhS(int i);
    void dmaAddhD(int i);
    void dmaGoNs(int i);
    void dmaMov(int i);
};
