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

void SdMmc::sendInterrupt(int bit)
{
    // Set the interrupt's request bit
    sdIrqStatus |= BIT(bit);

    // Send an interrupt to the ARM9 if conditions are met
    if (sdIrqStatus & ~sdIrqMask)
        core->interrupts.sendInterrupt(true, 16);
}

void SdMmc::runCommand()
{
    // Ensure the command is being run in normal mode
    if (cardStatus & BIT(5))
    {
        LOG_WARN("Tried to run normal SD command in app mode\n");
        cardStatus &= ~BIT(5);
        return;
    }

    // Execute a normal SD command
    switch (sdCmd & 0x3F)
    {
        case 55: return appCmd(); // APP_CMD

        default:
            LOG_WARN("Unknown SD command: CMD%d\n", sdCmd & 0x3F);
            return;
    }
}

void SdMmc::runAppCommand()
{
    // Ensure the command is being run in app mode
    if (~cardStatus & BIT(5))
    {
        LOG_WARN("Tried to run app SD command in normal mode\n");
        return;
    }

    // Return to normal mode for the next command
    cardStatus &= ~BIT(5);

    // Execute an app SD command
    switch (sdCmd & 0x3F)
    {
        case 41: return acmd41(); // ACMD41

        default:
            LOG_WARN("Unknown SD command: ACMD%d\n", sdCmd & 0x3F);
            return;
    }
}

void SdMmc::appCmd()
{
    // Switch to app command mode
    cardStatus |= BIT(5);
    sdResponse[0] = cardStatus;
}

void SdMmc::acmd41()
{
    // Stub ACMD41 to get through the ARM9 boot ROM
    LOG_WARN("Stubbed SD command: ACMD41\n");
    sdResponse[0] = 0x80000000;
}

void SdMmc::writeSdCmd(uint16_t mask, uint16_t value)
{
    // Write to the SD_CMD register
    sdCmd = (sdCmd & ~mask) | (value & mask);

    // Send a response end interrupt to indicate command completion
    sendInterrupt(0);

    // Run the command
    switch (uint8_t type = (sdCmd >> 6) & 0x3)
    {
        case 0: return runCommand();
        case 1: return runAppCommand();

        default:
            LOG_WARN("Unknown SD command type: %d\n", type);
            return;
    }
}

void SdMmc::writeSdIrqStatus(uint32_t mask, uint32_t value)
{
    // Acknowledge bits in the SD_IRQ_STATUS register
    sdIrqStatus &= ~(value & mask);
}

void SdMmc::writeSdIrqMask(uint32_t mask, uint32_t value)
{
    // Write to the SD_IRQ_MASK register
    mask &= 0x8B7F031D;
    sdIrqMask = (sdIrqMask & ~mask) | (value & mask);
}
