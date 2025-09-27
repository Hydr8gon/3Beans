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

class Core;

class Ndma {
public:
    Ndma(Core *core): core(core) {}

    void setDrq(uint8_t type);
    void clearDrq(uint8_t type);
    void update();

    uint32_t readSad(int i) { return ndmaSad[i]; }
    uint32_t readDad(int i) { return ndmaDad[i]; }
    uint32_t readTcnt(int i) { return ndmaTcnt[i]; }
    uint32_t readWcnt(int i) { return ndmaWcnt[i]; }
    uint32_t readFdata(int i) { return ndmaFdata[i]; }
    uint32_t readCnt(int i) { return ndmaCnt[i]; }

    void writeSad(int i, uint32_t mask, uint32_t value);
    void writeDad(int i, uint32_t mask, uint32_t value);
    void writeTcnt(int i, uint32_t mask, uint32_t value);
    void writeWcnt(int i, uint32_t mask, uint32_t value);
    void writeFdata(int i, uint32_t mask, uint32_t value);
    void writeCnt(int i, uint32_t mask, uint32_t value);

private:
    Core *core;

    uint32_t srcAddrs[8] = {};
    uint32_t dstAddrs[8] = {};
    uint32_t drqMask = 0xFFFF0000;
    uint8_t runMask = 0;

    uint32_t ndmaSad[8] = {};
    uint32_t ndmaDad[8] = {};
    uint32_t ndmaTcnt[8] = {};
    uint32_t ndmaWcnt[8] = {};
    uint32_t ndmaFdata[8] = {};
    uint32_t ndmaCnt[8] = {};

    bool shouldTransfer(int i, uint8_t type);
    void transferBlock(int i);
};
