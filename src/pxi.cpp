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

#include "core.h"

void Pxi::writePxiSync11(uint32_t mask, uint32_t value)
{
    // Send 8 bits to the ARM9 if data is written
    if (mask & 0xFF00)
        pxiSync9 = (pxiSync9 & ~0xFF) | ((value >> 8) & 0xFF);

    // Send an interrupt to the ARM9 if requested and enabled
    if ((value & mask & BIT(30)) && (pxiSync9 & BIT(31)))
        core->cpus[ARM9].sendInterrupt(12);

    // Write to the PXI_SYNC11 interrupt enable bit
    mask &= 0x80000000;
    pxiSync11 = (pxiSync11 & ~mask) | (value & mask);
}

void Pxi::writePxiSync9(uint32_t mask, uint32_t value)
{
    // Send 8 bits to the ARM11 if data is written
    if (mask & 0xFF00)
        pxiSync11 = (pxiSync11 & ~0xFF) | ((value >> 8) & 0xFF);

    // Send an interrupt to the ARM11 if requested and enabled
    if ((value & mask & 0x60000000) && (pxiSync11 & BIT(31)))
        LOG_WARN("Unimplemented ARM11 interrupt request\n");

    // Write to the PXI_SYNC9 interrupt enable bit
    mask &= 0x80000000;
    pxiSync9 = (pxiSync9 & ~mask) | (value & mask);
}
