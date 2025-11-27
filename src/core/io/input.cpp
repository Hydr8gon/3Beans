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

#include "../core.h"

void Input::pressKey(int key) {
    // Clear key bits to indicate presses
    if (key < 12)
        hidPad &= ~BIT(key);
}

void Input::releaseKey(int key) {
    // Set key bits to indicate releases
    if (key < 12)
        hidPad |= BIT(key);
}

void Input::pressScreen(int x, int y) {
    // Set the touch state to active, with coordinates converted to 12-bit
    touchX = std::min(0xFFF, std::max(0, x * 0xFFF / 320));
    touchY = std::min(0xFFF, std::max(0, y * 0xFFF / 240));
    touchActive = true;
}

void Input::releaseScreen() {
    // Set the touch state to inactive and reset coordinates
    touchX = touchY = 0xFFF;
    touchActive = false;
}

void Input::setLStick(int x, int y) {
    // Set the left stick coordinates, converting them to unsigned
    stickLX = std::min(0xFFF, std::max(0, x + 0x7FF));
    stickLY = std::min(0xFFF, std::max(0, y + 0x7FF));
}

void Input::pressHome() {
    // Set a bit to request a home button press
    homeState |= BIT(0);
}

void Input::releaseHome() {
    // Set a bit to request a home button release
    homeState |= BIT(1);
}

void Input::updateHome() {
    // Trigger MCU interrupts for home button presses and releases
    for (int i = 0; i < 2; i++) {
        if (~homeState & BIT(i)) continue;
        core->i2c.mcuInterrupt(BIT(2 + i));
        homeState &= ~BIT(i);
    }
}

uint8_t Input::spiTransfer(uint8_t value) {
    // End an SPI bus 1 transfer and trigger an interrupt when its length is reached
    if (--spiCount == 0) {
        spiFifoCnt &= ~BIT(15);
        if (spiFifoIntMask & BIT(0)) {
            spiFifoIntStat |= BIT(0);
            core->interrupts.sendInterrupt(ARM11, 0x57);
        }
    }

    // Catch SPI accesses with unknown devices
    if (uint8_t dev = (spiFifoCnt >> 6) & 0x3) {
        LOG_WARN("Accessing SPI bus 1 with unknown device: 0x%X\n", dev);
        return 0;
    }

    // Set the index byte on first write, and the page byte on index zero
    if (++spiTotal == 1) {
        spiIndex = value;
        return 0;
    }
    else if (spiIndex == 0) {
        spiPage = value;
        return 0;
    }

    // Handle SPI bus 1 accesses based on the page and index bytes
    switch ((spiPage << 8) | spiIndex) {
    case 0x674D: // Touch pressed
        // Read a value containing the touch pressed bit
        return (touchActive << 7);

    case 0xFB03: // Touch/stick data
        // Read the touch and stick coordinates, with slight variation for touches
        if (spiTotal <= 11) return (touchX + ((spiTotal >> 1) & 0x1)) >> ((~spiTotal & 0x1) * 8);
        if (spiTotal <= 21) return (touchY + ((spiTotal >> 1) & 0x1)) >> ((~spiTotal & 0x1) * 8);
        if (spiTotal <= 37) return stickLY >> ((~spiTotal & 0x1) * 8);
        if (spiTotal <= 53) return stickLX >> ((~spiTotal & 0x1) * 8);
        return 0;

    default:
        // Catch SPI bus 1 accesses with unknown registers
        LOG_WARN("Accessing unknown SPI bus 1 register: 0x%X:0x%X\n", spiPage, spiIndex);
        return 0;
    }
}

uint32_t Input::readSpiFifoData() {
    // Read up to 4 bytes from SPI bus 1 if transferring in the read direction
    uint32_t value = 0;
    if ((spiFifoCnt & 0xA000) == 0x8000 && spiFifoSelect)
        for (int i = 0; i < 32 && spiCount; i += 8)
            value |= (spiTransfer(0) << i);
    return value;
}

void Input::writeSpiFifoCnt(uint32_t mask, uint32_t value) {
    // Write to the SPIBUS1_FIFO_CNT register
    mask &= 0xB0C7;
    uint32_t old = spiFifoCnt;
    spiFifoCnt = (spiFifoCnt & ~mask) | (value & mask);

    // Reload SPI bus 1 transfer length and chip select if the start bit was newly set
    if (!(~old & spiFifoCnt & BIT(15))) return;
    spiCount = spiFifoBlklen;
    spiFifoSelect |= BIT(0);
}

void Input::writeSpiFifoSelect(uint32_t mask, uint32_t value) {
    // Write to the SPIBUS1_FIFO_SELECT register
    mask &= 0x1;
    spiFifoSelect = (spiFifoSelect & ~mask) | (value & mask);

    // Reset SPI bus 1 transfer state when the chip is deselected
    if (!spiFifoSelect)
        spiTotal = 0;
}

void Input::writeSpiFifoBlklen(uint32_t mask, uint32_t value) {
    // Write to the SPIBUS1_FIFO_BLKLEN register
    mask &= 0x1FFFFF;
    spiFifoBlklen = (spiFifoBlklen & ~mask) | (value & mask);
}

void Input::writeSpiFifoData(uint32_t mask, uint32_t value) {
    // Write up to 4 bytes to SPI bus 1 if transferring in the write direction
    if ((spiFifoCnt & 0xA000) == 0xA000 && spiFifoSelect)
        for (int i = 0; i < 32 && spiCount; i += 8)
            spiTransfer((value & mask) >> i);
}

void Input::writeSpiFifoIntMask(uint32_t mask, uint32_t value) {
    // Write to the SPIBUS1_FIFO_INT_MASK register
    mask &= 0xF;
    spiFifoIntMask = (spiFifoIntMask & ~mask) | (value & mask);
}

void Input::writeSpiFifoIntStat(uint32_t mask, uint32_t value) {
    // Acknowledge bits in the SPIBUS1_FIFO_INT_STAT register
    spiFifoIntStat &= ~(value & mask);
}
