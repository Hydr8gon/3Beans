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

class Y2r {
public:
    Y2r(Core *core, bool id): core(core), id(id) {}
    void update();

    uint32_t readCnt() { return y2rCnt; }
    uint16_t readWidth() { return y2rWidth; }
    uint16_t readHeight() { return y2rHeight; }
    uint32_t readOutputRgba();

    void writeCnt(uint32_t mask, uint32_t value);
    void writeWidth(uint16_t mask, uint16_t value);
    void writeHeight(uint16_t mask, uint16_t value);
    void writeInputY(uint32_t mask, uint32_t value);
    void writeInputU(uint32_t mask, uint32_t value);
    void writeInputV(uint32_t mask, uint32_t value);

private:
    Core *core;
    bool id;

    std::queue<uint8_t> inputs[3];
    std::queue<uint8_t> output;
    uint32_t lineBuf[0x2000] = {};
    uint16_t outputLines = 0;
    bool scheduled = false;

    uint32_t y2rCnt = 0;
    uint16_t y2rWidth = 0;
    uint16_t y2rHeight = 0;

    void outputPixel(uint32_t ofs, uint8_t y, uint8_t u, uint8_t v);
    void triggerFifo();
};
