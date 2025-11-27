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
#include "../core.h"

void Sha::hash1(uint32_t *src) {
    // Generate the rest of the input based on existing values
    for (int i = 16; i < 80; i++) {
        src[i] = src[i - 3] ^ src[i - 8] ^ src[i - 14] ^ src[i - 16];
        src[i] = ROR32(src[i], 31);
    }

    // Use the current hash as a base
    uint32_t hash[8];
    memcpy(hash, shaHash, sizeof(hash));

    // Hash the input based on SHA1 pseudocode from GBATEK
    for (int i = 0; i < 80; i++) {
        // Calculate a value for updating the hash
        uint32_t val;
        if (i < 20) val = 0x5A827999 + hash[4] + (hash[3] ^ (hash[1] & (hash[2] ^ hash[3])));
        else if (i < 40) val = 0x6ED9EBA1 + hash[4] + (hash[1] ^ hash[2] ^ hash[3]);
        else if (i < 60) val = 0x8F1BBCDC + hash[4] + ((hash[1] & hash[2]) | (hash[3] & (hash[1] | hash[2])));
        else val = 0xCA62C1D6 + hash[4] + (hash[1] ^ hash[2] ^ hash[3]);

        // Update the hash
        hash[4] = hash[3];
        hash[3] = hash[2];
        hash[2] = ROR32(hash[1], 2);
        hash[1] = hash[0];
        hash[0] = ROR32(hash[0], 27) + src[i] + val;
    }

    // Apply the hash
    for (int i = 0; i < 8; i++)
        shaHash[i] += hash[i];
}

void Sha::hash2(uint32_t *src) {
    // Define the table of constants used during hashing
    static const uint32_t table[64] = {
        0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
        0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
        0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
        0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
        0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
        0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
        0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
        0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
    };

    // Generate the rest of the input based on existing values
    for (int i = 16; i < 64; i++) {
        uint32_t r0 = ROR32(src[i - 15], 7) ^ ROR32(src[i - 15], 18) ^ (src[i - 15] >> 3);
        uint32_t r1 = ROR32(src[i - 2], 17) ^ ROR32(src[i - 2], 19) ^ (src[i - 2] >> 10);
        src[i] = r0 + r1 + src[i - 16] + src[i - 7];
    }

    // Use the current hash as a base
    uint32_t hash[8];
    memcpy(hash, shaHash, sizeof(hash));

    // Hash the input based on pseudocode from https://en.wikipedia.org/wiki/SHA-2
    for (int i = 0; i < 64; i++) {
        // Calculate values for updating the hash
        uint32_t r0 = ROR32(hash[4], 6) ^ ROR32(hash[4], 11) ^ ROR32(hash[4], 25);
        uint32_t a0 = (hash[4] & hash[5]) ^ (~hash[4] & hash[6]);
        uint32_t v0 = r0 + a0 + hash[7] + src[i] + table[i];
        uint32_t r1 = ROR32(hash[0], 2) ^ ROR32(hash[0], 13) ^ ROR32(hash[0], 22);
        uint32_t a1 = (hash[0] & hash[1]) ^ (hash[0] & hash[2]) ^ (hash[1] & hash[2]);
        uint32_t v1 = r1 + a1;

        // Update the hash
        for (int j = 7; j > 0; j--)
            hash[j] = hash[j - 1];
        hash[0] = v0 + v1;
        hash[4] += v0;
    }

    // Apply the hash
    for (int i = 0; i < 8; i++)
        shaHash[i] += hash[i];
}

void Sha::initFifo() {
    // Start the FIFO and reset the block counter
    fifoRunning = true;
    shaBlkcnt = 0;

    // Define initial hashes for each mode
    static const uint32_t hashes[3][8] = {
        { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 }, // SHA256
        { 0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939, 0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4 }, // SHA224
        { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0, 0x00000000, 0x00000000, 0x00000000 } // SHA1
    };

    // Initialize the hash based on the selected mode
    uint8_t mode = (shaCnt >> 4) & 0x3;
    LOG_INFO("SHA FIFO starting in mode %d\n", mode);
    switch (mode) {
    case 0: case 1: // SHA256/SHA224
        memcpy(shaHash, hashes[mode], sizeof(shaHash));
        return;

    default: // SHA1
        memcpy(shaHash, hashes[2], sizeof(shaHash));
        return;
    }
}

void Sha::pushFifo() {
    // Forward data to the output FIFO and mark it as non-empty if enabled
    if ((shaCnt & BIT(8)) && outFifo.size() < 16) {
        outFifo.push(fifoValue);
        shaCnt |= BIT(9);
    }

    // Push data to the input FIFO and reset the current value
    inFifo.push(fifoValue);
    fifoValue = fifoMask = 0;
}

void Sha::triggerFifo() {
    // Schedule a FIFO update if one hasn't been already
    if (scheduled) return;
    core->schedule(Task(SHA0_UPDATE + arm9), 1);
    scheduled = true;
}

void Sha::update() {
    // Add a footer to the final block, ensuring data is a multiple of 16 words
    if (shaCnt & (fifoRunning << 1)) {
        // Append an end bit to the data and adjust length for missing bytes
        if (fifoMask) {
            // Put the bit at the end of an unfinished word and push it
            uint32_t b = 0x80, s = 0;
            while (!(fifoMask & (b << s))) s += 8;
            fifoValue |= (b << (s -= 8));
            shaBlkcnt += 3 - (s >> 3);
            pushFifo();
        }
        else {
            // Push a new word containing the end bit
            inFifo.push(0x80000000);
        }

        // Append padding and 64-bit length, adjusting if a new block is required
        while ((inFifo.size() & 0xF) != 14) inFifo.push(0);
        uint64_t len = uint64_t(shaBlkcnt) << 3;
        inFifo.push(len >> 32);
        inFifo.push(len >> 0);
    }

    // Process FIFO blocks when active and available
    while (fifoRunning && inFifo.size() >= 16) {
        // Receive an input block from the FIFO
        uint32_t src[80];
        for (int i = 0; i < 16; i++) {
            src[i] = inFifo.front();
            inFifo.pop();
        }

        // Restore icon flags if modified by cart auto-boot so that verification passes
        if (iconFlags != -1 && src[10] == (iconFlags | BIT(25))) {
            src[10] = iconFlags;
            iconFlags = -1;
        }

        // Hash the block and update input length
        if (shaCnt & BIT(5)) // SHA1
            hash1(src);
        else // SHA256/SHA224
            hash2(src);
    }

    // Check ARM9 DMA conditions
    if (arm9) {
        // Set or clear the SHA in DRQs
        if (inFifo.empty() && outFifo.empty()) {
            core->ndma.setDrq(0xA);
            if (shaCnt & BIT(2))
                core->cdmas[XDMA].setDrq(0x6);
        }
        else {
            core->ndma.clearDrq(0xA);
            core->cdmas[XDMA].clearDrq(0x6);
        }

        // Set or clear the SHA out DRQs
        if (outFifo.size() >= 16) {
            core->ndma.setDrq(0xB);
            if (shaCnt & BIT(10))
                core->cdmas[XDMA].setDrq(0x7);
        }
        else {
            core->ndma.clearDrq(0xB);
            core->cdmas[XDMA].clearDrq(0x7);
        }
    }

    // Disable the FIFO after the final block is processed
    scheduled = false;
    if (!(shaCnt & (fifoRunning << 1))) return;
    shaCnt &= ~BIT(1);
    fifoRunning = false;
    LOG_INFO("SHA FIFO finished processing\n");
}

uint32_t Sha::readFifo() {
    // Read a big endian value from the FIFO and update its empty bit
    if (outFifo.empty()) return 0;
    uint32_t value = BSWAP32(outFifo.front());
    outFifo.pop();
    if (outFifo.empty()) shaCnt &= ~BIT(9);
    triggerFifo();
    return value;
}

uint32_t Sha::readHash(int i) {
    // Read from a part of the SHA_HASH value based on endian settings
    return (shaCnt & BIT(3)) ? BSWAP32(shaHash[i]) : shaHash[i];
}

void Sha::writeCnt(uint32_t mask, uint32_t value) {
    // Write to the SHA_CNT register
    uint32_t mask2 = (mask & 0x53E);
    shaCnt = (shaCnt & ~mask2) | (value & mask2);

    // Start processing the FIFO if triggered
    if (value & mask & BIT(0))
        initFifo();
    triggerFifo();
}

void Sha::writeBlkcnt(uint32_t mask, uint32_t value) {
    // Write to the SHA_BLKCNT input length
    shaBlkcnt = (shaBlkcnt & ~mask) | (value & mask);
}

void Sha::writeHash(int i, uint32_t mask, uint32_t value) {
    // Write to part of the SHA_HASH value based on endian settings
    shaHash[i] = (shaCnt & BIT(3)) ? (shaHash[i] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (shaHash[i] & ~mask) | (value & mask);
}

void Sha::writeFifo(uint32_t mask, uint32_t value) {
    // Write a big endian value to the FIFO, pushing it once a full word is received
    if (inFifo.size() == 16) return;
    fifoValue |= BSWAP32(value & mask);
    if ((fifoMask |= BSWAP32(mask)) != -1) return;
    shaBlkcnt += 4;
    pushFifo();
    triggerFifo();
}
