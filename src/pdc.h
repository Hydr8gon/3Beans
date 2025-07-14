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

#include <atomic>
#include <cstdint>
#include <queue>
#include <mutex>

class Core;

class Pdc {
public:
    Pdc(Core *core): core(core) {}
    uint32_t *getFrame();
    void drawFrame();

    uint32_t readFramebufLt0(int i) { return pdcFramebufLt0[i]; }
    uint32_t readFramebufLt1(int i) { return pdcFramebufLt1[i]; }
    uint32_t readFramebufFormat(int i) { return pdcFramebufFormat[i]; }
    uint32_t readInterruptType(int i) { return pdcInterruptType[i]; }
    uint32_t readFramebufSelAck(int i) { return pdcFramebufSelAck[i]; }
    uint32_t readFramebufStep(int i) { return pdcFramebufStep[i]; }

    void writeFramebufLt0(int i, uint32_t mask, uint32_t value);
    void writeFramebufLt1(int i, uint32_t mask, uint32_t value);
    void writeFramebufFormat(int i, uint32_t mask, uint32_t value);
    void writeInterruptType(int i, uint32_t mask, uint32_t value);
    void writeFramebufSelAck(int i, uint32_t mask, uint32_t value);
    void writeFramebufStep(int i, uint32_t mask, uint32_t value);

private:
    Core *core;

    std::queue<uint32_t*> buffers;
    std::atomic<bool> ready;
    std::mutex mutex;

    uint32_t pdcFramebufLt0[2] = {};
    uint32_t pdcFramebufLt1[2] = {};
    uint32_t pdcFramebufFormat[2] = {};
    uint32_t pdcInterruptType[2] = {};
    uint32_t pdcFramebufSelAck[2] = {};
    uint32_t pdcFramebufStep[2] = {};

    void drawScreen(int i, uint32_t *buffer);
};
