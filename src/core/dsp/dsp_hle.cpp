/*
    Copyright 2023-2026 Hydr8gon

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

#include "../core.h"

void DspHle::update() {
    // HLE the Teak based on its current state
    switch (state) {
    case STATE_HANDSHAKE:
        // Send the initial handshake responses
        sendResponse(0, 0x1);
        sendResponse(1, 0x1);
        sendResponse(2, 0x1);

        // Schedule the next stage shortly
        state = STATE_INITPIPES;
        core.schedule(DSP_HLE_UPDATE, 10000);
        return;

    case STATE_INITPIPES:
        // Fill a struct with info for pipe 4
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB3 << 1), 0xDEF);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB4 << 1), 0x80);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB5 << 1), 0x0);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB6 << 1), 0x20);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB7 << 1), 0x4);

        // Fill a struct with info for pipe 5
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB8 << 1), 0xE2F);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCB9 << 1), 0x80);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCBA << 1), 0x0);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCBB << 1), 0x0);
        core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (0xCBC << 1), 0x5);

        // Send the pipe struct base address and set the ready semaphore
        sendResponse(2, 0xC9F);
        setSemaphore(15);

        // Schedule the next stage shortly
        state = STATE_PIPE_5;
        core.schedule(DSP_HLE_UPDATE, 10000);
        return;

    case STATE_PIPE_5:
        // Send a pipe 5 response and set the ready semaphore
        sendResponse(2, 0x5);
        setSemaphore(15);

        // Schedule the next stage in a bit
        state = STATE_PIPE_4;
        core.schedule(DSP_HLE_UPDATE, 100000);
        return;

    case STATE_PIPE_4:
        // Send a pipe 4 response and set the ready semaphore
        sendResponse(2, 0x4);
        setSemaphore(15);

        // Schedule the first periodic update in a while
        state = STATE_READY;
        core.schedule(DSP_HLE_UPDATE, 10000000);
        return;

    case STATE_READY:
        // Send a pipe 4 response to pretend things are happening
        sendResponse(2, 0x4);
        setSemaphore(15);

        // Schedule the next periodic update in a while
        core.schedule(DSP_HLE_UPDATE, 10000000);
        return;
    }
}

void DspHle::sendResponse(int i, uint16_t value) {
    // Write to a response register and set its update flag
    dspRep[i] = value;
    LOG_INFO("Writing DSP reply register %d: 0x%X\n", i, dspRep[i]);
    dspPsts |= BIT(10 + i);

    // Trigger an interrupt if enabled for the written reply
    if (dspPcfg & BIT(9 + i))
        core.interrupts.sendInterrupt(ARM11, 0x4A);
}

void DspHle::setSemaphore(int bit) {
    // Set an outgoing semaphore bit
    dspSem |= BIT(bit);
    updateSemaIrq();
}

void DspHle::updateSemaIrq() {
    // Update the semaphore IRQ flag and trigger an interrupt if set
    if (!(dspSem & ~dspPmask)) {
        dspPsts &= ~BIT(9);
    }
    else if (~dspPsts & BIT(9)) {
        LOG_INFO("Triggering DSP-to-ARM semaphore with flags: 0x%X\n", dspSem & ~dspPmask);
        core.interrupts.sendInterrupt(ARM11, 0x4A);
        dspPsts |= BIT(9);
    }
}

uint16_t DspHle::readPdata() {
    // Pop a value from the read FIFO and update status bits
    uint16_t value = readFifo.front();
    readFifo.pop();
    dspPsts &= ~((readFifo.empty() << 6) | BIT(5)); // Not empty, full
    return value;
}

uint16_t DspHle::readRep(int i) {
    // Read from a DSP response register and clear its update flag
    dspPsts &= ~BIT(10 + i);
    return dspRep[i];
}

void DspHle::writePdata(uint16_t mask, uint16_t value) {
    // Write a value to DMA instantly and adjust the address
    switch (dspPcfg >> 12) {
        case 7: core.memory.write<uint16_t>(ARM11, dspPadr & ~0x1, value); break; // AHBM
        case 5: core.memory.write<uint16_t>(ARM11, 0x1FF00000 + (dspPadr << 1), value); break; // Code
        case 0: core.memory.write<uint16_t>(ARM11, 0x1FF40000 + (dspPadr << 1), value); break; // Data
    default:
        LOG_CRIT("Attempted DSP DMA read from unknown area: %d\n", area);
        break;
    }
    dspPadr += (dspPcfg >> 1) & 0x1;

    // Trigger a FIFO empty interrupt if enabled
    if (dspPcfg & BIT(8))
        core.interrupts.sendInterrupt(ARM11, 0x4A);
}

void DspHle::writePadr(uint16_t mask, uint16_t value) {
    // Write to the DSP data address register
    dspPadr = (dspPadr & ~mask) | (value & mask);
}

void DspHle::writePcfg(uint16_t mask, uint16_t value) {
    // Write to the DSP configuration register
    uint16_t old = dspPcfg;
    dspPcfg = (dspPcfg & ~mask) | (value & mask);

    // Start a read FIFO transfer if its bit was newly set
    if (~old & dspPcfg & BIT(4)) {
        uint8_t len = (dspPcfg >> 2) & 0x3;
        len = len ? (4 << len) : 1;
        readFifo = {};

        // Fill the read DMA with all values instantly and adjust the address
        for (int i = 0; i < len; i++) {
            switch (dspPcfg >> 12) {
                case 7: readFifo.push(core.memory.read<uint16_t>(ARM11, dspPadr & ~0x1)); break; // AHBM
                case 5: readFifo.push(core.memory.read<uint16_t>(ARM11, 0x1FF00000 + (dspPadr << 1))); break; // Code
                case 0: readFifo.push(core.memory.read<uint16_t>(ARM11, 0x1FF40000 + (dspPadr << 1))); break; // Data
            default:
                LOG_CRIT("Attempted DSP DMA read from unknown area: %d\n", area);
                readFifo.push(0);
                break;
            }
            dspPadr += (dspPcfg >> 1) & 0x1;
        }

        // Update read FIFO status and trigger an interrupt if an enabled condition is met
        dspPsts |= BIT(6) | ((readFifo.size() >= 16) << 5); // Not empty, full
        if (dspPcfg & dspPsts & 0x60)
            core.interrupts.sendInterrupt(ARM11, 0x4A);
    }

    // Reset the DSP state if the reset bit was newly set
    if (!(~old & dspPcfg & BIT(0))) return;
    LOG_INFO("Restarting HLE DSP execution\n");
    state = STATE_HANDSHAKE;
    core.schedule(DSP_HLE_UPDATE, 10000);
}

void DspHle::writePsem(uint16_t mask, uint16_t value) {
    // Ignore incoming semaphores so they appear acknowledged
    LOG_INFO("Triggering ARM-to-DSP semaphore with flags: 0x%X\n", value);
}

void DspHle::writePmask(uint16_t mask, uint16_t value) {
    // Set flags in the DSP semaphore interrupt mask
    dspPmask = (dspPmask & ~mask) | (value & mask);
    updateSemaIrq();
}

void DspHle::writePclear(uint16_t mask, uint16_t value) {
    // Clear flags in the DSP semaphore register
    dspSem &= ~(value & mask);
    updateSemaIrq();
}

void DspHle::writeCmd(int i, uint16_t mask, uint16_t value) {
    // Write to a DSP command register but ignore its flag to appear acknowledged
    dspCmd[i] = (dspCmd[i] & ~mask) | (value & mask);
    LOG_INFO("Writing DSP command register %d: 0x%X\n", i, dspCmd[i]);
}
