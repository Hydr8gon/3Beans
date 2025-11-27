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

class I2c {
public:
    I2c(Core *core): core(core) {}
    void mcuInterrupt(uint32_t mask);

    uint8_t readBusData(int i) { return i2cBusData[i]; }
    uint8_t readBusCnt(int i) { return i2cBusCnt[i]; }

    void writeBusData(int i, uint8_t value);
    void writeBusCnt(int i, uint8_t value);

private:
    Core *core;

    uint32_t writeCount = 0;
    uint8_t devAddr = 0;
    uint8_t regAddr = 0;
    bool mcuInc = false;

    uint8_t i2cBusData[3] = {};
    uint8_t i2cBusCnt[3] = {};

    uint32_t mcuIrqFlags = 0;
    uint32_t mcuIrqMask = 0;

    uint8_t readMcuIrqFlags(int i);
    uint8_t readMcuIrqMask(int i);
    uint8_t readRtcValue(int i);

    void writeMcuIrqMask(int i, uint8_t value);
    void writeMcuLcdPower(uint8_t value);
};
