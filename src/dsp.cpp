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

#include "core.h"

uint16_t Dsp::readData(uint16_t address) {
    // Read a value from DSP data memory if not within the I/O area
    if (address < miuIoBase || address >= miuIoBase + 0x800)
        return core->memory.read<uint16_t>(ARM11, 0x1FF40000 + (address << 1));

    // Read a value from a DSP I/O register
    switch (address - miuIoBase) {
        case 0x01A: return 0xC902; // Chip ID
        case 0x11E: return miuIoBase;

    default:
        // Catch reads from unknown I/O registers
        LOG_WARN("Unknown DSP I/O read: 0x%X\n", address);
        return 0;
    }
}

void Dsp::writeData(uint16_t address, uint16_t value) {
    // Write a value to DSP data memory if not within the I/O area
    if (address < miuIoBase || address >= miuIoBase + 0x800)
        return core->memory.write<uint16_t>(ARM11, 0x1FF40000 + (address << 1), value);

    // Write a value to a DSP I/O register
    switch (address - miuIoBase) {
        case 0x11E: return writeIoBase(value);

    default:
        // Catch writes to unknown I/O registers
        LOG_WARN("Unknown DSP I/O write: 0x%X\n", address);
        return;
    }
}

void Dsp::writeIoBase(uint16_t value) {
    // Write to the DSP I/O base address register
    miuIoBase = (value & 0xFC00);
    LOG_INFO("Remapping DSP I/O registers to 0x%X\n", miuIoBase);
}

void Dsp::writePcfg(uint16_t mask, uint16_t value) {
    // Write to the DSP_PCFG register
    dspPcfg = (dspPcfg & ~mask) | (value & mask);

    // Reset the DSP if the reset bit is set and unhalt on release
    if (dspPcfg & BIT(0)) {
        core->teak.cycles = -1;
        core->teak.regPc = 0;
    }
    else if (core->teak.cycles == -1) {
        LOG_INFO("Starting Teak DSP execution\n");
        core->teak.cycles = 0;
    }
}
