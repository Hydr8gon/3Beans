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
        case 0x200: return icuPending;
        case 0x204: return icuTrigger;
        case 0x206: return icuEnable[0];
        case 0x208: return icuEnable[1];
        case 0x20A: return icuEnable[2];
        case 0x20C: return icuEnable[3];
        case 0x20E: return icuMode;
        case 0x210: return icuInvert;
        case 0x212: return icuVector[0] >> 16;
        case 0x214: return icuVector[0] >> 0;
        case 0x216: return icuVector[1] >> 16;
        case 0x218: return icuVector[1] >> 0;
        case 0x21A: return icuVector[2] >> 16;
        case 0x21C: return icuVector[2] >> 0;
        case 0x21E: return icuVector[3] >> 16;
        case 0x220: return icuVector[3] >> 0;
        case 0x222: return icuVector[4] >> 16;
        case 0x224: return icuVector[4] >> 0;
        case 0x226: return icuVector[5] >> 16;
        case 0x228: return icuVector[5] >> 0;
        case 0x22A: return icuVector[6] >> 16;
        case 0x22C: return icuVector[6] >> 0;
        case 0x22E: return icuVector[7] >> 16;
        case 0x230: return icuVector[7] >> 0;
        case 0x232: return icuVector[8] >> 16;
        case 0x234: return icuVector[8] >> 0;
        case 0x236: return icuVector[9] >> 16;
        case 0x238: return icuVector[9] >> 0;
        case 0x23A: return icuVector[10] >> 16;
        case 0x23C: return icuVector[10] >> 0;
        case 0x23E: return icuVector[11] >> 16;
        case 0x240: return icuVector[11] >> 0;
        case 0x242: return icuVector[12] >> 16;
        case 0x244: return icuVector[12] >> 0;
        case 0x246: return icuVector[13] >> 16;
        case 0x248: return icuVector[13] >> 0;
        case 0x24A: return icuVector[14] >> 16;
        case 0x24C: return icuVector[14] >> 0;
        case 0x24E: return icuVector[15] >> 16;
        case 0x250: return icuVector[15] >> 0;
        case 0x252: return icuDisable;

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
        case 0x202: return writeIcuAck(value);
        case 0x204: return writeIcuTrigger(value);
        case 0x206: return writeIcuEnable(0, value);
        case 0x208: return writeIcuEnable(1, value);
        case 0x20A: return writeIcuEnable(2, value);
        case 0x20C: return writeIcuEnable(3, value);
        case 0x20E: return writeIcuMode(value);
        case 0x210: return writeIcuInvert(value);
        case 0x212: return writeIcuVectorH(0, value);
        case 0x214: return writeIcuVectorL(0, value);
        case 0x216: return writeIcuVectorH(1, value);
        case 0x218: return writeIcuVectorL(1, value);
        case 0x21A: return writeIcuVectorH(2, value);
        case 0x21C: return writeIcuVectorL(2, value);
        case 0x21E: return writeIcuVectorH(3, value);
        case 0x220: return writeIcuVectorL(3, value);
        case 0x222: return writeIcuVectorH(4, value);
        case 0x224: return writeIcuVectorL(4, value);
        case 0x226: return writeIcuVectorH(5, value);
        case 0x228: return writeIcuVectorL(5, value);
        case 0x22A: return writeIcuVectorH(6, value);
        case 0x22C: return writeIcuVectorL(6, value);
        case 0x22E: return writeIcuVectorH(7, value);
        case 0x230: return writeIcuVectorL(7, value);
        case 0x232: return writeIcuVectorH(8, value);
        case 0x234: return writeIcuVectorL(8, value);
        case 0x236: return writeIcuVectorH(9, value);
        case 0x238: return writeIcuVectorL(9, value);
        case 0x23A: return writeIcuVectorH(10, value);
        case 0x23C: return writeIcuVectorL(10, value);
        case 0x23E: return writeIcuVectorH(11, value);
        case 0x240: return writeIcuVectorL(11, value);
        case 0x242: return writeIcuVectorH(12, value);
        case 0x244: return writeIcuVectorL(12, value);
        case 0x246: return writeIcuVectorH(13, value);
        case 0x248: return writeIcuVectorL(13, value);
        case 0x24A: return writeIcuVectorH(14, value);
        case 0x24C: return writeIcuVectorL(14, value);
        case 0x24E: return writeIcuVectorH(15, value);
        case 0x250: return writeIcuVectorL(15, value);
        case 0x252: return writeIcuDisable(value);

    default:
        // Catch writes to unknown I/O registers
        LOG_WARN("Unknown DSP I/O write: 0x%X\n", address);
        return;
    }
}

uint32_t Dsp::getIcuVector() {
    // Get the ICU vector for the lowest pending and enabled vector bit
    if (!(icuEnable[3] & icuPending)) return 0;
    int v = 0;
    while (!(icuEnable[3] & icuPending & BIT(v))) v++;
    return icuVector[v];
}

void Dsp::updateIcuState() {
    // Update ICU state bits and set pending for newly-enabled bits
    bool hpi = (hpiSts & BIT(9)) || (hpiSts & hpiCfg & 0x3100);
    uint16_t state = (((hpi << 14) ^ icuInvert) | icuTrigger) & ~icuDisable;
    icuPending |= (state & ~icuState);
    icuState = state;

    // Set Teak interrupts to pending if they have enabled and pending bits
    uint8_t mask = 0;
    for (int i = 0; i < 4; i++)
        if (icuEnable[i] & icuPending)
            mask |= BIT(i);
    core->teak.setPendingIrqs(mask);
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
    (dspPsem & ~hpiMask) ? (hpiSts |= BIT(9)) : (hpiSts &= ~BIT(9));
    updateIcuState();
}

uint16_t Dsp::readHpiCmd(int i) {
    // Read from one of the DSP-side CMD registers and clear its update flags
    hpiSts &= ~BIT(i ? (11 + i) : 8);
    dspPsts &= ~BIT(13 + i);
    updateIcuState();
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

void Dsp::writeIcuAck(uint16_t value) {
    // Clear ICU pending bits if edge-triggered or inactive
    updateIcuState();
    icuPending &= ~(value & (icuMode | ~icuState));
}

void Dsp::writeIcuTrigger(uint16_t value) {
    // Write to the ICU manual trigger mask
    icuTrigger = value;
    updateIcuState();
}

void Dsp::writeIcuEnable(int i, uint16_t value) {
    // Write to one of the ICU enable masks
    icuEnable[i] = value;
    updateIcuState();
}

void Dsp::writeIcuMode(uint16_t value) {
    // Write to the ICU trigger mode mask
    icuMode = value;
}

void Dsp::writeIcuInvert(uint16_t value) {
    // Write to the ICU polarity inversion mask
    icuInvert = value;
    updateIcuState();
}

void Dsp::writeIcuVectorL(int i, uint16_t value) {
    // Write to the low part of an ICU vector address
    icuVector[i] = (icuVector[i] & ~0xFFFF) | value;
}

void Dsp::writeIcuVectorH(int i, uint16_t value) {
    // Write to the high part of an ICU vector address
    icuVector[i] = (icuVector[i] & 0xFFFF) | ((value & 0x8003) << 16);
}

void Dsp::writeIcuDisable(uint16_t value) {
    // Write to the ICU global disable mask
    icuDisable = value;
    updateIcuState();
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
    updateIcuState();
}
