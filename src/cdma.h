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

enum CdmaId {
    CDMA0,
    CDMA1,
    XDMA
};

class Cdma {
public:
    Cdma(Core *core, CdmaId id): core(core), id(id) {}
    void update();

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

    void writeDbgcmd(uint32_t mask, uint32_t value);
    void writeDbginst0(uint32_t mask, uint32_t value);
    void writeDbginst1(uint32_t mask, uint32_t value);

private:
    Core *core;
    CdmaId id;
    bool scheduled = false;

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
};
