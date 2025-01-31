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

// Define functions for each ARM offset variation (half type)
#define HALF_FUNCS(func) \
    int ArmInterp::func##Ofrm(uint32_t opcode) { return func##Of(opcode, -rp(opcode)); } \
    int ArmInterp::func##Ofim(uint32_t opcode) { return func##Of(opcode, -ipH(opcode)); } \
    int ArmInterp::func##Ofrp(uint32_t opcode) { return func##Of(opcode, rp(opcode)); } \
    int ArmInterp::func##Ofip(uint32_t opcode) { return func##Of(opcode, ipH(opcode)); } \
    int ArmInterp::func##Prrm(uint32_t opcode) { return func##Pr(opcode, -rp(opcode)); } \
    int ArmInterp::func##Prim(uint32_t opcode) { return func##Pr(opcode, -ipH(opcode)); } \
    int ArmInterp::func##Prrp(uint32_t opcode) { return func##Pr(opcode, rp(opcode)); } \
    int ArmInterp::func##Prip(uint32_t opcode) { return func##Pr(opcode, ipH(opcode)); } \
    int ArmInterp::func##Ptrm(uint32_t opcode) { return func##Pt(opcode, -rp(opcode)); } \
    int ArmInterp::func##Ptim(uint32_t opcode) { return func##Pt(opcode, -ipH(opcode)); } \
    int ArmInterp::func##Ptrp(uint32_t opcode) { return func##Pt(opcode, rp(opcode)); } \
    int ArmInterp::func##Ptip(uint32_t opcode) { return func##Pt(opcode, ipH(opcode)); }

// Define functions for each ARM offset variation (full type)
#define FULL_FUNCS(func) \
    int ArmInterp::func##Ofim(uint32_t opcode) { return func##Of(opcode, -ip(opcode)); } \
    int ArmInterp::func##Ofip(uint32_t opcode) { return func##Of(opcode, ip(opcode)); } \
    int ArmInterp::func##Ofrmll(uint32_t opcode) { return func##Of(opcode, -rpll(opcode)); } \
    int ArmInterp::func##Ofrmlr(uint32_t opcode) { return func##Of(opcode, -rplr(opcode)); } \
    int ArmInterp::func##Ofrmar(uint32_t opcode) { return func##Of(opcode, -rpar(opcode)); } \
    int ArmInterp::func##Ofrmrr(uint32_t opcode) { return func##Of(opcode, -rprr(opcode)); } \
    int ArmInterp::func##Ofrpll(uint32_t opcode) { return func##Of(opcode, rpll(opcode)); } \
    int ArmInterp::func##Ofrplr(uint32_t opcode) { return func##Of(opcode, rplr(opcode)); } \
    int ArmInterp::func##Ofrpar(uint32_t opcode) { return func##Of(opcode, rpar(opcode)); } \
    int ArmInterp::func##Ofrprr(uint32_t opcode) { return func##Of(opcode, rprr(opcode)); } \
    int ArmInterp::func##Prim(uint32_t opcode) { return func##Pr(opcode, -ip(opcode)); } \
    int ArmInterp::func##Prip(uint32_t opcode) { return func##Pr(opcode, ip(opcode)); } \
    int ArmInterp::func##Prrmll(uint32_t opcode) { return func##Pr(opcode, -rpll(opcode)); } \
    int ArmInterp::func##Prrmlr(uint32_t opcode) { return func##Pr(opcode, -rplr(opcode)); } \
    int ArmInterp::func##Prrmar(uint32_t opcode) { return func##Pr(opcode, -rpar(opcode)); } \
    int ArmInterp::func##Prrmrr(uint32_t opcode) { return func##Pr(opcode, -rprr(opcode)); } \
    int ArmInterp::func##Prrpll(uint32_t opcode) { return func##Pr(opcode, rpll(opcode)); } \
    int ArmInterp::func##Prrplr(uint32_t opcode) { return func##Pr(opcode, rplr(opcode)); } \
    int ArmInterp::func##Prrpar(uint32_t opcode) { return func##Pr(opcode, rpar(opcode)); } \
    int ArmInterp::func##Prrprr(uint32_t opcode) { return func##Pr(opcode, rprr(opcode)); } \
    int ArmInterp::func##Ptim(uint32_t opcode) { return func##Pt(opcode, -ip(opcode)); } \
    int ArmInterp::func##Ptip(uint32_t opcode) { return func##Pt(opcode, ip(opcode)); } \
    int ArmInterp::func##Ptrmll(uint32_t opcode) { return func##Pt(opcode, -rpll(opcode)); } \
    int ArmInterp::func##Ptrmlr(uint32_t opcode) { return func##Pt(opcode, -rplr(opcode)); } \
    int ArmInterp::func##Ptrmar(uint32_t opcode) { return func##Pt(opcode, -rpar(opcode)); } \
    int ArmInterp::func##Ptrmrr(uint32_t opcode) { return func##Pt(opcode, -rprr(opcode)); } \
    int ArmInterp::func##Ptrpll(uint32_t opcode) { return func##Pt(opcode, rpll(opcode)); } \
    int ArmInterp::func##Ptrplr(uint32_t opcode) { return func##Pt(opcode, rplr(opcode)); } \
    int ArmInterp::func##Ptrpar(uint32_t opcode) { return func##Pt(opcode, rpar(opcode)); } \
    int ArmInterp::func##Ptrprr(uint32_t opcode) { return func##Pt(opcode, rprr(opcode)); }

// Create functions for instructions that have offset variations (half type)
HALF_FUNCS(ldrsb)
HALF_FUNCS(ldrsh)
HALF_FUNCS(ldrh)
HALF_FUNCS(strh)
HALF_FUNCS(ldrd)
HALF_FUNCS(strd)

// Create functions for instructions that have offset variations (full type)
FULL_FUNCS(ldrb)
FULL_FUNCS(strb)
FULL_FUNCS(ldr)
FULL_FUNCS(str)

FORCE_INLINE uint32_t ArmInterp::ip(uint32_t opcode) { // #i (B/_)
    // Immediate offset for byte and word transfers
    return opcode & 0xFFF;
}

FORCE_INLINE uint32_t ArmInterp::ipH(uint32_t opcode) { // #i (SB/SH/H/D)
    // Immediate offset for signed, half-word, and double word transfers
    return ((opcode >> 4) & 0xF0) | (opcode & 0xF);
}

FORCE_INLINE uint32_t ArmInterp::rp(uint32_t opcode) { // Rm
    // Register offset for signed and half-word transfers
    return *registers[opcode & 0xF];
}

FORCE_INLINE uint32_t ArmInterp::rpll(uint32_t opcode) { // Rm,LSL #i
    // Logical shift left by immediate
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return value << shift;
}

FORCE_INLINE uint32_t ArmInterp::rplr(uint32_t opcode) { // Rm,LSR #i
    // Logical shift right by immediate
    // A shift of 0 translates to a shift of 32
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return shift ? (value >> shift) : 0;
}

FORCE_INLINE uint32_t ArmInterp::rpar(uint32_t opcode) { // Rm,ASR #i
    // Arithmetic shift right by immediate
    // A shift of 0 translates to a shift of 32
    int32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return value >> (shift ? shift : 31);
}

FORCE_INLINE uint32_t ArmInterp::rprr(uint32_t opcode) { // Rm,ROR #i
    // Rotate right by immediate
    // A shift of 0 translates to a1 rotate with carry of 1
    uint32_t value = *registers[opcode & 0xF];
    uint8_t shift = (opcode >> 7) & 0x1F;
    return shift ? ((value << (32 - shift)) | (value >> shift)) : (((cpsr & BIT(29)) << 2) | (value >> 1));
}

FORCE_INLINE int ArmInterp::ldrsbOf(uint32_t opcode, uint32_t op2) { // LDRSB Rd,[Rn,op2]
    // Signed byte load, pre-adjust without writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = (int8_t)core->memory.read<uint8_t>(id, op1 + op2);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrshOf(uint32_t opcode, uint32_t op2) { // LDRSH Rd,[Rn,op2]
    // Signed half-word load, pre-adjust without writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = (int16_t)core->memory.read<uint16_t>(id, op1 += op2);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrbOf(uint32_t opcode, uint32_t op2) { // LDRB Rd,[Rn,op2]
    // Byte load, pre-adjust without writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint8_t>(id, op1 + op2);

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strbOf(uint32_t opcode, uint32_t op2) { // STRB Rd,[Rn,op2]
    // Byte store, pre-adjust without writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    core->memory.write<uint8_t>(id, op1 + op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrhOf(uint32_t opcode, uint32_t op2) { // LDRH Rd,[Rn,op2]
    // Half-word load, pre-adjust without writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint16_t>(id, op1 += op2);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strhOf(uint32_t opcode, uint32_t op2) { // STRH Rd,[Rn,op2]
    // Half-word store, pre-adjust without writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    core->memory.write<uint16_t>(id, op1 + op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrOf(uint32_t opcode, uint32_t op2) { // LDR Rd,[Rn,op2]
    // Word load, pre-adjust without writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint32_t>(id, op1 += op2);

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strOf(uint32_t opcode, uint32_t op2) { // STR Rd,[Rn,op2]
    // Word store, pre-adjust without writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, op1 + op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrdOf(uint32_t opcode, uint32_t op2) { // LDRD Rd,[Rn,op2]
    // Double word load, pre-adjust without writeback
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *registers[op0] = core->memory.read<uint32_t>(id, op1 += op2);
    *registers[op0 + 1] = core->memory.read<uint32_t>(id, op1 + 4);
    return 2;
}

FORCE_INLINE int ArmInterp::strdOf(uint32_t opcode, uint32_t op2) { // STRD Rd,[Rn,op2]
    // Double word store, pre-adjust without writeback
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, op1 += op2, *registers[op0]);
    core->memory.write<uint32_t>(id, op1 + 4, *registers[op0 + 1]);
    return 2;
}

FORCE_INLINE int ArmInterp::ldrsbPr(uint32_t opcode, uint32_t op2) { // LDRSB Rd,[Rn,op2]!
    // Signed byte load, pre-adjust with writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    *op0 = (int8_t)core->memory.read<uint8_t>(id, *op1 += op2);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrshPr(uint32_t opcode, uint32_t op2) { // LDRSH Rd,[Rn,op2]!
    // Signed half-word load, pre-adjust with writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = *op1 += op2;
    *op0 = (int16_t)core->memory.read<uint16_t>(id, address);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrbPr(uint32_t opcode, uint32_t op2) { // LDRB Rd,[Rn,op2]!
    // Byte load, pre-adjust with writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint8_t>(id, *op1 += op2);

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strbPr(uint32_t opcode, uint32_t op2) { // STRB Rd,[Rn,op2]!
    // Byte store, pre-adjust with writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint8_t>(id, *op1 += op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrhPr(uint32_t opcode, uint32_t op2) { // LDRH Rd,[Rn,op2]!
    // Half-word load, pre-adjust with writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = *op1 += op2;
    *op0 = core->memory.read<uint16_t>(id, address);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strhPr(uint32_t opcode, uint32_t op2) { // STRH Rd,[Rn,op2]!
    // Half-word store, pre-adjust with writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint16_t>(id, *op1 += op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrPr(uint32_t opcode, uint32_t op2) { // LDR Rd,[Rn,op2]!
    // Word load, pre-adjust with writeback
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = *op1 += op2;
    *op0 = core->memory.read<uint32_t>(id, address);

    // Rotate misaligned reads
    if (address & 0x3) {
        uint8_t shift = (address & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strPr(uint32_t opcode, uint32_t op2) { // STR Rd,[Rn,op2]!
    // Word store, pre-adjust with writeback
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, *op1 += op2, op0);
    return 1;
}

FORCE_INLINE int ArmInterp::ldrdPr(uint32_t opcode, uint32_t op2) { // LDRD Rd,[Rn,op2]!
    // Double word load, pre-adjust with writeback
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    *registers[op0] = core->memory.read<uint32_t>(id, *op1 += op2);
    *registers[op0 + 1] = core->memory.read<uint32_t>(id, *op1 + 4);
    return 2;
}

FORCE_INLINE int ArmInterp::strdPr(uint32_t opcode, uint32_t op2) { // STRD Rd,[Rn,op2]!
    // Double word store, pre-adjust with writeback
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, *op1 += op2, *registers[op0]);
    core->memory.write<uint32_t>(id, *op1 + 4, *registers[op0 + 1]);
    return 2;
}

FORCE_INLINE int ArmInterp::ldrsbPt(uint32_t opcode, uint32_t op2) { // LDRSB Rd,[Rn],op2
    // Signed byte load, post-adjust
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *op0 = (int8_t)core->memory.read<uint8_t>(id, address);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrshPt(uint32_t opcode, uint32_t op2) { // LDRSH Rd,[Rn],op2
    // Signed half-word load, post-adjust
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *op0 = (int16_t)core->memory.read<uint16_t>(id, address);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::ldrbPt(uint32_t opcode, uint32_t op2) { // LDRB Rd,[Rn],op2
    // Byte load, post-adjust
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *op0 = core->memory.read<uint8_t>(id, address);

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strbPt(uint32_t opcode, uint32_t op2) { // STRB Rd,[Rn],op2
    // Byte store, post-adjust
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint8_t>(id, *op1, op0);
    *op1 += op2;
    return 1;
}

FORCE_INLINE int ArmInterp::ldrhPt(uint32_t opcode, uint32_t op2) { // LDRH Rd,[Rn],op2
    // Half-word load, post-adjust
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *op0 = core->memory.read<uint16_t>(id, address);

    // Handle pipelining
    if (op0 != registers[15]) return 1;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strhPt(uint32_t opcode, uint32_t op2) { // STRH Rd,[Rn],op2
    // Half-word store, post-adjust
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint16_t>(id, *op1, op0);
    *op1 += op2;
    return 1;
}

FORCE_INLINE int ArmInterp::ldrPt(uint32_t opcode, uint32_t op2) { // LDR Rd,[Rn],op2
    // Word load, post-adjust
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *op0 = core->memory.read<uint32_t>(id, address);

    // Rotate misaligned reads
    if (address & 0x3) {
        uint8_t shift = (address & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

FORCE_INLINE int ArmInterp::strPt(uint32_t opcode, uint32_t op2) { // STR Rd,[Rn],op2
    // Word store, post-adjust
    // When used as Rd, the program counter is read with +4
    uint32_t op0 = *registers[(opcode >> 12) & 0xF] + (((opcode & 0xF000) == 0xF000) << 2);
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, *op1, op0);
    *op1 += op2;
    return 1;
}

FORCE_INLINE int ArmInterp::ldrdPt(uint32_t opcode, uint32_t op2) { // LDRD Rd,[Rn],op2
    // Double word load, post-adjust
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    uint32_t address = (*op1 += op2) - op2;
    *registers[op0] = core->memory.read<uint32_t>(id, address);
    *registers[op0 + 1] = core->memory.read<uint32_t>(id, address + 4);
    return 2;
}

FORCE_INLINE int ArmInterp::strdPt(uint32_t opcode, uint32_t op2) { // STRD Rd,[Rn],op2
    // Double word store, post-adjust
    uint8_t op0 = (opcode >> 12) & 0xF;
    if (op0 == 15) return 1;
    uint32_t *op1 = registers[(opcode >> 16) & 0xF];
    core->memory.write<uint32_t>(id, *op1, *registers[op0]);
    core->memory.write<uint32_t>(id, *op1 + 4, *registers[op0 + 1]);
    *op1 += op2;
    return 2;
}

int ArmInterp::ldmda(uint32_t opcode) { // LDMDA Rn, <Rlist>
    // Block load, post-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, op0 += 4);
    }

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmda(uint32_t opcode) { // STMDA Rn, <Rlist>
    // Block store, post-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0 += 4, *registers[i]);
    }
    return m + (m < 2);
}

int ArmInterp::ldmia(uint32_t opcode) { // LDMIA Rn, <Rlist>
    // Block load, post-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, op0);
        op0 += 4;
    }

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmia(uint32_t opcode) { // STMIA Rn, <Rlist>
    // Block store, post-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0, *registers[i]);
        op0 += 4;
    }
    return m + (m < 2);
}

int ArmInterp::ldmdb(uint32_t opcode) { // LDMDB Rn, <Rlist>
    // Block load, pre-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, op0);
        op0 += 4;
    }

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdb(uint32_t opcode) { // STMDB Rn, <Rlist>
    // Block store, pre-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0, *registers[i]);
        op0 += 4;
    }
    return m + (m < 2);
}

int ArmInterp::ldmib(uint32_t opcode) { // LDMIB Rn, <Rlist>
    // Block load, pre-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, op0 += 4);
    }

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmib(uint32_t opcode) { // STMIB Rn, <Rlist>
    // Block store, pre-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0 += 4, *registers[i]);
    }
    return m + (m < 2);
}

int ArmInterp::ldmdaW(uint32_t opcode) { // LDMDA Rn!, <Rlist>
    // Block load, post-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] -= (m << 2));
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, address += 4);
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address - (m << 2);

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdaW(uint32_t opcode) { // STMDA Rn!, <Rlist>
    // Block store, post-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address += 4, *registers[i]);
    }
    *registers[op0] = address - (m << 2);
    return m + (m < 2);
}

int ArmInterp::ldmiaW(uint32_t opcode) { // LDMIA Rn!, <Rlist>
    // Block load, post-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] += (m << 2)) - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, address);
        address += 4;
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address;

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmiaW(uint32_t opcode) { // STMIA Rn!, <Rlist>
    // Block store, post-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, *registers[i]);
        address += 4;
    }
    *registers[op0] = address;
    return m + (m < 2);
}

int ArmInterp::ldmdbW(uint32_t opcode) { // LDMDB Rn!, <Rlist>
    // Block load, pre-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] -= (m << 2));
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, address);
        address += 4;
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address - (m << 2);

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdbW(uint32_t opcode) { // STMDB Rn!, <Rlist>
    // Block store, pre-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, *registers[i]);
        address += 4;
    }
    *registers[op0] = address - (m << 2);
    return m + (m < 2);
}

int ArmInterp::ldmibW(uint32_t opcode) { // LDMIB Rn!, <Rlist>
    // Block load, pre-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] += (m << 2)) - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, address += 4);
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address;

    // Handle pipelining and THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmibW(uint32_t opcode) { // STMIB Rn!, <Rlist>
    // Block store, pre-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address += 4, *registers[i]);
    }
    *registers[op0] = address;
    return m + (m < 2);
}

int ArmInterp::ldmdaU(uint32_t opcode) { // LDMDA Rn, <Rlist>^
    // User block load, post-decrement without writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, op0 += 4);
    }

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdaU(uint32_t opcode) { // STMDA Rn, <Rlist>^
    // User block store, post-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0 += 4, registersUsr[i]);
    }
    return m + (m < 2);
}

int ArmInterp::ldmiaU(uint32_t opcode) { // LDMIA Rn, <Rlist>^
    // User block load, post-increment without writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, op0);
        op0 += 4;
    }

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmiaU(uint32_t opcode) { // STMIA Rn, <Rlist>^
    // User block store, post-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0, registersUsr[i]);
        op0 += 4;
    }
    return m + (m < 2);
}

int ArmInterp::ldmdbU(uint32_t opcode) { // LDMDB Rn, <Rlist>^
    // User block load, pre-decrement without writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, op0);
        op0 += 4;
    }

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdbU(uint32_t opcode) { // STMDB Rn, <Rlist>^
    // User block store, pre-decrement without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0, registersUsr[i]);
        op0 += 4;
    }
    return m + (m < 2);
}

int ArmInterp::ldmibU(uint32_t opcode) { // LDMIB Rn, <Rlist>^
    // User block load, pre-increment without writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, op0 += 4);
    }

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmibU(uint32_t opcode) { // STMIB Rn, <Rlist>^
    // User block store, pre-increment without writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint32_t op0 = *registers[(opcode >> 16) & 0xF];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, op0 += 4, registersUsr[i]);
    }
    return m + (m < 2);
}

int ArmInterp::ldmdaUW(uint32_t opcode) { // LDMDA Rn!, <Rlist>^
    // User block load, post-decrement with writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] -= (m << 2));
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, address += 4);
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address - (m << 2);

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdaUW(uint32_t opcode) { // STMDA Rn!, <Rlist>^
    // User block store, post-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address += 4, registersUsr[i]);
    }
    *registers[op0] = address - (m << 2);
    return m + (m < 2);
}

int ArmInterp::ldmiaUW(uint32_t opcode) { // LDMIA Rn!, <Rlist>^
    // User block load, post-increment with writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] += (m << 2)) - (m << 2);
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, address);
        address += 4;
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address;

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmiaUW(uint32_t opcode) { // STMIA Rn!, <Rlist>^
    // User block store, post-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, registersUsr[i]);
        address += 4;
    }
    *registers[op0] = address;
    return m + (m < 2);
}

int ArmInterp::ldmdbUW(uint32_t opcode) { // LDMDB Rn!, <Rlist>^
    // User block load, pre-decrement with writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] -= (m << 2));
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, address);
        address += 4;
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address - (m << 2);

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmdbUW(uint32_t opcode) { // STMDB Rn!, <Rlist>^
    // User block store, pre-decrement with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0] - (m << 2);
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, registersUsr[i]);
        address += 4;
    }
    *registers[op0] = address - (m << 2);
    return m + (m < 2);
}

int ArmInterp::ldmibUW(uint32_t opcode) { // LDMIB Rn!, <Rlist>^
    // User block load, pre-increment with writeback; normal registers if branching
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = (*registers[op0] += (m << 2)) - (m << 2);
    uint32_t **regs = &registers[(~opcode & BIT(15)) >> 11];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        *regs[i] = core->memory.read<uint32_t>(id, address += 4);
    }

    // Load the writeback value if it's not last or is the only listed register
    if ((opcode & 0xFFFF & ~(BIT(op0 + 1) - 1)) || (opcode & 0xFFFF) == BIT(op0))
        *registers[op0] = address;

    // Handle pipelining and mode/THUMB switching
    if (~opcode & BIT(15)) return m + (m < 2);
    if (spsr) setCpsr(*spsr);
    cpsr |= (*registers[15] & 0x1) << 5;
    flushPipeline();
    return m + 4;
}

int ArmInterp::stmibUW(uint32_t opcode) { // STMIB Rn!, <Rlist>^
    // User block store, pre-increment with writeback
    uint8_t m = bitCount[opcode & 0xFF] + bitCount[(opcode >> 8) & 0xFF];
    uint8_t op0 = (opcode >> 16) & 0xF;
    uint32_t address = *registers[op0];
    for (int i = 0; i < 16; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address += 4, registersUsr[i]);
    }
    *registers[op0] = address;
    return m + (m < 2);
}

int ArmInterp::msrRc(uint32_t opcode) { // MSR CPSR,Rm
    // Write the first 8 bits of the status flags, only changing the CPU mode when not in user mode
    uint32_t op1 = *registers[opcode & 0xF];
    if (opcode & BIT(16)) {
        uint8_t mask = ((cpsr & 0x1F) == 0x10) ? 0xE0 : 0xFF;
        setCpsr((cpsr & ~mask) | (op1 & mask));
    }

    // Write the remaining 8-bit blocks of the status flags
    for (int i = 1; i < 4; i++) {
        if (opcode & BIT(16 + i))
            cpsr = (cpsr & ~(0xFF << (i << 3))) | (op1 & (0xFF << (i << 3)));
    }
    return 1;
}

int ArmInterp::msrRs(uint32_t opcode) { // MSR SPSR,Rm
    // Write the saved status flags in 8-bit blocks
    if (!spsr) return 1;
    uint32_t op1 = *registers[opcode & 0xF];
    for (int i = 0; i < 4; i++) {
        if (opcode & BIT(16 + i))
            *spsr = (*spsr & ~(0xFF << (i << 3))) | (op1 & (0xFF << (i << 3)));
    }
    return 1;
}

int ArmInterp::msrIc(uint32_t opcode) { // MSR CPSR,#i
    // Handle ARM11 hint opcodes
    if (id != ARM9 && !(opcode & 0xF0000)) {
        switch (opcode & 0xFF) {
            case 0x00: return 1; // NOP
            case 0x03: return halted = true; // WFI
            default: return unkArm(opcode);
        }
    }

    // Rotate the immediate value
    uint32_t value = opcode & 0xFF;
    uint8_t shift = (opcode >> 7) & 0x1E;
    uint32_t op1 = (value << (32 - shift)) | (value >> shift);

    // Write the first 8 bits of the status flags, only changing the CPU mode when not in user mode
    if (opcode & BIT(16)) {
        uint8_t mask = ((cpsr & 0x1F) == 0x10) ? 0xE0 : 0xFF;
        setCpsr((cpsr & ~mask) | (op1 & mask));
    }

    // Write the remaining 8-bit blocks of the status flags
    for (int i = 1; i < 4; i++) {
        if (opcode & BIT(16 + i))
            cpsr = (cpsr & ~(0xFF << (i << 3))) | (op1 & (0xFF << (i << 3)));
    }
    return 1;
}

int ArmInterp::msrIs(uint32_t opcode) { // MSR SPSR,#i
    // Rotate the immediate value
    if (!spsr) return 1;
    uint32_t value = opcode & 0xFF;
    uint8_t shift = (opcode >> 7) & 0x1E;
    uint32_t op1 = (value << (32 - shift)) | (value >> shift);

    // Write the saved status flags in 8-bit blocks
    for (int i = 0; i < 4; i++) {
        if (opcode & BIT(16 + i))
            *spsr = (*spsr & ~(0xFF << (i << 3))) | (op1 & (0xFF << (i << 3)));
    }
    return 1;
}

int ArmInterp::mrsRc(uint32_t opcode) { // MRS Rd,CPSR
    // Copy the status flags to a register
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    *op0 = cpsr;
    return 2;
}

int ArmInterp::mrsRs(uint32_t opcode) { // MRS Rd,SPSR
    // Copy the saved status flags to a register
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    if (spsr) *op0 = *spsr;
    return 2;
}

int ArmInterp::mrc(uint32_t opcode) { // MRC Pn,<cpopc>,Rd,Cn,Cm,<cp>
    // Read from a CP15 register
    uint32_t *op2 = registers[(opcode >> 12) & 0xF];
    uint8_t op3 = (opcode >> 16) & 0xF;
    uint8_t op4 = opcode & 0xF;
    uint8_t op5 = (opcode >> 5) & 0x7;
    *op2 = cp15.read(op3, op4, op5);
    return 1;
}

int ArmInterp::mcr(uint32_t opcode) { // MCR Pn,<cpopc>,Rd,Cn,Cm,<cp>
    // Write to a CP15 register
    uint32_t op2 = *registers[(opcode >> 12) & 0xF];
    uint8_t op3 = (opcode >> 16) & 0xF;
    uint8_t op4 = opcode & 0xF;
    uint8_t op5 = (opcode >> 5) & 0x7;
    cp15.write(op3, op4, op5, op2);
    return 1;
}

int ArmInterp::swpb(uint32_t opcode) { // SWPB Rd,Rm,[Rn]
    // Swap byte
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint8_t>(id, op2);
    core->memory.write<uint8_t>(id, op2, op1);
    return 2;
}

int ArmInterp::swp(uint32_t opcode) { // SWP Rd,Rm,[Rn]
    // Swap word
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[opcode & 0xF];
    uint32_t op2 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint32_t>(id, op2);
    core->memory.write<uint32_t>(id, op2, op1);

    // Rotate misaligned reads
    if (op2 & 0x3) {
        uint8_t shift = (op2 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }
    return 2;
}

int ArmInterp::ldrex(uint32_t opcode) { // LDREX Rd,[Rn]
    // Load word exclusively
    if (id == ARM9) return 1; // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 16) & 0xF];
    *op0 = core->memory.read<uint32_t>(id, excAddress = op1);
    exclusive = true;

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }

    // Handle pipelining and THUMB switching
    if (op0 != registers[15]) return 1;
    cpsr |= (*op0 & 0x1) << 5;
    flushPipeline();
    return 5;
}

int ArmInterp::strex(uint32_t opcode) { // STREX Rd,Rm,[Rn]
    // Store word exclusively
    // TODO: handle non-shared memory properly
    if (id == ARM9) return 1; // ARM11-exclusive
    uint32_t *op0 = registers[(opcode >> 12) & 0xF];
    uint32_t op1 = *registers[(opcode >> 0) & 0xF];
    uint32_t op2 = *registers[(opcode >> 16) & 0xF];
    if ((*op0 = !exclusive || excAddress != op2)) return 1;
    core->memory.write(id, op2, op1);

    // Update exclusive states on both cores
    if (core->arms[!id].exclusive && core->arms[!id].excAddress == op2)
        core->arms[!id].exclusive = false;
    exclusive = false;
    return 1;
}

int ArmInterp::clrex(uint32_t opcode) { // CLREX
    // Clear exclusive state
    exclusive = false;
    return 1;
}

int ArmInterp::ldrsbRegT(uint16_t opcode) { // LDRSB Rd,[Rb,Ro]
    // Signed byte load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = (int8_t)core->memory.read<uint8_t>(id, op1 + op2);
    return 1;
}

int ArmInterp::ldrshRegT(uint16_t opcode) { // LDRSH Rd,[Rb,Ro]
    // Signed half-word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = (int16_t)core->memory.read<uint16_t>(id, op1 += op2);
    return 1;
}

int ArmInterp::ldrbRegT(uint16_t opcode) { // LDRB Rd,[Rb,Ro]
    // Byte load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = core->memory.read<uint8_t>(id, op1 + op2);
    return 1;
}

int ArmInterp::strbRegT(uint16_t opcode) { // STRB Rd,[Rb,Ro]
    // Byte write, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    core->memory.write<uint8_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrhRegT(uint16_t opcode) { // LDRH Rd,[Rb,Ro]
    // Half-word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = core->memory.read<uint16_t>(id, op1 += op2);
    return 1;
}

int ArmInterp::strhRegT(uint16_t opcode) { // STRH Rd,[Rb,Ro]
    // Half-word write, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    core->memory.write<uint16_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrRegT(uint16_t opcode) { // LDR Rd,[Rb,Ro]
    // Word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    *op0 = core->memory.read<uint32_t>(id, op1 += op2);

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }
    return 1;
}

int ArmInterp::strRegT(uint16_t opcode) { // STR Rd,[Rb,Ro]
    // Word write, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = *registers[(opcode >> 6) & 0x7];
    core->memory.write<uint32_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrbImm5T(uint16_t opcode) { // LDRB Rd,[Rb,#i]
    // Byte load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode & 0x07C0) >> 6;
    *op0 = core->memory.read<uint8_t>(id, op1 + op2);
    return 1;
}

int ArmInterp::strbImm5T(uint16_t opcode) { // STRB Rd,[Rb,#i]
    // Byte store, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode & 0x07C0) >> 6;
    core->memory.write<uint8_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrhImm5T(uint16_t opcode) { // LDRH Rd,[Rb,#i]
    // Half-word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 5) & 0x3E;
    *op0 = core->memory.read<uint16_t>(id, op1 += op2);
    return 1;
}

int ArmInterp::strhImm5T(uint16_t opcode) { // STRH Rd,[Rb,#i]
    // Half-word store, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 5) & 0x3E;
    core->memory.write<uint16_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrImm5T(uint16_t opcode) { // LDR Rd,[Rb,#i]
    // Word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 4) & 0x7C;
    *op0 = core->memory.read<uint32_t>(id, op1 += op2);

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }
    return 1;
}

int ArmInterp::strImm5T(uint16_t opcode) { // STR Rd,[Rb,#i]
    // Word store, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[opcode & 0x7];
    uint32_t op1 = *registers[(opcode >> 3) & 0x7];
    uint32_t op2 = (opcode >> 4) & 0x7C;
    core->memory.write<uint32_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldrPcT(uint16_t opcode) { // LDR Rd,[PC,#i]
    // PC-relative word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[15] & ~0x3;
    uint32_t op2 = (opcode & 0xFF) << 2;
    *op0 = core->memory.read<uint32_t>(id, op1 += op2);

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }
    return 1;
}

int ArmInterp::ldrSpT(uint16_t opcode) { // LDR Rd,[SP,#i]
    // SP-relative word load, pre-adjust without writeback (THUMB)
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[13];
    uint32_t op2 = (opcode & 0xFF) << 2;
    *op0 = core->memory.read<uint32_t>(id, op1 += op2);

    // Rotate misaligned reads
    if (op1 & 0x3) {
        uint8_t shift = (op1 & 0x3) << 3;
        *op0 = (*op0 << (32 - shift)) | (*op0 >> shift);
    }
    return 1;
}

int ArmInterp::strSpT(uint16_t opcode) { // STR Rd,[SP,#i]
    // SP-relative word store, pre-adjust without writeback (THUMB)
    uint32_t op0 = *registers[(opcode >> 8) & 0x7];
    uint32_t op1 = *registers[13];
    uint32_t op2 = (opcode & 0xFF) << 2;
    core->memory.write<uint32_t>(id, op1 + op2, op0);
    return 1;
}

int ArmInterp::ldmiaT(uint16_t opcode) { // LDMIA Rb!,<Rlist>
    // Block load, post-increment with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF];
    uint32_t *op0 = registers[(opcode >> 8) & 0x7];
    uint32_t address = (*op0 += (m << 2)) - (m << 2);
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, address);
        address += 4;
    }
    return m + (m < 2);
}

int ArmInterp::stmiaT(uint16_t opcode) { // STMIA Rb!,<Rlist>
    // Block store, post-increment with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF];
    uint8_t op0 = (opcode >> 8) & 0x7;
    uint32_t address = *registers[op0];
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, *registers[i]);
        address += 4;
    }
    *registers[op0] = address;
    return m + (m < 2);
}

int ArmInterp::popT(uint16_t opcode) { // POP <Rlist>
    // SP-relative block load, post-increment with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF];
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, *registers[13]);
        *registers[13] += 4;
    }
    return m + (m < 2);
}

int ArmInterp::pushT(uint16_t opcode) { // PUSH <Rlist>
    // SP-relative block store, pre-decrement with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF];
    uint32_t address = (*registers[13] -= (m << 2));
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, *registers[i]);
        address += 4;
    }
    return m + (m < 2);
}

int ArmInterp::popPcT(uint16_t opcode) { // POP <Rlist>,PC
    // SP-relative block load, post-increment with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF] + 1;
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        *registers[i] = core->memory.read<uint32_t>(id, *registers[13]);
        *registers[13] += 4;
    }

    // Load the program counter and handle pipelining
    *registers[15] = core->memory.read<uint32_t>(id, *registers[13]);
    *registers[13] += 4;
    cpsr &= ~((~(*registers[15]) & 0x1) << 5);
    flushPipeline();
    return m + 4;
}

int ArmInterp::pushLrT(uint16_t opcode) { // PUSH <Rlist>,LR
    // SP-relative block store, pre-decrement with writeback (THUMB)
    uint8_t m = bitCount[opcode & 0xFF] + 1;
    uint32_t address = (*registers[13] -= (m << 2));
    for (int i = 0; i < 8; i++) {
        if (~opcode & BIT(i)) continue;
        core->memory.write<uint32_t>(id, address, *registers[i]);
        address += 4;
    }

    // Store the link register
    core->memory.write<uint32_t>(id, address, *registers[14]);
    return m + (m < 2);
}
