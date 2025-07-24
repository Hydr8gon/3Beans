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

#include "core.h"

void I2c::mcuInterrupt(uint32_t mask) {
    // Set MCU interrupt flags and trigger if a set flag is enabled
    if ((mcuIrqFlags |= mask) & ~mcuIrqMask)
        core->interrupts.sendInterrupt(ARM11, 0x71);
}

void I2c::writeBusData(int i, uint8_t value) {
    // Write to one of the I2C_BUS_DATA registers
    i2cBusData[i] = value;
}

void I2c::writeBusCnt(int i, uint8_t value) {
    // Write to one of the I2C_BUS_CNT registers and check some bits
    i2cBusCnt[i] = value & 0x7F;
    if (~value & BIT(7)) return; // Enable
    if (value & BIT(1)) writeCount = 0; // Start

    // Trigger an interrupt for the current bus if enabled
    if (value & BIT(6)) {
        uint8_t types[] = { 0x54, 0x55, 0x5C };
        core->interrupts.sendInterrupt(ARM11, types[i]);
    }

    // Handle I2C reads if the direction is set to read
    if (value & BIT(5)) {
        switch ((i << 16) | (devAddr << 8) | ((regAddr += mcuInc) - mcuInc)) {
            case 0x14B00: i2cBusData[i] = 0x13; return; // Version high
            case 0x14B01: i2cBusData[i] = 0x41; return; // Version low
            case 0x14B0B: i2cBusData[i] = 0x64; return; // Battery percent
            case 0x14B0F: i2cBusData[i] = 0x02; return; // Power flags
            case 0x14B10: i2cBusData[i] = readMcuIrqFlags(0); return;
            case 0x14B11: i2cBusData[i] = readMcuIrqFlags(1); return;
            case 0x14B12: i2cBusData[i] = readMcuIrqFlags(2); return;
            case 0x14B13: i2cBusData[i] = readMcuIrqFlags(3); return;
            case 0x14B18: i2cBusData[i] = readMcuIrqMask(0); return;
            case 0x14B19: i2cBusData[i] = readMcuIrqMask(1); return;
            case 0x14B1A: i2cBusData[i] = readMcuIrqMask(2); return;
            case 0x14B1B: i2cBusData[i] = readMcuIrqMask(3); return;
            case 0x14B30: i2cBusData[i] = readRtcValue(0); return;
            case 0x14B31: i2cBusData[i] = readRtcValue(1); return;
            case 0x14B32: i2cBusData[i] = readRtcValue(2); return;
            case 0x14B33: i2cBusData[i] = readRtcValue(3); return;
            case 0x14B34: i2cBusData[i] = readRtcValue(4); return;
            case 0x14B35: i2cBusData[i] = readRtcValue(5); return;
            case 0x14B36: i2cBusData[i] = readRtcValue(6); return;

        default:
            LOG_WARN("Unknown I2C bus %d read from device 0x%X, register 0x%X\n", i, devAddr, regAddr);
            i2cBusData[i] = 0;
            return;
        }
    }

    // Set device and register addresses on the first two writes
    i2cBusCnt[i] |= BIT(4); // Acknowledge
    if (++writeCount == 1) {
        devAddr = i2cBusData[i];
        return;
    }
    else if (writeCount == 2) {
        regAddr = i2cBusData[i];
        if (i == 1 && (devAddr & 0xFE) == 0x4A) // MCU
            mcuInc = (regAddr != 0x29 && regAddr != 0x2D && regAddr != 0x4F && regAddr != 0x61 && regAddr != 0x7F);
        return;
    }

    // Handle I2C writes if the direction is set to write
    switch ((i << 16) | (devAddr << 8) | ((regAddr += mcuInc) - mcuInc)) {
        case 0x14A18: return writeMcuIrqMask(0, i2cBusData[i]);
        case 0x14A19: return writeMcuIrqMask(1, i2cBusData[i]);
        case 0x14A1A: return writeMcuIrqMask(2, i2cBusData[i]);
        case 0x14A1B: return writeMcuIrqMask(3, i2cBusData[i]);
        case 0x14A22: return writeMcuLcdPower(i2cBusData[i]);

    default:
        LOG_WARN("Unknown I2C bus %d write to device 0x%X, register 0x%X\n", i, devAddr, regAddr);
        return;
    }
}

uint8_t I2c::readMcuIrqFlags(int i) {
    // Read MCU interrupt flags and clear them
    uint8_t value = mcuIrqFlags >> (i << 3);
    mcuIrqFlags &= ~(0xFF << (i << 3));
    return value;
}

uint8_t I2c::readMcuIrqMask(int i) {
    // Read part of the MCU interrupt mask
    return mcuIrqMask >> (i << 3);
}

uint8_t I2c::readRtcValue(int i) {
    // Get the local time and adjust values for the DS
    std::time_t t = std::time(nullptr);
    std::tm *time = std::localtime(&t);
    time->tm_year %= 100; // 2000-2099
    time->tm_mon++; // Starts at 1

    // Read the requested value converted to BCD format
    switch (i) {
        case 0: return ((time->tm_sec / 10) << 4) | (time->tm_sec % 10);
        case 1: return ((time->tm_min / 10) << 4) | (time->tm_min % 10);
        case 2: return ((time->tm_hour / 10) << 4) | (time->tm_hour % 10);
        case 3: return 0; // TODO: day of week
        case 4: return ((time->tm_mday / 10) << 4) | (time->tm_mday % 10);
        case 5: return ((time->tm_mon / 10) << 4) | (time->tm_mon % 10);
        default: return ((time->tm_year / 10) << 4) | (time->tm_year % 10);
    }
}

void I2c::writeMcuIrqMask(int i, uint8_t value) {
    // Write part of the MCU interrupt mask
    mcuIrqMask = (mcuIrqMask & ~(0xFF << (i << 3))) | (value << (i << 3));
    mcuInterrupt(0);
}

void I2c::writeMcuLcdPower(uint8_t value) {
    // Fake LCD power control by simply firing interrupts
    if (value & BIT(0)) mcuInterrupt(BIT(24) | BIT(26) | BIT(28)); // Power off
    if (value & BIT(1)) mcuInterrupt(BIT(25)); // LCD power on
    if (value & BIT(2)) mcuInterrupt(BIT(26)); // Bottom backlight off
    if (value & BIT(3)) mcuInterrupt(BIT(27)); // Bottom backlight on
    if (value & BIT(4)) mcuInterrupt(BIT(28)); // Top backlight off
    if (value & BIT(5)) mcuInterrupt(BIT(29)); // Top backlight on
}
