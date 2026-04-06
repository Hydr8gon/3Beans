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

// Define base addresses for pipes
#define PIPE_BASE 0xC9F
#define PIPE2_OUT 0xDEF
#define PIPE2_IN 0xE2F

// Define DSP struct addresses
#define FRAME_COUNT 0xBFFF
#define INPUT_CONFIG 0x9E92
#define INPUT_STATUS 0x8680
#define INPUT_ADPCM 0xA792
#define DSP_CONFIG 0x9430
#define DSP_STATUS 0x8400
#define OUT_SAMPLES 0x8540
#define MIX_SAMPLES 0x9492
#define COMPRESS_TABLE 0x8710
#define DEBUG_STATS 0x8410
#define UNK_COEFFS1 0xA912
#define UNK_COEFFS2 0xAA12
#define UNK_COEFFS3 0xAAD2
#define QUAD_FILTER1 0xAC52
#define QUAD_FILTER2 0xAC5C

void DspHle::setAudClock(DspClock clock) {
    // Stop the HLE frame event if its clock is disabled
    if (clock == CLK_OFF) {
        cycles = 0;
        return;
    }

    // Set the frame event frequency and schedule it if newly enabled
    cycles = ((clock == CLK_33KHZ) ? 8130 : 5589) * 8 * 20;
    if (scheduled || state != STATE_RUNNING) return;
    core.schedule(DSP_HLE_UPDATE, cycles);
    scheduled = true;
}

void DspHle::update() {
    // HLE the Teak based on its current state
    switch (state) {
    case STATE_HANDSHAKE:
        // Send the initial handshake responses
        sendResponse(0, 0x1);
        sendResponse(1, 0x1);
        sendResponse(2, 0x1);

        // Schedule the next stage shortly
        state = STATE_INIT;
        core.schedule(DSP_HLE_UPDATE, 10000);
        return;

    case STATE_INIT:
        // Fill a struct with info for the pipe 2 output FIFO
        writeData(PIPE_BASE + 20, PIPE2_OUT); // Address
        writeData(PIPE_BASE + 21, 0x80); // Size
        writeData(PIPE_BASE + 22, 0x0); // Start
        writeData(PIPE_BASE + 23, 0x0); // End
        writeData(PIPE_BASE + 24, 0x4); // Info

        // Fill a struct with info for the pipe 2 input FIFO
        writeData(PIPE_BASE + 25, PIPE2_IN); // Address
        writeData(PIPE_BASE + 26, 0x80); // Size
        writeData(PIPE_BASE + 27, 0x0); // Start
        writeData(PIPE_BASE + 28, 0x0); // End
        writeData(PIPE_BASE + 29, 0x5); // Info

        // Send the pipe struct base address and set the ready semaphore
        sendResponse(2, PIPE_BASE);
        setSemaphore(15);

        // Schedule the next stage in a bit
        state = STATE_RECEIVE;
        core.schedule(DSP_HLE_UPDATE, 100000);
        return;

    case STATE_RECEIVE:
        // Acknowledge pipe 2's input and set the ready semaphore
        sendResponse(2, 0x5);
        setSemaphore(15);

        // Schedule the next stage in a bit
        state = STATE_REPLY;
        core.schedule(DSP_HLE_UPDATE, 100000);
        return;

    case STATE_REPLY:
        // Write DSP struct addresses to pipe 2's output
        writeData(PIPE2_OUT + 0x0, 0xF); // Count
        writeData(PIPE2_OUT + 0x1, FRAME_COUNT);
        writeData(PIPE2_OUT + 0x2, INPUT_CONFIG);
        writeData(PIPE2_OUT + 0x3, INPUT_STATUS);
        writeData(PIPE2_OUT + 0x4, INPUT_ADPCM);
        writeData(PIPE2_OUT + 0x5, DSP_CONFIG);
        writeData(PIPE2_OUT + 0x6, DSP_STATUS);
        writeData(PIPE2_OUT + 0x7, OUT_SAMPLES);
        writeData(PIPE2_OUT + 0x8, MIX_SAMPLES);
        writeData(PIPE2_OUT + 0x9, COMPRESS_TABLE);
        writeData(PIPE2_OUT + 0xA, DEBUG_STATS);
        writeData(PIPE2_OUT + 0xB, UNK_COEFFS1);
        writeData(PIPE2_OUT + 0xC, UNK_COEFFS2);
        writeData(PIPE2_OUT + 0xD, UNK_COEFFS3);
        writeData(PIPE2_OUT + 0xE, QUAD_FILTER1);
        writeData(PIPE2_OUT + 0xF, QUAD_FILTER2);

        // Adjust pipe 2's output end and signal that it's ready
        writeData(PIPE_BASE + 23, 0x20);
        sendResponse(2, 0x4);
        setSemaphore(15);

        // Schedule the first frame event
        state = STATE_RUNNING;
        goto schedule;

    case STATE_RUNNING:
        // Send a frame's worth of samples and swap the base pointer
        // TODO: actually produce audio
        for (int i = 0; i < 8 * 20; i++)
            core.csnd.sampleDsp(0, 0);
        base ^= 0x10000;

        // Signal that a frame is finished
        sendResponse(2, 0x4);
        setSemaphore(15);

    schedule:
        // Schedule the next frame event if enabled
        if (cycles)
            core.schedule(DSP_HLE_UPDATE, cycles);
        else
            scheduled = false;
        return;
    }
}

uint16_t DspHle::readData(uint32_t address) {
    // Read a value from DSP data memory
    return core.memory.read<uint16_t>(ARM11, 0x1FF40000 + ((address << 1) & 0x3FFFE));
}

void DspHle::writeData(uint32_t address, uint16_t value) {
    // Write a value to DSP data memory
    core.memory.write<uint16_t>(ARM11, 0x1FF40000 + ((address << 1) & 0x3FFFE), value);
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
    if (scheduled) return;
    core.schedule(DSP_HLE_UPDATE, 10000);
    scheduled = true;
}

void DspHle::writePsem(uint16_t mask, uint16_t value) {
    // Ignore incoming semaphores so they appear acknowledged
    if (value & mask)
        LOG_INFO("Triggering ARM-to-DSP semaphore with flags: 0x%X\n", value & mask);
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
