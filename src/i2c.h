/*
    Copyright 2023-2024 Hydr8gon

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

    uint8_t readI2cBusData(int i) { return i2cBusData[i]; }
    uint8_t readI2cBusCnt(int i) { return i2cBusCnt[i]; }

    void writeI2cBusData(int i, uint8_t value);
    void writeI2cBusCnt(int i, uint8_t value);

private:
    Core *core;

    uint32_t writeCount = 0;
    uint8_t devAddress = 0;
    uint8_t regAddress = 0;

    uint8_t i2cBusData[3] = {};
    uint8_t i2cBusCnt[3] = {};
};
