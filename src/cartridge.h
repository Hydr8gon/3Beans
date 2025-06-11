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
#include <string>

class Core;

enum CartCmd {
    CMD_NONE = 0,
    CMD_CHIP
};

class Cartridge {
public:
    Cartridge(Core *core, std::string &cartPath);
    ~Cartridge();
    void wordReady();

    uint16_t readCfg9CardPower() { return cfg9CardPower; }
    uint16_t readNtrMcnt() { return ntrMcnt; }
    uint32_t readNtrRomcnt() { return ntrRomcnt; }
    uint32_t readNtrData();

    void writeCfg9CardPower(uint16_t mask, uint16_t value);
    void writeNtrMcnt(uint16_t mask, uint16_t value);
    void writeNtrRomcnt(uint32_t mask, uint32_t value);
    void writeNtrCmdL(uint32_t mask, uint32_t value);
    void writeNtrCmdH(uint32_t mask, uint32_t value);

private:
    Core *core;
    FILE *cartFile;

    CartCmd cartCmd = CMD_NONE;
    uint16_t blockSize = 0;
    uint16_t readCount = 0;
    bool ctrMode = false;

    uint16_t cfg9CardPower = 0x1;
    uint16_t ntrMcnt = 0;
    uint32_t ntrRomcnt = 0;
    uint64_t ntrCmd = 0;
};
