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

uint32_t Cp15::read(uint8_t cn, uint8_t cm, uint8_t cp)
{
    // Read a value from a CP15 register
    switch ((cn << 16) | (cm << 8) | (cp << 0))
    {
        case 0x000000: return 0x41059461; // Main ID
        case 0x000001: return 0x0F0D2112; // Cache type
        case 0x010000: return ctrlReg;    // Control
        case 0x090100: return dtcmReg;    // DTCM base/size
        case 0x090101: return itcmReg;    // ITCM size

        default:
            LOG_WARN("Unknown ARM%d CP15 register read: C%d,C%d,%d\n", (id == ARM9) ? 9 : 11, cn, cm, cp);
            return 0;
    }
}

void Cp15::write(uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value)
{
    // Write a value to a CP15 register
    switch ((cn << 16) | (cm << 8) | (cp << 0))
    {
        case 0x010000: // Control
            // Set the control bits that are writable
            ctrlReg = (ctrlReg & ~0xFF085) | (value & 0xFF085);
            exceptAddr = (ctrlReg & BIT(13)) ? 0xFFFF0000 : 0x00000000;
            if (id != ARM9) return;
            dtcmRead = (ctrlReg & BIT(16)) && !(ctrlReg & BIT(17));
            dtcmWrite = (ctrlReg & BIT(16));
            itcmRead = (ctrlReg & BIT(18)) && !(ctrlReg & BIT(19));
            itcmWrite = (ctrlReg & BIT(18));
            return;

        case 0x090100: // DTCM base/size
            // Set the DTCM address and size with a minimum of 4KB
            dtcmReg = value;
            if (id != ARM9) return;
            dtcmAddr = dtcmReg & 0xFFFFF000;
            dtcmSize = std::max(0x1000, 0x200 << ((dtcmReg >> 1) & 0x1F));
            LOG_INFO("ARM9 DTCM remapped to 0x%08X with size 0x%X\n", dtcmAddr, dtcmSize);
            return;

        case 0x070004: case 0x070802: // Wait for interrupt
            core->cpus[id].halted = true;
            return;

        case 0x090101: // ITCM size
            // Set the ITCM size with a minimum of 4KB
            itcmReg = value;
            if (id != ARM9) return;
            itcmSize = std::max(0x1000, 0x200 << ((itcmReg >> 1) & 0x1F));
            LOG_INFO("ARM9 ITCM remapped with size 0x%X\n", itcmSize);
            return;

        default:
            LOG_WARN("Unknown ARM%d CP15 register write: C%d,C%d,%d\n", (id == ARM9) ? 9 : 11, cn, cm, cp);
            return;
    }
}
