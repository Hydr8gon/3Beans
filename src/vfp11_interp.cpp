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
#include <cmath>

void Vfp11Interp::readSingleS(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP10 single read instruction
    uint8_t sn = (cn << 1) | (cp >> 2);
    switch (cpopc) {
        case 0x0: return fmrs(rd, sn);
        case 0x7: return fmrx(rd, sn);

    default:
        // Catch unknown VFP10 single read opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 single read opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::readSingleD(uint8_t cpopc, uint32_t *rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP11 single read instruction
    switch (cpopc) {
    default:
        // Catch unknown VFP11 single read opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 single read opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::writeSingleS(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP10 single write instruction
    uint8_t sn = (cn << 1) | (cp >> 2);
    switch (cpopc) {
        case 0x0: return fmsr(sn, rd);
        case 0x7: return fmxr(sn, rd);

    default:
        // Catch unknown VFP10 single write opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 single write opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::writeSingleD(uint8_t cpopc, uint32_t rd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP11 single write instruction
    switch (cpopc) {
    default:
        // Catch unknown VFP11 single write opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 single write opcode bits: 0x%X\n", id, cpopc);
        return;
    }
}

void Vfp11Interp::readDoubleS(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm) {
    // Stub VFP10 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double read opcode\n", id);
}

void Vfp11Interp::readDoubleD(uint8_t cpopc, uint32_t *rd, uint32_t *rn, uint8_t cm) {
    // Stub VFP11 double reads for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double read opcode\n", id);
}

void Vfp11Interp::writeDoubleS(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm) {
    // Stub VFP10 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP10 double write opcode\n", id);
}

void Vfp11Interp::writeDoubleD(uint8_t cpopc, uint32_t rd, uint32_t rn, uint8_t cm) {
    // Stub VFP11 double writes for now
    LOG_CRIT("Unhandled ARM11 core %d VFP11 double write opcode\n", id);
}

void Vfp11Interp::loadMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP10 memory load instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (uint8_t puw = ((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fldmia(fd, *rn, ofs);
        case 0x3: return fldmiaW(fd, rn, ofs);
        case 0x4: return fldsM(fd, *rn, ofs);
        case 0x5: return fldmdbW(fd, rn, ofs);
        case 0x6: return fldsP(fd, *rn, ofs);

    default:
        // Catch unknown VFP10 memory load opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 memory load opcode bits: 0x%X\n", id, puw);
        return;
    }
}

void Vfp11Interp::loadMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP11 memory load instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (uint8_t puw = ((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fldmia(fd, *rn, ofs);
        case 0x3: return fldmiaW(fd, rn, ofs);
        case 0x4: return flddM(fd, *rn, ofs);
        case 0x5: return fldmdbW(fd, rn, ofs);
        case 0x6: return flddP(fd, *rn, ofs);

    default:
        // Catch unknown VFP11 memory load opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 memory load opcode bits: 0x%X\n", id, puw);
        return;
    }
}

void Vfp11Interp::storeMemoryS(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP10 memory store instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (uint8_t puw = ((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fstmia(fd, *rn, ofs);
        case 0x3: return fstmiaW(fd, rn, ofs);
        case 0x4: return fstsM(fd, *rn, ofs);
        case 0x5: return fstmdbW(fd, rn, ofs);
        case 0x6: return fstsP(fd, *rn, ofs);

    default:
        // Catch unknown VFP10 memory store opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 memory store opcode bits: 0x%X\n", id, puw);
        return;
    }
}

void Vfp11Interp::storeMemoryD(uint8_t cpopc, uint8_t cd, uint32_t *rn, uint8_t ofs) {
    // Execute a VFP11 memory store instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 1) & 0x1);
    switch (uint8_t puw = ((cpopc >> 1) & 0x6) | (cpopc & 0x1)) {
        case 0x2: return fstmia(fd, *rn, ofs);
        case 0x3: return fstmiaW(fd, rn, ofs);
        case 0x4: return fstdM(fd, *rn, ofs);
        case 0x5: return fstmdbW(fd, rn, ofs);
        case 0x6: return fstdP(fd, *rn, ofs);

    default:
        // Catch unknown VFP11 memory store opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 memory store opcode bits: 0x%X\n", id, puw);
        return;
    }
}

void Vfp11Interp::dataOperS(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP10 data operation instruction
    uint8_t fd = (cd << 1) | ((cpopc >> 2) & 0x1);
    uint8_t fn = (cn << 1) | ((cp >> 2) & 0x1);
    uint8_t fm = (cm << 1) | ((cp >> 0) & 0x1);
    switch (uint8_t pqrs = (cpopc & 0x8) | ((cpopc << 1) & 0x6) | ((cp >> 1) & 0x1)) {
        case 0x0: return fmacs(fd, fn, fm);
        case 0x1: return fnmacs(fd, fn, fm);
        case 0x2: return fmscs(fd, fn, fm);
        case 0x3: return fnmscs(fd, fn, fm);
        case 0x4: return fmuls(fd, fn, fm);
        case 0x5: return fnmuls(fd, fn, fm);
        case 0x6: return fadds(fd, fn, fm);
        case 0x7: return fsubs(fd, fn, fm);
        case 0x8: return fdivs(fd, fn, fm);

    case 0xF:
        // Execute a VFP10 data operation extension instruction
        switch (uint8_t ext = (cn << 1) | (cp >> 2)) {
            case 0x00: return fcpys(fd, fm);
            case 0x01: return fabss(fd, fm);
            case 0x02: return fnegs(fd, fm);
            case 0x03: return fsqrts(fd, fm);
            case 0x08: return fcmps(fd, fm);
            case 0x09: return fcmps(fd, fm); // TODO: E-variant
            case 0x0A: return fcmpzs(fd, fm);
            case 0x0B: return fcmpzs(fd, fm); // TODO: E-variant
            case 0x10: return fuitos(fd, fm);
            case 0x11: return fsitos(fd, fm);
            case 0x18: return ftouis(fd, fm);
            case 0x19: return ftouis(fd, fm); // TODO: Z-variant
            case 0x1A: return ftosis(fd, fm);
            case 0x1B: return ftosis(fd, fm); // TODO: Z-variant

        default:
            // Catch unknown VFP10 data operation extension bits
            LOG_CRIT("Unknown ARM11 core %d VFP10 data operation extension bits: 0x%X\n", id, ext);
            return;
        }

    default:
        // Catch unknown VFP10 data operation opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP10 data operation opcode bits: 0x%X\n", id, pqrs);
        return;
    }
}

void Vfp11Interp::dataOperD(uint8_t cpopc, uint8_t cd, uint8_t cn, uint8_t cm, uint8_t cp) {
    // Execute a VFP11 data operation instruction
    switch (uint8_t pqrs = (cpopc & 0x8) | ((cpopc << 1) & 0x6) | ((cp >> 1) & 0x1)) {
    case 0xF:
        // Execute a VFP11 data operation extension instruction
        switch (uint8_t ext = (cn << 1) | (cp >> 2)) {
        default:
            // Catch unknown VFP11 data operation extension bits
            LOG_CRIT("Unknown ARM11 core %d VFP11 data operation extension bits: 0x%X\n", id, ext);
            return;
        }

    default:
        // Catch unknown VFP11 data operation opcode bits
        LOG_CRIT("Unknown ARM11 core %d VFP11 data operation opcode bits: 0x%X\n", id, pqrs);
        return;
    }
}

bool Vfp11Interp::checkEnable() {
    // Check if the VFP is enabled and trigger an undefined exception if not
    if (fpexc & BIT(30)) return true;
    *core->arms[id].registers[15] -= 4;
    core->arms[id].exception(0x04);
    return false;
}

void Vfp11Interp::fmrs(uint32_t *rd, uint8_t sn) { // FMRS Rd,Sn
    // Read from a single register if enabled
    if (!checkEnable()) return;
    *rd = regs.u32[sn];
}

void Vfp11Interp::fmrx(uint32_t *rd, uint8_t sys) { // FMRX Rd,sys
    // Read from a VFP system register
    // TODO: enforce access permissions
    switch (sys) {
        case 0x00: *rd = 0x410120B4; return; // FPSID
        case 0x10: *rd = fpexc; return; // FPEXC

    case 0x02: // FPSCR
        // Read from FPSCR if enabled, or move VFP flags to ARM for FMSTAT
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
    // Write to a single register if enabled
    if (!checkEnable()) return;
    regs.u32[sn] = rd;
}

void Vfp11Interp::fmxr(uint8_t sys, uint32_t rd) { // FMXR sys,Rd
    // Write to a VFP system register
    // TODO: enforce access permissions
    switch (sys) {
        case 0x10: fpexc = rd; return; // FPEXC

    case 0x02: // FPSCR
        // Write to the FPSCR register if enabled
        // TODO: handle different modes and traps/exceptions
        if (!checkEnable()) return;
        fpscr = rd;
        vecLength = ((fpscr >> 16) & 0x7) + 1;
        vecStride = (fpscr & BIT(21));
        return;

    default:
        // Catch unknown VFP system register writes
        LOG_CRIT("Unknown write to ARM11 core %d VFP system register: 0x%X\n", id, sys);
        return;
    }
}

void Vfp11Interp::fldsP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDS Fd,[Rn,+ofs]
    // Load a single register from memory with positive offset if enabled
    if (!checkEnable()) return;
    regs.u32[fd] = core->cp15.read<uint32_t>(id, rn + (ofs << 2));
}

void Vfp11Interp::fldsM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDS Fd,[Rn,-ofs]
    // Load a single register from memory with negative offset if enabled
    if (!checkEnable()) return;
    regs.u32[fd] = core->cp15.read<uint32_t>(id, rn - (ofs << 2));
}

void Vfp11Interp::flddP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDD Fd,[Rn,+ofs]
    // Load a double register from memory with positive offset if enabled
    if (!checkEnable()) return;
    regs.u32[fd & ~0x1] = core->cp15.read<uint32_t>(id, rn + (ofs << 2) + 0);
    regs.u32[fd | 0x1] = core->cp15.read<uint32_t>(id, rn + (ofs << 2) + 4);
}

void Vfp11Interp::flddM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDD Fd,[Rn,-ofs]
    // Load a double register from memory with negative offset if enabled
    if (!checkEnable()) return;
    regs.u32[fd & ~0x1] = core->cp15.read<uint32_t>(id, rn - (ofs << 2) + 0);
    regs.u32[fd | 0x1] = core->cp15.read<uint32_t>(id, rn - (ofs << 2) + 4);
}

void Vfp11Interp::fldmia(uint8_t fd, uint32_t rn, uint8_t ofs) { // FLDMIA Rn,<Flist>
    // Load multiple VFP registers from memory with post-increment if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        regs.u32[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, rn + (i << 2));
}

void Vfp11Interp::fldmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FLDMIA Rn!,<Flist>
    // Load multiple VFP registers from memory with post-increment and writeback if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        regs.u32[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, *rn + (i << 2));
    *rn += (ofs << 2);
}

void Vfp11Interp::fldmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FLDMDB Rn!,<Flist>
    // Load multiple VFP registers from memory with pre-decrement and writeback if enabled
    if (!checkEnable()) return;
    *rn -= (ofs << 2);
    for (int i = 0; i < ofs; i++)
        regs.u32[(fd + i) & 0x1F] = core->cp15.read<uint32_t>(id, *rn + (i << 2));
}

void Vfp11Interp::fstsP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTS Fd,[Rn,+ofs]
    // Store a single register to memory with positive offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn + (ofs << 2), regs.u32[fd]);
}

void Vfp11Interp::fstsM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTS Fd,[Rn,-ofs]
    // Store a single register to memory with negative offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn - (ofs << 2), regs.u32[fd]);
}

void Vfp11Interp::fstdP(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTD Fd,[Rn,+ofs]
    // Store a double register to memory with positive offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn + (ofs << 2) + 0, regs.u32[fd & ~0x1]);
    core->cp15.write<uint32_t>(id, rn + (ofs << 2) + 4, regs.u32[fd | 0x1]);
}

void Vfp11Interp::fstdM(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTD Fd,[Rn,-ofs]
    // Store a double register to memory with negative offset if enabled
    if (!checkEnable()) return;
    core->cp15.write<uint32_t>(id, rn - (ofs << 2) + 0, regs.u32[fd & ~0x1]);
    core->cp15.write<uint32_t>(id, rn - (ofs << 2) + 4, regs.u32[fd | 0x1]);
}

void Vfp11Interp::fstmia(uint8_t fd, uint32_t rn, uint8_t ofs) { // FSTMIA Rn,<Flist>
    // Store multiple VFP registers to memory with post-increment if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, rn + (i << 2), regs.u32[(fd + i) & 0x1F]);
}

void Vfp11Interp::fstmiaW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FSTMIA Rn!,<Flist>
    // Store multiple VFP registers to memory with post-increment and writeback if enabled
    if (!checkEnable()) return;
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, *rn + (i << 2), regs.u32[(fd + i) & 0x1F]);
    *rn += (ofs << 2);
}

void Vfp11Interp::fstmdbW(uint8_t fd, uint32_t *rn, uint8_t ofs) { // FSTMDB Rn!,<Flist>
    // Store multiple VFP registers to memory with pre-decrement and writeback if enabled
    if (!checkEnable()) return;
    *rn -= (ofs << 2);
    for (int i = 0; i < ofs; i++)
        core->cp15.write<uint32_t>(id, *rn + (i << 2), regs.u32[(fd + i) & 0x1F]);
}

// Perform a single data operation in scalar, mixed, or vector mode if enabled
#define FDOPS_FUNC(name, sign, op0) void Vfp11Interp::name(uint8_t fd, uint8_t fn, uint8_t fm) { \
    if (!checkEnable()) return; \
    if (vecLength == 1 || fd < 8) { \
        regs.flt[fd] = sign(regs.flt[fn] op0 regs.flt[fm]); \
    } \
    else if (fm < 8) { \
        float *bd = &regs.flt[fd & 0x18], *bn = &regs.flt[fn & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = sign(bn[(fn + i) & 0x7] op0 regs.flt[fm]); \
    } \
    else { \
        float *bd = &regs.flt[fd & 0x18], *bn = &regs.flt[fn & 0x18], *bm = &regs.flt[fm & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = sign(bn[(fn + i) & 0x7] op0 bm[(fm + i) & 0x7]); \
    } \
}

FDOPS_FUNC(fadds, +, +) // FADDS Fd,Fn,Fm
FDOPS_FUNC(fsubs, +, -) // FSUBS Fd,Fn,Fm
FDOPS_FUNC(fdivs, +, /) // FDIVS Fd,Fn,Fm
FDOPS_FUNC(fmuls, +, *) // FMULS Fd,Fn,Fm
FDOPS_FUNC(fnmuls, -, *) // FNMULS Fd,Fn,Fm

// Perform a single accumulative data operation in scalar, mixed, or vector mode if enabled
#define FDOCS_FUNC(name, sign, op0, op1) void Vfp11Interp::name(uint8_t fd, uint8_t fn, uint8_t fm) { \
    if (!checkEnable()) return; \
    if (vecLength == 1 || fd < 8) { \
        regs.flt[fd] = sign(regs.flt[fn] op0 regs.flt[fm]) op1 regs.flt[fd]; \
    } \
    else if (fm < 8) { \
        float *bd = &regs.flt[fd & 0x18], *bn = &regs.flt[fn & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = sign(bn[(fn + i) & 0x7] op0 regs.flt[fm]) op1 bd[(fd + i) & 0x7]; \
    } \
    else { \
        float *bd = &regs.flt[fd & 0x18], *bn = &regs.flt[fn & 0x18], *bm = &regs.flt[fm & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = sign(bn[(fn + i) & 0x7] op0 bm[(fm + i) & 0x7]) op1 bd[(fd + i) & 0x7]; \
    } \
}

FDOCS_FUNC(fmacs, +, *, +) // FMACS Fd,Fn,Fm
FDOCS_FUNC(fnmacs, -, *, +) // FNMACS Fd,Fn,Fm
FDOCS_FUNC(fmscs, +, *, -) // FMSCS Fd,Fn,Fm
FDOCS_FUNC(fnmscs, -, *, -) // FNMSCS Fd,Fn,Fm

// Perform a single extended data operation in scalar, mixed, or vector mode if enabled
#define FDOES_FUNC(name, op0) void Vfp11Interp::name(uint8_t fd, uint8_t fm) { \
    if (!checkEnable()) return; \
    if (vecLength == 1 || fd < 8) { \
        regs.flt[fd] = op0(regs.flt[fm]); \
    } \
    else if (fm < 8) { \
        float *bd = &regs.flt[fd & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = op0(regs.flt[fm]); \
    } \
    else { \
        float *bd = &regs.flt[fd & 0x18], *bm = &regs.flt[fm & 0x18]; \
        for (int i = 0; i < (vecLength << vecStride); i += (1 << vecStride)) \
            bd[(fd + i) & 0x7] = op0(bm[(fm + i) & 0x7]); \
    } \
}

FDOES_FUNC(fcpys, +) // FCPYS Fd,Fm
FDOES_FUNC(fabss, fabsf) // FABSS Fd,Fm
FDOES_FUNC(fnegs, -) // FNEGS Fd,Fm
FDOES_FUNC(fsqrts, sqrtf) // FSQRTS Fd,Fm

void Vfp11Interp::fcmps(uint8_t fd, uint8_t fm) { // FCMPS Fd,Fm
    // Compare a single register with another and set flags if enabled
    if (!checkEnable()) return;
    float res = regs.flt[fd] - regs.flt[fm];
    bool n = (res < 0);
    bool z = (res == 0);
    bool c = (res >= 0 || std::isnan(res));
    bool v = std::isnan(res);
    fpscr = (fpscr & ~0xF0000000) | (n << 31) | (z << 30) | (c << 29) | (v << 28);
}

void Vfp11Interp::fcmpzs(uint8_t fd, uint8_t fm) { // FCMPZS Fd
    // Compare a single register with zero and set flags if enabled
    if (!checkEnable()) return;
    bool n = (regs.flt[fd] < 0);
    bool z = (regs.flt[fd] == 0);
    bool c = (regs.flt[fd] >= 0 || std::isnan(regs.flt[fd]));
    bool v = std::isnan(regs.flt[fd]);
    fpscr = (fpscr & ~0xF0000000) | (n << 31) | (z << 30) | (c << 29) | (v << 28);
}

void Vfp11Interp::fuitos(uint8_t fd, uint8_t fm) { // FUITOS Fd,Im
    // Convert an unsigned integer to a float if enabled
    if (!checkEnable()) return;
    regs.flt[fd] = float(regs.u32[fm]);
}

void Vfp11Interp::fsitos(uint8_t fd, uint8_t fm) { // FSITOS Fd,Im
    // Convert a signed integer to a float if enabled
    if (!checkEnable()) return;
    regs.flt[fd] = float(regs.i32[fm]);
}

void Vfp11Interp::ftouis(uint8_t fd, uint8_t fm) { // FTOUIS Id,Fm
    // Convert a float to an unsigned integer if enabled
    if (!checkEnable()) return;
    regs.u32[fd] = uint32_t(regs.flt[fm]);
}

void Vfp11Interp::ftosis(uint8_t fd, uint8_t fm) { // FTOSIS Id,Fm
    // Convert a float to a signed integer if enabled
    if (!checkEnable()) return;
    regs.i32[fd] = int32_t(regs.flt[fm]);
}
