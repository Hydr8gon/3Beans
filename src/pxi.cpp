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

uint32_t Pxi::readRecv(bool arm9) {
    // Ensure the FIFO is enabled
    if (~pxiCnt[arm9] & BIT(15))
        return pxiRecv[arm9];

    // Report an error if the FIFO is empty
    if (pxiCnt[arm9] & BIT(8)) {
        pxiCnt[arm9] |= BIT(14);
        return pxiRecv[arm9];
    }

    // Receive a value from the FIFO
    pxiRecv[arm9] = fifos[!arm9].front();
    fifos[!arm9].pop();

    if (fifos[!arm9].empty()) {
        // Set the empty bits if the FIFO is now empty
        pxiCnt[arm9] |= BIT(8);
        pxiCnt[!arm9] |= BIT(0);

        // Send a send FIFO empty interrupt to the other CPU if enabled
        if (pxiCnt[!arm9] & BIT(2))
            core->interrupts.sendInterrupt(arm9, arm9 ? 13 : 0x52);
    }
    else if (fifos[!arm9].size() == 15) {
        // Clear the full bits if the FIFO is no longer full
        pxiCnt[arm9] &= ~BIT(9);
        pxiCnt[!arm9] &= ~BIT(1);
    }
    return pxiRecv[arm9];
}

void Pxi::writeSync(bool arm9, uint32_t mask, uint32_t value)
{
    // Send 8 bits to the other CPU if data is written
    if (mask & 0xFF00)
        pxiSync[!arm9] = (pxiSync[!arm9] & ~0xFF) | ((value >> 8) & 0xFF);

    if (arm9) {
        // Send interrupts to the ARM11 if requested and enabled
        if ((value & mask & BIT(29)) && (pxiSync[false] & BIT(31)))
            core->interrupts.sendInterrupt(false, 0x50);
        if ((value & mask & BIT(30)) && (pxiSync[false] & BIT(31)))
            core->interrupts.sendInterrupt(false, 0x51);
    }
    else {
        // Send and interrupt to the ARM9 if requested and enabled
        if ((value & mask & BIT(30)) && (pxiSync[true] & BIT(31)))
            core->interrupts.sendInterrupt(true, 12);
    }

    // Write to this CPU's PXI_SYNC interrupt enable bit
    mask &= 0x80000000;
    pxiSync[arm9] = (pxiSync[arm9] & ~mask) | (value & mask);
}

void Pxi::writeCnt(bool arm9, uint16_t mask, uint16_t value) {
    // Check the interrupt conditions before writing to the register
    bool sendCond = (pxiCnt[arm9] & BIT(0)) && (pxiCnt[arm9] & BIT(2));
    bool recvCond = (~pxiCnt[arm9] & BIT(8)) && (pxiCnt[arm9] & BIT(10));

    // Clear this CPU's send FIFO if the clear bit is set
    if ((value & mask & BIT(3)) && !fifos[arm9].empty()) {
        // Empty the FIFO
        fifos[arm9] = {};
        pxiRecv[!arm9] = 0;

        // Set the FIFO empty bits and clear the FIFO full bits
        pxiCnt[arm9] = (pxiCnt[arm9] | BIT(0)) & ~BIT(1);
        pxiCnt[!arm9] = (pxiCnt[!arm9] | BIT(8)) & ~BIT(9);
    }

    // Write to this CPU's PXI_CNT enable bits
    mask &= 0x8404;
    pxiCnt[arm9] = (pxiCnt[arm9] & ~mask) | (value & mask);

    // Acknowledge the error bit if it's set
    if (value & BIT(14))
        pxiCnt[arm9] &= ~BIT(14);

    // Trigger a send FIFO empty interrupt if the condition changed to true
    if (!sendCond && (pxiCnt[arm9] & BIT(0)) && (pxiCnt[arm9] & BIT(2)))
        core->interrupts.sendInterrupt(arm9, arm9 ? 13 : 0x52);

    // Trigger a receive FIFO not empty interrupt if the condition changed to true
    if (!recvCond && (~pxiCnt[arm9] & BIT(8)) && (pxiCnt[arm9] & BIT(10)))
        core->interrupts.sendInterrupt(arm9, arm9 ? 14 : 0x53);
}

void Pxi::writeSend(bool arm9, uint32_t mask, uint32_t value) {
    // Ensure the FIFO is enabled
    if (~pxiCnt[arm9] & BIT(15))
        return;

    // Report an error if the FIFO is full
    if (pxiCnt[arm9] & BIT(1)) {
        pxiCnt[arm9] |= BIT(14);
        return;
    }

    // Send a value to the FIFO
    fifos[arm9].push(value & mask);

    if (fifos[arm9].size() == 1) {
        // Clear the empty bits if the FIFO is no longer empty
        pxiCnt[arm9] &= ~BIT(0);
        pxiCnt[!arm9] &= ~BIT(8);

        // Send a receive FIFO not empty interrupt to the other CPU if enabled
        if (pxiCnt[!arm9] & BIT(10))
            core->interrupts.sendInterrupt(arm9, arm9 ? 14 : 0x53);
    }
    else if (fifos[arm9].size() == 16) {
        // Set the full bits if the FIFO is now full
        pxiCnt[arm9] |= BIT(1);
        pxiCnt[!arm9] |= BIT(9);
    }
}
