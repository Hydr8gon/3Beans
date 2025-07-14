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

#include <cstring>
#include "core.h"

uint32_t *Pdc::getFrame() {
    // Get the next frame in the queue when one is ready
    if (!ready.load()) return nullptr;
    mutex.lock();
    uint32_t *fb = buffers.front();
    buffers.pop();
    ready.store(!buffers.empty());
    mutex.unlock();
    return fb;
}

void Pdc::drawScreen(int i, uint32_t *buffer) {
    // Draw a screen's framebuffer in the selected format if enabled
    if (~pdcInterruptType[i] & BIT(0)) return;
    int width = (i ? 320 : 400);
    uint32_t base = (pdcFramebufSelAck[i] & BIT(0)) ? pdcFramebufLt1[i] : pdcFramebufLt0[i];
    switch (pdcFramebufFormat[i] & 0x7) {
    case 0: // RGBA8
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t address = base + (x + 1) * pdcFramebufStep[i] - (y - 1) * 4;
                uint32_t color = core->memory.read<uint32_t>(ARM11A, address);
                uint8_t r = (color >> 24) & 0xFF;
                uint8_t g = (color >> 16) & 0xFF;
                uint8_t b = (color >> 8) & 0xFF;
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;

    case 1: // RGB8
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t address = base + (x + 1) * pdcFramebufStep[i] - (y - 1) * 3;
                uint8_t r = core->memory.read<uint8_t>(ARM11, address + 2);
                uint8_t g = core->memory.read<uint8_t>(ARM11, address + 1);
                uint8_t b = core->memory.read<uint8_t>(ARM11, address + 0);
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;

    case 2: // RGB565
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t address = base + (x + 1) * pdcFramebufStep[i] - (y - 1) * 2;
                uint16_t color = core->memory.read<uint16_t>(ARM11, address);
                uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
                uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
                uint8_t b = ((color >> 0) & 0x1F) * 255 / 31;
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;

    case 3: // RGB5A1
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t address = base + (x + 1) * pdcFramebufStep[i] - (y - 1) * 2;
                uint16_t color = core->memory.read<uint16_t>(ARM11, address);
                uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
                uint8_t g = ((color >> 6) & 0x1F) * 255 / 31;
                uint8_t b = ((color >> 1) & 0x1F) * 255 / 31;
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;

    default: // RGBA4
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t address = base + (x + 1) * pdcFramebufStep[i] - (y - 1) * 2;
                uint16_t color = core->memory.read<uint16_t>(ARM11, address);
                uint8_t r = ((color >> 12) & 0xF) * 255 / 15;
                uint8_t g = ((color >> 8) & 0xF) * 255 / 15;
                uint8_t b = ((color >> 4) & 0xF) * 255 / 15;
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;
    }
}

void Pdc::drawFrame() {
    // Allow up to 2 framebuffers to be queued
    if (buffers.size() == 2) return;
    uint32_t *buffer = new uint32_t[400 * 480];
    memset(buffer, 0, 400 * 480 * sizeof(uint32_t));

    // Draw the top and bottom screens
    drawScreen(0, &buffer[0]);
    drawScreen(1, &buffer[240 * 400 + 40]);

    // Add the frame to the queue
    mutex.lock();
    buffers.push(buffer);
    ready.store(true);
    mutex.unlock();

    // Trigger PDC interrupts at V-blank if not disabled
    // TODO: handle timings for different modes properly
    if (((pdcInterruptType[0] >> 8) & 0x7) != 7)
        core->interrupts.sendInterrupt(ARM11, 0x2A);
    if (((pdcInterruptType[1] >> 8) & 0x7) != 7)
        core->interrupts.sendInterrupt(ARM11, 0x2B);
}

void Pdc::writeFramebufLt0(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_LT0 register
    mask &= 0xFFFFFFF0;
    pdcFramebufLt0[i] = (pdcFramebufLt0[i] & ~mask) | (value & mask);
}

void Pdc::writeFramebufLt1(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_LT1 register
    mask &= 0xFFFFFFF0;
    pdcFramebufLt1[i] = (pdcFramebufLt1[i] & ~mask) | (value & mask);
}

void Pdc::writeFramebufFormat(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_FORMAT register
    // TODO: handle zoom bits and maybe others?
    mask &= 0xFFFF0377;
    pdcFramebufFormat[i] = (pdcFramebufFormat[i] & ~mask) | (value & mask);
}

void Pdc::writeInterruptType(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_INTERRUPT_TYPE register
    mask &= 0x10701;
    pdcInterruptType[i] = (pdcInterruptType[i] & ~mask) | (value & mask);
}

void Pdc::writeFramebufSelAck(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_SEL_ACK register
    // TODO: handle bits other than buffer select?
    mask &= 0x1;
    pdcFramebufSelAck[i] = (pdcFramebufSelAck[i] & ~mask) | (value & mask);
}

void Pdc::writeFramebufStep(int i, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_STEP register
    mask &= 0xFFFFFFF0;
    pdcFramebufStep[i] = (pdcFramebufStep[i] & ~mask) | (value & mask);
}
