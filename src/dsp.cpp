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
        case 0x0C0: return dspRep[0];
        case 0x0C2: return readHpiCmd(0);
        case 0x0C4: return dspRep[1];
        case 0x0C6: return readHpiCmd(1);
        case 0x0C8: return dspRep[2];
        case 0x0CA: return readHpiCmd(2);
        case 0x0CC: return dspSem;
        case 0x0CE: return hpiMask;
        case 0x0D2: return dspPsem;
        case 0x0D4: return hpiCfg;
        case 0x0D6: return hpiSts;
        case 0x0D8: return dspPsts;
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
        case 0x0C0: return writeHpiRep(0, value);
        case 0x0C4: return writeHpiRep(1, value);
        case 0x0C8: return writeHpiRep(2, value);
        case 0x0CC: return writeHpiSem(value);
        case 0x0CE: return writeHpiMask(value);
        case 0x0D0: return writeHpiClear(value);
        case 0x0D4: return writeHpiCfg(value);
        case 0x11E: return writeIoBase(value);

    default:
        // Catch writes to unknown I/O registers
        LOG_WARN("Unknown DSP I/O write: 0x%X\n", address);
        return;
    }
}

void Dsp::updateArmSemIrq() {
    // Update the ARM-side semaphore IRQ flag and trigger an interrupt if set
    if (!(dspSem & ~dspPmask)) {
        dspPsts &= ~BIT(9);
    }
    else if (~dspPsts & BIT(9)) {
        core->interrupts.sendInterrupt(ARM11, 0x4A);
        dspPsts |= BIT(9);
    }
}

void Dsp::updateDspSemIrq() {
    // Update the DSP-side semaphore IRQ flag and trigger an interrupt if set
    if (!(dspPsem & ~hpiMask)) {
        hpiSts &= ~BIT(9);
    }
    else if (~hpiSts & BIT(9)) {
        LOG_CRIT("Unhandled DSP semaphore interrupt triggered\n");
        hpiSts |= BIT(9);
    }
}

uint16_t Dsp::readHpiCmd(int i) {
    // Read from one of the DSP-side CMD registers and clear its update flags
    hpiSts &= ~BIT(i ? (11 + i) : 8);
    dspPsts &= ~BIT(13 + i);
    return dspCmd[i];
}

void Dsp::writeHpiRep(int i, uint16_t value) {
    // Write to one of the DSP-side REP registers and set its update flags
    dspRep[i] = value;
    LOG_INFO("Writing DSP reply register %d: 0x%X\n", i, dspRep[i]);
    hpiSts |= BIT(5 + i);
    dspPsts |= BIT(10 + i);

    // Trigger an ARM-side interrupt if enabled for the written reply
    if (dspPcfg & BIT(9 + i))
        core->interrupts.sendInterrupt(ARM11, 0x4A);
}

void Dsp::writeHpiSem(uint16_t value) {
    // Write to the DSP-side semaphore flag register
    dspSem = value;
    updateArmSemIrq();
}

void Dsp::writeHpiMask(uint16_t value) {
    // Write to the DSP-side semaphore interrupt mask
    hpiMask = value;
    updateDspSemIrq();
}

void Dsp::writeHpiClear(uint16_t value) {
    // Clear semaphore flags in the ARM-side register
    dspPsem &= ~value;
    updateDspSemIrq();
}

void Dsp::writeHpiCfg(uint16_t value) {
    // Write to the HPI control register
    hpiCfg = (value & 0x3104);
    if (hpiCfg & BIT(2))
        LOG_CRIT("Unhandled DSP big-endian I/O mode enabled\n");
}

void Dsp::writeIoBase(uint16_t value) {
    // Write to the DSP I/O base address register
    miuIoBase = (value & 0xFC00);
    LOG_INFO("Remapping DSP I/O registers to 0x%X\n", miuIoBase);
}

uint16_t Dsp::readRep(int i) {
    // Read from one of the ARM-side REP registers and clear its update flags
    dspPsts &= ~BIT(10 + i);
    hpiSts &= ~BIT(5 + i);
    return dspRep[i];
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

void Dsp::writePsem(uint16_t mask, uint16_t value) {
    // Write to the ARM-side semaphore flag register
    dspPsem = (dspPsem & ~mask) | (value & mask);
    updateDspSemIrq();
}

void Dsp::writePmask(uint16_t mask, uint16_t value) {
    // Write to the ARM-side semaphore interrupt mask
    dspPmask = (dspPmask & ~mask) | (value & mask);
    updateArmSemIrq();
}

void Dsp::writePclear(uint16_t mask, uint16_t value) {
    // Clear semaphore flags in the DSP-side register
    dspSem &= ~(value & mask);
    updateArmSemIrq();
}

void Dsp::writeCmd(int i, uint16_t mask, uint16_t value) {
    // Write to one of the ARM-side CMD registers and set its update flags
    dspCmd[i] = (dspCmd[i] & ~mask) | (value & mask);
    LOG_INFO("Writing DSP command register %d: 0x%X\n", i, dspCmd[i]);
    dspPsts |= BIT(13 + i);
    hpiSts |= BIT(i ? (11 + i) : 8);

    // Trigger a DSP-side interrupt if enabled for the written command
    if (hpiCfg & BIT(i ? (11 + i) : 8))
        LOG_CRIT("Unhandled DSP command interrupt triggered\n");
}
