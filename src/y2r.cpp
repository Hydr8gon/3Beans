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

void Y2r::outputPixel(uint32_t ofs, uint8_t y, uint8_t u, uint8_t v) {
    // Swizzle the pixel buffer offset if enabled
    if (y2rCnt & BIT(12)) {
        uint32_t s = (ofs % y2rWidth), t = (ofs / y2rWidth);
        ofs = (s & 0x1) | ((s << 1) & 0x4) | ((s << 2) & 0x10);
        ofs |= ((t << 1) & 0x2) | ((t << 2) & 0x8) | ((t << 3) & 0x20);
        ofs += (s & ~0x7) << 3;
    }

    // Stub conversion by simply using luminance for now
    // TODO: do this properly, obviously
    lineBuf[ofs] = (y << 24) | (y << 16) | (y << 8) | 0xFF;

    // Push the buffer to the output FIFO in the selected format once 8 lines are done
    if (ofs != y2rWidth * 8 - 1) return;
    switch ((y2rCnt >> 8) & 0x3) {
    case 0x0: // RGBA8
        for (int i = 0; i < y2rWidth * 8; i++) {
            output.push(lineBuf[i] >> 0);
            output.push(lineBuf[i] >> 8);
            output.push(lineBuf[i] >> 16);
            output.push(lineBuf[i] >> 24);
        }
        return;
    case 0x1: // RGB8
        for (int i = 0; i < y2rWidth * 8; i++) {
            output.push(lineBuf[i] >> 8);
            output.push(lineBuf[i] >> 16);
            output.push(lineBuf[i] >> 24);
        }
        return;
    case 0x2: // RGB5A1
        for (int i = 0; i < y2rWidth * 8; i++) {
            uint8_t r = lineBuf[i] >> 24, g = lineBuf[i] >> 16, b = lineBuf[i] >> 8;
            uint16_t value = ((r * 31 / 255) << 11) | ((g * 31 / 255) << 6) | ((b * 31 / 255) << 1) | 0x1;
            output.push(value >> 0);
            output.push(value >> 8);
        }
        return;
    default: // RGB565
        for (int i = 0; i < y2rWidth * 8; i++) {
            uint8_t r = lineBuf[i] >> 24, g = lineBuf[i] >> 16, b = lineBuf[i] >> 8;
            uint16_t value = ((r * 31 / 255) << 11) | ((g * 63 / 255) << 5) | (b * 31 / 255);
            output.push(value >> 0);
            output.push(value >> 8);
        }
        return;
    }
}

void Y2r::triggerFifo() {
    // Schedule a FIFO update if one hasn't been already
    if (scheduled) return;
    core->schedule(Task(Y2R0_UPDATE + id), 1);
    scheduled = true;
}

void Y2r::update() {
    // Convert YUV input to RGBA output until finished or more input is needed
    bool conds[3] = {};
    while (y2rCnt & BIT(31)) {
        // Process data if there's enough for 8 lines based on input format
        uint8_t y, u, v;
        switch (y2rCnt & 0x7) {
        case 0x0: // 8-bit 4:2:2
            // Check if there are enough inputs using 8-bit 4:2:2 format
            conds[0] = (inputs[0].size() < y2rWidth * 8);
            conds[1] = (inputs[1].size() < y2rWidth * 4);
            conds[2] = (inputs[2].size() < y2rWidth * 4);
            if (conds[0] || conds[1] || conds[2]) goto finish;

            // Process 8 lines of data using 8-bit 4:2:2 format
            for (uint32_t i = 0; i < y2rWidth * 8; i++) {
                y = inputs[0].front(), inputs[0].pop();
                if (!(i & 0x1)) {
                    u = inputs[1].front(), inputs[1].pop();
                    v = inputs[2].front(), inputs[2].pop();
                }
                outputPixel(i, y, u, v);
            }
            break;

        case 0x1: // 8-bit 4:2:0
            // Check if there are enough inputs using 8-bit 4:2:0 format
            conds[0] = (inputs[0].size() < y2rWidth * 8);
            conds[1] = (inputs[1].size() < y2rWidth * 2);
            conds[2] = (inputs[2].size() < y2rWidth * 2);
            if (conds[0] || conds[1] || conds[2]) goto finish;

            // Process 8 lines of data using 8-bit 4:2:0 format
            for (uint32_t i = 0; i < y2rWidth * 8; i++) {
                y = inputs[0].front(), inputs[0].pop();
                if (!(i & 0x3)) {
                    u = inputs[1].front(), inputs[1].pop();
                    v = inputs[2].front(), inputs[2].pop();
                }
                outputPixel(i, y, u, v);
            }
            break;

        default:
            // Catch unimplemented input formats
            y2rCnt &= ~BIT(31);
            goto finish;
        }

        // Count the processed lines and finish converting if at the end
        outputLines += 8;
        if (outputLines < y2rHeight) continue;
        y2rCnt &= ~BIT(31);
        LOG_INFO("Y2R conversion reached end of data\n");
        break;
    }

finish:
    // Update input FIFO IRQ/DRQ states if enabled
    for (int i = 0; i < 3; i++) {
        if ((y2rCnt & BIT(31)) && conds[i]) {
            y2rCnt |= BIT(24 + i);
            if (y2rCnt & BIT(22)) {
                core->cdmas[CDMA0].setDrq((id ? 0x12 : 0x6) + i);
                core->cdmas[CDMA1].setDrq((id ? 0x12 : 0x6) + i);
            }
        }
        else {
            core->cdmas[CDMA0].clearDrq((id ? 0x12 : 0x6) + i);
            core->cdmas[CDMA1].clearDrq((id ? 0x12 : 0x6) + i);
        }
    }

    // Update output FIFO IRQ/DRQ states if enabled
    if (!output.empty()) {
        y2rCnt |= BIT(28);
        if (y2rCnt & BIT(23)) {
            core->cdmas[CDMA0].setDrq(id ? 0x16 : 0xA);
            core->cdmas[CDMA1].setDrq(id ? 0x16 : 0xA);
        }
    }
    else {
        core->cdmas[CDMA0].clearDrq(id ? 0x16 : 0xA);
        core->cdmas[CDMA1].clearDrq(id ? 0x16 : 0xA);
    }

    // Trigger an ARM11 general interrupt if enabled and any condition was set
    if ((y2rCnt & BIT(29)) && (y2rCnt & 0x1F000000))
        core->interrupts.sendInterrupt(ARM11, id ? 0x4E : 0x4B);
    scheduled = false;
}

uint32_t Y2r::readOutputRgba() {
    // Trigger an ARM11 transfer end interrupt if enabled and about to run out
    if ((y2rCnt & 0xC0000000) == BIT(30) && !output.empty() && output.size() <= 4)
        core->interrupts.sendInterrupt(ARM11, id ? 0x4E : 0x4B);

    // Read up to 4 bytes from the RGBA output FIFO
    // TODO: figure out FIFO size/limits
    uint32_t value = 0;
    for (int i = 0; i < 32 && !output.empty(); i += 8)
        value |= output.front() << i, output.pop();
    triggerFifo();
    return value;
}

void Y2r::writeCnt(uint32_t mask, uint32_t value) {
    // Write to the Y2R control register
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    uint32_t mask2 = (mask & 0xE0C31F07), old = y2rCnt;
    y2rCnt = (y2rCnt & ~mask2) | (value & mask2);

    // Acknowledge IRQ bits if set and reset the FIFO if newly started
    y2rCnt &= ~((value &= mask) & 0x1F000000);
    if (!(~old & value & BIT(31))) return;
    outputLines = 0;
    triggerFifo();

    // Log the conversion and check for any missing features used
    LOG_INFO("Starting Y2R conversion with input format 0x%X, output format 0x%X, width %d, and height %d\n",
        y2rCnt & 0x7, (y2rCnt >> 8) & 0x3, y2rWidth, y2rHeight);
    if ((y2rCnt & 0x7) >= 2)
        LOG_CRIT("Y2R conversion using unimplemented input format: 0x%X\n", y2rCnt & 0x7);
    if (uint32_t bits = y2rCnt & 0x30C00)
        LOG_WARN("Y2R conversion using unhandled control bits: 0x%X\n", bits);
}

void Y2r::writeWidth(uint16_t mask, uint16_t value) {
    // Write to the Y2R input width register
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x3F8;
    y2rWidth = (y2rWidth & ~mask) | (value & mask);
}

void Y2r::writeHeight(uint16_t mask, uint16_t value) {
    // Write to the Y2R input height register
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    mask &= 0x3FF;
    y2rHeight = (y2rHeight & ~mask) | (value & mask);
}

void Y2r::writeInputY(uint32_t mask, uint32_t value) {
    // Write up to 4 bytes to the Y input FIFO
    // TODO: figure out FIFO size/limits
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    for (int i = 0; i < 32; i += 8)
        if (mask & BIT(i)) inputs[0].push(value >> i);
    triggerFifo();
}

void Y2r::writeInputU(uint32_t mask, uint32_t value) {
    // Write up to 4 bytes to the U input FIFO
    // TODO: figure out FIFO size/limits
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    for (int i = 0; i < 32; i += 8)
        if (mask & BIT(i)) inputs[1].push(value >> i);
    triggerFifo();
}

void Y2r::writeInputV(uint32_t mask, uint32_t value) {
    // Write up to 4 bytes to the V input FIFO
    // TODO: figure out FIFO size/limits
    if (id && !core->n3dsMode) return; // N3DS-exclusive
    for (int i = 0; i < 32; i += 8)
        if (mask & BIT(i)) inputs[2].push(value >> i);
    triggerFifo();
}
