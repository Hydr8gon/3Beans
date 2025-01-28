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

#include "core.h"

void I2c::writeI2cBusData(int i, uint8_t value) {
    // Write to one of the I2C_BUS_DATA registers
    i2cBusData[i] = value;
}

void I2c::writeI2cBusCnt(int i, uint8_t value) {
    // Write to one of the I2C_BUS_CNT registers and check some bits
    i2cBusCnt[i] = value & 0x7F;
    if (~value & BIT(7)) return; // Enable
    if (value & BIT(1)) writeCount = 0; // Start

    // Trigger an interrupt for the current bus if enabled
    if (value & BIT(6)) {
        uint8_t types[] = { 0x54, 0x55, 0x5C };
        core->interrupts.sendInterrupt(false, types[i]);
    }

    // Handle I2C reads if the direction is set to read
    if (value & BIT(5)) {
        LOG_WARN("Unknown I2C bus %d read from device 0x%02X, register 0x%02X\n", i, devAddress, regAddress);
        i2cBusData[i] = 0;
        return;
    }

    // Handle I2C writes, setting the device and register addresses first
    if (++writeCount == 1)
        devAddress = i2cBusData[i];
    else if (writeCount == 2)
        regAddress = i2cBusData[i];
    else
        LOG_WARN("Unknown I2C bus %d write to device 0x%02X, register 0x%02X\n", i, devAddress, regAddress);
    i2cBusCnt[i] |= BIT(4); // Acknowledge
}
