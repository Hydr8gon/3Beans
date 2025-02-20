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

uint32_t Vfp11Interp::readSingleS(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Catch the special FMSTAT encoding and prevent it from nuking the PC
    uint32_t value = 0;
    if (cpopc == 0x7 && ((cn << 1) | (cp >> 2)) == 0x2) {
        uint32_t opcode = core->cp15.read<uint32_t>(id, *core->arms[id].registers[15] - 8);
        if (((opcode >> 12) & 0xF) == 15)
            value = *core->arms[id].registers[(opcode >> 12) & 0xF];
    }

    // Stub VFP10 single reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 single read opcode bits: 0x%X\n", id, cpopc);
    return value;
}

uint32_t Vfp11Interp::readSingleD(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP11 single reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 single read opcode bits: 0x%X\n", id, cpopc);
    return 0;
}

void Vfp11Interp::writeSingleS(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value) {
    // Stub VFP10 single writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 single write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::writeSingleD(uint8_t cpopc, uint8_t cn, uint8_t cm, uint8_t cp, uint32_t value) {
    // Stub VFP11 single writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 single write opcode bits: 0x%X\n", id, cpopc);
}

uint64_t Vfp11Interp::readDoubleS(uint8_t cpopc, uint8_t cm) {
    // Stub VFP10 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double read opcode bits: 0x%X\n", id, cpopc);
    return 0;
}

uint64_t Vfp11Interp::readDoubleD(uint8_t cpopc, uint8_t cm) {
    // Stub VFP11 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double read opcode bits: 0x%X\n", id, cpopc);
    return 0;
}

void Vfp11Interp::writeDoubleS(uint8_t cpopc, uint8_t cm, uint64_t value) {
    // Stub VFP10 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::writeDoubleD(uint8_t cpopc, uint8_t cm, uint64_t value) {
    // Stub VFP11 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::loadMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs) {
    // Stub VFP10 memory loads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 memory load opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::loadMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs) {
    // Stub VFP11 memory loads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 memory load opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::storeMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs) {
    // Stub VFP10 memory stores for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 memory store opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::storeMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint16_t ofs) {
    // Stub VFP11 memory stores for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 memory store opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::dataOperS(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP10 data operations for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 data operation opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::dataOperD(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP11 data operations for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 data operation opcode bits: 0x%X\n", id, cpopc);
}
