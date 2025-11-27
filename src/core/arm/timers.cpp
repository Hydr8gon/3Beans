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

void Timers::resetCycles() {
    // Adjust timer end cycles for a global cycle reset
    for (CpuId id = ARM11A; id < ARM9; id = CpuId(id + 1))
        for (int i = 0; i < 2; i++)
            endCyclesMp[id][i] -= core->globalCycles;
    for (int i = 0; i < 4; i++)
        endCyclesTm[i] -= core->globalCycles;
}

void Timers::setMpScale(int scale) {
    // Update active ARM11 timers with the old scale and reschedule with the new one
    for (int id = 0; id < MAX_CPUS - 1; id++)
        for (int i = 0; i < 2; i++)
            if (mpTmcnt[id][i] & BIT(0))
                readMpCounter(CpuId(id), i);
    mpScale = scale;
    for (int id = 0; id < MAX_CPUS - 1; id++)
        for (int i = 0; i < 2; i++)
            if (mpTmcnt[id][i] & BIT(0))
                scheduleMp(CpuId(id), i);
}

void Timers::scheduleMp(CpuId id, int i) {
    // Schedule a timer underflow using its prescaler, with half the ARM11 frequency as a base
    if (~mpTmcnt[id][i] & BIT(0)) return;
    uint64_t cycles = (uint64_t(mpCounter[id][i]) + 1) * (((mpTmcnt[id][i]) >> 8) + 1) * 2 / mpScale;
    core->schedule(Task(TMR11A_UNDERFLOW0 + id * 2 + i), cycles);
    endCyclesMp[id][i] = core->globalCycles + cycles;
}

void Timers::underflowMp(CpuId id, int i) {
    // Ensure underflow should still occur at the current timestamp
    if (!(mpTmcnt[id][i] & BIT(0)) || endCyclesMp[id][i] != core->globalCycles)
        return;

    // Reload the timer or stop at zero
    if (mpTmcnt[id][i] & BIT(1)) {
        mpCounter[id][i] = mpReload[id][i];
        scheduleMp(id, i);
    }
    else {
        mpCounter[id][i] = 0;
        mpTmcnt[id][i] &= ~BIT(0);
    }

    // Trigger an underflow interrupt if enabled
    if (~mpTmcnt[id][i] & BIT(2)) return;
    core->interrupts.sendInterrupt(id, 0x1D + i);
    mpTmirq[id][i] |= BIT(0);
}

void Timers::overflowTm(int i) {
    // Ensure overflow should still occur at the current timestamp
    if (!(tmCntH[i] & BIT(7)) || (!countUp[i] && endCyclesTm[i] != core->globalCycles))
        return;

    // Reload the timer and trigger an overflow interrupt if enabled
    timers[i] = tmCntL[i];
    if (tmCntH[i] & BIT(6))
        core->interrupts.sendInterrupt(ARM9, i + 8);

    // Schedule the next timer overflow if not in count-up mode
    if (!countUp[i]) {
        uint64_t cycles = (0x10000 - timers[i]) << shifts[i];
        core->schedule(Task(TMR9_OVERFLOW0 + i), cycles);
        endCyclesTm[i] = core->globalCycles + cycles;
    }

    // If the next timer is in count-up mode, increment it now
    if (i < 3 && countUp[i + 1] && ++timers[i + 1] == 0)
        overflowTm(i + 1);
}

uint32_t Timers::readMpCounter(CpuId id, int i) {
    // Read one of an ARM11 core's counters, updating it if it's running on the scheduler
    if (mpTmcnt[id][i] & BIT(0)) {
        uint64_t value = std::max<int64_t>(0, endCyclesMp[id][i] - core->globalCycles);
        mpCounter[id][i] = (value / (((mpTmcnt[id][i]) >> 8) + 1) * mpScale / 2);
    }
    return mpCounter[id][i];
}

uint16_t Timers::readTmCntL(int i) {
    // Read the current timer value, updating it if it's running on the scheduler
    if ((tmCntH[i] & BIT(7)) && !countUp[i])
        timers[i] = std::min<uint64_t>(0xFFFF, 0x10000 - ((endCyclesTm[i] - core->globalCycles) >> shifts[i]));
    return timers[i];
}

void Timers::writeMpReload(CpuId id, int i, uint32_t mask, uint32_t value) {
    // Only allow access to extra core timers if the cores are enabled
    if ((id == ARM11C || id == ARM11D) && !(core->interrupts.readCfg11MpBootcnt(id) & BIT(4)))
        return;

    // Write to one of an ARM11 core's timer reloads and reload its counter
    mpReload[id][i] = (mpReload[id][i] & ~mask) | (value & mask);
    mpCounter[id][i] = mpReload[id][i];
    scheduleMp(id, i);
}

void Timers::writeMpCounter(CpuId id, int i, uint32_t mask, uint32_t value) {
    // Only allow access to extra core timers if the cores are enabled
    if ((id == ARM11C || id == ARM11D) && !(core->interrupts.readCfg11MpBootcnt(id) & BIT(4)))
        return;

    // Write to one of an ARM11 core's timer counters and update its scheduling
    mpCounter[id][i] = (mpCounter[id][i] & ~mask) | (value & mask);
    scheduleMp(id, i);
}

void Timers::writeMpTmcnt(CpuId id, int i, uint32_t mask, uint32_t value) {
    // Only allow access to extra core timers if the cores are enabled
    if ((id == ARM11C || id == ARM11D) && !(core->interrupts.readCfg11MpBootcnt(id) & BIT(4)))
        return;

    // Ensure the timer value is updated
    readMpCounter(id, i);

    // Check if watchdog mode is enabled on timer 1
    if (i == 1 && (value & mask & BIT(3)))
        LOG_CRIT("ARM11 core %d enabled unhandled watchdog mode on timer 1\n", id);

    // Write to one of an ARM11 core's timer control registers and update its scheduling
    mask &= 0xFF07;
    mpTmcnt[id][i] = (mpTmcnt[id][i] & ~mask) | (value & mask);
    scheduleMp(id, i);
}

void Timers::writeMpTmirq(CpuId id, int i, uint32_t mask, uint32_t value) {
    // Only allow access to extra core timers if the cores are enabled
    if ((id == ARM11C || id == ARM11D) && !(core->interrupts.readCfg11MpBootcnt(id) & BIT(4)))
        return;

    // Acknowledge one of an ARM11 core's timer interrupt bits
    mask &= 0x1;
    mpTmirq[id][i] &= ~(value & mask);
}

void Timers::writeTmCntL(int i, uint16_t mask, uint16_t value) {
    // Write to one of the TMCNT_L reload values
    tmCntL[i] = (tmCntL[i] & ~mask) | (value & mask);
}

void Timers::writeTmCntH(int i, uint16_t mask, uint16_t value) {
    // Ensure the timer value is updated
    bool dirty = false;
    readTmCntL(i);

    // Reload the timer if the enable bit changes from 0 to 1
    if (!(tmCntH[i] & BIT(7)) && (value & mask & BIT(7))) {
        timers[i] = tmCntL[i];
        dirty = true;
    }

    // Write to one of the TMCNT_H registers
    mask &= 0xC7;
    tmCntH[i] = (tmCntH[i] & ~mask) | (value & mask);
    countUp[i] = (i > 0 && (tmCntH[i] & BIT(2)));

    // Update the timer shift based on its prescaler, with half the ARM9 frequency as a base
    uint8_t shift = ((tmCntH[i] & 0x3) && !countUp[i]) ? (6 + ((tmCntH[i] & 0x3) << 1)) : 2;
    if (shifts[i] != shift) {
        shifts[i] = shift;
        dirty = true;
    }

    // Schedule a timer overflow if the timer changed and isn't in count-up mode
    if (dirty && (tmCntH[i] & BIT(7)) && !countUp[i]) {
        uint64_t cycles = (0x10000 - timers[i]) << shifts[i];
        core->schedule(Task(TMR9_OVERFLOW0 + i), cycles);
        endCyclesTm[i] = core->globalCycles + cycles;
    }
}
