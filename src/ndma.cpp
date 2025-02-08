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

#include "core.h"

void Ndma::triggerMode(uint8_t mode) {
    // Schedule transfers on enabled channels of the requested mode
    for (int i = 0; i < 8; i++)
        if (((ndmaCnt[i] >> 24) & 0x9F) == (0x80 | mode))
            runMask |= BIT(i);
    if (runMask) core->schedule(NDMA_UPDATE, 1);
}

void Ndma::update() {
    // Trigger scheduled transfers and acknowledge them
    for (int i = 0; runMask >> i; i++) {
        if (~runMask & BIT(i)) continue;
        transferBlock(i);
    }
    runMask = 0;
}

void Ndma::transferBlock(int i) {
    // Set the destination address step or handle special cases
    int dstStep;
    switch ((ndmaCnt[i] >> 10) & 0x3) {
        case 0: dstStep = 4; break; // Increment
        case 1: dstStep = -4; break; // Decrement
        case 2: dstStep = 0; break; // Fixed

    default: // Reserved
        LOG_CRIT("NDMA channel %d triggered with unhandled destination step\n", i);
        return;
    }

    // Set the source address step or handle special cases
    int srcStep;
    switch ((ndmaCnt[i] >> 13) & 0x3) {
        case 0: srcStep = 4; break; // Increment
        case 1: srcStep = -4; break; // Decrement
        case 2: srcStep = 0; break; // Fixed

    default: // Fill
        srcAddrs[i] = 0x10002018 + i * 0x1C;
        srcStep = 0;
        break;
    }

    // Get the logical and physical block sizes
    int32_t logiSize = ndmaWcnt[i] ? ndmaWcnt[i] : 0x1000000;
    uint16_t physSize = (1 << ((ndmaCnt[i] >> 16) & 0xF));
    LOG_INFO("NDMA channel %d transferring from 0x%08X to 0x%08X with size 0x%X in 0x%X units\n",
        i, srcAddrs[i], dstAddrs[i], logiSize << 2, physSize << 2);

    // Perform an NDMA transfer based on the repeat mode
    if (ndmaCnt[i] & (BIT(28) | BIT(29))) { // Immediate/infinite
        // Transfer a logical block made up of physical blocks
        for (; logiSize > 0; logiSize -= physSize) {
            for (uint16_t j = 0; j < physSize; j++) {
                uint32_t value = core->memory.read<uint32_t>(ARM9, srcAddrs[i]);
                core->memory.write<uint32_t>(ARM9, dstAddrs[i], value);
                srcAddrs[i] += srcStep;
                dstAddrs[i] += dstStep;
            }
        }

        // Trigger an interrupt if enabled and end if immediate
        if (ndmaCnt[i] & BIT(30))
            core->interrupts.sendInterrupt(true, i);
        if (ndmaCnt[i] & BIT(28))
            ndmaCnt[i] &= ~BIT(31);
    }
    else { // Repeat until total
        for (; logiSize > 0; logiSize -= physSize) {
            for (uint16_t j = 0; j < physSize; j++) {
                // Transfer a word of a logical block made of physical blocks
                uint32_t value = core->memory.read<uint32_t>(ARM9, srcAddrs[i]);
                core->memory.write<uint32_t>(ARM9, dstAddrs[i], value);
                srcAddrs[i] += srcStep;
                dstAddrs[i] += dstStep;

                // Trigger an interrupt and end if the word total is reached
                if (wordCounts[i]--) continue;
                if (ndmaCnt[i] & BIT(30))
                    core->interrupts.sendInterrupt(true, i);
                ndmaCnt[i] &= ~BIT(31);
                logiSize = 0;
                break;
            }
        }
    }

    // Reload destination and source addresses if enabled
    if (ndmaCnt[i] & BIT(12)) dstAddrs[i] = ndmaDad[i];
    if (ndmaCnt[i] & BIT(15)) srcAddrs[i] = ndmaSad[i];
}

void Ndma::writeSad(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxSAD registers
    mask &= 0xFFFFFFFC;
    ndmaSad[i] = (ndmaSad[i] & ~mask) | (value & mask);
}

void Ndma::writeDad(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxDAD registers
    mask &= 0xFFFFFFFC;
    ndmaDad[i] = (ndmaDad[i] & ~mask) | (value & mask);
}

void Ndma::writeTcnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxTCNT registers
    mask &= 0xFFFFFFF;
    ndmaTcnt[i] = (ndmaTcnt[i] & ~mask) | (value & mask);
}

void Ndma::writeWcnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxWCNT registers
    mask &= 0xFFFFFF;
    ndmaWcnt[i] = (ndmaWcnt[i] & ~mask) | (value & mask);
}

void Ndma::writeFdata(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxFDATA registers
    ndmaFdata[i] = (ndmaFdata[i] & ~mask) | (value & mask);
}

void Ndma::writeCnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the NDMAxCNT registers
    mask &= 0xFF0FFFFF;
    uint32_t old = ndmaCnt[i];
    ndmaCnt[i] = (ndmaCnt[i] & ~mask) | (value & mask);

    // Reload internal registers if a channel is newly started
    if (!(~old & ndmaCnt[i] & BIT(31))) return;
    wordCounts[i] = ndmaTcnt[i] ? ndmaTcnt[i] : 0x10000000;
    srcAddrs[i] = ndmaSad[i];
    dstAddrs[i] = ndmaDad[i];

    // Handle immediate transfers or catch unimplemented modes
    uint8_t mode = (ndmaCnt[i] >> 24) & 0x1F;
    if (mode >= 0x10) // Immediate
        transferBlock(i);
    else if (mode != 0x8 && mode != 0x9)
        LOG_CRIT("NDMA channel %d started in unimplemented mode: 0x%X\n", i, mode);
}
