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

void Aes::rolKey(uint32_t *key, uint8_t rol) {
    // Rotate a key left by entire words
    while (rol >= 32) {
        uint32_t temp = key[3];
        key[3] = key[2];
        key[2] = key[1];
        key[1] = key[0];
        key[0] = temp;
        rol -= 32;
    }

    // Rotate a key left by less than a word
    if (!rol) return;
    uint32_t temp = key[3];
    key[3] = (key[3] << rol) | (key[2] >> (32 - rol));
    key[2] = (key[2] << rol) | (key[1] >> (32 - rol));
    key[1] = (key[1] << rol) | (key[0] >> (32 - rol));
    key[0] = (key[0] << rol) | (temp >> (32 - rol));
}

void Aes::xorKey(uint32_t *dst, uint32_t *src) {
    // Bitwise exclusive or one key with another
    for (int i = 0; i < 4; i++)
        dst[i] ^= src[i];
}

void Aes::addKey(uint32_t *dst, uint32_t *src) {
    // Add one key to another
    bool over = false;
    for (int i = 0; i < 4; i++) {
        uint32_t res = dst[i] + src[i] + over;
        over = (res < dst[i] || (res == dst[i] && over));
        dst[i] = res;
    }
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
    // Get the current AES mode and reload block counters
    uint8_t mode = (aesCnt >> 27) & 0x7;
    curBlock = (aesBlkcnt >> 16);
    curExtra = (mode < 2) ? (aesBlkcnt & 0xFFFF) : 0;
    aesCnt &= ~BIT(21); // CCM verify
    LOG_INFO("AES FIFO starting in mode %d\n", mode);

    // Initialize encryption/decryption based on the mode
    switch (mode) {
    case 0: case 1: // CCM
        initKey(false);
        ctr[0] = cbc[0] = (aesIv[0] << 24);
        ctr[1] = cbc[1] = (aesIv[1] << 24) | (aesIv[0] >> 8);
        ctr[2] = cbc[2] = (aesIv[2] << 24) | (aesIv[1] >> 8);
        ctr[3] = cbc[3] = (0x2 << 24) | (aesIv[2] >> 8);
        cbc[0] |= (curBlock << 4);
        cbc[3] |= ((curExtra > 0) << 30) | ((aesCnt << 11) & 0x38000000);
        cryptBlock<false>(cbc, cbc);
        return;

    case 2: case 3: // CTR
        initKey(false);
        memcpy(ctr, aesIv, sizeof(ctr));
        return;

    case 4: case 5: // CBC decrypt/encrypt
        initKey(mode == 4);
        memcpy(cbc, aesIv, sizeof(cbc));
        return;

    default: // ECB decrypt/encrypt
        initKey(mode == 6);
        return;
    }
}

void Aes::triggerFifo() {
    // Schedule a FIFO update if one hasn't been already
    if (scheduled) return;
    core->schedule(AES_UPDATE, 1);
    scheduled = true;
}

void Aes::update() {
    // Process FIFO blocks when active and available
    while ((aesCnt & BIT(31)) && writeFifo.size() >= 4 && readFifo.size() <= 12) {
        // Receive an input block from the write FIFO based on endian settings
        uint32_t src[4], dst[4];
        for (int i = 0; i < 4; i++) {
            src[(aesCnt & BIT(25)) ? (3 - i) : i] = writeFifo.front();
            writeFifo.pop();
        }

        // Process a CCM extra block and send it to the read FIFO if enabled
        if (curExtra) {
            for (int i = 0; i < 4; i++)
                cbc[i] ^= src[i];
            cryptBlock<false>(cbc, cbc);
            if (aesCnt & BIT(19))
                for (int i = 0; i < 4; i++)
                    readFifo.push(src[(aesCnt & BIT(25)) ? (3 - i) : i]);
            curExtra--;
            continue;
        }

        // Handle the block based on the selected mode
        uint8_t mode = (aesCnt >> 27) & 0x7;
        switch (mode) {
        case 0: // CCM decrypt
            for (int i = 0, c = 1; i < 4; i++)
                ctr[i] += c, c = (ctr[i] == 0);
            cryptBlock<false>(ctr, dst);
            for (int i = 0; i < 4; i++)
                cbc[i] ^= (dst[i] ^= src[i]);
            cryptBlock<false>(cbc, cbc);
            break;

        case 1: // CCM encrypt
            for (int i = 0, c = 1; i < 4; i++)
                ctr[i] += c, c = (ctr[i] == 0);
            cryptBlock<false>(ctr, dst);
            for (int i = 0; i < 4; i++)
                cbc[i] ^= src[i], dst[i] ^= src[i];
            cryptBlock<false>(cbc, cbc);
            break;

        case 2: case 3: // CTR
            cryptBlock<false>(ctr, dst);
            for (int i = 0, c = 1; i < 4; i++) {
                ctr[i] += c;
                c = (ctr[i] == 0);
                dst[i] ^= src[i];
            }
            break;

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

        case 6: // ECB decrypt
            cryptBlock<true>(src, dst);
            break;

        default: // ECB encrypt
            cryptBlock<false>(src, dst);
            break;
        }

        // Send an output block to the read FIFO based on endian settings
        for (int i = 0; i < 4; i++)
            readFifo.push(dst[(aesCnt & BIT(25)) ? (3 - i) : i]);

        // Disable the FIFO once all blocks are processed and trigger an interrupt if enabled
        if (--curBlock > 0) continue;
        aesCnt &= ~BIT(31);
        if (aesCnt & BIT(30))
            core->interrupts.sendInterrupt(ARM9, 15);
        LOG_INFO("AES FIFO finished processing\n");

        // Calculate a CCM MAC for applicable modes
        if (mode > 1) continue;
        ctr[0] &= ~0xFFFFFF;
        cryptBlock<false>(ctr, ctr);
        for (int i = 0; i < 4; i++)
            cbc[i] ^= ctr[i];

        // Create a mask based on the MAC length
        uint32_t mask[4] = {};
        uint8_t len = std::max(4U, ((aesCnt >> 15) & 0xE) + 2);
        for (int i = 0; i < len; i += 2)
            mask[i / 2] |= 0xFFFF << ((i << 4) & 0x10);

        // Handle final verification steps for CCM modes
        if (mode == 1) { // Encrypt
            // Append the MAC to the output data (possibly overflowing the FIFO, but that's fine)
            for (int i = 0; i < 4; i++) {
                int j = (aesCnt & BIT(25)) ? (3 - i) : i;
                readFifo.push(cbc[j] & mask[j]);
            }
        }
        else if (~aesCnt & BIT(20)) {
            // Catch unhandled verification using the write FIFO
            LOG_CRIT("Unhandled AES-CCM MAC verification source used: FIFO\n");
        }
        else { // Decrypt
            // Verify decryption by comparing the MAC with one provided via registers
            bool fail = false;
            for (int i = 0; i < 4; i++)
                if ((fail = (cbc[i] ^ aesMac[i]) & mask[i])) break;
            aesCnt |= (!fail << 21);
        }
    }

    // Update FIFO sizes and check NDMA conditions
    aesCnt = (aesCnt & ~0x3FF) | (std::min<uint8_t>(16, readFifo.size()) << 5) | writeFifo.size();
    if (writeFifo.size() <= ((aesCnt >> 10) & 0xC))
        core->ndma.triggerMode(0x8); // AES in
    if (readFifo.size() >= ((aesCnt >> 12) & 0xC) + 4)
        core->ndma.triggerMode(0x9); // AES out
    scheduled = false;
}

void Aes::flushKeyFifo(bool keyX) {
    // Flush FIFO values to the selected key based on endian settings
    uint32_t *key = (keyX ? keysX : keys)[aesKeycnt & 0x3F];
    std::queue<uint32_t> &fifo = keyX ? keyXFifo : keyFifo;
    if (key < keys[4]) fifo = {}; // DSi keys
    for (int i = 0; !fifo.empty(); i++) {
        key[(aesCnt & BIT(25)) ? (3 - i) : i] = fifo.front();
        fifo.pop();
    }
}

void Aes::flushKeyYFifo() {
    // Flush FIFO values to key Y based on endian settings
    uint32_t *keyY = keysY[aesKeycnt & 0x3F];
    if (keyY < keysY[4]) keyYFifo = {}; // DSi keys
    for (int i = 0; !keyYFifo.empty(); i++) {
        keyY[(aesCnt & BIT(25)) ? (3 - i) : i] = keyYFifo.front();
        keyYFifo.pop();
    }

    // Generate a normal key after updating key Y
    generateKey(aesKeycnt & 0x3F);
}

void Aes::generateKey(int i) {
    // Generate a key using key X and key Y based on the current mode
    memcpy(keys[i], keysX[i], sizeof(keys[0]));
    if ((aesKeycnt & BIT(6)) || i < 4) { // DSi
        uint32_t seed[4] = { 0x1A4F3E79, 0x2A680F5F, 0x29590258, 0xFFFEFB4E };
        xorKey(keys[i], keysY[i]);
        addKey(keys[i], seed);
        rolKey(keys[i], 42);
    }
    else { // 3DS
        uint32_t seed[4] = { 0x5D52768A, 0x024591DC, 0xC5FE0408, 0x1FF9E9AA };
        rolKey(keys[i], 2);
        xorKey(keys[i], keysY[i]);
        addKey(keys[i], seed);
        rolKey(keys[i], 87);
    }
}

uint32_t Aes::readRdfifo() {
    // Pop a value from the read FIFO based on endian settings
    if (readFifo.empty()) return aesRdfifo;
    aesRdfifo = (aesCnt & BIT(23)) ? BSWAP32(readFifo.front()) : readFifo.front();
    readFifo.pop();
    triggerFifo();
    return aesRdfifo;
}

void Aes::writeCnt(uint32_t mask, uint32_t value) {
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
    triggerFifo();
}

void Aes::writeBlkcnt(uint32_t mask, uint32_t value) {
    // Write to the AES_BLKCNT register
    aesBlkcnt = (aesBlkcnt & ~mask) | (value & mask);
}

void Aes::writeWrfifo(uint32_t mask, uint32_t value) {
    // Push a value to the write FIFO based on endian settings
    if (writeFifo.size() == 16) return;
    writeFifo.push((aesCnt & BIT(23)) ? BSWAP32(value & mask) : (value & mask));
    triggerFifo();
}

void Aes::writeKeysel(uint8_t value) {
    // Write to the AES_KEYSEL pending key slot
    aesKeysel = (aesKeysel & ~0x3F) | (value & 0x3F);
}

void Aes::writeKeycnt(uint8_t value) {
    // Write to the AES_KEYCNT register and flush the key FIFOs if requested
    aesKeycnt = (aesKeycnt & ~0x7F) | (value & 0x7F);
    if (~value & BIT(7)) return;
    flushKeyFifo(false);
    flushKeyFifo(true);
    flushKeyYFifo();
}

void Aes::writeIv(int i, uint32_t mask, uint32_t value) {
    // Write to part of the AES_IV value based on endian settings
    aesIv[i] = (aesCnt & BIT(23)) ? (aesIv[i] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (aesIv[i] & ~mask) | (value & mask);
}

void Aes::writeMac(int i, uint32_t mask, uint32_t value) {
    // Write to part of the AES_MAC value based on endian settings
    aesMac[i] = (aesCnt & BIT(23)) ? (aesMac[i] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (aesMac[i] & ~mask) | (value & mask);
}

void Aes::writeKey(int i, int j, uint32_t mask, uint32_t value) {
    // Write to part of one of the DSi key slots based on endian settings
    keys[i][j] = (aesCnt & BIT(23)) ? (keys[i][j] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (keys[i][j] & ~mask) | (value & mask);
}

void Aes::writeKeyx(int i, int j, uint32_t mask, uint32_t value) {
    // Write to part of one of the DSi key X slots based on endian settings
    keysX[i][j] = (aesCnt & BIT(23)) ? (keys[i][j] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (keys[i][j] & ~mask) | (value & mask);
}

void Aes::writeKeyy(int i, int j, uint32_t mask, uint32_t value) {
    // Write to part of one of the DSi key Y slots based on endian settings
    keysY[i][j] = (aesCnt & BIT(23)) ? (keys[i][j] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (keys[i][j] & ~mask) | (value & mask);

    // Generate a normal key when the last word is written to
    if (j == 3)
        generateKey(i);
}

void Aes::writeKeyfifo(uint32_t mask, uint32_t value) {
    // Push a value to the key FIFO based on endian settings and flush when full
    keyFifo.push((aesCnt & BIT(23)) ? BSWAP32(value & mask) : (value & mask));
    if (keyFifo.size() == 4) flushKeyFifo(false);
}

void Aes::writeKeyxfifo(uint32_t mask, uint32_t value) {
    // Push a value to the key X FIFO based on endian settings and flush when full
    keyXFifo.push((aesCnt & BIT(23)) ? BSWAP32(value & mask) : (value & mask));
    if (keyXFifo.size() == 4) flushKeyFifo(true);
}

void Aes::writeKeyyfifo(uint32_t mask, uint32_t value) {
    // Push a value to the key Y FIFO based on endian settings and flush when full
    keyYFifo.push((aesCnt & BIT(23)) ? BSWAP32(value & mask) : (value & mask));
    if (keyYFifo.size() == 4) flushKeyYFifo();
}
