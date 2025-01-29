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

    uint32_t readFramebufLt0(bool bot) { return pdcFramebufLt0[bot]; }
    uint32_t readFramebufFormat(bool bot) { return pdcFramebufFormat[bot]; }
    uint32_t readInterruptType(bool bot) { return pdcInterruptType[bot]; }

    void writeFramebufLt0(bool bot, uint32_t mask, uint32_t value);
    void writeFramebufFormat(bool bot, uint32_t mask, uint32_t value);
    void writeInterruptType(bool bot, uint32_t mask, uint32_t value);

private:
    Core *core;

    std::queue<uint32_t*> buffers;
    std::atomic<bool> ready;
    std::mutex mutex;

    uint32_t pdcFramebufLt0[2] = {};
    uint32_t pdcFramebufFormat[2] = {};
    uint32_t pdcInterruptType[2] = {};

    void drawScreen(bool bot, uint32_t *buffer);
};
