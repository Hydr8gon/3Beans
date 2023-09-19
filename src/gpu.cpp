/*
    Copyright 2023 Hydr8gon

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

uint32_t *Gpu::getFrame()
{
    // Get the next frame in the queue when one is ready
    if (!ready.load()) return nullptr;
    mutex.lock();
    uint32_t *fb = buffers.front();
    buffers.pop();
    ready.store(!buffers.empty());
    mutex.unlock();
    return fb;
}

void Gpu::drawFrame()
{
    // Allow up to 2 framebuffers to be queued
    if (buffers.size() == 2) return;
    uint32_t *buffer = new uint32_t[400 * 480];

    // Draw the top screen's framebuffer
    for (int y = 0; y < 240; y++)
    {
        for (int x = 0; x < 400; x++)
        {
            uint32_t color = core->memory.read<uint32_t>(ARM11A, pdcFramebufLt0[false] + ((x + 1) * 240 - y) * 4);
            uint8_t r = (color >> 24) & 0xFF;
            uint8_t g = (color >> 16) & 0xFF;
            uint8_t b = (color >> 8) & 0xFF;
            buffer[y * 400 + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }

    // Draw the bottom screen's framebuffer
    for (int y = 0; y < 240; y++)
    {
        for (int x = 0; x < 320; x++)
        {
            uint32_t color = core->memory.read<uint32_t>(ARM11A, pdcFramebufLt0[true] + ((x + 1) * 240 - y) * 4);
            uint8_t r = (color >> 24) & 0xFF;
            uint8_t g = (color >> 16) & 0xFF;
            uint8_t b = (color >> 8) & 0xFF;
            buffer[(y + 240) * 400 + (x + 40)] = (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }

    // Add the frame to the queue
    mutex.lock();
    buffers.push(buffer);
    ready.store(true);
    mutex.unlock();
}

void Gpu::writePdcFramebufLt0(bool bot, uint32_t mask, uint32_t value)
{
    // Write to a screen's PDC_FRAMEBUF_LT0 register
    pdcFramebufLt0[bot] = (pdcFramebufLt0[bot] & ~mask) | (value & mask);
}
