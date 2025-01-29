/*
    Copyright 2023-2024 Hydr8gon

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

void Pdc::drawScreen(bool bot, uint32_t *buffer) {
    // Draw a screen's framebuffer in the selected format if enabled
    if (~pdcInterruptType[bot] & BIT(0)) return;
    int width = (bot ? 320 : 400);
    switch (uint8_t fmt = pdcFramebufFormat[bot] & 0x7) {
    case 0: // RGBA8
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t color = core->memory.read<uint32_t>(ARM11A, pdcFramebufLt0[bot] + (x * 240 + 239 - y) * 4);
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
                uint32_t address = pdcFramebufLt0[bot] + (x * 240 + 239 - y) * 3;
                uint8_t r = core->memory.read<uint8_t>(ARM11A, address + 2);
                uint8_t g = core->memory.read<uint8_t>(ARM11A, address + 1);
                uint8_t b = core->memory.read<uint8_t>(ARM11A, address + 0);
                buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
            }
        }
        return;

    default:
        LOG_CRIT("Unimplemented framebuffer format: %d\n", fmt);
        return;
    }
}

void Pdc::drawFrame() {
    // Allow up to 2 framebuffers to be queued
    if (buffers.size() == 2) return;
    uint32_t *buffer = new uint32_t[400 * 480];
    memset(buffer, 0, 400 * 480 * sizeof(uint32_t));

    // Draw the top and bottom screens
    drawScreen(false, &buffer[0]);
    drawScreen(true, &buffer[240 * 400 + 40]);

    // Add the frame to the queue
    mutex.lock();
    buffers.push(buffer);
    ready.store(true);
    mutex.unlock();
}

void Pdc::writeFramebufLt0(bool bot, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_LT0 register
    pdcFramebufLt0[bot] = (pdcFramebufLt0[bot] & ~mask) | (value & mask);
}

void Pdc::writeFramebufFormat(bool bot, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_FRAMEBUF_FORMAT register
    // TODO: handle zoom bits and maybe others?
    pdcFramebufFormat[bot] = (pdcFramebufFormat[bot] & ~mask) | (value & mask);
}

void Pdc::writeInterruptType(bool bot, uint32_t mask, uint32_t value) {
    // Write to a screen's PDC_INTERRUPT_TYPE register
    // TODO: actually implement PDC interrupts
    pdcInterruptType[bot] = (pdcInterruptType[bot] & ~mask) | (value & mask);
}
