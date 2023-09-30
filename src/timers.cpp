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

void Timers::resetCycles()
{
    // Adjust timer end cycles for a global cycle reset
    for (int i = 0; i < 4; i++)
        endCycles[i] -= core->globalCycles;
}

void Timers::overflow(int i)
{
    // Ensure the timer is still meant to overflow at the current timestamp
    if (!(tmCntH[i] & BIT(7)) || (!countUp[i] && endCycles[i] != core->globalCycles))
        return;

    // Reload the timer and trigger an overflow interrupt if enabled
    timers[i] = tmCntL[i];
    if (tmCntH[i] & BIT(6))
        core->interrupts.sendInterrupt(true, i + 8);

    // Schedule the next timer overflow if not in count-up mode
    if (!countUp[i])
    {
        uint32_t cycles = (0x10000 - timers[i]) << shifts[i];
        core->schedule(Task(TIMER0_OVERFLOW + i), cycles);
        endCycles[i] = core->globalCycles + cycles;
    }

    // If the next timer is in count-up mode, increment it now
    if (i < 3 && countUp[i + 1] && ++timers[i + 1] == 0)
        overflow(i + 1);
}

uint16_t Timers::readTmCntL(int i)
{
    // Read the current timer value, updating it if it's running on the scheduler
    if ((tmCntH[i] & BIT(7)) && !countUp[i])
        timers[i] = 0x10000 - ((endCycles[i] - core->globalCycles) >> shifts[i]);
    return timers[i];
}

void Timers::writeTmCntL(int i, uint16_t mask, uint16_t value)
{
    // Write to one of the TMCNT_L reload values
    tmCntL[i] = (tmCntL[i] & ~mask) | (value & mask);
}

void Timers::writeTmCntH(int i, uint16_t mask, uint16_t value)
{
    // Ensure the timer value is updated
    bool dirty = false;
    readTmCntL(i);

    // Reload the timer if the enable bit changes from 0 to 1
    if (!(tmCntH[i] & BIT(7)) && (value & mask & BIT(7)))
    {
        timers[i] = tmCntL[i];
        dirty = true;
    }

    // Write to one of the TMCNT_H registers
    mask &= 0xC7;
    tmCntH[i] = (tmCntH[i] & ~mask) | (value & mask);
    countUp[i] = (i > 0 && (tmCntH[i] & BIT(2)));

    // Update the timer shift based on the prescaler, with the ARM9 frequency as a base (?)
    uint8_t shift = ((tmCntH[i] & 0x3) && !countUp[i]) ? (5 + ((tmCntH[i] & 0x3) << 1)) : 1;
    if (shifts[i] != shift)
    {
        shifts[i] = shift;
        dirty = true;
    }

    // Schedule a timer overflow if the timer changed and isn't in count-up mode
    if (dirty && (tmCntH[i] & BIT(7)) && !countUp[i])
    {
        uint32_t cycles = (0x10000 - timers[i]) << shifts[i];
        core->schedule(Task(TIMER0_OVERFLOW + i), cycles);
        endCycles[i] = core->globalCycles + cycles;
    }
}
