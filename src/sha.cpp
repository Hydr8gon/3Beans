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

    // Hash the input
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
    switch (uint8_t mode = (shaCnt >> 4) & 0x3) {
    case 0: case 1: // SHA256/SHA224
        memcpy(shaHash, hashes[mode], sizeof(shaHash));
        return;

    default: // SHA1
        memcpy(shaHash, hashes[2], sizeof(shaHash));
        LOG_WARN("SHA FIFO started with unimplemented mode: SHA1\n");
        return;
    }
}

void Sha::processFifo() {
    // Add a footer to the final block, ensuring data is a multiple of 16 words
    if (shaCnt & (fifoRunning << 1)) {
        // Append an end bit to the data and adjust length for missing bytes
        if (fifoMask) {
            // Put the bit at the end of an unfinished word and push it
            uint32_t b = 0x80, s = 0;
            while (!(fifoMask & (b << s))) s += 8;
            fifoValue |= (b << (s -= 8));
            shaBlkcnt -= ((16 - fifo.size()) << 2) - (3 - (s >> 3));
            fifo.push(fifoValue);
            fifoValue = fifoMask = 0;
        }
        else {
            // Push a new word containing the end bit
            shaBlkcnt -= (16 - fifo.size()) << 2;
            fifo.push(0x80000000);
        }

        // Append padding and 64-bit length, adjusting if a new block is required
        while ((fifo.size() & 0xF) != 14) fifo.push(0);
        uint64_t len = uint64_t(shaBlkcnt + 64) << 3;
        fifo.push(len >> 32);
        fifo.push(len >> 0);
        if (fifo.size() > 16) shaBlkcnt -= 64;
    }

    // Process FIFO blocks when active and available
    while (fifoRunning && fifo.size() >= 16) {
        // Receive an input block from the FIFO
        uint32_t src[64];
        for (int i = 0; i < 16; i++) {
            src[i] = fifo.front();
            fifo.pop();
        }

        // Hash the block and update input length
        if (((shaCnt >> 4) & 0x3) < 2) // SHA256/SHA224
            hash2(src);
        shaBlkcnt += 64;
    }

    // Disable the FIFO after the final block is processed
    if (!(shaCnt & (fifoRunning << 1))) return;
    shaCnt &= ~BIT(1);
    fifoRunning = false;
}

uint32_t Sha::readShaHash(int i) {
    // Read from a part of the SHA_HASH value based on endian settings
    return (shaCnt & BIT(3)) ? bswap_32(shaHash[i]) : shaHash[i];
}

void Sha::writeShaCnt(uint32_t mask, uint32_t value) {
    // Write to the SHA_CNT register
    bool start = (value & mask & ~shaCnt & BIT(0));
    mask &= 0x53E;
    shaCnt = (shaCnt & ~mask) | (value & mask);

    // Start processing the FIFO if triggered
    if (start) initFifo();
    processFifo();
}

void Sha::writeShaBlkcnt(uint32_t mask, uint32_t value) {
    // Write to the SHA_BLKCNT input length
    shaBlkcnt = (shaBlkcnt & ~mask) | (value & mask);
}

void Sha::writeShaHash(int i, uint32_t mask, uint32_t value) {
    // Write to part of the SHA_HASH value based on endian settings
    shaHash[i] = (shaCnt & BIT(3)) ? (shaHash[i] & ~bswap_32(mask)) |
        bswap_32(value & mask) : (shaHash[i] & ~mask) | (value & mask);
}

void Sha::writeShaFifo(uint32_t mask, uint32_t value) {
    // Write a big endian value to the FIFO, pushing it once a full word is received
    if (fifo.size() == 16) return;
    fifoValue |= bswap_32(value & mask);
    if ((fifoMask |= bswap_32(mask)) != -1) return;
    fifo.push(fifoValue);
    fifoValue = fifoMask = 0;
    processFifo();
}
