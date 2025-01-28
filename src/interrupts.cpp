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

#include "core.h"

void Interrupts::sendInterrupt(bool arm9, int type) {
    // Send an interrupt to the ARM9
    if (arm9) {
        // Set the interrupt's request bit
        if (type != -1)
            irqIf |= BIT(type);

        // Schedule an interrupt if any requested interrupts are enabled
        if (irqIe & irqIf)
            core->schedule(ARM9_INTERRUPT, 2);
        return;
    }

    // Send an interrupt to the ARM11 cores
    for (int id = 0; id < 2; id++) {
        if (mpIle[id] && mpIge) {
            // Set the interrupt's pending bit
            if (type != -1)
                mpIp[id][type >> 5] |= BIT(type & 0x1F);

            // Schedule an interrupt if any pending interrupts are enabled
            for (int i = 0; i < 4; i++) {
                if (!(mpIe[i] & mpIp[id][i])) continue;
                core->schedule(Task(ARM11A_INTERRUPT + id), 1);
                break;
            }
        }
    }
}

void Interrupts::interrupt(CpuId id) {
    // Trigger an interrupt on a CPU if enabled and unhalt it
    if (~core->cpus[id].cpsr & BIT(7))
        core->cpus[id].exception(0x18);
    core->cpus[id].halted = false;
}

uint32_t Interrupts::readMpAck(CpuId id) {
    // Get the ID of the next pending interrupt and switch it to active
    for (int i = 0; i < 4; i++) {
        for (int bit = 0; bit < 32; bit++) {
            if (!(mpIe[i] & mpIp[id][i] & BIT(bit))) continue;
            mpIp[id][i] &= ~BIT(bit);
            mpIa[id][i] |= BIT(bit);
            return (i << 5) + bit;
        }
    }
    return 0x3FF; // None
}

void Interrupts::writeMpIle(CpuId id, uint32_t mask, uint32_t value) {
    // Write to a core's MP_ILE local interrupt enable bit
    mask &= 0x1;
    mpIle[id] = (mpIle[id] & ~mask) | (value & mask);
}

void Interrupts::writeMpEoi(CpuId id, uint32_t mask, uint32_t value) {
    // Clear the active bit for a given interrupt type
    uint8_t type = (value & mask & 0x7F);
    mpIa[id][type >> 5] &= ~BIT(type & 0x1F);
}

void Interrupts::writeMpIge(uint32_t mask, uint32_t value) {
    // Write to the MP_IGE global interrupt enable bit
    mask &= 0x1;
    mpIge = (mpIge & ~mask) | (value & mask);
}

void Interrupts::writeMpIeSet(int i, uint32_t mask, uint32_t value) {
    // Set MP_IE interrupt enable bits and check if an interrupt should occur
    mpIe[i] |= (value & mask);
    checkInterrupt(false);
}

void Interrupts::writeMpIeClear(int i, uint32_t mask, uint32_t value) {
    // Clear MP_IE interrupt enable bits
    if (!i) mask &= ~0xFFFF; // Always set
    mpIe[i] &= ~(value & mask);
}

void Interrupts::writeIrqIe(uint32_t mask, uint32_t value) {
    // Write to the IRQ_IE register and check if an interrupt should occur
    mask &= 0x3FFFFFFF;
    irqIe = (irqIe & ~mask) | (value & mask);
    checkInterrupt(true);
}

void Interrupts::writeIrqIf(uint32_t mask, uint32_t value) {
    // Clear bits in the IRQ_IF register
    irqIf &= ~(value & mask);
}
