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

#include "../core.h"
#include "gpu_render_ogl.h"

GpuRenderOgl::GpuRenderOgl(Core *core, std::function<void()> &contextFunc): core(core), contextFunc(contextFunc) {
    // Create a framebuffer for rendering
    contextFunc();
    GLuint fb, tex;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    // Create a texture to back the framebuffer
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Bind everything for drawing and reading
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    contextFunc();
}

uint32_t GpuRenderOgl::getSwizzle(int x, int y) {
    // Convert buffer coordinates to a swizzled offset
    uint32_t ofs = (((y >> 3) * (bufWidth >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    ofs |= ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
    return ofs;
}

void GpuRenderOgl::submitVertex(SoftVertex &vertex) {
    // Pretend to draw something for now
    drawDirty = true;
}

void GpuRenderOgl::flushData() {
    // Check if anything has been drawn
    if (!drawDirty) return;
    contextFunc();

    // Stub the color buffer with magenta
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    // Read the color buffer and write it to memory based on format
    uint32_t *data;
    switch (colbufFmt) {
    case COL_RGBA8:
        data = new uint32_t[bufWidth * bufHeight];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x++)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 4, data[y * bufWidth + x]);
        break;
    case COL_RGB8:
        data = new uint32_t[bufWidth * bufHeight];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < bufHeight; y++) {
            for (int x = 0; x < bufWidth; x++) {
                uint32_t src = data[y * bufWidth + x], dst = colbufAddr + getSwizzle(x, y) * 3;
                core->memory.write<uint8_t>(ARM11, dst + 0, src >> 8);
                core->memory.write<uint8_t>(ARM11, dst + 1, src >> 16);
                core->memory.write<uint8_t>(ARM11, dst + 2, src >> 24);
            }
        }
        break;
    case COL_RGB565:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    case COL_RGB5A1:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    case COL_RGBA4:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    }

    // Clean up and finish
    contextFunc();
    delete[] data;
    drawDirty = false;
}

void GpuRenderOgl::setBufferDims(uint16_t width, uint16_t height, bool flip) {
    // Flush the old buffers and set new dimensions
    // TODO: use the Y-flip value
    flushData();
    bufWidth = width;
    bufHeight = height;

    // Create a render texture with the dimensions and update the viewport
    contextFunc();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glViewport(0, 0, bufWidth, bufHeight);
    contextFunc();
}

void GpuRenderOgl::setColbufAddr(uint32_t address) {
    // Flush the old color buffer and set a new address
    flushData();
    colbufAddr = address;
}

void GpuRenderOgl::setColbufFmt(ColbufFmt format) {
    // Flush the old color buffer and set a new format
    flushData();
    colbufFmt = format;
}
