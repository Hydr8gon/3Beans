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

#include "arm_interp.h"
#include "core.h"

// Define functions for each ARM shift variation
#define ALU_FUNCS(func, S) \
    int ArmInterp::func##Lli(uint32_t opcode) { return func(opcode, lli##S(opcode)); } \
    int ArmInterp::func##Llr(uint32_t opcode) { return func(opcode, llr##S(opcode)) + 1; } \
    int ArmInterp::func##Lri(uint32_t opcode) { return func(opcode, lri##S(opcode)); } \
    int ArmInterp::func##Lrr(uint32_t opcode) { return func(opcode, lrr##S(opcode)) + 1; } \
    int ArmInterp::func##Ari(uint32_t opcode) { return func(opcode, ari##S(opcode)); } \
    int ArmInterp::func##Arr(uint32_t opcode) { return func(opcode, arr##S(opcode)) + 1; } \
    int ArmInterp::func##Rri(uint32_t opcode) { return func(opcode, rri##S(opcode)); } \
    int ArmInterp::func##Rrr(uint32_t opcode) { return func(opcode, rrr##S(opcode)) + 1; } \
    int ArmInterp::func##Imm(uint32_t opcode) { return func(opcode, imm##S(opcode)); }

// Create functions for instructions that have shift variations
ALU_FUNCS(_and,)
ALU_FUNCS(ands, S)
ALU_FUNCS(eor,)
ALU_FUNCS(eors, S)
ALU_FUNCS(sub,)
ALU_FUNCS(subs,)
ALU_FUNCS(rsb,)
ALU_FUNCS(rsbs,)
ALU_FUNCS(add,)
ALU_FUNCS(adds,)
ALU_FUNCS(adc,)
ALU_FUNCS(adcs,)
ALU_FUNCS(sbc,)
ALU_FUNCS(sbcs,)
ALU_FUNCS(rsc,)
ALU_FUNCS(rscs,)
ALU_FUNCS(tst, S)
ALU_FUNCS(teq, S)
ALU_FUNCS(cmp, S)
ALU_FUNCS(cmn, S)
ALU_FUNCS(orr,)
ALU_FUNCS(orrs, S)
ALU_FUNCS(mov,)
ALU_FUNCS(movs, S)
ALU_FUNCS(bic,)
ALU_FUNCS(bics, S)
ALU_FUNCS(mvn,)
ALU_FUNCS(mvns, S)

FORCE_INLINE int32_t ArmInterp::clampQ(int64_t value) {
    // Clamp value and set Q flag for saturated operations
    if (value > 0x7FFFFFFFLL) {
        cpsr |= BIT(27);
        return 0x7FFFFFFF;
    }
    else if (value < -0x80000000LL) {
        cpsr |= BIT(27);
        return -0x80000000;
    }
    return int32_t(value);
}

FORCE_INLINE uint32_t ArmInterp::lli(uint32_t opcode) { // Rm,LSL #i
    // Logical shift left by immediate
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return value << shift;
}

FORCE_INLINE uint32_t ArmInterp::llr(uint32_t opcode) { // Rm,LSL Rs
    // Logical shift left by register
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    return (shift < 32) ? (value << shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::lri(uint32_t opcode) { // Rm,LSR #i
    // Logical shift right by immediate
    // A shift of 0 translates to a shift of 32
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return shift ? (value >> shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::lrr(uint32_t opcode) { // Rm,LSR Rs
    // Logical shift right by register
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    return (shift < 32) ? (value >> shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::ari(uint32_t opcode) { // Rm,ASR #i
    // Arithmetic shift right by immediate
    // A shift of 0 translates to a shift of 32
    int32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return value >> (shift ? shift : 31);
}

FORCE_INLINE uint32_t ArmInterp::arr(uint32_t opcode) { // Rm,ASR Rs
    // Arithmetic shift right by register
    // When used as Rm, the program counter is read with +4
    int32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    return value >> ((shift < 32) ? shift : 31);
}

FORCE_INLINE uint32_t ArmInterp::rri(uint32_t opcode) { // Rm,ROR #i
    // Rotate right by immediate
    // A shift of 0 translates to a rotate with carry of 1
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return shift ? ((value << (32 - shift)) | (value >> shift)) : (((cpsr & BIT(29)) << 2) | (value >> 1));
}

FORCE_INLINE uint32_t ArmInterp::rrr(uint32_t opcode) { // Rm,ROR Rs
    // Rotate right by register
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    return (value << (32 - (shift & 0x1F))) | (value >> ((shift & 0x1F)));
}

FORCE_INLINE uint32_t ArmInterp::imm(uint32_t opcode) { // #i
    // Rotate 8-bit immediate right by a multiple of 2
    uint32_t value = opcode & 0xFF;
    uint8_t shift = (opcode >> 7) & 0x1E;
    return (value << (32 - shift)) | (value >> shift);
}

FORCE_INLINE uint32_t ArmInterp::lliS(uint32_t opcode) { // Rm,LSL #i (S)
    // Logical shift left by immediate and set carry flag
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT(32 - shift)) << 29);
    return value << shift;
}

FORCE_INLINE uint32_t ArmInterp::llrS(uint32_t opcode) { // Rm,LSL Rs (S)
    // Logical shift left by register and set carry flag
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((shift <= 32 && (value & BIT(32 - shift))) << 29);
    return (shift < 32) ? (value << shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::lriS(uint32_t opcode) { // Rm,LSR #i (S)
    // Logical shift right by immediate and set carry flag
    // A shift of 0 translates to a shift of 32
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT(shift ? (shift - 1) : 31)) << 29);
    return shift ? (value >> shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::lrrS(uint32_t opcode) { // Rm,LSR Rs (S)
    // Logical shift right by register and set carry flag
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((shift <= 32 && (value & BIT(shift - 1))) << 29);
    return (shift < 32) ? (value >> shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::ariS(uint32_t opcode) { // Rm,ASR #i (S)
    // Arithmetic shift right by immediate and set carry flag
    // A shift of 0 translates to a shift of 32
    int32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT(shift ? (shift - 1) : 31)) << 29);
    return value >> (shift ? shift : 31);
}

FORCE_INLINE uint32_t ArmInterp::arrS(uint32_t opcode) { // Rm,ASR Rs (S)
    // Arithmetic shift right by register and set carry flag
    // When used as Rm, the program counter is read with +4
    int32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT((shift <= 32) ? (shift - 1) : 31)) << 29);
    return value >> ((shift < 32) ? shift : 31);
}

FORCE_INLINE uint32_t ArmInterp::rriS(uint32_t opcode) { // Rm,ROR #i (S)
    // Rotate right by immediate and set carry flag
    // A shift of 0 translates to a rotate with carry of 1
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    uint32_t res = shift ? ((value << (32 - shift)) | (value >> shift)) : (((cpsr & BIT(29)) << 2) | (value >> 1));
    cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT(shift ? (shift - 1) : 0)) << 29);
    return res;
}

FORCE_INLINE uint32_t ArmInterp::rrrS(uint32_t opcode) { // Rm,ROR Rs (S)
    // Rotate right by register and set carry flag
    // When used as Rm, the program counter is read with +4
    uint32_t value = *registers[opcode & 0xF] + (((opcode & 0xF) == 0xF) << 2);
    uint8_t shift = *registers[(opcode >> 8) & 0xF];
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT((shift - 1) & 0x1F)) << 29);
    return (value << (32 - (shift & 0x1F))) | (value >> ((shift & 0x1F)));
}

FORCE_INLINE uint32_t ArmInterp::immS(uint32_t opcode) { // #i (S)
    // Rotate 8-bit immediate right by a multiple of 2 and set carry flag
    uint32_t value = opcode & 0xFF;
    uint8_t shift = (opcode >> 7) & 0x1E;
    if (shift > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(value & BIT(shift - 1)) << 29);
    return (value << (32 - shift)) | (value >> shift);
}

FORCE_INLINE int ArmInterp::_and(uint32_t opcode, uint32_t op2) { // AND Rd,Rn,op2
    // Bitwise and
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 & op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::eor(uint32_t opcode, uint32_t op2) { // EOR Rd,Rn,op2
    // Bitwise exclusive or
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 ^ op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::sub(uint32_t opcode, uint32_t op2) { // SUB Rd,Rn,op2
    // Subtraction
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 - op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::rsb(uint32_t opcode, uint32_t op2) { // RSB Rd,Rn,op2
    // Reverse subtraction
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op2 - op1;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::add(uint32_t opcode, uint32_t op2) { // ADD Rd,Rn,op2
    // Addition
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 + op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::adc(uint32_t opcode, uint32_t op2) { // ADC Rd,Rn,op2
    // Addition with carry
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 + op2 + ((cpsr & BIT(29)) >> 29);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::sbc(uint32_t opcode, uint32_t op2) { // SBC Rd,Rn,op2
    // Subtraction with carry
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 - op2 - 1 + ((cpsr & BIT(29)) >> 29);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::rsc(uint32_t opcode, uint32_t op2) { // RSC Rd,Rn,op2
    // Reverse subtraction with carry
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op2 - op1 - 1 + ((cpsr & BIT(29)) >> 29);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::tst(uint32_t opcode, uint32_t op2) { // TST Rn,op2
    // Test bits and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    uint32_t res = op1 & op2;
    cpsr = (cpsr & ~0xC0000000) | (res & BIT(31)) | ((res == 0) << 30);
    return 1;
}

FORCE_INLINE int ArmInterp::teq(uint32_t opcode, uint32_t op2) { // TEQ Rn,op2
    // Test bits and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    uint32_t res = op1 ^ op2;
    cpsr = (cpsr & ~0xC0000000) | (res & BIT(31)) | ((res == 0) << 30);
    return 1;
}

FORCE_INLINE int ArmInterp::cmp(uint32_t opcode, uint32_t op2) { // CMP Rn,op2
    // Compare and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    uint32_t res = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 >= res) << 29) | (((op2 ^ op1) & ~(res ^ op2) & BIT(31)) >> 3);
    return 1;
}

FORCE_INLINE int ArmInterp::cmn(uint32_t opcode, uint32_t op2) { // CMN Rn,op2
    // Compare negative and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    uint32_t res = op1 + op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 > res) << 29) | ((~(op2 ^ op1) & (res ^ op2) & BIT(31)) >> 3);
    return 1;
}

FORCE_INLINE int ArmInterp::orr(uint32_t opcode, uint32_t op2) { // ORR Rd,Rn,op2
    // Bitwise or
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 | op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::mov(uint32_t opcode, uint32_t op2) { // MOV Rd,op2
    // Move
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    *op0 = op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::bic(uint32_t opcode, uint32_t op2) { // BIC Rd,Rn,op2
    // Bit clear
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 & ~op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::mvn(uint32_t opcode, uint32_t op2) { // MVN Rd,op2
    // Move negative
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    *op0 = ~op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::ands(uint32_t opcode, uint32_t op2) { // ANDS Rd,Rn,op2
    // Bitwise and and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 & op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::eors(uint32_t opcode, uint32_t op2) { // EORS Rd,Rn,op2
    // Bitwise exclusive or and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 ^ op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::subs(uint32_t opcode, uint32_t op2) { // SUBS Rd,Rn,op2
    // Subtraction and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 >= *op0) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::rsbs(uint32_t opcode, uint32_t op2) { // RSBS Rd,Rn,op2
    // Reverse subtraction and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op2 - op1;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op2 >= *op0) << 29) | (((op1 ^ op2) & ~(*op0 ^ op1) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::adds(uint32_t opcode, uint32_t op2) { // ADDS Rd,Rn,op2
    // Addition and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 + op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 > *op0) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::adcs(uint32_t opcode, uint32_t op2) { // ADCS Rd,Rn,op2
    // Addition with carry and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 + op2 + ((cpsr & BIT(29)) >> 29);
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((op1 > *op0 ||
        (op2 == -1 && (cpsr & BIT(29)))) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::sbcs(uint32_t opcode, uint32_t op2) { // SBCS Rd,Rn,op2
    // Subtraction with carry and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 - op2 - 1 + ((cpsr & BIT(29)) >> 29);
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((op1 >= *op0 &&
        (op2 != -1 || (cpsr & BIT(29)))) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::rscs(uint32_t opcode, uint32_t op2) { // RSCS Rd,Rn,op2
    // Reverse subtraction with carry and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op2 - op1 - 1 + ((cpsr & BIT(29)) >> 29);
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((op2 >= *op0 &&
        (op1 != -1 || (cpsr & BIT(29)))) << 29) | (((op1 ^ op2) & ~(*op0 ^ op1) & BIT(31)) >> 3);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::orrs(uint32_t opcode, uint32_t op2) { // ORRS Rd,Rn,op2
    // Bitwise or and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 | op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::movs(uint32_t opcode, uint32_t op2) { // MOVS Rd,op2
    // Move and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    *op0 = op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::bics(uint32_t opcode, uint32_t op2) { // BICS Rd,Rn,op2
    // Bit clear and set flags
    // When used as Rn when shifting by register, the program counter is read with +4
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF] + (((opcode & 0x20F0010) == 0xF0010) << 2);
    *op0 = op1 & ~op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

FORCE_INLINE int ArmInterp::mvns(uint32_t opcode, uint32_t op2) { // MVNS Rd,op2
    // Move negative and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    *op0 = ~op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);

    // Handle pipelining and mode switching
    if (op0 != registers[15]) return 1;
    if (spsr) setCpsr(*spsr);
    flushPipeline();
    return 3;
}

int ArmInterp::mul(uint32_t opcode) { // MUL Rd,Rm,Rs
    // Multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 8) & 0xF];
    *op0 = op1 * op2;
    return 2;
}

int ArmInterp::mla(uint32_t opcode) { // MLA Rd,Rm,Rs,Rn
    // Multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 8) & 0xF];
    uint32_t op3 = *registers[(opcode >> 12) & 0xF];
    *op0 = op1 * op2 + op3;
    return 2;
}

int ArmInterp::umull(uint32_t opcode) { // UMULL RdLo,RdHi,Rm,Rs
    // Unsigned long multiplication
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t op3 = *registers[(opcode >> 8) & 0xF];
    uint64_t res = uint64_t(op2) * op3;
    *op0 = res, *op1 = res >> 32;
    return 3;
}

int ArmInterp::umlal(uint32_t opcode) { // UMLAL RdLo,RdHi,Rm,Rs
    // Unsigned long multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t op3 = *registers[(opcode >> 8) & 0xF];
    uint64_t res = uint64_t(op2) * op3;
    res += (uint64_t(*op1) << 32) | *op0;
    *op0 = res, *op1 = res >> 32;
    return 3;
}

int ArmInterp::smull(uint32_t opcode) { // SMULL RdLo,RdHi,Rm,Rs
    // Signed long multiplication
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int32_t op2 = *registers[opcode & 0xF];
    int32_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = int64_t(op2) * op3;
    *op0 = res, *op1 = res >> 32;
    return 3;
}

int ArmInterp::smlal(uint32_t opcode) { // SMLAL RdLo,RdHi,Rm,Rs
    // Signed long multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int32_t op2 = *registers[opcode & 0xF];
    int32_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = int64_t(op2) * op3;
    res += (int64_t(*op1) << 32) | *op0;
    *op0 = res, *op1 = res >> 32;
    return 3;
}

int ArmInterp::muls(uint32_t opcode) { // MULS Rd,Rm,Rs
    // Multiplication and set flags
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 8) & 0xF];
    *op0 = op1 * op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 4;
}

int ArmInterp::mlas(uint32_t opcode) { // MLAS Rd,Rm,Rs,Rn
    // Multiplication with accumulate and set flags
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 8) & 0xF];
    uint32_t op3 = *registers[(opcode >> 12) & 0xF];
    *op0 = op1 * op2 + op3;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 4;
}

int ArmInterp::umulls(uint32_t opcode) { // UMULLS RdLo,RdHi,Rm,Rs
    // Unsigned long multiplication and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t op3 = *registers[(opcode >> 8) & 0xF];
    uint64_t res = uint64_t(op2) * op3;
    *op0 = res, *op1 = res >> 32;
    cpsr = (cpsr & ~0xC0000000) | (*op1 & BIT(31)) | ((res == 0) << 30);
    return 5;
}

int ArmInterp::umlals(uint32_t opcode) { // UMLALS RdLo,RdHi,Rm,Rs
    // Unsigned long multiplication with accumulate and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t op3 = *registers[(opcode >> 8) & 0xF];
    uint64_t res = uint64_t(op2) * op3;
    res += (uint64_t(*op1) << 32) | *op0;
    *op0 = res, *op1 = res >> 32;
    cpsr = (cpsr & ~0xC0000000) | (*op1 & BIT(31)) | ((res == 0) << 30);
    return 5;
}

int ArmInterp::smulls(uint32_t opcode) { // SMULLS RdLo,RdHi,Rm,Rs
    // Signed long multiplication and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int32_t op2 = *registers[opcode & 0xF];
    int32_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = int64_t(op2) * op3;
    *op0 = res, *op1 = res >> 32;
    cpsr = (cpsr & ~0xC0000000) | (*op1 & BIT(31)) | ((res == 0) << 30);
    return 5;
}

int ArmInterp::smlals(uint32_t opcode) { // SMLALS RdLo,RdHi,Rm,Rs
    // Signed long multiplication with accumulate and set flags
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int32_t op2 = *registers[opcode & 0xF];
    int32_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = int64_t(op2) * op3;
    res += (int64_t(*op1) << 32) | *op0;
    *op0 = res, *op1 = res >> 32;
    cpsr = (cpsr & ~0xC0000000) | (*op1 & BIT(31)) | ((res == 0) << 30);
    return 5;
}

int ArmInterp::smulbb(uint32_t opcode) { // SMULBB Rd,Rm,Rs
    // Signed half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    *op0 = op1 * op2;
    return 1;
}

int ArmInterp::smulbt(uint32_t opcode) { // SMULBT Rd,Rm,Rs
    // Signed half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    *op0 = op1 * op2;
    return 1;
}

int ArmInterp::smultb(uint32_t opcode) { // SMULTB Rd,Rm,Rs
    // Signed half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF] >> 16;
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    *op0 = op1 * op2;
    return 1;
}

int ArmInterp::smultt(uint32_t opcode) { // SMULTT Rd,Rm,Rs
    // Signed half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF] >> 16;
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    *op0 = op1 * op2;
    return 1;
}

int ArmInterp::smulwb(uint32_t opcode) { // SMULWB Rd,Rm,Rs
    // Signed word by half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    *op0 = ((int64_t)op1 * op2) >> 16;
    return 1;
}

int ArmInterp::smulwt(uint32_t opcode) { // SMULWT Rd,Rm,Rs
    // Signed word by half-word multiplication
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    *op0 = ((int64_t)op1 * op2) >> 16;
    return 1;
}

int ArmInterp::smlabb(uint32_t opcode) { // SMLABB Rd,Rm,Rs,Rn
    // Signed half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = int64_t(op1 * op2) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlabt(uint32_t opcode) { // SMLABT Rd,Rm,Rs,Rn
    // Signed half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = int64_t(op1 * op2) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlatb(uint32_t opcode) { // SMLATB Rd,Rm,Rs,Rn
    // Signed half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF] >> 16;
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = int64_t(op1 * op2) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlatt(uint32_t opcode) { // SMLATT Rd,Rm,Rs,Rn
    // Signed half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int16_t op1 = *registers[opcode & 0xF] >> 16;
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = int64_t(op1 * op2) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlawb(uint32_t opcode) { // SMLAWB Rd,Rm,Rs,Rn
    // Signed word by half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF];
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = ((int64_t(op1) * op2) >> 16) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlawt(uint32_t opcode) { // SMLAWT Rd,Rm,Rs,Rn
    // Signed word by half-word multiplication with accumulate and set Q flag
    uint32_t *op0 = registers[(opcode >> 16) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int16_t op2 = *registers[(opcode >> 8) & 0xF] >> 16;
    int32_t op3 = *registers[(opcode >> 12) & 0xF];
    int64_t res = ((int64_t(op1) * op2) >> 16) + op3;
    cpsr |= (res != int32_t(*op0 = res)) << 27; // Q
    return 1;
}

int ArmInterp::smlalbb(uint32_t opcode) { // SMLALBB RdLo,RdHi,Rm,Rs
    // Signed long half-word multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int16_t op2 = *registers[opcode & 0xF];
    int16_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = (int64_t(*op1) << 32) | *op0;
    res += op2 * op3;
    *op0 = res, *op1 = res >> 32;
    return 2;
}

int ArmInterp::smlalbt(uint32_t opcode) { // SMLALBT RdLo,RdHi,Rm,Rs
    // Signed long half-word multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int16_t op2 = *registers[opcode & 0xF];
    int16_t op3 = *registers[(opcode >> 8) & 0xF] >> 16;
    int64_t res = (int64_t(*op1) << 32) | *op0;
    res += op2 * op3;
    *op0 = res, *op1 = res >> 32;
    return 2;
}

int ArmInterp::smlaltb(uint32_t opcode) { // SMLALTB RdLo,RdHi,Rm,Rs
    // Signed long half-word multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int16_t op2 = *registers[opcode & 0xF] >> 16;
    int16_t op3 = *registers[(opcode >> 8) & 0xF];
    int64_t res = (int64_t(*op1) << 32) | *op0;
    res += op2 * op3;
    *op0 = res, *op1 = res >> 32;
    return 2;
}

int ArmInterp::smlaltt(uint32_t opcode) { // SMLALTT RdLo,RdHi,Rm,Rs
    // Signed long half-word multiplication with accumulate
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    int16_t op2 = *registers[opcode & 0xF] >> 16;
    int16_t op3 = *registers[(opcode >> 8) & 0xF] >> 16;
    int64_t res = (int64_t(*op1) << 32) | *op0;
    res += op2 * op3;
    *op0 = res, *op1 = res >> 32;
    return 2;
}

int ArmInterp::qadd(uint32_t opcode) { // QADD Rd,Rm,Rn
    // Signed saturated addition
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = clampQ(int64_t(op1) + op2);
    return 1;
}

int ArmInterp::qsub(uint32_t opcode) { // QSUB Rd,Rm,Rn
    // Signed saturated subtraction
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = clampQ(int64_t(op1) - op2);
    return 1;
}

int ArmInterp::qdadd(uint32_t opcode) { // QDADD Rd,Rm,Rn
    // Signed saturated double and addition
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = clampQ(int64_t(op1) + clampQ(int64_t(op2) * 2));
    return 1;
}

int ArmInterp::qdsub(uint32_t opcode) { // QDSUB Rd,Rm,Rn
    // Signed saturated double and subtraction
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int32_t op1 = *registers[opcode & 0xF];
    int32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = clampQ(int64_t(op1) - clampQ(int64_t(op2) * 2));
    return 1;
}

int ArmInterp::clz(uint32_t opcode) { // CLZ Rd,Rm
    // Count leading zeros
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    for (*op0 = 32; op1 != 0; op1 >>= 1, (*op0)--);
    return 1;
}

int ArmInterp::sxtab16(uint32_t opcode) { // SXTAB Rd,Rm,ROR #imm
    // Sign-extend two bytes with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    uint16_t res1 = int8_t(op1 >> (shift + 0)) + op2;
    uint16_t res2 = int8_t(op1 >> (shift + 16)) + op2;
    *op0 = (res2 << 16) | res1;
    return 1;
}

int ArmInterp::sxtab(uint32_t opcode) { // SXTAB Rd,Rn,Rm,ROR #imm
    // Sign-extend byte with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    *op0 = int8_t(op1 >> shift) + op2;
    return 1;
}

int ArmInterp::sxtah(uint32_t opcode) { // SXTAH Rd,Rn,Rm,ROR #imm
    // Sign-extend half-word with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    *op0 = int16_t((op1 >> shift) | (op1 << (32 - shift))) + op2;
    return 1;
}

int ArmInterp::uxtab16(uint32_t opcode) { // UXTAB Rd,Rn,Rm,ROR #imm
    // Zero-extend two bytes with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    uint16_t res1 = uint8_t(op1 >> (shift + 0)) + op2;
    uint16_t res2 = uint8_t(op1 >> (shift + 16)) + op2;
    *op0 = (res2 << 16) | res1;
    return 1;
}

int ArmInterp::uxtab(uint32_t opcode) { // UXTAB Rd,Rn,Rm,ROR #imm
    // Zero-extend byte with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    *op0 = uint8_t(op1 >> shift) + op2;
    return 1;
}

int ArmInterp::uxtah(uint32_t opcode) { // UXTAH Rd,Rn,Rm,ROR #imm
    // Zero-extend half-word with rotation and optional addition
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = (~opcode & 0xF0000) ? *registers[(opcode >> 16) & 0xF] : 0;
    uint8_t shift = (opcode >> 7) & 0x18;
    *op0 = uint16_t((op1 >> shift) | (op1 << (32 - shift))) + op2;
    return 1;
}

int ArmInterp::rev16(uint32_t opcode) { // REV16 Rd,Rm
    // Reverse byte order of two half-words
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    *op0 = ((op1 >> 8) & 0xFF00FF) | ((op1 << 8) & 0xFF00FF00);
    return 1;
}

int ArmInterp::revsh(uint32_t opcode) { // REVSH Rd,Rm
    // Reverse byte order of half-word and sign-extend
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    *op0 = int16_t(((op1 >> 8) & 0xFF) | (op1 << 8));
    return 1;
}

int ArmInterp::rev(uint32_t opcode) { // REV Rd,Rm
    // Reverse byte order of word
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    *op0 = BSWAP32(op1);
    return 1;
}

int ArmInterp::pkhbt(uint32_t opcode) { // PKHBT Rd,Rn,Rm,LSL #i
    // Combine a register's low half with a shifted register's high half
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = lli(opcode);
    *op0 = (op2 & 0xFFFF0000) | (op1 & 0xFFFF);
    return 1;
}

int ArmInterp::pkhtb(uint32_t opcode) { // PKHTB Rd,Rn,Rm,ASR #i
    // Combine a register's high half with a shifted register's low half
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = ari(opcode);
    *op0 = (op1 & 0xFFFF0000) | (op2 & 0xFFFF);
    return 1;
}

int ArmInterp::sadd8(uint32_t opcode) { // SADD8 Rd,Rn,Rm
    // Signed parallel 8-bit addition and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0, cpsr &= ~0xF0000;
    for (int i = 0; i < 32; i += 8) {
        int tmp = int8_t(op1 >> i) + int8_t(op2 >> i);
        *op0 |= (tmp & 0xFF) << i;
        cpsr |= (tmp >= 0) << (16 + (i >> 3));
    }
    return 1;
}

int ArmInterp::uadd8(uint32_t opcode) { // UADD8 Rd,Rn,Rm
    // Unsigned parallel 8-bit addition and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = ((op1 >> 0) & 0xFF00FF) + ((op2 >> 0) & 0xFF00FF);
    uint32_t tmp2 = ((op1 >> 8) & 0xFF00FF) + ((op2 >> 8) & 0xFF00FF);
    *op0 = ((tmp2 << 8) & 0xFF00FF00) | (tmp1 & 0xFF00FF);
    cpsr = (cpsr & ~0xF0000) | ((tmp1 & BIT(8)) << 8) | ((tmp2 & BIT(8)) << 9) |
        ((tmp1 & BIT(24)) >> 6) | ((tmp2 & BIT(24)) >> 5);
    return 1;
}

int ArmInterp::uqadd8(uint32_t opcode) { // UQADD8 Rd,Rn,Rm
    // Unsigned parallel 8-bit addition with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0;
    for (int i = 0; i < 32; i += 8)
        *op0 |= std::min(0xFF, std::max(0, int((op1 >> i) & 0xFF) + int((op2 >> i) & 0xFF))) << i;
    return 1;
}

int ArmInterp::uhadd8(uint32_t opcode) { // UHADD8 Rd,Rn,Rm
    // Unsigned parallel 8-bit addition and halve the results
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = ((op1 >> 0) & 0xFF00FF) + ((op2 >> 0) & 0xFF00FF);
    uint32_t tmp2 = ((op1 >> 8) & 0xFF00FF) + ((op2 >> 8) & 0xFF00FF);
    *op0 = ((tmp2 << 7) & 0xFF00FF00) | ((tmp1 >> 1) & 0xFF00FF);
    return 1;
}

int ArmInterp::sadd16(uint32_t opcode) { // SADD16 Rd,Rn,Rm
    // Signed parallel 16-bit addition and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    int32_t tmp1 = int16_t(op1 >> 0) + int16_t(op2 >> 0);
    int32_t tmp2 = int16_t(op1 >> 16) + int16_t(op2 >> 16);
    *op0 = ((tmp2 << 16) & 0xFFFF0000) | (tmp1 & 0xFFFF);
    cpsr = (cpsr & ~0xF0000) | ((tmp1 >= 0) ? 0x30000 : 0) | ((tmp2 >= 0) ? 0xC0000 : 0);
    return 1;
}

int ArmInterp::uadd16(uint32_t opcode) { // UADD16 Rd,Rn,Rm
    // Unsigned parallel 16-bit addition and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = ((op1 >> 0) & 0xFFFF) + ((op2 >> 0) & 0xFFFF);
    uint32_t tmp2 = ((op1 >> 16) & 0xFFFF) + ((op2 >> 16) & 0xFFFF);
    *op0 = ((tmp2 << 16) & 0xFFFF0000) | (tmp1 & 0xFFFF);
    cpsr = (cpsr & ~0xF0000) | ((tmp1 & BIT(16)) ? 0x30000 : 0) | ((tmp2 & BIT(16)) ? 0xC0000 : 0);
    return 1;
}

int ArmInterp::uqadd16(uint32_t opcode) { // UQADD16 Rd,Rn,Rm
    // Unsigned parallel 16-bit addition with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = std::min(0xFFFF, std::max(0, int((op1 >> 0) & 0xFFFF) + int((op2 >> 0) & 0xFFFF)));
    uint32_t tmp2 = std::min(0xFFFF, std::max(0, int((op1 >> 16) & 0xFFFF) + int((op2 >> 16) & 0xFFFF)));
    *op0 = ((tmp2 & 0xFFFF) << 16) | (tmp1 & 0xFFFF);
    return 1;
}

int ArmInterp::uhadd16(uint32_t opcode) { // UHADD16 Rd,Rn,Rm
    // Unsigned parallel 16-bit addition and halve the results
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = ((op1 >> 0) & 0xFFFF) + ((op2 >> 0) & 0xFFFF);
    uint32_t tmp2 = ((op1 >> 16) & 0xFFFF) + ((op2 >> 16) & 0xFFFF);
    *op0 = ((tmp2 << 15) & 0xFFFF0000) | ((tmp1 >> 1) & 0xFFFF);
    return 1;
}

int ArmInterp::ssub8(uint32_t opcode) { // SSUB8 Rd,Rn,Rm
    // Signed parallel 8-bit subtraction and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0, cpsr &= ~0xF0000;
    for (int i = 0; i < 32; i += 8) {
        int tmp = int8_t(op1 >> i) - int8_t(op2 >> i);
        *op0 |= (tmp & 0xFF) << i;
        cpsr |= (tmp >= 0) << (16 + (i >> 3));
    }
    return 1;
}

int ArmInterp::qsub8(uint32_t opcode) { // QSUB8 Rd,Rn,Rm
    // Signed parallel 8-bit subtraction with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0;
    for (int i = 0; i < 32; i += 8)
        *op0 |= (std::min(0x7F, std::max(-0x80, int8_t(op1 >> i) - int8_t(op2 >> i))) & 0xFF) << i;
    return 1;
}

int ArmInterp::usub8(uint32_t opcode) { // USUB8 Rd,Rn,Rm
    // Unsigned parallel 8-bit subtraction and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0, cpsr &= ~0xF0000;
    for (int i = 0; i < 32; i += 8) {
        int tmp = int((op1 >> i) & 0xFF) - int((op2 >> i) & 0xFF);
        *op0 |= (tmp & 0xFF) << i;
        cpsr |= (tmp >= 0) << (16 + (i >> 3));
    }
    return 1;
}

int ArmInterp::uqsub8(uint32_t opcode) { // UQSUB8 Rd,Rn,Rm
    // Unsigned parallel 8-bit subtraction with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0;
    for (int i = 0; i < 32; i += 8)
        *op0 |= std::min(0xFF, std::max(0, int((op1 >> i) & 0xFF) - int((op2 >> i) & 0xFF))) << i;
    return 1;
}

int ArmInterp::ssub16(uint32_t opcode) { // SSUB16 Rd,Rn,Rm
    // Signed parallel 16-bit subtraction and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    int32_t tmp1 = int16_t(op1 >> 0) - int16_t(op2 >> 0);
    int32_t tmp2 = int16_t(op1 >> 16) - int16_t(op2 >> 16);
    *op0 = ((tmp2 << 16) & 0xFFFF0000) | (tmp1 & 0xFFFF);
    cpsr = (cpsr & ~0xF0000) | ((tmp1 >= 0) ? 0x30000 : 0) | ((tmp2 >= 0) ? 0xC0000 : 0);
    return 1;
}

int ArmInterp::qsub16(uint32_t opcode) { // QSUB16 Rd,Rn,Rm
    // Signed parallel 16-bit subtraction with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    int32_t tmp1 = std::min(0x7FFF, std::max(-0x8000, int16_t(op1 >> 0) - int16_t(op2 >> 0)));
    int32_t tmp2 = std::min(0x7FFF, std::max(-0x8000, int16_t(op1 >> 16) - int16_t(op2 >> 16)));
    *op0 = ((tmp2 & 0xFFFF) << 16) | (tmp1 & 0xFFFF);
    return 1;
}

int ArmInterp::usub16(uint32_t opcode) { // USUB16 Rd,Rn,Rm
    // Unsigned parallel 16-bit subtraction and set GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    int32_t tmp1 = int((op1 >> 0) & 0xFFFF) - int((op2 >> 0) & 0xFFFF);
    int32_t tmp2 = int((op1 >> 16) & 0xFFFF) - int((op2 >> 16) & 0xFFFF);
    *op0 = ((tmp2 << 16) & 0xFFFF0000) | (tmp1 & 0xFFFF);
    cpsr = (cpsr & ~0xF0000) | ((tmp1 >= 0) ? 0x30000 : 0) | ((tmp2 >= 0) ? 0xC0000 : 0);
    return 1;
}

int ArmInterp::uqsub16(uint32_t opcode) { // UQSUB16 Rd,Rn,Rm
    // Unsigned parallel 16-bit subtraction with saturation
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    uint32_t tmp1 = std::min(0xFFFF, std::max(0, int((op1 >> 0) & 0xFFFF) - int((op2 >> 0) & 0xFFFF)));
    uint32_t tmp2 = std::min(0xFFFF, std::max(0, int((op1 >> 16) & 0xFFFF) - int((op2 >> 16) & 0xFFFF)));
    *op0 = ((tmp2 & 0xFFFF) << 16) | (tmp1 & 0xFFFF);
    return 1;
}

int ArmInterp::sel(uint32_t opcode) { // SEL Rd,Rn,Rm
    // Select bytes based on GE flags
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = 0;
    for (int i = 0; i < 4; i++)
        *op0 |= ((cpsr & BIT(16 + i)) ? op1 : op2) & (0xFF << (i << 3));
    return 1;
}

FORCE_INLINE int ArmInterp::ssat(uint32_t opcode, uint32_t op2) { // SSAT Rd,#sat,op2
    // Signed saturate a 32-bit value within a bit range and set Q flag
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int32_t op1 = (1 << ((opcode >> 16) & 0x1F)) - 1;
    *op0 = std::max<int32_t>(~op1, std::min<int32_t>(op1, op2));
    cpsr |= (*op0 != op2) << 27; // Q
    return 1;
}

FORCE_INLINE int ArmInterp::usat(uint32_t opcode, uint32_t op2) { // USAT Rd,#sat,op2
    // Unsigned saturate a 32-bit value within a bit range and set Q flag
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = (1 << ((opcode >> 16) & 0x1F)) - 1;
    *op0 = std::max<int32_t>(0, std::min<int32_t>(op1, op2));
    cpsr |= (*op0 != op2) << 27; // Q
    return 1;
}

int ArmInterp::ssatLli(uint32_t opcode) { return ssat(opcode, lli(opcode)); } // SSAT Rd,#sat,Rm,LSL #i
int ArmInterp::ssatAri(uint32_t opcode) { return ssat(opcode, ari(opcode)); } // SSAT Rd,#sat,Rm,ASR #i
int ArmInterp::usatLli(uint32_t opcode) { return usat(opcode, lli(opcode)); } // USAT Rd,#sat,Rm,LSL #i
int ArmInterp::usatAri(uint32_t opcode) { return usat(opcode, ari(opcode)); } // USAT Rd,#sat,Rm,ASR #i

int ArmInterp::ssat16(uint32_t opcode) { // SSAT16 Rd,#sat,Rm
    // Signed saturate two 16-bit values within a bit range and set Q flag
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    int16_t op1 = (1 << ((opcode >> 16) & 0xF)) - 1;
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = std::max<int16_t>(~op1, std::min<int16_t>(op1, op2 >> 0)) & 0xFFFF;
    *op0 |= std::max<int16_t>(~op1, std::min<int16_t>(op1, op2 >> 16)) << 16;
    cpsr |= (*op0 != op2) << 27; // Q
    return 1;
}

int ArmInterp::usat16(uint32_t opcode) { // USAT16 Rd,#sat,Rm
    // Unsigned saturate two 16-bit values within a bit range and set Q flag
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint16_t op1 = (1 << ((opcode >> 16) & 0xF)) - 1;
    uint32_t op2 = *registers[opcode & 0xF];
    *op0 = std::max<int>(0, std::min<int>(op1, int16_t(op2 >> 0))) & 0xFFFF;
    *op0 |= std::max<int>(0, std::min<int>(op1, int16_t(op2 >> 16))) << 16;
    cpsr |= (*op0 != op2) << 27; // Q
    return 1;
}

int ArmInterp::addRegT(uint16_t opcode) { // ADD Rd,Rs,Rn
    // Addition and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = op1 + op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 > *op0) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::subRegT(uint16_t opcode) { // SUB Rd,Rs,Rn
    // Subtraction and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 >= *op0) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::addHT(uint16_t opcode) { // ADD Rd,Rs
    // Addition (THUMB)
    uint32_t *op0 = registers[((opcode >> 4) & 0x8) | (opcode & 0x7)];
    uint32_t op2 = *registers[(opcode >> 3) & 0xF];
    *op0 += op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

int ArmInterp::cmpHT(uint16_t opcode) { // CMP Rd,Rs
    // Compare and set flags (THUMB)
    uint32_t op1 = *registers[((opcode >> 4) & 0x8) | (opcode & 0x7)];
    uint32_t op2 = *registers[(opcode >> 3) & 0xF];
    uint32_t res = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 >= res) << 29) | (((op2 ^ op1) & ~(res ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::movHT(uint16_t opcode) { // MOV Rd,Rs
    // Move (THUMB)
    uint32_t *op0 = registers[((opcode >> 4) & 0x8) | (opcode & 0x7)];
    uint32_t op2 = *registers[(opcode >> 3) & 0xF];
    *op0 = op2;

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 3;
}

int ArmInterp::addPcT(uint16_t opcode) { // ADD Rd,PC,#i
    // Program counter addition (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[15] & ~3;
    uint32_t op2 = (opcode & 0xFF) << 2;
    *op0 = op1 + op2;
    return 1;
}

int ArmInterp::addSpT(uint16_t opcode) { // ADD Rd,SP,#i
    // Stack pointer addition (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[13];
    uint32_t op2 = (opcode & 0xFF) << 2;
    *op0 = op1 + op2;
    return 1;
}

int ArmInterp::addSpImmT(uint16_t opcode) { // ADD SP,#i
    // Stack pointer addition (THUMB)
    uint32_t *op0 = registers[13];
    uint32_t op2 = ((opcode & BIT(7)) ? (0 - (opcode & 0x7F)) : (opcode & 0x7F)) << 2;
    *op0 += op2;
    return 1;
}

int ArmInterp::lslImmT(uint16_t opcode) { // LSL Rd,Rs,#i
    // Logical shift left and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint8_t op2 = (opcode >> 6) & 0x1F;
    *op0 = op1 << op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    if (op2 > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(op1 & BIT(32 - op2)) << 29);
    return 1;
}

int ArmInterp::lsrImmT(uint16_t opcode) { // LSR Rd,Rs,#i
    // Logical shift right and set flags (THUMB)
    // A shift of 0 translates to a shift of 32
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint8_t op2 = (opcode >> 6) & 0x1F;
    *op0 = op2 ? (op1 >> op2) : 0;
    cpsr = (cpsr & ~0xE0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((bool)(op1 & BIT(op2 ? (op2 - 1) : 31)) << 29);
    return 1;
}

int ArmInterp::asrImmT(uint16_t opcode) { // ASR Rd,Rs,#i
    // Arithmetic shift right and set flags (THUMB)
    // A shift of 0 translates to a shift of 32
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint8_t op2 = (opcode >> 6) & 0x1F;
    *op0 = op2 ? ((int32_t)op1 >> op2) : ((op1 & BIT(31)) ? -1 : 0);
    cpsr = (cpsr & ~0xE0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((bool)(op1 & BIT(op2 ? (op2 - 1) : 31)) << 29);
    return 1;
}

int ArmInterp::addImm3T(uint16_t opcode) { // ADD Rd,Rs,#i
    // Addition and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 6) & 0x7;
    *op0 = op1 + op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 > *op0) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::subImm3T(uint16_t opcode) { // SUB Rd,Rs,#i
    // Subtraction and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 6) & 0x7;
    *op0 = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 >= *op0) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::addImm8T(uint16_t opcode) { // ADD Rd,#i
    // Addition and set flags (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[(opcode >> 8) & 0x7];
    uint32_t op2 = opcode & 0xFF;
    *op0 += op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 > *op0) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::subImm8T(uint16_t opcode) { // SUB Rd,#i
    // Subtraction and set flags (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[(opcode >> 8) & 0x7];
    uint32_t op2 = opcode & 0xFF;
    *op0 -= op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) |
        ((op1 >= *op0) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::cmpImm8T(uint16_t opcode) { // CMP Rd,#i
    // Compare and set flags (THUMB)
    uint32_t op1 = *registers[(opcode >> 8) & 0x7];
    uint32_t op2 = opcode & 0xFF;
    uint32_t res = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 >= res) << 29) | (((op2 ^ op1) & ~(res ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::movImm8T(uint16_t opcode) { // MOV Rd,#i
    // Move and set flags (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op2 = opcode & 0xFF;
    *op0 = op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::lslDpT(uint16_t opcode) { // LSL Rd,Rs
    // Logical shift left and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint8_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = (op2 < 32) ? (*op0 << op2) : 0;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    if (op2 > 0) cpsr = (cpsr & ~BIT(29)) | ((op2 <= 32 && (op1 & BIT(32 - op2))) << 29);
    return 1;
}

int ArmInterp::lsrDpT(uint16_t opcode) { // LSR Rd,Rs
    // Logical shift right and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint8_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = (op2 < 32) ? (*op0 >> op2) : 0;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    if (op2 > 0) cpsr = (cpsr & ~BIT(29)) | ((op2 <= 32 && (op1 & BIT(op2 - 1))) << 29);
    return 1;
}

int ArmInterp::asrDpT(uint16_t opcode) { // ASR Rd,Rs
    // Arithmetic shift right and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint8_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = (op2 < 32) ? ((int32_t)(*op0) >> op2) : ((*op0 & BIT(31)) ? -1 : 0);
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    if (op2 > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(op1 & BIT((op2 <= 32) ? (op2 - 1) : 31)) << 29);
    return 1;
}

int ArmInterp::rorDpT(uint16_t opcode) { // ROR Rd,Rs
    // Rotate right and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint8_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = (*op0 << (32 - (op2 & 0x1F))) | (*op0 >> (op2 & 0x1F));
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    if (op2 > 0) cpsr = (cpsr & ~BIT(29)) | ((bool)(op1 & BIT((op2 - 1) & 0x1F)) << 29);
    return 1;
}

int ArmInterp::andDpT(uint16_t opcode) { // AND Rd,Rs
    // Bitwise and and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 &= op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::eorDpT(uint16_t opcode) { // EOR Rd,Rs
    // Bitwise exclusive or and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 ^= op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::adcDpT(uint16_t opcode) { // ADC Rd,Rs
    // Addition with carry and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 += op2 + ((cpsr & BIT(29)) >> 29);
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((op1 > *op0 ||
        (op2 == -1 && (cpsr & BIT(29)))) << 29) | ((~(op2 ^ op1) & (*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::sbcDpT(uint16_t opcode) { // SBC Rd,Rs
    // Subtraction with carry and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = op1 - op2 - 1 + ((cpsr & BIT(29)) >> 29);
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((op1 >= *op0 &&
        (op2 != -1 || (cpsr & BIT(29)))) << 29) | (((op2 ^ op1) & ~(*op0 ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::tstDpT(uint16_t opcode) { // TST Rd,Rs
    // Test bits and set flags (THUMB)
    uint32_t op1 = *registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    uint32_t res = op1 & op2;
    cpsr = (cpsr & ~0xC0000000) | (res & BIT(31)) | ((res == 0) << 30);
    return 1;
}

int ArmInterp::cmpDpT(uint16_t opcode) { // CMP Rd,Rs
    // Compare and set flags (THUMB)
    uint32_t op1 = *registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    uint32_t res = op1 - op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 >= res) << 29) | (((op2 ^ op1) & ~(res ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::cmnDpT(uint16_t opcode) { // CMN Rd,Rs
    // Compare negative and set flags (THUMB)
    uint32_t op1 = *registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    uint32_t res = op1 + op2;
    cpsr = (cpsr & ~0xF0000000) | (res & BIT(31)) | ((res == 0) << 30) |
        ((op1 > res) << 29) | ((~(op2 ^ op1) & (res ^ op2) & BIT(31)) >> 3);
    return 1;
}

int ArmInterp::orrDpT(uint16_t opcode) { // ORR Rd,Rs
    // Bitwise or and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 |= op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::bicDpT(uint16_t opcode) { // BIC Rd,Rs
    // Bit clear and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 &= ~op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::mvnDpT(uint16_t opcode) { // MVN Rd,Rs
    // Move negative and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = ~op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 1;
}

int ArmInterp::negDpT(uint16_t opcode) { // NEG Rd,Rs
    // Negation and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op2 = *registers[(opcode >> 3) & 0x7];
    *op0 = -op2;
    cpsr = (cpsr & ~0xF0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30) | ((*op0 <= 0) << 29);
    return 1;
}

int ArmInterp::mulDpT(uint16_t opcode) { // MUL Rd,Rs
    // Multiplication and set flags (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    int32_t op2 = *registers[opcode & 0x7];
    *op0 = op1 * op2;
    cpsr = (cpsr & ~0xC0000000) | (*op0 & BIT(31)) | ((*op0 == 0) << 30);
    return 4;
}

int ArmInterp::sxtbT(uint16_t opcode) { // SXTB Rd,Rm
    // Sign-extend byte (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = int8_t(op1);
    return 1;
}

int ArmInterp::sxthT(uint16_t opcode) { // SXTH Rd,Rm
    // Sign-extend half-word (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = int16_t(op1);
    return 1;
}

int ArmInterp::uxtbT(uint16_t opcode) { // UXTB Rd,Rm
    // Zero-extend byte (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = uint8_t(op1);
    return 1;
}

int ArmInterp::uxthT(uint16_t opcode) { // UXTH Rd,Rm
    // Zero-extend half-word (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = uint16_t(op1);
    return 1;
}

int ArmInterp::rev16T(uint16_t opcode) { // REV16 Rd,Rm
    // Reverse byte order of two half-words (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = ((op1 >> 8) & 0xFF00FF) | ((op1 << 8) & 0xFF00FF00);
    return 1;
}

int ArmInterp::revshT(uint16_t opcode) { // REVSH Rd,Rm
    // Reverse byte order of half-word and sign-extend (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = int16_t(((op1 >> 8) & 0xFF) | (op1 << 8));
    return 1;
}

int ArmInterp::revT(uint16_t opcode) { // REV Rd,Rm
    // Reverse byte order of word (THUMB)
    if (id == ARM9) return unkArm(opcode); // ARM11-exclusive
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    *op0 = BSWAP32(op1);
    return 1;
}
