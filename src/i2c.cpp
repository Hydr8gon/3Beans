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
        core->interrupts.sendInterrupt(false, 0x71);
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
        core->interrupts.sendInterrupt(false, types[i]);
    }

    // Handle I2C reads if the direction is set to read
    if (value & BIT(5)) {
        switch ((i << 16) | (devAddr << 8) | regAddr) {
            case 0x14B0F: i2cBusData[i] = readMcuPowerFlags(); return;
            case 0x14B10: i2cBusData[i] = readMcuIrqFlags(0); return;
            case 0x14B11: i2cBusData[i] = readMcuIrqFlags(1); return;
            case 0x14B12: i2cBusData[i] = readMcuIrqFlags(2); return;
            case 0x14B13: i2cBusData[i] = readMcuIrqFlags(3); return;
            case 0x14B18: i2cBusData[i] = readMcuIrqMask(0); return;
            case 0x14B19: i2cBusData[i] = readMcuIrqMask(1); return;
            case 0x14B1A: i2cBusData[i] = readMcuIrqMask(2); return;
            case 0x14B1B: i2cBusData[i] = readMcuIrqMask(3); return;

        default:
            LOG_WARN("Unknown I2C bus %d read from device 0x%02X, register 0x%02X\n", i, devAddr, regAddr);
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
    switch ((i << 16) | (devAddr << 8) | regAddr) {
        case 0x14A18: return writeMcuIrqMask(0, i2cBusData[i]);
        case 0x14A19: return writeMcuIrqMask(1, i2cBusData[i]);
        case 0x14A1A: return writeMcuIrqMask(2, i2cBusData[i]);
        case 0x14A1B: return writeMcuIrqMask(3, i2cBusData[i]);
        case 0x14A22: return writeMcuLcdPower(i2cBusData[i]);

    default:
        LOG_WARN("Unknown I2C bus %d write to device 0x%02X, register 0x%02X\n", i, devAddr, regAddr);
        return;
    }
}

uint8_t I2c::readMcuPowerFlags() {
    // Report the shell as being open
    regAddr += mcuInc;
    return 0x2;
}

uint8_t I2c::readMcuIrqFlags(int i) {
    // Read MCU interrupt flags and clear them
    uint8_t value = mcuIrqFlags >> (i << 3);
    mcuIrqFlags &= ~(0xFF << (i << 3));
    regAddr += mcuInc;
    return value;
}

uint8_t I2c::readMcuIrqMask(int i) {
    // Read part of the MCU interrupt mask
    regAddr += mcuInc;
    return mcuIrqMask >> (i << 3);
}

void I2c::writeMcuIrqMask(int i, uint8_t value) {
    // Write part of the MCU interrupt mask
    mcuIrqMask = (mcuIrqMask & ~(0xFF << (i << 3))) | (value << (i << 3));
    mcuInterrupt(0);
    regAddr += mcuInc;
}

void I2c::writeMcuLcdPower(uint8_t value) {
    // Fake LCD power control by simply firing interrupts
    if (value & BIT(0)) mcuInterrupt(BIT(24) | BIT(26) | BIT(28)); // Power off
    if (value & BIT(1)) mcuInterrupt(BIT(25)); // LCD power on
    if (value & BIT(2)) mcuInterrupt(BIT(26)); // Bottom backlight off
    if (value & BIT(3)) mcuInterrupt(BIT(27)); // Bottom backlight on
    if (value & BIT(4)) mcuInterrupt(BIT(28)); // Top backlight off
    if (value & BIT(5)) mcuInterrupt(BIT(29)); // Top backlight on
    regAddr += mcuInc;
}
