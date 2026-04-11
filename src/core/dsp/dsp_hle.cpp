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
#define ADPCM_COEFFS 0xA792
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
        writeData(PIPE2_OUT + 0x4, ADPCM_COEFFS);
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
        // Process a frame and signal that it's finished
        processFrame();
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

void DspHle::processFrame() {
    // Alternate the frame base pointer
    frameBase ^= 0x20000;
    int16_t samples[8 * 20][2] = {};

    // Loop through inputs to update and mix them
    for (int i = 0; i < 24; i++) {
        // Get an input's base addresses and dirty flags
        uint16_t cfgBase = INPUT_CONFIG + i * 0x60;
        uint16_t stsBase = INPUT_STATUS + i * 0x6;
        uint32_t dirty = readData(cfgBase + 0x0) | (readData(cfgBase + 0x1) << 16);
        writeData(cfgBase + 0x0, 0), writeData(cfgBase + 0x1, 0);
        InputState &input = inputs[i];

        // Update the ADPCM coefficients if dirty
        if (dirty & BIT(2)) {
            uint16_t coeffBase = ADPCM_COEFFS + i * 0x10;
            for (int j = 0; j < 8; j++) {
                input.adpcmCoeffs[j][0] = readData(coeffBase + j * 2 + 0);
                input.adpcmCoeffs[j][1] = readData(coeffBase + j * 2 + 1);
            }
        }

        // Update embedded buffer parameters if dirty
        if (dirty & (BIT(4) | BIT(21) | BIT(30))) { // TODO: which bits are which?
            input.position = (readData(cfgBase + 0x52) << 16) | readData(cfgBase + 0x53);
            input.buffers[4].address = (readData(cfgBase + 0x56) << 16) | readData(cfgBase + 0x57);
            input.buffers[4].count = (readData(cfgBase + 0x58) << 16) | readData(cfgBase + 0x59);
            input.format = readData(cfgBase + 0x5A);
            input.buffers[4].adpcmPrev[0] = readData(cfgBase + 0x5C);
            input.buffers[4].adpcmPrev[1] = readData(cfgBase + 0x5D);
            input.buffers[4].seqId = readData(cfgBase + 0x5F);
        }

        // Update the play status if dirty
        bool wasPlaying = (input.playFlags & BIT(0));
        if (dirty & BIT(16)) {
            uint16_t playStatus = readData(cfgBase + 0x50);
            input.playFlags = (playStatus ? BIT(0) : BIT(8));
        }

        // Update the resample rate if dirty
        if (dirty & BIT(18)) {
            uint32_t rate = readData(cfgBase + 0x1A) | (readData(cfgBase + 0x1B) << 16);
            input.rate = *(float*)&rate;
        }

        // Update queued buffer parameters if dirty
        if (dirty & BIT(19)) {
            uint16_t bufDirty = readData(cfgBase + 0x25);
            writeData(cfgBase + 0x25, 0);
            for (int b = 0; b < 4; b++) {
                if (~bufDirty & BIT(b)) continue;
                uint16_t bufBase = cfgBase + 0x26 + b * 0xA;
                input.buffers[b].address = (readData(bufBase + 0x0) << 16) | readData(bufBase + 0x1);
                input.buffers[b].count = (readData(bufBase + 0x2) << 16) | readData(bufBase + 0x3);
                input.buffers[b].adpcmPrev[0] = readData(bufBase + 0x5);
                input.buffers[b].adpcmPrev[1] = readData(bufBase + 0x6);
                input.buffers[b].seqId = readData(bufBase + 0x8);
            }
        }

        // Update the sync count if dirty
        if (dirty & BIT(28))
            input.syncCount = readData(cfgBase + 0x51);

        // Mix input into the frame output if it's playing
        if (input.playFlags & BIT(0)) {
            // Load the embedded buffer at playback start
            InputBuffer &cur = input.current;
            if (!wasPlaying) cur = input.buffers[4];

            // Step through buffers until frame end or input end
            for (int j = 0; j < 8 * 20; j++) {
                uint32_t position = input.position;
                if (position < cur.count) {
                    // Add a sample based on format and adjust the position (half-volume for now)
                    int32_t value;
                    switch (input.format & 0xF) {
                    case 0x0: case 0x1: case 0x3: // PCM8 mono
                        value = core.memory.read<uint8_t>(ARM11, cur.address + position);
                        samples[j][0] += int16_t(value << 8) / 2;
                        samples[j][1] += int16_t(value << 8) / 2;
                        break;
                    case 0x2: // PCM8 stereo
                        value = core.memory.read<uint16_t>(ARM11, cur.address + position * 2);
                        samples[j][0] += int16_t(value << 8) / 2;
                        samples[j][1] += int16_t(value & 0xFF00) / 2;
                        break;
                    case 0x4: case 0x5: case 0x7: // PCM16 mono
                        value = core.memory.read<uint16_t>(ARM11, cur.address + position * 2);
                        samples[j][0] += int16_t(value) / 2;
                        samples[j][1] += int16_t(value) / 2;
                        break;
                    case 0x6: // PCM16 stereo
                        value = core.memory.read<uint32_t>(ARM11, cur.address + position * 4);
                        samples[j][0] += int16_t(value >> 0) / 2;
                        samples[j][1] += int16_t(value >> 16) / 2;
                        break;
                    case 0x8: case 0x9: case 0xB: { // ADPCM mono
                        uint32_t block = (position / 14) * 8, sample = position % 14;
                        uint8_t header = core.memory.read<uint8_t>(ARM11, cur.address + block);
                        int8_t nybble = core.memory.read<uint8_t>(ARM11, cur.address + block + sample / 2 + 1);
                        uint8_t shift = (header & 0xF) + 11;
                        int16_t *coeffs = input.adpcmCoeffs[header >> 4];
                        nybble = int8_t((sample & 0x1) ? nybble : (nybble << 4)) >> 4;
                        value = (nybble << shift) + coeffs[0] * cur.adpcmPrev[0] + coeffs[1] * cur.adpcmPrev[1];
                        value = std::max(-0x8000, std::min(0x7FFF, (value + 0x400) >> 11));
                        cur.adpcmPrev[1] = cur.adpcmPrev[0];
                        cur.adpcmPrev[0] = value;
                        samples[j][0] += value / 2;
                        samples[j][1] += value / 2;
                        break;
                    }}
                    input.position += input.rate;
                }
                else {
                    // Increment the sequence at the end of a buffer
                    input.playFlags |= BIT(8);
                    cur.seqId++;

                    // Try to find the next sequence in the buffer queue
                    InputBuffer *buffer = nullptr;
                    for (int b = 0; b < 4; b++) {
                        if (input.buffers[b].seqId != cur.seqId) continue;
                        buffer = &input.buffers[b];
                        break;
                    }

                    // Stop playing if a buffer wasn't found
                    if (!buffer) {
                        input.playFlags &= ~BIT(0);
                        break;
                    }

                    // Load the next buffer and dequeue it
                    cur = *buffer;
                    buffer->seqId = 0;
                    input.position = 0;
                    j--; // Redo sample
                }
            }
        }

        // Update the input status struct
        writeData(stsBase + 0x0, input.playFlags);
        writeData(stsBase + 0x1, input.syncCount);
        writeData(stsBase + 0x2, uint32_t(input.position) >> 16);
        writeData(stsBase + 0x3, uint32_t(input.position) >> 0);
        writeData(stsBase + 0x4, input.current.seqId);
    }

    // Output a frame's worth of samples
    for (int i = 0; i < 8 * 20; i++)
        core.csnd.sampleDsp(samples[i][0], samples[i][1]);
}

uint16_t DspHle::readData(uint16_t address) {
    // Read a value from the previous frame and forward it to the next one
    uint16_t value = core.memory.read<uint16_t>(ARM11, (frameBase ^ 0x20000) + (address << 1));
    writeData(address, value);
    return value;
}

void DspHle::writeData(uint16_t address, uint16_t value) {
    // Write a value to the current frame's data block
    core.memory.write<uint16_t>(ARM11, frameBase + (address << 1), value);
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
