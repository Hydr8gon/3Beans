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

void Vfp11Interp::readSingleS(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP10 single read instruction
    uint8_t sn = (cn << 1) | (cp >> 2);
    switch (cpopc) {
        case 0x0: return fmrs(rd, sn); // FMRS Rd,Sn
        case 0x7: return fmrx(rd, sn); // FMRX Rd,sys

    default:
        // Catch unknown VFP10 single read opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 single read opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::readSingleD(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP11 single reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 single read opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::writeSingleS(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP10 single write instruction
    uint8_t sn = (cn << 1) | (cp >> 2);
    switch (cpopc) {
        case 0x0: return fmsr(sn, rd); // FMSR Sn,Rd
        case 0x7: return fmxr(sn, rd); // FMXR sys,Rd

    default:
        // Catch unknown VFP10 single write opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 single write opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::writeSingleD(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP11 single writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 single write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::readDoubleS(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm) {
    // Stub VFP10 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double read opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::readDoubleD(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm) {
    // Stub VFP11 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double read opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::writeDoubleS(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm) {
    // Stub VFP10 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::writeDoubleD(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm) {
    // Stub VFP11 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double write opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::loadMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP10 memory load instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fldmia(fd, *rn, ofs); // FLDMIA Rn,<Flist>
        case 0x3: return fldmiaW(fd, rn, ofs); // FLDMIA Rn!,<Flist>
        case 0x4: return fldsM(fd, *rn, ofs); // FLDS Fd,[Rn,-ofs]
        case 0x5: return fldmdbW(fd, rn, ofs); // FLDMDB Rn!,<Flist>
        case 0x6: return fldsP(fd, *rn, ofs); // FLDS Fd,[Rn,+ofs]

    default:
        // Catch unknown VFP10 memory load opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 memory load opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::loadMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP11 memory load instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fldmia(fd, *rn, ofs); // FLDMIA Rn,<Flist>
        case 0x3: return fldmiaW(fd, rn, ofs); // FLDMIA Rn!,<Flist>
        case 0x4: return flddM(fd, *rn, ofs); // FLDD Fd,[Rn,-ofs]
        case 0x5: return fldmdbW(fd, rn, ofs); // FLDMDB Rn!,<Flist>
        case 0x6: return flddP(fd, *rn, ofs); // FLDD Fd,[Rn,+ofs]

    default:
        // Catch unknown VFP11 memory load opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 memory load opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::storeMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP10 memory store instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fstmia(fd, *rn, ofs); // FSTMIA Rn,<Flist>
        case 0x3: return fstmiaW(fd, rn, ofs); // FSTMIA Rn!,<Flist>
        case 0x4: return fstsM(fd, *rn, ofs); // FSTS Fd,[Rn,-ofs]
        case 0x5: return fstmdbW(fd, rn, ofs); // FSTMDB Rn!,<Flist>
        case 0x6: return fstsP(fd, *rn, ofs); // FSTS Fd,[Rn,+ofs]

    default:
        // Catch unknown VFP10 memory store opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 memory store opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::storeMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP11 memory store instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fstmia(fd, *rn, ofs); // FSTMIA Rn,<Flist>
        case 0x3: return fstmiaW(fd, rn, ofs); // FSTMIA Rn!,<Flist>
        case 0x4: return fstdM(fd, *rn, ofs); // FSTD Fd,[Rn,-ofs]
        case 0x5: return fstmdbW(fd, rn, ofs); // FSTMDB Rn!,<Flist>
        case 0x6: return fstdP(fd, *rn, ofs); // FSTD Fd,[Rn,+ofs]

    default:
        // Catch unknown VFP11 memory store opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 memory store opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::dataOperS(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP10 data operations for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 data operation opcode bits: 0x%X\n", id, cpopc);
}

void Vfp11Interp::dataOperD(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Stub VFP11 data operations for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 data operation opcode bits: 0x%X\n", id, cpopc);
}

bool Vfp11Interp::checkEnable() {
    // Check if the VFP is enabled and trigger an undefined exception if not
    if (fpexc & BIT(30)) return true;
    *core->arms[id].registers[15] -= 4;
    core->arms[id].exception(0x04);
    return false;
}

void Vfp11Interp::fmrs(uint32_t *rd, uint8_t sn) { // FMRS Rd,Sn
    // Read from a VFP single register if enabled
    if (checkEnable())
        *rd = registers[sn];
}

void Vfp11Interp::fmrx(uint32_t *rd, uint8_t sys) { // FMRX Rd,sys
    // Read from a VFP system register, with enable check for FPSCR
    // TODO: enforce access permissions
    switch (sys) {
        case 0x00: *rd = 0x410120B4; return; // FPSID
        case 0x10: *rd = fpexc; return; // FPEXC

    case 0x02: // FPSCR
        // Move VFP flags to ARM for the special FMSTAT encoding
        if (!checkEnable()) return;
        if (rd == core->arms[id].registers[15]) // FMSTAT
            core->arms[id].cpsr = (core->arms[id].cpsr & ~0xF0000000) | (fpscr & 0xF0000000);
        else
            *rd = fpscr;
        return;

    default:
        // Catch unknown VFP system register reads
        LOG_CRIT("Unknown read from ARM11 core %d VFP system register: 0x%X\n", id, sys);
        return;
    }
}

void Vfp11Interp::fmsr(uint8_t sn, uint32_t rd) { // FMSR Sn,Rd
    // Write to a VFP single register if enabled
    if (checkEnable())
        registers[sn] = rd;
}

void Vfp11Interp::fmxr(uint8_t sys, uint32_t rd) { // FMXR sys,Rd
    // Write to a VFP system register, with enable check for FPSCR
    // TODO: enforce access permissions
    switch (sys) {
        case 0x10: fpexc = rd; return; // FPEXC
        case 0x02: if (checkEnable()) fpscr = rd; return; // FPSCR

    default:
        // Catch unknown VFP system register writes
        LOG_CRIT("Unknown write to ARM11 core %d VFP system register: 0x%X\n", id, sys);
        return;
    }
}

void Vfp11Interp::fldsP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDS Fd,[Rn,+ofs]
    // Load a VFP single register from memory with positive offset if enabled
    if (checkEnable())
        registers[fd] = core->cp15.read<uint32_t>(id, rn + (ofs << 2));
}

void Vfp11Interp::fldsM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDS Fd,[Rn,-ofs]
    // Load a VFP single register from memory with negative offset if enabled
    if (checkEnable())
        registers[fd] = core->cp15.read<uint32_t>(id, rn - (ofs << 2));
}

void Vfp11Interp::flddP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDD Fd,[Rn,+ofs]
    // Load a VFP double register from memory with positive offset if enabled
    if (!checkEnable()) return;
    registers[(fd + 0) & 0x1F] = core->cp15.read<uint32_t>(id, rn + (ofs << 2) + 0);
    registers[(fd + 1) & 0x1F] = core->cp15.read<uint32_t>(id, rn + (ofs << 2) + 4);
}

void Vfp11Interp::flddM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDD Fd,[Rn,-ofs]
    // Load a VFP double register from memory with negative offset if enabled
    if (!checkEnable()) return;
    registers[(fd + 0) & 0x1F] = core->cp15.read<uint32_t>(id, rn - (ofs << 2) + 0);
    registers[(fd + 1) & 0x1F] = core->cp15.read<uint32_t>(id, rn - (ofs << 2) + 4);
}

void Vfp11Interp::fldmia(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDMIA Rn,<Flist>
    // Load multiple VFP registers from memory with post-increment if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        registers[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, rn + (i << 2));
}

void Vfp11Interp::fldmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FLDMIA Rn!,<Flist>
    // Load multiple VFP registers from memory with post-increment and writeback if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        registers[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, *rn + (i << 2));
    *rn += (ofs << 2);
}

void Vfp11Interp::fldmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FLDMDB Rn!,<Flist>
    // Load multiple VFP registers from memory with pre-decrement and writeback if enabled
    if (!checkEnable()) return;
    *rn -= (ofs << 2);
    for (int i = 0; i < ofs; i++)
        registers[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, *rn + (i << 2));
}

void Vfp11Interp::fstsP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTS Fd,[Rn,+ofs]
    // Store a VFP single register to memory with positive offset if enabled
    if (checkEnable())
        core->cp15.write<uint32_t>(id, rn + (ofs << 2), registers[fd]);
}

void Vfp11Interp::fstsM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTS Fd,[Rn,-ofs]
    // Store a VFP single register to memory with negative offset if enabled
    if (checkEnable())
        core->cp15.write<uint32_t>(id, rn - (ofs << 2), registers[fd]);
}

void Vfp11Interp::fstdP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTD Fd,[Rn,+ofs]
    // Store a VFP double register to memory with positive offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn + (ofs << 2) + 0, registers[(fd + 0) & 0x1F]);
    core->cp15.write<uint32_t>(id, rn + (ofs << 2) + 4, registers[(fd + 1) & 0x1F]);
}

void Vfp11Interp::fstdM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTD Fd,[Rn,-ofs]
    // Store a VFP double register to memory with negative offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn - (ofs << 2) + 0, registers[(fd + 0) & 0x1F]);
    core->cp15.write<uint32_t>(id, rn - (ofs << 2) + 4, registers[(fd + 1) & 0x1F]);
}

void Vfp11Interp::fstmia(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTMIA Rn,<Flist>
    // Store multiple VFP registers to memory with post-increment if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, rn + (i << 2), registers[(fd + i) & 0x1F]);
}

void Vfp11Interp::fstmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FSTMIA Rn!,<Flist>
    // Store multiple VFP registers to memory with post-increment and writeback if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, *rn + (i << 2), registers[(fd + i) & 0x1F]);
    *rn += (ofs << 2);
}

void Vfp11Interp::fstmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FSTMDB Rn!,<Flist>
    // Store multiple VFP registers to memory with pre-decrement and writeback if enabled
    if (!checkEnable()) return;
    *rn -= (ofs << 2);
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, *rn + (i << 2), registers[(fd + i) & 0x1F]);
}
