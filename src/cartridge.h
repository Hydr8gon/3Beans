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
#include <string>

class Core;

enum ReplyCmd {
    REPLY_NONE = 0,
    REPLY_CHIP,
    REPLY_HEADER,
    REPLY_ROM,
    REPLY_PROM
};

class Cartridge {
public:
    Cartridge(Core *core, std::string &cartPath);
    ~Cartridge();

    void ntrWordReady();
    void ctrWordReady();

    uint16_t readCfg9CardPower() { return cfg9CardPower; }
    uint16_t readNtrMcnt() { return ntrMcnt; }
    uint32_t readNtrRomcnt() { return ntrRomcnt; }
    uint32_t readNtrData();
    uint32_t readCtrCnt() { return ctrCnt; }
    uint32_t readCtrBlkcnt() { return ctrBlkcnt; }
    uint32_t readCtrSeccnt() { return ctrSeccnt; }
    uint32_t readCtrFifo();

    void writeCfg9CardPower(uint16_t mask, uint16_t value);
    void writeNtrMcnt(uint16_t mask, uint16_t value);
    void writeNtrRomcnt(uint32_t mask, uint32_t value);
    void writeNtrCmd(int i, uint32_t mask, uint32_t value);
    void writeCtrCnt(uint32_t mask, uint32_t value);
    void writeCtrBlkcnt(uint32_t mask, uint32_t value);
    void writeCtrSeccnt(uint32_t mask, uint32_t value);
    void writeCtrCmd(int i, uint32_t mask, uint32_t value);

private:
    Core *core;
    FILE *cartFile;
    static const uint16_t ctrClocks[8];

    uint32_t cartBase = -1;
    uint32_t cartBlock[0x200] = {};
    std::queue<uint32_t> ctrFifo;
    bool ctrMode = false;

    ReplyCmd ntrReply = REPLY_NONE;
    ReplyCmd ctrReply = REPLY_NONE;
    uint16_t ntrCount = 0;
    uint32_t ctrCount = 0;
    uint32_t ctrAddress = 0;

    uint16_t cfg9CardPower = 0x1;
    uint16_t ntrMcnt = 0;
    uint32_t ntrRomcnt = 0;
    uint32_t ntrCmd[2] = {};
    uint32_t ctrCnt = 0;
    uint32_t ctrBlkcnt = 0;
    uint32_t ctrSeccnt = 0;
    uint32_t ctrCmd[4] = {};

    uint32_t readCart(uint32_t address);
};
