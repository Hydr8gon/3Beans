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

class Input {
public:
    Input(Core *core): core(core) {}

    void pressKey(int key);
    void releaseKey(int key);
    void pressScreen(int x, int y);
    void releaseScreen();
    void setLStick(int x, int y);

    uint32_t readSpiFifoCnt() { return spiFifoCnt; }
    uint32_t readSpiFifoSelect() { return spiFifoSelect; }
    uint32_t readSpiFifoBlklen() { return spiFifoBlklen; }
    uint32_t readSpiFifoData();
    uint32_t readSpiFifoIntMask() { return spiFifoIntMask; }
    uint32_t readSpiFifoIntStat() { return spiFifoIntStat; }
    uint16_t readHidPad() { return hidPad; }

    void writeSpiFifoCnt(uint32_t mask, uint32_t value);
    void writeSpiFifoSelect(uint32_t mask, uint32_t value);
    void writeSpiFifoBlklen(uint32_t mask, uint32_t value);
    void writeSpiFifoData(uint32_t mask, uint32_t value);
    void writeSpiFifoIntMask(uint32_t mask, uint32_t value);
    void writeSpiFifoIntStat(uint32_t mask, uint32_t value);

private:
    Core *core;

    uint16_t stickLX = 0x7FF;
    uint16_t stickLY = 0x7FF;
    uint16_t touchX = 0xFFF;
    uint16_t touchY = 0xFFF;
    bool touchActive = false;

    uint32_t spiCount = 0;
    uint32_t spiTotal = 0;
    uint8_t spiIndex = 0;
    uint8_t spiPage = 0;

    uint32_t spiFifoCnt = 0;
    uint32_t spiFifoSelect = 0;
    uint32_t spiFifoBlklen = 0;
    uint32_t spiFifoIntMask = 0;
    uint32_t spiFifoIntStat = 0;
    uint16_t hidPad = 0xFFF;

    uint8_t spiTransfer(uint8_t value);
};
