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

void Dsp::resetCycles() {
    // Adjust timer end cycles for a global cycle reset
    for (int i = 0; i < 2; i++)
        if (tmrCycles[i] != -1)
            tmrCycles[i] -= core->globalCycles;
}

uint32_t Dsp::getMiuAddr(uint16_t address) {
    // Convert a DSP word address to an ARM byte address using MIU data pages
    uint16_t page = miuPageZ;
    if (miuMisc & BIT(6)) { // PGM
        if (address < miuBoundX)
            page = miuPageX;
        else if (address < miuBoundY)
            page = miuPageY;
    }
    return 0x1FF40000 + ((page << 17) & 0x20000) + (address << 1);
}

uint16_t Dsp::readData(uint16_t address) {
    // Read a value from DSP data memory if not within the I/O area
    if (address < miuIoBase || address >= miuIoBase + 0x800)
        return core->memory.read<uint16_t>(ARM11, getMiuAddr(address));

    // Read a value from a DSP I/O register
    switch (address - miuIoBase) {
        case 0x01A: return 0xC902; // Chip ID
        case 0x020: return tmrCtrl[0];
        case 0x024: return tmrReload[0] >> 0;
        case 0x026: return tmrReload[0] >> 16;
        case 0x028: return readTmrCount(0) >> 0;
        case 0x02A: return readTmrCount(0) >> 16;
        case 0x030: return tmrCtrl[1];
        case 0x034: return tmrReload[1] >> 0;
        case 0x036: return tmrReload[1] >> 16;
        case 0x038: return readTmrCount(1) >> 0;
        case 0x03A: return readTmrCount(1) >> 16;
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
        case 0x10E: return miuPageX;
        case 0x110: return miuPageY;
        case 0x112: return miuPageZ;
        case 0x114: return miuSize0;
        case 0x11A: return miuMisc;
        case 0x11E: return miuIoBase;
        case 0x184: return dmaStart;
        case 0x188: return readDmaEnd(0);
        case 0x18A: return readDmaEnd(1);
        case 0x18C: return readDmaEnd(2);
        case 0x1BE: return dmaSelect;
        case 0x1C0: return dmaSrcAddr[dmaSelect] >> 0;
        case 0x1C2: return dmaSrcAddr[dmaSelect] >> 16;
        case 0x1C4: return dmaDstAddr[dmaSelect] >> 0;
        case 0x1C6: return dmaDstAddr[dmaSelect] >> 16;
        case 0x1C8: return dmaSize[0][dmaSelect];
        case 0x1CA: return dmaSize[1][dmaSelect];
        case 0x1CC: return dmaSize[2][dmaSelect];
        case 0x1CE: return dmaSrcStep[0][dmaSelect];
        case 0x1D0: return dmaDstStep[0][dmaSelect];
        case 0x1D2: return dmaSrcStep[1][dmaSelect];
        case 0x1D4: return dmaDstStep[1][dmaSelect];
        case 0x1D6: return dmaSrcStep[2][dmaSelect];
        case 0x1D8: return dmaDstStep[2][dmaSelect];
        case 0x1DA: return dmaAreaCfg[dmaSelect];
        case 0x1DE: return dmaCtrl[dmaSelect];
        case 0x200: return icuPending;
        case 0x202: return 0; // Prevent log spam
        case 0x204: return icuTrigger;
        case 0x206: return icuEnable[0];
        case 0x208: return icuEnable[1];
        case 0x20A: return icuEnable[2];
        case 0x20C: return icuEnable[3];
        case 0x20E: return icuMode;
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
        case 0x2A0: return audOutCtrl;
        case 0x2BE: return audOutEnable;
        case 0x2C2: return audOutStatus;
        case 0x2CA: return audOutFlush;

    default:
        // Catch reads from unknown I/O registers
        LOG_WARN("Unknown DSP I/O read: 0x%X\n", address - miuIoBase);
        return 0;
    }
}

void Dsp::writeData(uint16_t address, uint16_t value) {
    // Write a value to DSP data memory if not within the I/O area
    if (address < miuIoBase || address >= miuIoBase + 0x800)
        return core->memory.write<uint16_t>(ARM11, getMiuAddr(address), value);

    // Write a value to a DSP I/O register
    switch (address - miuIoBase) {
        case 0x020: return writeTmrCtrl(0, value);
        case 0x022: return writeTmrEvent(0, value);
        case 0x024: return writeTmrReloadL(0, value);
        case 0x026: return writeTmrReloadH(0, value);
        case 0x030: return writeTmrCtrl(1, value);
        case 0x032: return writeTmrEvent(1, value);
        case 0x034: return writeTmrReloadL(1, value);
        case 0x036: return writeTmrReloadH(1, value);
        case 0x0C0: return writeHpiRep(0, value);
        case 0x0C4: return writeHpiRep(1, value);
        case 0x0C8: return writeHpiRep(2, value);
        case 0x0CC: return writeHpiSem(value);
        case 0x0CE: return writeHpiMask(value);
        case 0x0D0: return writeHpiClear(value);
        case 0x0D4: return writeHpiCfg(value);
        case 0x10E: return writeMiuPageX(value);
        case 0x110: return writeMiuPageY(value);
        case 0x112: return writeMiuPageZ(value);
        case 0x114: return writeMiuSize0(value);
        case 0x11A: return writeMiuMisc(value);
        case 0x11E: return writeMiuIoBase(value);
        case 0x184: return writeDmaStart(value);
        case 0x1BE: return writeDmaSelect(value);
        case 0x1C0: return writeDmaSrcAddrL(value);
        case 0x1C2: return writeDmaSrcAddrH(value);
        case 0x1C4: return writeDmaDstAddrL(value);
        case 0x1C6: return writeDmaDstAddrH(value);
        case 0x1C8: return writeDmaSize(0, value);
        case 0x1CA: return writeDmaSize(1, value);
        case 0x1CC: return writeDmaSize(2, value);
        case 0x1CE: return writeDmaSrcStep(0, value);
        case 0x1D0: return writeDmaDstStep(0, value);
        case 0x1D2: return writeDmaSrcStep(1, value);
        case 0x1D4: return writeDmaDstStep(1, value);
        case 0x1D6: return writeDmaSrcStep(2, value);
        case 0x1D8: return writeDmaDstStep(2, value);
        case 0x1DA: return writeDmaAreaCfg(value);
        case 0x1DE: return writeDmaCtrl(value);
        case 0x202: return writeIcuAck(value);
        case 0x204: return writeIcuTrigger(value);
        case 0x206: return writeIcuEnable(0, value);
        case 0x208: return writeIcuEnable(1, value);
        case 0x20A: return writeIcuEnable(2, value);
        case 0x20C: return writeIcuEnable(3, value);
        case 0x20E: return writeIcuMode(value);
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
        case 0x2A0: return writeAudOutCtrl(value);
        case 0x2BE: return writeAudOutEnable(value);
        case 0x2C6: return writeAudOutFifo(value);
        case 0x2CA: return writeAudOutFlush(value);

    default:
        // Catch writes to unknown I/O registers
        LOG_WARN("Unknown DSP I/O write: 0x%X\n", address - miuIoBase);
        return;
    }
}

void Dsp::underflowTmr(int i) {
    // Ensure underflow should still occur at the current timestamp
    if (tmrCycles[i] != core->globalCycles)
        return;

    // Set a timer's signal and schedule a clear if enabled
    tmrSignals[i] = true;
    updateIcuState();
    if (uint8_t clr = tmrCtrl[i] >> 14)
        core->schedule(Task(DSP_UNSIGNAL0 + i), 2 << clr);

    // Stop or reload the timer based on its mode
    switch (uint8_t mode = (tmrCtrl[i] >> 2) & 0x7) {
    default: // Watchdog modes
        LOG_CRIT("Unhandled DSP timer %d underflow in watchdog mode %d\n", i, mode - 3);
    case 0x0: case 0x3: case 0x7: // Stop at zero
        tmrCount[i] = 0;
        tmrCycles[i] = -1;
        return;

    case 0x1: // Wrap to reload
        tmrCount[i] = tmrReload[i];
        return scheduleTmr(i);

    case 0x2: // Wrap to max
        tmrCount[i] = 0xFFFFFFFF;
        return scheduleTmr(i);
    }
}

void Dsp::unsignalTmr(int i) {
    // Clear a timer's output signal automatically
    tmrSignals[i] = false;
    updateIcuState();
}

void Dsp::scheduleTmr(int i) {
    // Unschedule the timer if stopped, using external clock, or in event mode
    if ((tmrCtrl[i] & 0x1100) || !tmrCount[i] || ((tmrCtrl[i] >> 2) & 0x7) == 0x3) {
        tmrCycles[i] = -1;
        return;
    }

    // Schedule a timer underflow using its current counter and prescaler
    uint8_t shift = (tmrCtrl[i] & 0x3);
    shift += 1 + (shift == 3);
    tmrCycles[i] = (uint64_t(tmrCount[i]) + 1) << shift;
    if (i) tmrCycles[i] = (tmrCycles[i] * 5) / 4; // 1.25x slower
    core->schedule(Task(DSP_UNDERFLOW0 + i), tmrCycles[i]);
    tmrCycles[i] += core->globalCycles;
}

void Dsp::flushAudOut() {
    // Empty the audio output FIFO, sending left and right samples to be mixed
    while (audOutFifo.size() >= 2) {
        uint16_t value = audOutFifo.front();
        audOutFifo.pop();
        core->csnd.sampleDsp(value, audOutFifo.front());
        audOutFifo.pop();
    }

    // Clear the FIFO full bit and set the FIFO empty bit
    audOutStatus = (audOutStatus & ~BIT(3)) | BIT(4);
    updateIcuState();
}

void Dsp::sendAudio() {
    // Ignore and unschedule the audio FIFO event if stopped
    if (!audOutEnable || !audCycles) {
        audScheduled = false;
        return;
    }

    // Flush the audio output FIFO and reschedule the event
    flushAudOut();
    core->schedule(DSP_SEND_AUDIO, audCycles);
}

void Dsp::setAudClock(DspClock clock) {
    // Stop the audio FIFO event if its clock is disabled
    if (clock == CLK_OFF) {
        audCycles = 0;
        return;
    }

    // Set the audio FIFO event frequency and schedule it if newly enabled
    audCycles = ((clock == CLK_32KHZ) ? 8192 : 5632) * 8;
    if (audScheduled || !audOutEnable) return;
    core->schedule(DSP_SEND_AUDIO, audCycles);
    audScheduled = true;
}

uint32_t Dsp::getIcuVector() {
    // Get the ICU vector for the lowest pending and enabled vector bit
    if (!(icuEnable[3] & icuPending)) return 0;
    int v = 0;
    while (!(icuEnable[3] & icuPending & BIT(v))) v++;
    return icuVector[v];
}

uint16_t Dsp::dmaRead(uint8_t area, uint32_t address) {
    // Read a 16-bit value from a DMA area if it exists
    // TODO: handle restricted AHBM addresses
    switch (area) {
        case 7: return core->memory.read<uint16_t>(ARM11, address & ~0x1); // AHBM
        case 5: return core->memory.read<uint16_t>(ARM11, 0x1FF00000 + ((address << 1) & 0x3FFFE)); // Code
        case 0: return core->memory.read<uint16_t>(ARM11, 0x1FF40000 + ((address << 1) & 0x3FFFE)); // Data
        case 1: return readData(miuIoBase + (address & 0x7FF)); // MMIO

    default:
        // Catch DMA reads from unknown areas
        LOG_CRIT("Attempted DSP DMA read from unknown area: %d\n", area);
        return 0;
    }
}

void Dsp::dmaWrite(uint8_t area, uint32_t address, uint16_t value) {
    // Write a 16-bit value to a DMA area if it exists
    // TODO: handle restricted AHBM addresses
    switch (area) {
        case 7: return core->memory.write<uint16_t>(ARM11, address & ~0x1, value); // AHBM
        case 5: return core->memory.write<uint16_t>(ARM11, 0x1FF00000 + ((address << 1) & 0x3FFFE), value); // Code
        case 0: return core->memory.write<uint16_t>(ARM11, 0x1FF40000 + ((address << 1) & 0x3FFFE), value); // Data
        case 1: return writeData(miuIoBase + (address & 0x7FF), value); // MMIO

    default:
        // Catch DMA writes to unknown areas
        LOG_CRIT("Attempted DSP DMA read from unknown area: %d\n", area);
        return;
    }
}

void Dsp::dmaTransfer(int i) {
    // Get the source and destination parameters
    uint8_t srcArea = (dmaAreaCfg[i] >> 0) & 0xF;
    uint8_t dstArea = (dmaAreaCfg[i] >> 4) & 0xF;
    uint32_t srcAddr = dmaSrcAddr[i], dstAddr = dmaDstAddr[i];
    LOG_INFO("Performing DSP DMA%d transfer from 0x%X in area 0x%X to 0x%X in area 0x%X with size 0x%X * 0x%X * 0x%X\n",
        i, srcAddr, srcArea, dstAddr, dstArea, dmaSize[0][i], dmaSize[1][i], dmaSize[2][i]);

    // Perform a DSP DMA transfer all at once using its 3D size and steps
    for (int z = 0; z < dmaSize[2][i]; z++) {
        for (int y = 0; y < dmaSize[1][i]; y++) {
            if (dmaAreaCfg[i] & BIT(10)) {
                // Transfer two 16-bit words at once, with addresses aligned to 32-bit
                uint32_t srcI = (srcArea == 7) ? 2 : 1, dstI = (dstArea == 7) ? 2 : 1;
                uint32_t srcM = ~((1 << srcI) - 1), dstM = ~((1 << dstI) - 1);
                for (int x = 0; x < dmaSize[0][i]; x += 2) {
                    dmaWrite(dstArea, (dstAddr & dstM), dmaRead(srcArea, (srcAddr & srcM)));
                    dmaWrite(dstArea, (dstAddr & dstM) + dstI, dmaRead(srcArea, (srcAddr & srcM) + srcI));
                    srcAddr += dmaSrcStep[0][i];
                    dstAddr += dmaDstStep[0][i];
                }
            }
            else {
                // Transfer 16-bit words individually
                for (int x = 0; x < dmaSize[0][i]; x++) {
                    dmaWrite(dstArea, dstAddr, dmaRead(srcArea, srcAddr));
                    srcAddr += dmaSrcStep[0][i];
                    dstAddr += dmaDstStep[0][i];
                }
            }
            srcAddr += dmaSrcStep[1][i] - dmaSrcStep[0][i];
            dstAddr += dmaDstStep[1][i] - dmaDstStep[0][i];
        }
        srcAddr += dmaSrcStep[2][i] - dmaSrcStep[1][i];
        dstAddr += dmaDstStep[2][i] - dmaDstStep[1][i];
    }

    // Set end bits for each size and signal interrupts if enabled
    for (int j = 0; j < 3; j++) {
        dmaEnd[i] |= BIT(i);
        if (dmaCtrl[i] & BIT(j))
            dmaSignals[j] = true;
    }
    updateIcuState();
}

void Dsp::updateIcuState() {
    // Update ICU state bits and set pending for newly-enabled bits
    bool dma = (dmaSignals[0] || dmaSignals[1] || dmaSignals[2]);
    bool hpi = (hpiSts & BIT(9)) || (hpiSts & ~hpiCfg & 0x3100);
    bool aud0 = (audOutCtrl & 0xF00) && (audOutStatus & BIT(4));
    bool tmr0 = tmrSignals[0] ^ ((tmrCtrl[0] >> 6) & 0x1);
    bool tmr1 = tmrSignals[1] ^ ((tmrCtrl[1] >> 6) & 0x1);
    uint16_t state = ((dma << 15) | (hpi << 14) | (aud0 << 11) | (tmr0 << 10) | (tmr1 << 9) | icuTrigger) & ~icuDisable;
    uint16_t edge = (state & ~icuState);
    icuPending |= edge;
    icuState = state;

    // Set Teak interrupts to pending if they have enabled and pending bits
    uint8_t mask = 0;
    for (int i = 0; i < 4; i++)
        if (icuEnable[i] & edge)
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

void Dsp::updateReadFifo() {
    // Fill the read FIFO until it's finished or full
    uint8_t area = (dspPcfg >> 12);
    while (readLength > 0 && readFifo.size() < 16) {
        // Read a value from DMA and adjust the address and length
        readFifo.push(dmaRead(area, (dmaSrcAddr[0] & ~0xFFFF) | dspPadr));
        dspPadr += (dspPcfg >> 1) & 0x1;
        readLength -= (readLength <= 16);
    }

    // Update read FIFO status and trigger an interrupt if an enabled condition is met
    dspPsts &= ~(readLength == 0); // Active
    dspPsts |= BIT(6) | ((readFifo.size() >= 16) << 5); // Not empty, full
    if (dspPcfg & dspPsts & 0x60)
        core->interrupts.sendInterrupt(ARM11, 0x4A);
}

uint32_t Dsp::readTmrCount(int i) {
    // Update the counter based on its underflow timestamp if scheduled
    if (tmrCycles[i] != -1) {
        uint8_t shift = (tmrCtrl[i] & 0x3);
        shift += 1 + (shift == 3);
        uint64_t value = std::max<int64_t>(0, tmrCycles[i] - core->globalCycles);
        if (i) value = (value * 4) / 5; // 1.25x slower
        tmrCount[i] = (value >> shift);
    }

    // Read the current counter if updating, or the latched counter if frozen
    return (tmrCtrl[i] & BIT(9)) ? tmrCount[i] : tmrLatches[i];
}

uint16_t Dsp::readHpiCmd(int i) {
    // Read from one of the DSP-side CMD registers and clear its update flags
    hpiSts &= ~BIT(i ? (11 + i) : 8);
    dspPsts &= ~BIT(13 + i);
    updateIcuState();
    return dspCmd[i];
}

uint16_t Dsp::readDmaEnd(int i) {
    // Read from one of the DMA end flag registers and clear it
    uint16_t value = dmaEnd[i];
    dmaEnd[i] = 0;
    dmaSignals[i] = false;
    updateIcuState();
    return value;
}

void Dsp::writeTmrCtrl(int i, uint16_t value) {
    // Update the timer before changing things
    readTmrCount(i);

    // Write to one of the timer control registers, with bit 8/9 optionally force set
    // TODO: handle the breakpoint request bit
    uint16_t old = tmrCtrl[i];
    tmrCtrl[i] = (value & 0xFB5F) | ((tmrCtrl[!i] & BIT(13)) >> (5 - i));

    // Update the other control register if its forced bit was newly set
    if (~old & tmrCtrl[i] & BIT(13))
        writeTmrCtrl(!i, tmrCtrl[!i]);

    // Latch the timer's counter if its update bit was newly cleared
    if (old & ~tmrCtrl[i] & BIT(9))
        tmrLatches[i] = tmrCount[i];

    // Reload the timer's counter if requested
    if (value & BIT(10))
        tmrCount[i] = tmrReload[i];

    // Clear the timer's signal if requested
    if (value & BIT(7))
        tmrSignals[i] = false;

    // Reschedule the timer after changing things
    updateIcuState();
    scheduleTmr(i);
}

void Dsp::writeTmrEvent(int i, uint16_t value) {
    // Trigger a timer event for certain modes if bit 0 is set
    if (~value & BIT(0)) return;
    uint8_t mode = (tmrCtrl[i] >> 2) & 0x7;
    if (mode == 3) { // Event count
        // Decrement the counter until it hits zero, and signal when it does
        if (!tmrCount[i] || --tmrCount[i]) return;
        tmrCycles[i] = core->globalCycles;
        underflowTmr(i);
    }
    else if (mode >= 4 && mode <= 6) { // Watchdog
        // Reload the counter and reschedule the timer
        tmrCount[i] = tmrReload[i];
        scheduleTmr(i);
    }
}

void Dsp::writeTmrReloadL(int i, uint16_t value) {
    // Write to the low part of a timer reload value
    tmrReload[i] = (tmrReload[i] & ~0xFFFF) | value;
}

void Dsp::writeTmrReloadH(int i, uint16_t value) {
    // Write to the high part of a timer reload value
    tmrReload[i] = (tmrReload[i] & 0xFFFF) | ((value & 0x8003) << 16);
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

void Dsp::writeMiuPageX(uint16_t value) {
    // Write to the MIU page X register
    miuPageX = value;
}

void Dsp::writeMiuPageY(uint16_t value) {
    // Write to the MIU page Y register
    miuPageY = (value & 0xFF);
}

void Dsp::writeMiuPageZ(uint16_t value) {
    // Write to the MIU page Z register
    miuPageZ = value;
}

void Dsp::writeMiuSize0(uint16_t value) {
    // Write to the MIU page 0 size register and auto-adjust page Y
    miuSize0 = std::min(0x4000, std::max(0x100, value & 0x7F00)) | (value & 0x3F);
    miuBoundX = ((miuSize0 << 10) & 0xFC00);
    miuBoundY = ((miuSize0 << 2) & 0x1FC00) + miuBoundX;
}

void Dsp::writeMiuMisc(uint16_t value) {
    // Write to the MIU miscellaneous config register
    // TODO: handle bits other than PGM?
    miuMisc = (value & 0x79) | 0x4;
}

void Dsp::writeMiuIoBase(uint16_t value) {
    // Write to the MIU I/O base address register
    miuIoBase = (value & 0xFC00);
    LOG_INFO("Remapping DSP I/O registers to 0x%X\n", miuIoBase);
}

void Dsp::writeDmaStart(uint16_t value) {
    // Perform DMA transfers for any started channels (except 0 for PDATA)
    for (int i = 1; i < 8; i++)
        if (value & BIT(i)) dmaTransfer(i);
}

void Dsp::writeDmaSelect(uint16_t value) {
    // Write to the DMA channel select register
    dmaSelect = (value & 0x7);
}

void Dsp::writeDmaSrcAddrL(uint16_t value) {
    // Write to the low part of the selected DMA source address
    dmaSrcAddr[dmaSelect] = (dmaSrcAddr[dmaSelect] & ~0xFFFF) | value;
}

void Dsp::writeDmaSrcAddrH(uint16_t value) {
    // Write to the high part of the selected DMA source address
    dmaSrcAddr[dmaSelect] = (dmaSrcAddr[dmaSelect] & 0xFFFF) | (value << 16);
}

void Dsp::writeDmaDstAddrL(uint16_t value) {
    // Write to the low part of the selected DMA destination address
    dmaDstAddr[dmaSelect] = (dmaDstAddr[dmaSelect] & ~0xFFFF) | value;
}

void Dsp::writeDmaDstAddrH(uint16_t value) {
    // Write to the high part of the selected DMA destination address
    dmaDstAddr[dmaSelect] = (dmaDstAddr[dmaSelect] & 0xFFFF) | (value << 16);
}

void Dsp::writeDmaSize(int i, uint16_t value) {
    // Write to one of the selected DMA size registers with a minimum of 1
    dmaSize[i][dmaSelect] = std::max<uint16_t>(0x1, value);
}

void Dsp::writeDmaSrcStep(int i, uint16_t value) {
    // Write to one of the selected DMA source step registers
    dmaSrcStep[i][dmaSelect] = value;
}

void Dsp::writeDmaDstStep(int i, uint16_t value) {
    // Write to one of the selected DMA destination step registers
    dmaDstStep[i][dmaSelect] = value;
}

void Dsp::writeDmaAreaCfg(uint16_t value) {
    // Write to the selected DMA area config register
    dmaAreaCfg[dmaSelect] = (value & 0xF7FF);
}

void Dsp::writeDmaCtrl(uint16_t value) {
    // Write to the selected DMA control register and transfer if started
    // TODO: handle bits other than interrupt enable?
    dmaCtrl[dmaSelect] = (value & 0xFF);
    if (value & BIT(14)) dmaTransfer(dmaSelect);
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

void Dsp::writeAudOutCtrl(uint16_t value) {
    // Write to the audio output control register
    // TODO: handle bits other than interrupt enable?
    audOutCtrl = value;
    updateIcuState();
}

void Dsp::writeAudOutEnable(uint16_t value) {
    // Write to the audio output enable register
    audOutEnable = (value & 0x8000);

    // Schedule the audio FIFO event at the current frequency if newly enabled
    if (audScheduled || !audOutEnable || !audCycles) return;
    core->schedule(DSP_SEND_AUDIO, audCycles);
    audScheduled = true;
}

void Dsp::writeAudOutFifo(uint16_t value) {
    // Write a 16-bit sample to the audio output FIFO if there's room
    if (audOutFifo.size() >= 16) return;
    audOutFifo.push(value);

    // Clear the FIFO empty bit and set the FIFO full bit if full
    audOutStatus = (audOutStatus & ~BIT(4)) | ((audOutFifo.size() >= 16) << 3);
    updateIcuState();
}

void Dsp::writeAudOutFlush(uint16_t value) {
    // Write to the audio output flush register and flush the FIFO if requested
    audOutFlush = (value & 0x3);
    if (value & BIT(2)) flushAudOut();
}

uint16_t Dsp::readPdata() {
    // Pop a value from the read FIFO and update it if necessary
    uint16_t value = readFifo.front();
    readFifo.pop();
    dspPsts &= ~((readFifo.empty() << 6) | BIT(5)); // Not empty, full
    if (readLength > 0) updateReadFifo();
    return value;
}

uint16_t Dsp::readRep(int i) {
    // Read from one of the ARM-side REP registers and clear its update flags
    dspPsts &= ~BIT(10 + i);
    hpiSts &= ~BIT(5 + i);
    return dspRep[i];
}

void Dsp::writePdata(uint16_t mask, uint16_t value) {
    // Write a value to DMA instantly and adjust the address
    dmaWrite(dspPcfg >> 12, (dmaDstAddr[0] & ~0xFFFF) | dspPadr, value & mask);
    dspPadr += (dspPcfg >> 1) & 0x1;

    // Trigger a FIFO empty interrupt if enabled
    if (dspPcfg & BIT(8))
        core->interrupts.sendInterrupt(ARM11, 0x4A);
}

void Dsp::writePadr(uint16_t mask, uint16_t value) {
    // Write to the DSP_PADR register
    dspPadr = (dspPadr & ~mask) | (value & mask);
}

void Dsp::writePcfg(uint16_t mask, uint16_t value) {
    // Write to the DSP_PCFG register
    uint16_t old = dspPcfg;
    dspPcfg = (dspPcfg & ~mask) | (value & mask);

    // Start or stop a read FIFO transfer based on its start bit
    if (dspPcfg & BIT(4)) {
        uint8_t len = (dspPcfg >> 2) & 0x3;
        readLength = len ? (4 << len) : 1;
        dspPsts |= BIT(0); // Active
        updateReadFifo();
    }
    else {
        readLength = 0;
    }

    // Reset the DSP if the reset bit was newly set
    if (!(~old & dspPcfg & BIT(0))) return;
    LOG_INFO("Restarting Teak DSP execution\n");
    core->teak.cycles = 0;
    core->teak.regPc = 0;
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
