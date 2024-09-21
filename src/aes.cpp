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

#include <byteswap.h>
#include <cstring>

#include "core.h"

Aes::Aes(Core *core): core(core) {
    // Generate temporary pow and log tables
    uint8_t pow[0x100], log[0x100];
    for (int i = 0, x = 1; i < 0x100; i++) {
        pow[i] = x; log[x] = i;
        x ^= (x << 1) ^ ((x & 0x80) ? 0x11B : 0);
    }

    // Generate the AES forward and reverse S-boxes
    for (int i = 0; i < 0x100; i++) {
        uint8_t x = i ? pow[0xFF - log[i]] : 0;
        x = x ^ ROL8(x, 1) ^ ROL8(x, 2) ^ ROL8(x, 3) ^ ROL8(x, 4) ^ 0x63;
        fsBox[i] = x; rsBox[x] = i;
    }

    // Generate the AES forward and reverse tables
    for (int i = 0; i < 0x100; i++) {
        uint8_t x = (fsBox[i] << 1) ^ ((fsBox[i] & 0x80) ? 0x11B : 0);
        fTable[i] = (fsBox[i] * 0x10101) ^ (x * 0x1000001);
        rTable[i] = (x = rsBox[i]) ? (pow[(log[x] + log[0xE]) % 0xFF] << 24) | (pow[(log[x] + log[0x9]) % 0xFF] << 16) |
            (pow[(log[x] + log[0xD]) % 0xFF] << 8) | pow[(log[x] + log[0xB]) % 0xFF] : 0;
    }
}

uint32_t Aes::scatter8(uint8_t *t, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    // Perform a scatter operation with an 8-bit table
    return (t[(d >> 24) & 0xFF] << 24) | (t[(c >> 16) & 0xFF] << 16) | (t[(b >> 8) & 0xFF] << 8) | t[a & 0xFF];
}

uint32_t Aes::scatter32(uint32_t *t, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    // Perform a scatter operation with a 32-bit table
    return t[(d >> 24) & 0xFF] ^ ROR32(t[(c >> 16) & 0xFF], 8) ^ ROR32(t[(b >> 8) & 0xFF], 16) ^ ROR32(t[a & 0xFF], 24);
}

void Aes::initKey(bool decrypt) {
    // Initialize the ring key values
    for (int i = 0, x = 1; i < 44; i += 4) {
        uint32_t y;
        for (int j = 3; j >= 0; j--)
            rKey[i + j] = y = (i ? (y ^ rKey[i + j - 4]) : keys[curKey][j]);
        y = scatter8(fsBox, y, y, y, y);
        y = ROL32(y, 8) ^ (x << 24);
        x = (x << 1) ^ ((x & 0x80) ? 0x11B : 0);
    }

    // Swap values for decryption
    if (!decrypt) return;
    for (int i = 0; i < 20; i += 4)
        for (int j = 0; j < 4; j++)
            SWAP(rKey[i + j], rKey[40 - i + j])

    // Modify values for decryption
    for (int i = 4; i < 40; i++) {
        uint32_t x = scatter8(fsBox, rKey[i], rKey[i], rKey[i], rKey[i]);
        rKey[i] = scatter32(rTable, x, x, x, x);
    }
}

template <bool decrypt> void Aes::cryptBlock(uint32_t *src, uint32_t *dst) {
    // Set initial values for encryption/decryption
    uint32_t y0 = rKey[0] ^ src[0];
    uint32_t y1 = rKey[1] ^ src[1];
    uint32_t y2 = rKey[2] ^ src[2];
    uint32_t y3 = rKey[3] ^ src[3];

    // Encrypt/decrypt a block of data
    for (int i = 4; i < 40; i += 4) {
        uint32_t x0 = rKey[i + 0] ^ (decrypt ? scatter32(rTable, y3, y2, y1, y0) : scatter32(fTable, y1, y2, y3, y0));
        uint32_t x1 = rKey[i + 1] ^ (decrypt ? scatter32(rTable, y0, y3, y2, y1) : scatter32(fTable, y2, y3, y0, y1));
        uint32_t x2 = rKey[i + 2] ^ (decrypt ? scatter32(rTable, y1, y0, y3, y2) : scatter32(fTable, y3, y0, y1, y2));
        uint32_t x3 = rKey[i + 3] ^ (decrypt ? scatter32(rTable, y2, y1, y0, y3) : scatter32(fTable, y0, y1, y2, y3));
        y0 = x0; y1 = x1; y2 = x2; y3 = x3;
    }

    // Output the final encrypted/decrypted values
    dst[0] = rKey[40] ^ (decrypt ? scatter8(rsBox, y3, y2, y1, y0) : scatter8(fsBox, y1, y2, y3, y0));
    dst[1] = rKey[41] ^ (decrypt ? scatter8(rsBox, y0, y3, y2, y1) : scatter8(fsBox, y2, y3, y0, y1));
    dst[2] = rKey[42] ^ (decrypt ? scatter8(rsBox, y1, y0, y3, y2) : scatter8(fsBox, y3, y0, y1, y2));
    dst[3] = rKey[43] ^ (decrypt ? scatter8(rsBox, y2, y1, y0, y3) : scatter8(fsBox, y0, y1, y2, y3));
}

void Aes::initFifo() {
    // Reload the internal block counter
    curBlock = aesBlkcnt;

    // Initialize encryption/decryption based on the selected mode
    switch (uint8_t mode = (aesCnt >> 27) & 0x7) {
    case 4: case 5: // CBC decrypt/encrypt
        initKey(mode == 4);
        memcpy(cbc, aesIv, sizeof(cbc));
        return;

    default: // Unimplemented
        LOG_WARN("AES FIFO started with unimplemented mode: %d\n", mode);
        return;
    }
}

void Aes::processFifo() {
    // Process FIFO blocks when active and available
    while ((aesCnt & BIT(31)) && writeFifo.size() >= 4 && readFifo.size() <= 12) {
        // Receive an input block from the write FIFO based on endian settings
        uint32_t src[4], dst[4];
        for (int i = 0; i < 4; i++) {
            src[(aesCnt & BIT(25)) ? (3 - i) : i] = writeFifo.front();
            writeFifo.pop();
        }

        // Handle the block based on the selected mode
        switch ((aesCnt >> 27) & 0x7) { // Mode
        case 4: // CBC decrypt
            cryptBlock<true>(src, dst);
            for (int i = 0; i < 4; i++)
                dst[i] ^= cbc[i];
            memcpy(cbc, src, sizeof(cbc));
            break;

        case 5: // CBC encrypt
            for (int i = 0; i < 4; i++)
                src[i] ^= cbc[i];
            cryptBlock<false>(src, dst);
            memcpy(cbc, dst, sizeof(cbc));
            break;

        default: // Unimplemented
            memcpy(src, dst, sizeof(src));
            break;
        }

        // Send an output block to the read FIFO based on endian settings
        for (int i = 0; i < 4; i++)
            readFifo.push(dst[(aesCnt & BIT(25)) ? (3 - i) : i]);

        // Disable the FIFO once all blocks are processed and trigger an interrupt if enabled
        if (--curBlock > 0) continue;
        aesCnt &= ~BIT(31);
        if (aesCnt & BIT(30))
            core->interrupts.sendInterrupt(true, 15);
    }

    // Update the FIFO word counts in AES_CNT
    aesCnt = (aesCnt & ~0x3FF) | (readFifo.size() << 5) | writeFifo.size();
}

void Aes::flushKeyFifo() {
    // Flush FIFO values to the selected key based on endian settings
    uint32_t *key = keys[aesKeycnt & 0x3F];
    if (key < keys[4]) keyFifo = {}; // DSi keys
    for (int i = 0; !keyFifo.empty(); i++) {
        key[(aesCnt & BIT(25)) ? (3 - i) : i] = keyFifo.front();
        keyFifo.pop();
    }
}

uint32_t Aes::readAesRdfifo() {
    // Pop a value from the read FIFO based on endian settings
    if (readFifo.empty()) return aesRdfifo;
    aesRdfifo = (aesCnt & BIT(23)) ? bswap_32(readFifo.front()) : readFifo.front();
    readFifo.pop();
    processFifo();
    return aesRdfifo;
}

void Aes::writeAesCnt(uint32_t mask, uint32_t value) {
    // Handle write-only bits that trigger events
    if (value & mask & BIT(10)) writeFifo = {}; // Empty write FIFO (?)
    if (value & mask & BIT(11)) readFifo = {}; // Empty read FIFO (?)
    if (value & mask & BIT(26)) curKey = aesKeysel; // Apply key slot

    // Write to the AES_CNT register
    mask &= 0xFBDFF000;
    bool start = (value & mask & ~aesCnt & BIT(31));
    aesCnt = (aesCnt & ~mask) | (value & mask);

    // Start processing a new set of FIFO blocks if triggered
    if (!start) return;
    initFifo();
    processFifo();
}

void Aes::writeAesBlkcnt(uint16_t mask, uint16_t value) {
    // Write to the AES_BLKCNT transfer length
    aesBlkcnt = (aesBlkcnt & ~mask) | (value & mask);
}

void Aes::writeAesWrfifo(uint32_t mask, uint32_t value) {
    // Push a value to the write FIFO based on endian settings
    if (writeFifo.size() == 16) return;
    writeFifo.push((aesCnt & BIT(23)) ? bswap_32(value & mask) : (value & mask));
    processFifo();
}

void Aes::writeAesKeysel(uint8_t value) {
    // Write to the AES_KEYSEL pending key slot
    aesKeysel = (aesKeysel & ~0x3F) | (value & 0x3F);
}

void Aes::writeAesKeycnt(uint8_t value) {
    // Write to the AES_KEYCNT register, flushing the key FIFO if requested
    if (value & BIT(7)) flushKeyFifo();
    aesKeycnt = (aesKeycnt & ~0x7F) | (value & 0x7F);
}

void Aes::writeAesIv(int i, uint32_t mask, uint32_t value) {
    // Write to part of the AES_IV value based on endian settings
    aesIv[i] = (aesCnt & BIT(23)) ? (aesIv[i] & ~bswap_32(mask)) |
        bswap_32(value & mask) : (aesIv[i] & ~mask) | (value & mask);
}

void Aes::writeAesKeyfifo(uint32_t mask, uint32_t value) {
    // Push a value to the key FIFO based on endian settings, flushing when it's full
    keyFifo.push((aesCnt & BIT(23)) ? bswap_32(value & mask) : (value & mask));
    if (keyFifo.size() == 4) flushKeyFifo();
}
