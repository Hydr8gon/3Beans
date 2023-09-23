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

#pragma once

#include <cstdint>
#include <queue>
class Core;

class Aes
{
    public:
        Aes(Core *core);

        uint32_t readAesCnt() { return aesCnt; }
        uint32_t readAesRdfifo();
        uint8_t readAesKeysel() { return aesKeysel; }
        uint8_t readAesKeycnt() { return aesKeycnt; }

        void writeAesCnt(uint32_t mask, uint32_t value);
        void writeAesBlkcnt(uint16_t mask, uint16_t value);
        void writeAesWrfifo(uint32_t mask, uint32_t value);
        void writeAesKeysel(uint8_t value);
        void writeAesKeycnt(uint8_t value);
        void writeAesIv(int i, uint32_t mask, uint32_t value);
        void writeAesKeyfifo(uint32_t mask, uint32_t value);

    private:
        Core *core;
        uint8_t fsBox[0x100];
        uint8_t rsBox[0x100];
        uint32_t fTable[0x100];
        uint32_t rTable[0x100];

        uint32_t keys[0x40][4] = {};
        uint32_t rKey[44] = {};
        uint32_t cbc[4] = {};
        uint16_t curBlock = 0;
        uint8_t curKey = 0;

        std::queue<uint32_t> writeFifo;
        std::queue<uint32_t> readFifo;
        std::queue<uint32_t> keyFifo;

        uint32_t aesCnt = 0;
        uint16_t aesBlkcnt = 0;
        uint32_t aesRdfifo = 0;
        uint8_t aesKeysel = 0;
        uint8_t aesKeycnt = 0;
        uint32_t aesIv[4] = {};

        static uint32_t scatter8(uint8_t *t, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
        static uint32_t scatter32(uint32_t *t, uint32_t a, uint32_t b, uint32_t c, uint32_t d);

        void initKey(bool decrypt);
        template <bool decrypt> void cryptBlock(uint32_t *src, uint32_t *dst);

        void initFifo();
        void processFifo();
        void flushKeyFifo();
};
