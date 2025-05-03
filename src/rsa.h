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

class Core;

class Rsa {
public:
    Rsa(Core *core): core(core) {}

    uint32_t readCnt() { return rsaCnt; }
    uint32_t readSlotcnt(int i) { return rsaSlotcnt[i]; }
    uint32_t readSlotsize(int i) { return expFifos[i].size(); }
    uint32_t readMod(int i);
    uint32_t readData(int i);

    void writeCnt(uint32_t mask, uint32_t value);
    void writeSlotcnt(int i, uint32_t mask, uint32_t value);
    void writeMod(int i, uint32_t mask, uint32_t value);
    void writeData(int i, uint32_t mask, uint32_t value);
    void writeExpfifo(uint32_t mask, uint32_t value);

private:
    Core *core;
    std::deque<uint32_t> expFifos[4];

    uint32_t rsaCnt = 0;
    uint32_t rsaSlotcnt[4] = {};
    uint32_t rsaMods[4][0x40] = {};
    uint32_t rsaData[0x40] = {};

    void calculate();
    void mulMod(uint32_t *dst, uint32_t *src, uint8_t size);
};
