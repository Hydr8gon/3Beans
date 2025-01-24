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

#include <cstdint>
#include <queue>

class Core;

class Sha {
public:
    Sha(Core *core): core(core) {}

    uint32_t readShaCnt() { return shaCnt; }
    uint32_t readShaBlkcnt() { return shaBlkcnt; }
    uint32_t readShaHash(int i);
    uint32_t readShaFifo();

    void writeShaCnt(uint32_t mask, uint32_t value);
    void writeShaBlkcnt(uint32_t mask, uint32_t value);
    void writeShaHash(int i, uint32_t mask, uint32_t value);
    void writeShaFifo(uint32_t mask, uint32_t value);

private:
    Core *core;

    std::queue<uint32_t> inFifo;
    std::queue<uint32_t> outFifo;
    uint32_t fifoValue = 0;
    uint32_t fifoMask = 0;
    bool fifoRunning = false;

    uint32_t shaCnt = 0;
    uint32_t shaBlkcnt = 0;
    uint32_t shaHash[8] = {};

    void hash2(uint32_t *src);
    void initFifo();
    void pushFifo();
    void processFifo();
};
