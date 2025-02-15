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

void Rsa::calculate() {
    // Get the exponent and endianness settings to use
    std::deque<uint32_t> &exp = expFifos[(rsaCnt >> 4) & 0x3];
    uint8_t i = (rsaCnt & BIT(9)) ? 0x3F : 0;
    int8_t inc = (rsaCnt & BIT(9)) ? -1 : 1;
    uint8_t size = exp.size();

    // Set initial large numbers for RSA calculation
    uint32_t base[0x40];
    memcpy(base, rsaData, sizeof(base));
    memset(rsaData, 0, sizeof(rsaData));
    rsaData[i] = 1;

    // Apply the exponent using multiplication and modulo after every step
    for (int j = 0; j < size; j++) {
        for (int k = 0; k < 32; k++) {
            if (exp[j] & BIT(k))
                mulMod(rsaData, base, i, inc, size);
            mulMod(base, base, i, inc, size);
        }
    }

    // Clear the busy bit and trigger an interrupt if enabled
    rsaCnt &= ~BIT(0);
    if (rsaCnt & BIT(1))
        core->interrupts.sendInterrupt(ARM9, 22);
    LOG_INFO("RSA calculation performed\n");
}

void Rsa::mulMod(uint32_t *dst, uint32_t *src, uint8_t i, int8_t inc, uint8_t size) {
    // Initialize arrays for the following calculations
    uint32_t *mod = rsaMods[(rsaCnt >> 4) & 0x3];
    uint32_t temp[0x80];
    uint32_t over = 0;
    uint8_t d = i, s = i, t = 0;
    temp[t + 0] = 0;

    // Calculate the lower half of a long multiplication
    for (int j = 0; j < size; j++) {
        temp[t + 1] = over;
        over = 0;
        for (int k = 0; k <= j; k++) {
            uint64_t val = uint64_t(dst[d + k * inc]) * src[s - k * inc];
            uint64_t res = val + ((uint64_t(temp[t + 1]) << 32) | temp[t + 0]);
            temp[t + 0] = (res >> 0);
            temp[t + 1] = (res >> 32);
            over += (res < val);
        }
        t += 1;
        s += inc;
    }

    // Calculate the upper half of a long multiplication
    for (int j = size - 1; j > 0; j--) {
        temp[t + 1] = over;
        over = 0;
        for (int k = 1; k <= j; k++) {
            uint64_t val = uint64_t(dst[d + k * inc]) * src[s - k * inc];
            uint64_t res = val + ((uint64_t(temp[t + 1]) << 32) | temp[t + 0]);
            temp[t + 0] = (res >> 0);
            temp[t + 1] = (res >> 32);
            over += (res < val);
        }
        t += 1;
        d += inc;
    }

    // Apply the selected modulo to the multiplication result
    bool check = true;
    for (int j = size; j >= 0; j--) {
        // Handle segments of the result that have leading zeros
        if (check) {
            // Check if the current segment is less than the modulo
            if (temp[j + size - 1] == 0) continue;
            for (int k = size - 1; k >= 0; k--) {
                if (int64_t cmp = int64_t(temp[j + k]) - mod[i + k * inc]) {
                    check = (cmp > 0);
                    break;
                }
            }
            if (!check) continue;

            // If not, subtract the modulo from the segment
            bool under = false;
            for (int k = 0; k < size; k++) {
                uint32_t res = temp[j + k] - mod[i + k * inc] - under;
                under = (res > temp[j + k] || (res == temp[j + k] && under));
                temp[j + k] = res;
            }
            check = (temp[j + size - 1] == 0);
            continue;
        }

        // Calculate a multiplication factor for the modulo
        uint32_t factor = mod[i + (size - 1) * inc];
        if (temp[j + size] >= factor)
            factor = 0xFFFFFFFF;
        else if (factor != 0)
            factor = ((uint64_t(temp[j + size]) << 32) | temp[j + size - 1]) / factor;

        // Subtract the multiplied modulo from the result segment
        uint32_t msw = 0;
        bool under = false;
        for (int k = 0; k < size; k++) {
            uint64_t val = uint64_t(mod[i + k * inc]) * factor + msw;
            uint32_t res = temp[j + k] - val - under;
            under = (res > temp[j + k] || (res == temp[j + k] && under));
            temp[j + k] = res;
            msw = (val >> 32);
        }
        uint32_t res = temp[j + size] - msw - under;
        under = (res > temp[j + size] || (res == temp[j + size] && under));
        temp[j + size] = res;

        // Add the modulo back to the segment until there's no underflow
        while (under) {
            for (int k = 0; k < size; k++) {
                res = temp[j + k] + mod[i + k * inc] + !under;
                under = !(res < temp[j + k] || (res == temp[j + k] && !under));
                temp[j + k] = res;
            }
            temp[j + size] += !under;
        }
        check = (temp[j + size - 1] == 0);
    }

    // Copy the final result to the destination
    for (int j = 0; j < size; j++)
        dst[i + j * inc] = temp[j];
}

uint32_t Rsa::readMod(int i) {
    // Read from part of the selected RSA_MOD value based on endian settings
    uint32_t value = rsaMods[(rsaCnt >> 4) & 0x3][i];
    return (rsaCnt & BIT(8)) ? BSWAP32(value) : value;
}

uint32_t Rsa::readData(int i) {
    // Read from part of the RSA_DATA value based on endian settings
    return (rsaCnt & BIT(8)) ? BSWAP32(rsaData[i]) : rsaData[i];
}

void Rsa::writeCnt(uint32_t mask, uint32_t value) {
    // Write to the RSA_CNT register and process data if triggered
    mask &= 0x333;
    bool start = (value & mask & ~rsaCnt & BIT(0));
    rsaCnt = (rsaCnt & ~mask) | (value & mask);
    if (start) calculate();
}

void Rsa::writeSlotcnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the RSA_SLOTCNT registers and clear its FIFO if triggered
    // TODO: handle the access disable bits
    if (value & mask & BIT(0)) expFifos[i].clear();
    mask &= 0x80000006;
    rsaSlotcnt[i] = (rsaSlotcnt[i] & ~mask) | (value & mask);
}

void Rsa::writeMod(int i, uint32_t mask, uint32_t value) {
    // Write to part of the selected RSA_MOD value based on endian settings
    uint32_t *mod = rsaMods[(rsaCnt >> 4) & 0x3];
    mod[i] = (rsaCnt & BIT(8)) ? (mod[i] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (mod[i] & ~mask) | (value & mask);
}

void Rsa::writeData(int i, uint32_t mask, uint32_t value) {
    // Write to part of the RSA_DATA value based on endian settings
    rsaData[i] = (rsaCnt & BIT(8)) ? (rsaData[i] & ~BSWAP32(mask)) |
        BSWAP32(value & mask) : (rsaData[i] & ~mask) | (value & mask);
}

void Rsa::writeExpfifo(uint32_t mask, uint32_t value) {
    // Check if the selected exponent FIFO has room
    uint8_t i = (rsaCnt >> 4) & 0x3;
    if (expFifos[i].size() >= 0x40) return;

    // Push a value to the FIFO based on endian settings and update the valid bit
    expFifos[i].push_front((rsaCnt & BIT(8)) ? BSWAP32(value & mask) : (value & mask));
    rsaSlotcnt[i] = (rsaSlotcnt[i] & ~BIT(0)) | (expFifos[i].size() >= 4 && !(expFifos[i].size() & 0x1));
}
