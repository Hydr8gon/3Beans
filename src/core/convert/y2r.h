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
    Y2r(Core &core, bool id): core(core), id(id) {}
    void update();

    uint32_t readCnt() { return y2rCnt; }
    uint16_t readWidth() { return y2rWidth; }
    uint16_t readHeight() { return y2rHeight; }
    uint16_t readMultiplyY() { return y2rMultiplyY; }
    uint16_t readMultiplyVr() { return y2rMultiplyVr; }
    uint16_t readMultiplyVg() { return y2rMultiplyVg; }
    uint16_t readMultiplyUg() { return y2rMultiplyUg; }
    uint16_t readMultiplyUb() { return y2rMultiplyUb; }
    uint16_t readOffsetR() { return y2rOffsetR; }
    uint16_t readOffsetG() { return y2rOffsetG; }
    uint16_t readOffsetB() { return y2rOffsetB; }
    uint16_t readAlpha() { return y2rAlpha; }
    uint32_t readOutputRgba();

    void writeCnt(uint32_t mask, uint32_t value);
    void writeWidth(uint16_t mask, uint16_t value);
    void writeHeight(uint16_t mask, uint16_t value);
    void writeMultiplyY(uint16_t mask, uint16_t value);
    void writeMultiplyVr(uint16_t mask, uint16_t value);
    void writeMultiplyVg(uint16_t mask, uint16_t value);
    void writeMultiplyUg(uint16_t mask, uint16_t value);
    void writeMultiplyUb(uint16_t mask, uint16_t value);
    void writeOffsetR(uint16_t mask, uint16_t value);
    void writeOffsetG(uint16_t mask, uint16_t value);
    void writeOffsetB(uint16_t mask, uint16_t value);
    void writeAlpha(uint16_t mask, uint16_t value);
    void writeInputY(uint32_t mask, uint32_t value);
    void writeInputU(uint32_t mask, uint32_t value);
    void writeInputV(uint32_t mask, uint32_t value);

private:
    Core &core;
    bool id;

    std::deque<uint8_t> inputs[3];
    std::queue<uint8_t> output;
    uint32_t lineBuf[0x2000] = {};
    uint16_t outputLines = 0;
    bool scheduled = false;

    uint32_t y2rCnt = 0;
    uint16_t y2rWidth = 0;
    uint16_t y2rHeight = 0;
    uint16_t y2rMultiplyY = 0;
    uint16_t y2rMultiplyVr = 0;
    uint16_t y2rMultiplyVg = 0;
    uint16_t y2rMultiplyUg = 0;
    uint16_t y2rMultiplyUb = 0;
    int16_t y2rOffsetR = 0;
    int16_t y2rOffsetG = 0;
    int16_t y2rOffsetB = 0;
    uint16_t y2rAlpha = 0;

    void outputPixel(uint32_t ofs, uint8_t y, uint8_t u, uint8_t v);
    void triggerFifo();
};
