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

template void Memory::ioWrite(CpuId, uint32_t, uint8_t);
template void Memory::ioWrite(CpuId, uint32_t, uint16_t);
template void Memory::ioWrite(CpuId, uint32_t, uint32_t);

template <typename T> void Memory::ioWrite(CpuId id, uint32_t address, T value) {
    // Mirror the ARM11 DSP register area
    if (id != ARM9 && (address >> 12) == 0x10203)
        address &= 0xFFFFF03F;

    // Write a value to one or more I/O registers
    for (uint32_t i = 0; i < sizeof(T);) {
        uint32_t base = address + i, size;
        uint32_t mask = (1ULL << ((sizeof(T) - i) << 3)) - 1;
        uint32_t data = value >> (i << 3);

        // Check registers that are shared between CPUs
        switch (base) {
            DEF_IO32(0x10101000, core->shas[0].writeCnt(IO_PARAMS)) // SHA0_CNT
            DEF_IO32(0x10101004, core->shas[0].writeBlkcnt(IO_PARAMS)) // SHA0_BLKCNT
            DEF_IO32(0x10101040, core->shas[0].writeHash(0, IO_PARAMS)) // SHA0_HASH0
            DEF_IO32(0x10101044, core->shas[0].writeHash(1, IO_PARAMS)) // SHA0_HASH1
            DEF_IO32(0x10101048, core->shas[0].writeHash(2, IO_PARAMS)) // SHA0_HASH2
            DEF_IO32(0x1010104C, core->shas[0].writeHash(3, IO_PARAMS)) // SHA0_HASH3
            DEF_IO32(0x10101050, core->shas[0].writeHash(4, IO_PARAMS)) // SHA0_HASH4
            DEF_IO32(0x10101054, core->shas[0].writeHash(5, IO_PARAMS)) // SHA0_HASH5
            DEF_IO32(0x10101058, core->shas[0].writeHash(6, IO_PARAMS)) // SHA0_HASH6
            DEF_IO32(0x1010105C, core->shas[0].writeHash(7, IO_PARAMS)) // SHA0_HASH7
            DEF_IO32(0x10102000, core->y2rs[0].writeCnt(IO_PARAMS)) // Y2R0_CNT
            DEF_IO16(0x10102004, core->y2rs[0].writeWidth(IO_PARAMS)) // Y2R0_WIDTH
            DEF_IO16(0x10102006, core->y2rs[0].writeHeight(IO_PARAMS)) // Y2R0_HEIGHT
            DEF_IO16(0x10102010, core->y2rs[0].writeMultiplyY(IO_PARAMS)) // Y2R0_MULTIPLY_Y
            DEF_IO16(0x10102012, core->y2rs[0].writeMultiplyVr(IO_PARAMS)) // Y2R0_MULTIPLY_VR
            DEF_IO16(0x10102014, core->y2rs[0].writeMultiplyVg(IO_PARAMS)) // Y2R0_MULTIPLY_VG
            DEF_IO16(0x10102016, core->y2rs[0].writeMultiplyUg(IO_PARAMS)) // Y2R0_MULTIPLY_UG
            DEF_IO16(0x10102018, core->y2rs[0].writeMultiplyUb(IO_PARAMS)) // Y2R0_MULTIPLY_UB
            DEF_IO16(0x1010201A, core->y2rs[0].writeOffsetR(IO_PARAMS)) // Y2R0_OFFSET_R
            DEF_IO16(0x1010201C, core->y2rs[0].writeOffsetG(IO_PARAMS)) // Y2R0_OFFSET_G
            DEF_IO16(0x1010201E, core->y2rs[0].writeOffsetB(IO_PARAMS)) // Y2R0_OFFSET_B
            DEF_IO16(0x10102020, core->y2rs[0].writeAlpha(IO_PARAMS)) // Y2R0_ALPHA
            DEF_IO16(0x10103000, core->csnd.writeMainVol(IO_PARAMS)) // CSND_MAIN_VOL
            DEF_IO16(0x10103002, core->csnd.writeMainCnt(IO_PARAMS)) // CSND_MAIN_CNT
            DEF_IO16(0x10103400, core->csnd.writeChanCnt(0, IO_PARAMS)) // CSND_CHAN0_CNT
            DEF_IO16(0x10103402, core->csnd.writeChanRate(0, IO_PARAMS)) // CSND_CHAN0_RATE
            DEF_IO16(0x10103404, core->csnd.writeChanRvol(0, IO_PARAMS)) // CSND_CHAN0_RVOL
            DEF_IO16(0x10103406, core->csnd.writeChanLvol(0, IO_PARAMS)) // CSND_CHAN0_LVOL
            DEF_IO32(0x1010340C, core->csnd.writeChanStart(0, IO_PARAMS)) // CSND_CHAN0_START
            DEF_IO32(0x10103410, core->csnd.writeChanSize(0, IO_PARAMS)) // CSND_CHAN0_SIZE
            DEF_IO32(0x10103414, core->csnd.writeChanLoop(0, IO_PARAMS)) // CSND_CHAN0_LOOP
            DEF_IO32(0x10103418, core->csnd.writeAdpcmStart(0, IO_PARAMS)) // CSND_ADPCM0_START
            DEF_IO32(0x1010341C, core->csnd.writeAdpcmLoop(0, IO_PARAMS)) // CSND_ADPCM0_LOOP
            DEF_IO16(0x10103420, core->csnd.writeChanCnt(1, IO_PARAMS)) // CSND_CHAN1_CNT
            DEF_IO16(0x10103422, core->csnd.writeChanRate(1, IO_PARAMS)) // CSND_CHAN1_RATE
            DEF_IO16(0x10103424, core->csnd.writeChanRvol(1, IO_PARAMS)) // CSND_CHAN1_RVOL
            DEF_IO16(0x10103426, core->csnd.writeChanLvol(1, IO_PARAMS)) // CSND_CHAN1_LVOL
            DEF_IO32(0x1010342C, core->csnd.writeChanStart(1, IO_PARAMS)) // CSND_CHAN1_START
            DEF_IO32(0x10103430, core->csnd.writeChanSize(1, IO_PARAMS)) // CSND_CHAN1_SIZE
            DEF_IO32(0x10103434, core->csnd.writeChanLoop(1, IO_PARAMS)) // CSND_CHAN1_LOOP
            DEF_IO32(0x10103438, core->csnd.writeAdpcmStart(1, IO_PARAMS)) // CSND_ADPCM1_START
            DEF_IO32(0x1010343C, core->csnd.writeAdpcmLoop(1, IO_PARAMS)) // CSND_ADPCM1_LOOP
            DEF_IO16(0x10103440, core->csnd.writeChanCnt(2, IO_PARAMS)) // CSND_CHAN2_CNT
            DEF_IO16(0x10103442, core->csnd.writeChanRate(2, IO_PARAMS)) // CSND_CHAN2_RATE
            DEF_IO16(0x10103444, core->csnd.writeChanRvol(2, IO_PARAMS)) // CSND_CHAN2_RVOL
            DEF_IO16(0x10103446, core->csnd.writeChanLvol(2, IO_PARAMS)) // CSND_CHAN2_LVOL
            DEF_IO32(0x1010344C, core->csnd.writeChanStart(2, IO_PARAMS)) // CSND_CHAN2_START
            DEF_IO32(0x10103450, core->csnd.writeChanSize(2, IO_PARAMS)) // CSND_CHAN2_SIZE
            DEF_IO32(0x10103454, core->csnd.writeChanLoop(2, IO_PARAMS)) // CSND_CHAN2_LOOP
            DEF_IO32(0x10103458, core->csnd.writeAdpcmStart(2, IO_PARAMS)) // CSND_ADPCM2_START
            DEF_IO32(0x1010345C, core->csnd.writeAdpcmLoop(2, IO_PARAMS)) // CSND_ADPCM2_LOOP
            DEF_IO16(0x10103460, core->csnd.writeChanCnt(3, IO_PARAMS)) // CSND_CHAN3_CNT
            DEF_IO16(0x10103462, core->csnd.writeChanRate(3, IO_PARAMS)) // CSND_CHAN3_RATE
            DEF_IO16(0x10103464, core->csnd.writeChanRvol(3, IO_PARAMS)) // CSND_CHAN3_RVOL
            DEF_IO16(0x10103466, core->csnd.writeChanLvol(3, IO_PARAMS)) // CSND_CHAN3_LVOL
            DEF_IO32(0x1010346C, core->csnd.writeChanStart(3, IO_PARAMS)) // CSND_CHAN3_START
            DEF_IO32(0x10103470, core->csnd.writeChanSize(3, IO_PARAMS)) // CSND_CHAN3_SIZE
            DEF_IO32(0x10103474, core->csnd.writeChanLoop(3, IO_PARAMS)) // CSND_CHAN3_LOOP
            DEF_IO32(0x10103478, core->csnd.writeAdpcmStart(3, IO_PARAMS)) // CSND_ADPCM3_START
            DEF_IO32(0x1010347C, core->csnd.writeAdpcmLoop(3, IO_PARAMS)) // CSND_ADPCM3_LOOP
            DEF_IO16(0x10103480, core->csnd.writeChanCnt(4, IO_PARAMS)) // CSND_CHAN4_CNT
            DEF_IO16(0x10103482, core->csnd.writeChanRate(4, IO_PARAMS)) // CSND_CHAN4_RATE
            DEF_IO16(0x10103484, core->csnd.writeChanRvol(4, IO_PARAMS)) // CSND_CHAN4_RVOL
            DEF_IO16(0x10103486, core->csnd.writeChanLvol(4, IO_PARAMS)) // CSND_CHAN4_LVOL
            DEF_IO32(0x1010348C, core->csnd.writeChanStart(4, IO_PARAMS)) // CSND_CHAN4_START
            DEF_IO32(0x10103490, core->csnd.writeChanSize(4, IO_PARAMS)) // CSND_CHAN4_SIZE
            DEF_IO32(0x10103494, core->csnd.writeChanLoop(4, IO_PARAMS)) // CSND_CHAN4_LOOP
            DEF_IO32(0x10103498, core->csnd.writeAdpcmStart(4, IO_PARAMS)) // CSND_ADPCM4_START
            DEF_IO32(0x1010349C, core->csnd.writeAdpcmLoop(4, IO_PARAMS)) // CSND_ADPCM4_LOOP
            DEF_IO16(0x101034A0, core->csnd.writeChanCnt(5, IO_PARAMS)) // CSND_CHAN5_CNT
            DEF_IO16(0x101034A2, core->csnd.writeChanRate(5, IO_PARAMS)) // CSND_CHAN5_RATE
            DEF_IO16(0x101034A4, core->csnd.writeChanRvol(5, IO_PARAMS)) // CSND_CHAN5_RVOL
            DEF_IO16(0x101034A6, core->csnd.writeChanLvol(5, IO_PARAMS)) // CSND_CHAN5_LVOL
            DEF_IO32(0x101034AC, core->csnd.writeChanStart(5, IO_PARAMS)) // CSND_CHAN5_START
            DEF_IO32(0x101034B0, core->csnd.writeChanSize(5, IO_PARAMS)) // CSND_CHAN5_SIZE
            DEF_IO32(0x101034B4, core->csnd.writeChanLoop(5, IO_PARAMS)) // CSND_CHAN5_LOOP
            DEF_IO32(0x101034B8, core->csnd.writeAdpcmStart(5, IO_PARAMS)) // CSND_ADPCM5_START
            DEF_IO32(0x101034BC, core->csnd.writeAdpcmLoop(5, IO_PARAMS)) // CSND_ADPCM5_LOOP
            DEF_IO16(0x101034C0, core->csnd.writeChanCnt(6, IO_PARAMS)) // CSND_CHAN6_CNT
            DEF_IO16(0x101034C2, core->csnd.writeChanRate(6, IO_PARAMS)) // CSND_CHAN6_RATE
            DEF_IO16(0x101034C4, core->csnd.writeChanRvol(6, IO_PARAMS)) // CSND_CHAN6_RVOL
            DEF_IO16(0x101034C6, core->csnd.writeChanLvol(6, IO_PARAMS)) // CSND_CHAN6_LVOL
            DEF_IO32(0x101034CC, core->csnd.writeChanStart(6, IO_PARAMS)) // CSND_CHAN6_START
            DEF_IO32(0x101034D0, core->csnd.writeChanSize(6, IO_PARAMS)) // CSND_CHAN6_SIZE
            DEF_IO32(0x101034D4, core->csnd.writeChanLoop(6, IO_PARAMS)) // CSND_CHAN6_LOOP
            DEF_IO32(0x101034D8, core->csnd.writeAdpcmStart(6, IO_PARAMS)) // CSND_ADPCM6_START
            DEF_IO32(0x101034DC, core->csnd.writeAdpcmLoop(6, IO_PARAMS)) // CSND_ADPCM6_LOOP
            DEF_IO16(0x101034E0, core->csnd.writeChanCnt(7, IO_PARAMS)) // CSND_CHAN7_CNT
            DEF_IO16(0x101034E2, core->csnd.writeChanRate(7, IO_PARAMS)) // CSND_CHAN7_RATE
            DEF_IO16(0x101034E4, core->csnd.writeChanRvol(7, IO_PARAMS)) // CSND_CHAN7_RVOL
            DEF_IO16(0x101034E6, core->csnd.writeChanLvol(7, IO_PARAMS)) // CSND_CHAN7_LVOL
            DEF_IO32(0x101034EC, core->csnd.writeChanStart(7, IO_PARAMS)) // CSND_CHAN7_START
            DEF_IO32(0x101034F0, core->csnd.writeChanSize(7, IO_PARAMS)) // CSND_CHAN7_SIZE
            DEF_IO32(0x101034F4, core->csnd.writeChanLoop(7, IO_PARAMS)) // CSND_CHAN7_LOOP
            DEF_IO32(0x101034F8, core->csnd.writeAdpcmStart(7, IO_PARAMS)) // CSND_ADPCM7_START
            DEF_IO32(0x101034FC, core->csnd.writeAdpcmLoop(7, IO_PARAMS)) // CSND_ADPCM7_LOOP
            DEF_IO16(0x10103500, core->csnd.writeChanCnt(8, IO_PARAMS)) // CSND_CHAN8_CNT
            DEF_IO16(0x10103502, core->csnd.writeChanRate(8, IO_PARAMS)) // CSND_CHAN8_RATE
            DEF_IO16(0x10103504, core->csnd.writeChanRvol(8, IO_PARAMS)) // CSND_CHAN8_RVOL
            DEF_IO16(0x10103506, core->csnd.writeChanLvol(8, IO_PARAMS)) // CSND_CHAN8_LVOL
            DEF_IO32(0x1010350C, core->csnd.writeChanStart(8, IO_PARAMS)) // CSND_CHAN8_START
            DEF_IO32(0x10103510, core->csnd.writeChanSize(8, IO_PARAMS)) // CSND_CHAN8_SIZE
            DEF_IO32(0x10103514, core->csnd.writeChanLoop(8, IO_PARAMS)) // CSND_CHAN8_LOOP
            DEF_IO32(0x10103518, core->csnd.writeAdpcmStart(8, IO_PARAMS)) // CSND_ADPCM8_START
            DEF_IO32(0x1010351C, core->csnd.writeAdpcmLoop(8, IO_PARAMS)) // CSND_ADPCM8_LOOP
            DEF_IO16(0x10103520, core->csnd.writeChanCnt(9, IO_PARAMS)) // CSND_CHAN9_CNT
            DEF_IO16(0x10103522, core->csnd.writeChanRate(9, IO_PARAMS)) // CSND_CHAN9_RATE
            DEF_IO16(0x10103524, core->csnd.writeChanRvol(9, IO_PARAMS)) // CSND_CHAN9_RVOL
            DEF_IO16(0x10103526, core->csnd.writeChanLvol(9, IO_PARAMS)) // CSND_CHAN9_LVOL
            DEF_IO32(0x1010352C, core->csnd.writeChanStart(9, IO_PARAMS)) // CSND_CHAN9_START
            DEF_IO32(0x10103530, core->csnd.writeChanSize(9, IO_PARAMS)) // CSND_CHAN9_SIZE
            DEF_IO32(0x10103534, core->csnd.writeChanLoop(9, IO_PARAMS)) // CSND_CHAN9_LOOP
            DEF_IO32(0x10103538, core->csnd.writeAdpcmStart(9, IO_PARAMS)) // CSND_ADPCM9_START
            DEF_IO32(0x1010353C, core->csnd.writeAdpcmLoop(9, IO_PARAMS)) // CSND_ADPCM9_LOOP
            DEF_IO16(0x10103540, core->csnd.writeChanCnt(10, IO_PARAMS)) // CSND_CHAN10_CNT
            DEF_IO16(0x10103542, core->csnd.writeChanRate(10, IO_PARAMS)) // CSND_CHAN10_RATE
            DEF_IO16(0x10103544, core->csnd.writeChanRvol(10, IO_PARAMS)) // CSND_CHAN10_RVOL
            DEF_IO16(0x10103546, core->csnd.writeChanLvol(10, IO_PARAMS)) // CSND_CHAN10_LVOL
            DEF_IO32(0x1010354C, core->csnd.writeChanStart(10, IO_PARAMS)) // CSND_CHAN10_START
            DEF_IO32(0x10103550, core->csnd.writeChanSize(10, IO_PARAMS)) // CSND_CHAN10_SIZE
            DEF_IO32(0x10103554, core->csnd.writeChanLoop(10, IO_PARAMS)) // CSND_CHAN10_LOOP
            DEF_IO32(0x10103558, core->csnd.writeAdpcmStart(10, IO_PARAMS)) // CSND_ADPCM10_START
            DEF_IO32(0x1010355C, core->csnd.writeAdpcmLoop(10, IO_PARAMS)) // CSND_ADPCM10_LOOP
            DEF_IO16(0x10103560, core->csnd.writeChanCnt(11, IO_PARAMS)) // CSND_CHAN11_CNT
            DEF_IO16(0x10103562, core->csnd.writeChanRate(11, IO_PARAMS)) // CSND_CHAN11_RATE
            DEF_IO16(0x10103564, core->csnd.writeChanRvol(11, IO_PARAMS)) // CSND_CHAN11_RVOL
            DEF_IO16(0x10103566, core->csnd.writeChanLvol(11, IO_PARAMS)) // CSND_CHAN11_LVOL
            DEF_IO32(0x1010356C, core->csnd.writeChanStart(11, IO_PARAMS)) // CSND_CHAN11_START
            DEF_IO32(0x10103570, core->csnd.writeChanSize(11, IO_PARAMS)) // CSND_CHAN11_SIZE
            DEF_IO32(0x10103574, core->csnd.writeChanLoop(11, IO_PARAMS)) // CSND_CHAN11_LOOP
            DEF_IO32(0x10103578, core->csnd.writeAdpcmStart(11, IO_PARAMS)) // CSND_ADPCM11_START
            DEF_IO32(0x1010357C, core->csnd.writeAdpcmLoop(11, IO_PARAMS)) // CSND_ADPCM11_LOOP
            DEF_IO16(0x10103580, core->csnd.writeChanCnt(12, IO_PARAMS)) // CSND_CHAN12_CNT
            DEF_IO16(0x10103582, core->csnd.writeChanRate(12, IO_PARAMS)) // CSND_CHAN12_RATE
            DEF_IO16(0x10103584, core->csnd.writeChanRvol(12, IO_PARAMS)) // CSND_CHAN12_RVOL
            DEF_IO16(0x10103586, core->csnd.writeChanLvol(12, IO_PARAMS)) // CSND_CHAN12_LVOL
            DEF_IO32(0x1010358C, core->csnd.writeChanStart(12, IO_PARAMS)) // CSND_CHAN12_START
            DEF_IO32(0x10103590, core->csnd.writeChanSize(12, IO_PARAMS)) // CSND_CHAN12_SIZE
            DEF_IO32(0x10103594, core->csnd.writeChanLoop(12, IO_PARAMS)) // CSND_CHAN12_LOOP
            DEF_IO32(0x10103598, core->csnd.writeAdpcmStart(12, IO_PARAMS)) // CSND_ADPCM12_START
            DEF_IO32(0x1010359C, core->csnd.writeAdpcmLoop(12, IO_PARAMS)) // CSND_ADPCM12_LOOP
            DEF_IO16(0x101035A0, core->csnd.writeChanCnt(13, IO_PARAMS)) // CSND_CHAN13_CNT
            DEF_IO16(0x101035A2, core->csnd.writeChanRate(13, IO_PARAMS)) // CSND_CHAN13_RATE
            DEF_IO16(0x101035A4, core->csnd.writeChanRvol(13, IO_PARAMS)) // CSND_CHAN13_RVOL
            DEF_IO16(0x101035A6, core->csnd.writeChanLvol(13, IO_PARAMS)) // CSND_CHAN13_LVOL
            DEF_IO32(0x101035AC, core->csnd.writeChanStart(13, IO_PARAMS)) // CSND_CHAN13_START
            DEF_IO32(0x101035B0, core->csnd.writeChanSize(13, IO_PARAMS)) // CSND_CHAN13_SIZE
            DEF_IO32(0x101035B4, core->csnd.writeChanLoop(13, IO_PARAMS)) // CSND_CHAN13_LOOP
            DEF_IO32(0x101035B8, core->csnd.writeAdpcmStart(13, IO_PARAMS)) // CSND_ADPCM13_START
            DEF_IO32(0x101035BC, core->csnd.writeAdpcmLoop(13, IO_PARAMS)) // CSND_ADPCM13_LOOP
            DEF_IO16(0x101035C0, core->csnd.writeChanCnt(14, IO_PARAMS)) // CSND_CHAN14_CNT
            DEF_IO16(0x101035C2, core->csnd.writeChanRate(14, IO_PARAMS)) // CSND_CHAN14_RATE
            DEF_IO16(0x101035C4, core->csnd.writeChanRvol(14, IO_PARAMS)) // CSND_CHAN14_RVOL
            DEF_IO16(0x101035C6, core->csnd.writeChanLvol(14, IO_PARAMS)) // CSND_CHAN14_LVOL
            DEF_IO32(0x101035CC, core->csnd.writeChanStart(14, IO_PARAMS)) // CSND_CHAN14_START
            DEF_IO32(0x101035D0, core->csnd.writeChanSize(14, IO_PARAMS)) // CSND_CHAN14_SIZE
            DEF_IO32(0x101035D4, core->csnd.writeChanLoop(14, IO_PARAMS)) // CSND_CHAN14_LOOP
            DEF_IO32(0x101035D8, core->csnd.writeAdpcmStart(14, IO_PARAMS)) // CSND_ADPCM14_START
            DEF_IO32(0x101035DC, core->csnd.writeAdpcmLoop(14, IO_PARAMS)) // CSND_ADPCM14_LOOP
            DEF_IO16(0x101035E0, core->csnd.writeChanCnt(15, IO_PARAMS)) // CSND_CHAN15_CNT
            DEF_IO16(0x101035E2, core->csnd.writeChanRate(15, IO_PARAMS)) // CSND_CHAN15_RATE
            DEF_IO16(0x101035E4, core->csnd.writeChanRvol(15, IO_PARAMS)) // CSND_CHAN15_RVOL
            DEF_IO16(0x101035E6, core->csnd.writeChanLvol(15, IO_PARAMS)) // CSND_CHAN15_LVOL
            DEF_IO32(0x101035EC, core->csnd.writeChanStart(15, IO_PARAMS)) // CSND_CHAN15_START
            DEF_IO32(0x101035F0, core->csnd.writeChanSize(15, IO_PARAMS)) // CSND_CHAN15_SIZE
            DEF_IO32(0x101035F4, core->csnd.writeChanLoop(15, IO_PARAMS)) // CSND_CHAN15_LOOP
            DEF_IO32(0x101035F8, core->csnd.writeAdpcmStart(15, IO_PARAMS)) // CSND_ADPCM15_START
            DEF_IO32(0x101035FC, core->csnd.writeAdpcmLoop(15, IO_PARAMS)) // CSND_ADPCM15_LOOP
            DEF_IO16(0x10103600, core->csnd.writeChanCnt(16, IO_PARAMS)) // CSND_CHAN16_CNT
            DEF_IO16(0x10103602, core->csnd.writeChanRate(16, IO_PARAMS)) // CSND_CHAN16_RATE
            DEF_IO16(0x10103604, core->csnd.writeChanRvol(16, IO_PARAMS)) // CSND_CHAN16_RVOL
            DEF_IO16(0x10103606, core->csnd.writeChanLvol(16, IO_PARAMS)) // CSND_CHAN16_LVOL
            DEF_IO32(0x1010360C, core->csnd.writeChanStart(16, IO_PARAMS)) // CSND_CHAN16_START
            DEF_IO32(0x10103610, core->csnd.writeChanSize(16, IO_PARAMS)) // CSND_CHAN16_SIZE
            DEF_IO32(0x10103614, core->csnd.writeChanLoop(16, IO_PARAMS)) // CSND_CHAN16_LOOP
            DEF_IO32(0x10103618, core->csnd.writeAdpcmStart(16, IO_PARAMS)) // CSND_ADPCM16_START
            DEF_IO32(0x1010361C, core->csnd.writeAdpcmLoop(16, IO_PARAMS)) // CSND_ADPCM16_LOOP
            DEF_IO16(0x10103620, core->csnd.writeChanCnt(17, IO_PARAMS)) // CSND_CHAN17_CNT
            DEF_IO16(0x10103622, core->csnd.writeChanRate(17, IO_PARAMS)) // CSND_CHAN17_RATE
            DEF_IO16(0x10103624, core->csnd.writeChanRvol(17, IO_PARAMS)) // CSND_CHAN17_RVOL
            DEF_IO16(0x10103626, core->csnd.writeChanLvol(17, IO_PARAMS)) // CSND_CHAN17_LVOL
            DEF_IO32(0x1010362C, core->csnd.writeChanStart(17, IO_PARAMS)) // CSND_CHAN17_START
            DEF_IO32(0x10103630, core->csnd.writeChanSize(17, IO_PARAMS)) // CSND_CHAN17_SIZE
            DEF_IO32(0x10103634, core->csnd.writeChanLoop(17, IO_PARAMS)) // CSND_CHAN17_LOOP
            DEF_IO32(0x10103638, core->csnd.writeAdpcmStart(17, IO_PARAMS)) // CSND_ADPCM17_START
            DEF_IO32(0x1010363C, core->csnd.writeAdpcmLoop(17, IO_PARAMS)) // CSND_ADPCM17_LOOP
            DEF_IO16(0x10103640, core->csnd.writeChanCnt(18, IO_PARAMS)) // CSND_CHAN18_CNT
            DEF_IO16(0x10103642, core->csnd.writeChanRate(18, IO_PARAMS)) // CSND_CHAN18_RATE
            DEF_IO16(0x10103644, core->csnd.writeChanRvol(18, IO_PARAMS)) // CSND_CHAN18_RVOL
            DEF_IO16(0x10103646, core->csnd.writeChanLvol(18, IO_PARAMS)) // CSND_CHAN18_LVOL
            DEF_IO32(0x1010364C, core->csnd.writeChanStart(18, IO_PARAMS)) // CSND_CHAN18_START
            DEF_IO32(0x10103650, core->csnd.writeChanSize(18, IO_PARAMS)) // CSND_CHAN18_SIZE
            DEF_IO32(0x10103654, core->csnd.writeChanLoop(18, IO_PARAMS)) // CSND_CHAN18_LOOP
            DEF_IO32(0x10103658, core->csnd.writeAdpcmStart(18, IO_PARAMS)) // CSND_ADPCM18_START
            DEF_IO32(0x1010365C, core->csnd.writeAdpcmLoop(18, IO_PARAMS)) // CSND_ADPCM18_LOOP
            DEF_IO16(0x10103660, core->csnd.writeChanCnt(19, IO_PARAMS)) // CSND_CHAN19_CNT
            DEF_IO16(0x10103662, core->csnd.writeChanRate(19, IO_PARAMS)) // CSND_CHAN19_RATE
            DEF_IO16(0x10103664, core->csnd.writeChanRvol(19, IO_PARAMS)) // CSND_CHAN19_RVOL
            DEF_IO16(0x10103666, core->csnd.writeChanLvol(19, IO_PARAMS)) // CSND_CHAN19_LVOL
            DEF_IO32(0x1010366C, core->csnd.writeChanStart(19, IO_PARAMS)) // CSND_CHAN19_START
            DEF_IO32(0x10103670, core->csnd.writeChanSize(19, IO_PARAMS)) // CSND_CHAN19_SIZE
            DEF_IO32(0x10103674, core->csnd.writeChanLoop(19, IO_PARAMS)) // CSND_CHAN19_LOOP
            DEF_IO32(0x10103678, core->csnd.writeAdpcmStart(19, IO_PARAMS)) // CSND_ADPCM19_START
            DEF_IO32(0x1010367C, core->csnd.writeAdpcmLoop(19, IO_PARAMS)) // CSND_ADPCM19_LOOP
            DEF_IO16(0x10103680, core->csnd.writeChanCnt(20, IO_PARAMS)) // CSND_CHAN20_CNT
            DEF_IO16(0x10103682, core->csnd.writeChanRate(20, IO_PARAMS)) // CSND_CHAN20_RATE
            DEF_IO16(0x10103684, core->csnd.writeChanRvol(20, IO_PARAMS)) // CSND_CHAN20_RVOL
            DEF_IO16(0x10103686, core->csnd.writeChanLvol(20, IO_PARAMS)) // CSND_CHAN20_LVOL
            DEF_IO32(0x1010368C, core->csnd.writeChanStart(20, IO_PARAMS)) // CSND_CHAN20_START
            DEF_IO32(0x10103690, core->csnd.writeChanSize(20, IO_PARAMS)) // CSND_CHAN20_SIZE
            DEF_IO32(0x10103694, core->csnd.writeChanLoop(20, IO_PARAMS)) // CSND_CHAN20_LOOP
            DEF_IO32(0x10103698, core->csnd.writeAdpcmStart(20, IO_PARAMS)) // CSND_ADPCM20_START
            DEF_IO32(0x1010369C, core->csnd.writeAdpcmLoop(20, IO_PARAMS)) // CSND_ADPCM20_LOOP
            DEF_IO16(0x101036A0, core->csnd.writeChanCnt(21, IO_PARAMS)) // CSND_CHAN21_CNT
            DEF_IO16(0x101036A2, core->csnd.writeChanRate(21, IO_PARAMS)) // CSND_CHAN21_RATE
            DEF_IO16(0x101036A4, core->csnd.writeChanRvol(21, IO_PARAMS)) // CSND_CHAN21_RVOL
            DEF_IO16(0x101036A6, core->csnd.writeChanLvol(21, IO_PARAMS)) // CSND_CHAN21_LVOL
            DEF_IO32(0x101036AC, core->csnd.writeChanStart(21, IO_PARAMS)) // CSND_CHAN21_START
            DEF_IO32(0x101036B0, core->csnd.writeChanSize(21, IO_PARAMS)) // CSND_CHAN21_SIZE
            DEF_IO32(0x101036B4, core->csnd.writeChanLoop(21, IO_PARAMS)) // CSND_CHAN21_LOOP
            DEF_IO32(0x101036B8, core->csnd.writeAdpcmStart(21, IO_PARAMS)) // CSND_ADPCM21_START
            DEF_IO32(0x101036BC, core->csnd.writeAdpcmLoop(21, IO_PARAMS)) // CSND_ADPCM21_LOOP
            DEF_IO16(0x101036C0, core->csnd.writeChanCnt(22, IO_PARAMS)) // CSND_CHAN22_CNT
            DEF_IO16(0x101036C2, core->csnd.writeChanRate(22, IO_PARAMS)) // CSND_CHAN22_RATE
            DEF_IO16(0x101036C4, core->csnd.writeChanRvol(22, IO_PARAMS)) // CSND_CHAN22_RVOL
            DEF_IO16(0x101036C6, core->csnd.writeChanLvol(22, IO_PARAMS)) // CSND_CHAN22_LVOL
            DEF_IO32(0x101036CC, core->csnd.writeChanStart(22, IO_PARAMS)) // CSND_CHAN22_START
            DEF_IO32(0x101036D0, core->csnd.writeChanSize(22, IO_PARAMS)) // CSND_CHAN22_SIZE
            DEF_IO32(0x101036D4, core->csnd.writeChanLoop(22, IO_PARAMS)) // CSND_CHAN22_LOOP
            DEF_IO32(0x101036D8, core->csnd.writeAdpcmStart(22, IO_PARAMS)) // CSND_ADPCM22_START
            DEF_IO32(0x101036DC, core->csnd.writeAdpcmLoop(22, IO_PARAMS)) // CSND_ADPCM22_LOOP
            DEF_IO16(0x101036E0, core->csnd.writeChanCnt(23, IO_PARAMS)) // CSND_CHAN23_CNT
            DEF_IO16(0x101036E2, core->csnd.writeChanRate(23, IO_PARAMS)) // CSND_CHAN23_RATE
            DEF_IO16(0x101036E4, core->csnd.writeChanRvol(23, IO_PARAMS)) // CSND_CHAN23_RVOL
            DEF_IO16(0x101036E6, core->csnd.writeChanLvol(23, IO_PARAMS)) // CSND_CHAN23_LVOL
            DEF_IO32(0x101036EC, core->csnd.writeChanStart(23, IO_PARAMS)) // CSND_CHAN23_START
            DEF_IO32(0x101036F0, core->csnd.writeChanSize(23, IO_PARAMS)) // CSND_CHAN23_SIZE
            DEF_IO32(0x101036F4, core->csnd.writeChanLoop(23, IO_PARAMS)) // CSND_CHAN23_LOOP
            DEF_IO32(0x101036F8, core->csnd.writeAdpcmStart(23, IO_PARAMS)) // CSND_ADPCM23_START
            DEF_IO32(0x101036FC, core->csnd.writeAdpcmLoop(23, IO_PARAMS)) // CSND_ADPCM23_LOOP
            DEF_IO16(0x10103700, core->csnd.writeChanCnt(24, IO_PARAMS)) // CSND_CHAN24_CNT
            DEF_IO16(0x10103702, core->csnd.writeChanRate(24, IO_PARAMS)) // CSND_CHAN24_RATE
            DEF_IO16(0x10103704, core->csnd.writeChanRvol(24, IO_PARAMS)) // CSND_CHAN24_RVOL
            DEF_IO16(0x10103706, core->csnd.writeChanLvol(24, IO_PARAMS)) // CSND_CHAN24_LVOL
            DEF_IO32(0x1010370C, core->csnd.writeChanStart(24, IO_PARAMS)) // CSND_CHAN24_START
            DEF_IO32(0x10103710, core->csnd.writeChanSize(24, IO_PARAMS)) // CSND_CHAN24_SIZE
            DEF_IO32(0x10103714, core->csnd.writeChanLoop(24, IO_PARAMS)) // CSND_CHAN24_LOOP
            DEF_IO32(0x10103718, core->csnd.writeAdpcmStart(24, IO_PARAMS)) // CSND_ADPCM24_START
            DEF_IO32(0x1010371C, core->csnd.writeAdpcmLoop(24, IO_PARAMS)) // CSND_ADPCM24_LOOP
            DEF_IO16(0x10103720, core->csnd.writeChanCnt(25, IO_PARAMS)) // CSND_CHAN25_CNT
            DEF_IO16(0x10103722, core->csnd.writeChanRate(25, IO_PARAMS)) // CSND_CHAN25_RATE
            DEF_IO16(0x10103724, core->csnd.writeChanRvol(25, IO_PARAMS)) // CSND_CHAN25_RVOL
            DEF_IO16(0x10103726, core->csnd.writeChanLvol(25, IO_PARAMS)) // CSND_CHAN25_LVOL
            DEF_IO32(0x1010372C, core->csnd.writeChanStart(25, IO_PARAMS)) // CSND_CHAN25_START
            DEF_IO32(0x10103730, core->csnd.writeChanSize(25, IO_PARAMS)) // CSND_CHAN25_SIZE
            DEF_IO32(0x10103734, core->csnd.writeChanLoop(25, IO_PARAMS)) // CSND_CHAN25_LOOP
            DEF_IO32(0x10103738, core->csnd.writeAdpcmStart(25, IO_PARAMS)) // CSND_ADPCM25_START
            DEF_IO32(0x1010373C, core->csnd.writeAdpcmLoop(25, IO_PARAMS)) // CSND_ADPCM25_LOOP
            DEF_IO16(0x10103740, core->csnd.writeChanCnt(26, IO_PARAMS)) // CSND_CHAN26_CNT
            DEF_IO16(0x10103742, core->csnd.writeChanRate(26, IO_PARAMS)) // CSND_CHAN26_RATE
            DEF_IO16(0x10103744, core->csnd.writeChanRvol(26, IO_PARAMS)) // CSND_CHAN26_RVOL
            DEF_IO16(0x10103746, core->csnd.writeChanLvol(26, IO_PARAMS)) // CSND_CHAN26_LVOL
            DEF_IO32(0x1010374C, core->csnd.writeChanStart(26, IO_PARAMS)) // CSND_CHAN26_START
            DEF_IO32(0x10103750, core->csnd.writeChanSize(26, IO_PARAMS)) // CSND_CHAN26_SIZE
            DEF_IO32(0x10103754, core->csnd.writeChanLoop(26, IO_PARAMS)) // CSND_CHAN26_LOOP
            DEF_IO32(0x10103758, core->csnd.writeAdpcmStart(26, IO_PARAMS)) // CSND_ADPCM26_START
            DEF_IO32(0x1010375C, core->csnd.writeAdpcmLoop(26, IO_PARAMS)) // CSND_ADPCM26_LOOP
            DEF_IO16(0x10103760, core->csnd.writeChanCnt(27, IO_PARAMS)) // CSND_CHAN27_CNT
            DEF_IO16(0x10103762, core->csnd.writeChanRate(27, IO_PARAMS)) // CSND_CHAN27_RATE
            DEF_IO16(0x10103764, core->csnd.writeChanRvol(27, IO_PARAMS)) // CSND_CHAN27_RVOL
            DEF_IO16(0x10103766, core->csnd.writeChanLvol(27, IO_PARAMS)) // CSND_CHAN27_LVOL
            DEF_IO32(0x1010376C, core->csnd.writeChanStart(27, IO_PARAMS)) // CSND_CHAN27_START
            DEF_IO32(0x10103770, core->csnd.writeChanSize(27, IO_PARAMS)) // CSND_CHAN27_SIZE
            DEF_IO32(0x10103774, core->csnd.writeChanLoop(27, IO_PARAMS)) // CSND_CHAN27_LOOP
            DEF_IO32(0x10103778, core->csnd.writeAdpcmStart(27, IO_PARAMS)) // CSND_ADPCM27_START
            DEF_IO32(0x1010377C, core->csnd.writeAdpcmLoop(27, IO_PARAMS)) // CSND_ADPCM27_LOOP
            DEF_IO16(0x10103780, core->csnd.writeChanCnt(28, IO_PARAMS)) // CSND_CHAN28_CNT
            DEF_IO16(0x10103782, core->csnd.writeChanRate(28, IO_PARAMS)) // CSND_CHAN28_RATE
            DEF_IO16(0x10103784, core->csnd.writeChanRvol(28, IO_PARAMS)) // CSND_CHAN28_RVOL
            DEF_IO16(0x10103786, core->csnd.writeChanLvol(28, IO_PARAMS)) // CSND_CHAN28_LVOL
            DEF_IO32(0x1010378C, core->csnd.writeChanStart(28, IO_PARAMS)) // CSND_CHAN28_START
            DEF_IO32(0x10103790, core->csnd.writeChanSize(28, IO_PARAMS)) // CSND_CHAN28_SIZE
            DEF_IO32(0x10103794, core->csnd.writeChanLoop(28, IO_PARAMS)) // CSND_CHAN28_LOOP
            DEF_IO32(0x10103798, core->csnd.writeAdpcmStart(28, IO_PARAMS)) // CSND_ADPCM28_START
            DEF_IO32(0x1010379C, core->csnd.writeAdpcmLoop(28, IO_PARAMS)) // CSND_ADPCM28_LOOP
            DEF_IO16(0x101037A0, core->csnd.writeChanCnt(29, IO_PARAMS)) // CSND_CHAN29_CNT
            DEF_IO16(0x101037A2, core->csnd.writeChanRate(29, IO_PARAMS)) // CSND_CHAN29_RATE
            DEF_IO16(0x101037A4, core->csnd.writeChanRvol(29, IO_PARAMS)) // CSND_CHAN29_RVOL
            DEF_IO16(0x101037A6, core->csnd.writeChanLvol(29, IO_PARAMS)) // CSND_CHAN29_LVOL
            DEF_IO32(0x101037AC, core->csnd.writeChanStart(29, IO_PARAMS)) // CSND_CHAN29_START
            DEF_IO32(0x101037B0, core->csnd.writeChanSize(29, IO_PARAMS)) // CSND_CHAN29_SIZE
            DEF_IO32(0x101037B4, core->csnd.writeChanLoop(29, IO_PARAMS)) // CSND_CHAN29_LOOP
            DEF_IO32(0x101037B8, core->csnd.writeAdpcmStart(29, IO_PARAMS)) // CSND_ADPCM29_START
            DEF_IO32(0x101037BC, core->csnd.writeAdpcmLoop(29, IO_PARAMS)) // CSND_ADPCM29_LOOP
            DEF_IO16(0x101037C0, core->csnd.writeChanCnt(30, IO_PARAMS)) // CSND_CHAN30_CNT
            DEF_IO16(0x101037C2, core->csnd.writeChanRate(30, IO_PARAMS)) // CSND_CHAN30_RATE
            DEF_IO16(0x101037C4, core->csnd.writeChanRvol(30, IO_PARAMS)) // CSND_CHAN30_RVOL
            DEF_IO16(0x101037C6, core->csnd.writeChanLvol(30, IO_PARAMS)) // CSND_CHAN30_LVOL
            DEF_IO32(0x101037CC, core->csnd.writeChanStart(30, IO_PARAMS)) // CSND_CHAN30_START
            DEF_IO32(0x101037D0, core->csnd.writeChanSize(30, IO_PARAMS)) // CSND_CHAN30_SIZE
            DEF_IO32(0x101037D4, core->csnd.writeChanLoop(30, IO_PARAMS)) // CSND_CHAN30_LOOP
            DEF_IO32(0x101037D8, core->csnd.writeAdpcmStart(30, IO_PARAMS)) // CSND_ADPCM30_START
            DEF_IO32(0x101037DC, core->csnd.writeAdpcmLoop(30, IO_PARAMS)) // CSND_ADPCM30_LOOP
            DEF_IO16(0x101037E0, core->csnd.writeChanCnt(31, IO_PARAMS)) // CSND_CHAN31_CNT
            DEF_IO16(0x101037E2, core->csnd.writeChanRate(31, IO_PARAMS)) // CSND_CHAN31_RATE
            DEF_IO16(0x101037E4, core->csnd.writeChanRvol(31, IO_PARAMS)) // CSND_CHAN31_RVOL
            DEF_IO16(0x101037E6, core->csnd.writeChanLvol(31, IO_PARAMS)) // CSND_CHAN31_LVOL
            DEF_IO32(0x101037EC, core->csnd.writeChanStart(31, IO_PARAMS)) // CSND_CHAN31_START
            DEF_IO32(0x101037F0, core->csnd.writeChanSize(31, IO_PARAMS)) // CSND_CHAN31_SIZE
            DEF_IO32(0x101037F4, core->csnd.writeChanLoop(31, IO_PARAMS)) // CSND_CHAN31_LOOP
            DEF_IO32(0x101037F8, core->csnd.writeAdpcmStart(31, IO_PARAMS)) // CSND_ADPCM31_START
            DEF_IO32(0x101037FC, core->csnd.writeAdpcmLoop(31, IO_PARAMS)) // CSND_ADPCM31_LOOP
            DEF_IO16(0x10122000, core->wifi.writeCmd(IO_PARAMS)) // WIFI_CMD
            DEF_IO32(0x10122004, core->wifi.writeCmdParam(IO_PARAMS)) // WIFI_CMD_PARAM
            DEF_IO32(0x1012201C, core->wifi.writeIrqStatus(IO_PARAMS)) // WIFI_IRQ_STATUS
            DEF_IO32(0x10122020, core->wifi.writeIrqMask(IO_PARAMS)) // WIFI_IRQ_MASK
            DEF_IO16(0x10122026, core->wifi.writeData16Blklen(IO_PARAMS)) // WIFI_DATA16_BLKLEN
            DEF_IO16(0x10122030, core->wifi.writeData16Fifo(IO_PARAMS)) // WIFI_DATA16_FIFO
            DEF_IO16(0x10122036, core->wifi.writeCardIrqStat(IO_PARAMS)) // WIFI_CARD_IRQ_STAT
            DEF_IO16(0x10122038, core->wifi.writeCardIrqMask(IO_PARAMS)) // WIFI_CARD_IRQ_MASK
            DEF_IO16(0x101220D8, core->wifi.writeDataCtl(IO_PARAMS)) // WIFI_DATA_CTL
            DEF_IO16(0x10122100, core->wifi.writeData32Irq(IO_PARAMS)) // WIFI_DATA32_IRQ
            DEF_IO16(0x10122104, core->wifi.writeData32Blklen(IO_PARAMS)) // WIFI_DATA32_BLKLEN
            DEF_IO32(0x10132000, core->y2rs[1].writeCnt(IO_PARAMS)) // Y2R1_CNT
            DEF_IO16(0x10132004, core->y2rs[1].writeWidth(IO_PARAMS)) // Y2R1_WIDTH
            DEF_IO16(0x10132006, core->y2rs[1].writeHeight(IO_PARAMS)) // Y2R1_HEIGHT
            DEF_IO16(0x10132010, core->y2rs[1].writeMultiplyY(IO_PARAMS)) // Y2R1_MULTIPLY_Y
            DEF_IO16(0x10132012, core->y2rs[1].writeMultiplyVr(IO_PARAMS)) // Y2R1_MULTIPLY_VR
            DEF_IO16(0x10132014, core->y2rs[1].writeMultiplyVg(IO_PARAMS)) // Y2R1_MULTIPLY_VG
            DEF_IO16(0x10132016, core->y2rs[1].writeMultiplyUg(IO_PARAMS)) // Y2R1_MULTIPLY_UG
            DEF_IO16(0x10132018, core->y2rs[1].writeMultiplyUb(IO_PARAMS)) // Y2R1_MULTIPLY_UB
            DEF_IO16(0x1013201A, core->y2rs[1].writeOffsetR(IO_PARAMS)) // Y2R1_OFFSET_R
            DEF_IO16(0x1013201C, core->y2rs[1].writeOffsetG(IO_PARAMS)) // Y2R1_OFFSET_G
            DEF_IO16(0x1013201E, core->y2rs[1].writeOffsetB(IO_PARAMS)) // Y2R1_OFFSET_B
            DEF_IO16(0x10132020, core->y2rs[1].writeAlpha(IO_PARAMS)) // Y2R1_ALPHA
            DEF_IO08(0x10140000, writeCfg11Wram32kCode(0, IO_PARAMS8)) // CFG11_WRAM_32K_CODE0
            DEF_IO08(0x10140001, writeCfg11Wram32kCode(1, IO_PARAMS8)) // CFG11_WRAM_32K_CODE1
            DEF_IO08(0x10140002, writeCfg11Wram32kCode(2, IO_PARAMS8)) // CFG11_WRAM_32K_CODE2
            DEF_IO08(0x10140003, writeCfg11Wram32kCode(3, IO_PARAMS8)) // CFG11_WRAM_32K_CODE3
            DEF_IO08(0x10140004, writeCfg11Wram32kCode(4, IO_PARAMS8)) // CFG11_WRAM_32K_CODE4
            DEF_IO08(0x10140005, writeCfg11Wram32kCode(5, IO_PARAMS8)) // CFG11_WRAM_32K_CODE5
            DEF_IO08(0x10140006, writeCfg11Wram32kCode(6, IO_PARAMS8)) // CFG11_WRAM_32K_CODE6
            DEF_IO08(0x10140007, writeCfg11Wram32kCode(7, IO_PARAMS8)) // CFG11_WRAM_32K_CODE7
            DEF_IO08(0x10140008, writeCfg11Wram32kData(0, IO_PARAMS8)) // CFG11_WRAM_32K_DATA0
            DEF_IO08(0x10140009, writeCfg11Wram32kData(1, IO_PARAMS8)) // CFG11_WRAM_32K_DATA1
            DEF_IO08(0x1014000A, writeCfg11Wram32kData(2, IO_PARAMS8)) // CFG11_WRAM_32K_DATA2
            DEF_IO08(0x1014000B, writeCfg11Wram32kData(3, IO_PARAMS8)) // CFG11_WRAM_32K_DATA3
            DEF_IO08(0x1014000C, writeCfg11Wram32kData(4, IO_PARAMS8)) // CFG11_WRAM_32K_DATA4
            DEF_IO08(0x1014000D, writeCfg11Wram32kData(5, IO_PARAMS8)) // CFG11_WRAM_32K_DATA5
            DEF_IO08(0x1014000E, writeCfg11Wram32kData(6, IO_PARAMS8)) // CFG11_WRAM_32K_DATA6
            DEF_IO08(0x1014000F, writeCfg11Wram32kData(7, IO_PARAMS8)) // CFG11_WRAM_32K_DATA7
            DEF_IO32(0x10140420, writeCfg11BrOverlayCnt(IO_PARAMS)) // CFG11_BR_OVERLAY_CNT
            DEF_IO32(0x10140424, writeCfg11BrOverlayVal(IO_PARAMS)) // CFG11_BR_OVERLAY_VAL
            DEF_IO32(0x10141200, core->gpu.writeCfg11GpuCnt(IO_PARAMS)) // CFG11_GPU_CNT
            DEF_IO32(0x10141300, core->interrupts.writeCfg11MpClkcnt(IO_PARAMS)) // CFG11_MP_CLKCNT
            DEF_IO32(0x10141304, writeCfg11MpCnt(IO_PARAMS)) // CFG11_MP_CNT
            DEF_IO08(0x10141312, core->interrupts.writeCfg11MpBootcnt(2, IO_PARAMS8)) // CFG11_MP_BOOTCNT2
            DEF_IO08(0x10141313, core->interrupts.writeCfg11MpBootcnt(3, IO_PARAMS8)) // CFG11_MP_BOOTCNT3
            DEF_IO32(0x10142800, core->input.writeSpiFifoCnt(IO_PARAMS)) // SPIBUS1_FIFO_CNT
            DEF_IO32(0x10142804, core->input.writeSpiFifoSelect(IO_PARAMS)) // SPIBUS1_FIFO_SELECT
            DEF_IO32(0x10142808, core->input.writeSpiFifoBlklen(IO_PARAMS)) // SPIBUS1_FIFO_BLKLEN
            DEF_IO32(0x1014280C, core->input.writeSpiFifoData(IO_PARAMS)) // SPIBUS1_FIFO_DATA
            DEF_IO32(0x10142818, core->input.writeSpiFifoIntMask(IO_PARAMS)) // SPIBUS1_FIFO_INT_MASK
            DEF_IO32(0x1014281C, core->input.writeSpiFifoIntStat(IO_PARAMS)) // SPIBUS1_FIFO_INT_STAT
            DEF_IO08(0x10144000, core->i2c.writeBusData(1, IO_PARAMS8)) // I2C_BUS1_DATA
            DEF_IO08(0x10144001, core->i2c.writeBusCnt(1, IO_PARAMS8)) // I2C_BUS1_CNT
            DEF_IO32(0x10145000, core->csnd.writeSndexcnt(IO_PARAMS)) // CODEC_SNDEXCNT
            DEF_IO08(0x10148000, core->i2c.writeBusData(2, IO_PARAMS8)) // I2C_BUS2_DATA
            DEF_IO08(0x10148001, core->i2c.writeBusCnt(2, IO_PARAMS8)) // I2C_BUS2_CNT
            DEF_IO08(0x10161000, core->i2c.writeBusData(0, IO_PARAMS8)) // I2C_BUS0_DATA
            DEF_IO08(0x10161001, core->i2c.writeBusCnt(0, IO_PARAMS8)) // I2C_BUS0_CNT
            DEF_IO32(0x10163000, core->pxi.writeSync(0, IO_PARAMS)) // PXI_SYNC11
            DEF_IO32(0x10163004, core->pxi.writeCnt(0, IO_PARAMS)) // PXI_CNT11
            DEF_IO32(0x10163008, core->pxi.writeSend(0, IO_PARAMS)) // PXI_SEND11
            DEF_IO16(0x10164000, core->cartridge.writeNtrMcnt(IO_PARAMS)) // NTRCARD_MCNT
            DEF_IO32(0x10164004, core->cartridge.writeNtrRomcnt(IO_PARAMS)) // NTRCARD_ROMCNT
            DEF_IO32(0x10164008, core->cartridge.writeNtrCmd(0, IO_PARAMS)) // NTRCARD_CMD0
            DEF_IO32(0x1016400C, core->cartridge.writeNtrCmd(1, IO_PARAMS)) // NTRCARD_CMD1
        }

        // Check registers that are exclusive to one CPU
        if (id != ARM9) { // ARM11
            switch (base) {
                DEF_IO32(0x10200D04, core->cdmas[CDMA0].writeDbgcmd(IO_PARAMS)) // CDMA0_DBGCMD
                DEF_IO32(0x10200D08, core->cdmas[CDMA0].writeDbginst0(IO_PARAMS)) // CDMA0_DBGINST0
                DEF_IO32(0x10200D0C, core->cdmas[CDMA0].writeDbginst1(IO_PARAMS)) // CDMA0_DBGINST1
                DEF_IO32(0x10200020, core->cdmas[CDMA0].writeInten(IO_PARAMS)) // CDMA0_INTEN
                DEF_IO32(0x1020002C, core->cdmas[CDMA0].writeIntclr(IO_PARAMS)) // CDMA0_INTCLR
                DEF_IO16(0x10203000, core->dsp.writePdata(IO_PARAMS)) // DSP_PDATA
                DEF_IO16(0x10203004, core->dsp.writePadr(IO_PARAMS)) // DSP_PADR
                DEF_IO16(0x10203008, core->dsp.writePcfg(IO_PARAMS)) // DSP_PCFG
                DEF_IO16(0x10203010, core->dsp.writePsem(IO_PARAMS)) // DSP_PSEM
                DEF_IO16(0x10203014, core->dsp.writePmask(IO_PARAMS)) // DSP_PMASK
                DEF_IO16(0x10203018, core->dsp.writePclear(IO_PARAMS)) // DSP_PCLEAR
                DEF_IO16(0x10203020, core->dsp.writeCmd(0, IO_PARAMS)) // DSP_CMD0
                DEF_IO16(0x10203028, core->dsp.writeCmd(1, IO_PARAMS)) // DSP_CMD1
                DEF_IO16(0x10203030, core->dsp.writeCmd(2, IO_PARAMS)) // DSP_CMD2
                DEF_IO32(0x10206D04, core->cdmas[CDMA1].writeDbgcmd(IO_PARAMS)) // CDMA1_DBGCMD
                DEF_IO32(0x10206D08, core->cdmas[CDMA1].writeDbginst0(IO_PARAMS)) // CDMA1_DBGINST0
                DEF_IO32(0x10206D0C, core->cdmas[CDMA1].writeDbginst1(IO_PARAMS)) // CDMA1_DBGINST1
                DEF_IO32(0x10206020, core->cdmas[CDMA1].writeInten(IO_PARAMS)) // CDMA1_INTEN
                DEF_IO32(0x1020602C, core->cdmas[CDMA1].writeIntclr(IO_PARAMS)) // CDMA1_INTCLR
                DEF_IO32(0x10301000, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301004, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301008, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x1030100C, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301010, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301014, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301018, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x1030101C, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301020, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301024, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301028, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x1030102C, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301030, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301034, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10301038, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x1030103C, core->shas[0].writeFifo(IO_PARAMS)) // SHA0_FIFO
                DEF_IO32(0x10302000, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302004, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302008, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030200C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302010, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302014, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302018, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030201C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302020, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302024, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302028, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030202C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302030, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302034, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302038, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030203C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302040, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302044, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302048, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030204C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302050, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302054, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302058, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030205C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302060, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302064, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302068, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030206C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302070, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302074, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302078, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x1030207C, core->y2rs[0].writeInputY(IO_PARAMS)) // Y2R0_INPUT_Y
                DEF_IO32(0x10302080, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302084, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302088, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x1030208C, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302090, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302094, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302098, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x1030209C, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020A0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020A4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020A8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020AC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020B0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020B4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020B8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020BC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020C0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020C4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020C8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020CC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020D0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020D4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020D8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020DC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020E0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020E4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020E8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020EC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020F0, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020F4, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020F8, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x103020FC, core->y2rs[0].writeInputU(IO_PARAMS)) // Y2R0_INPUT_U
                DEF_IO32(0x10302100, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302104, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302108, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030210C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302110, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302114, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302118, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030211C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302120, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302124, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302128, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030212C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302130, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302134, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302138, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030213C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302140, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302144, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302148, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030214C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302150, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302154, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302158, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030215C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302160, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302164, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302168, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030216C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302170, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302174, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10302178, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x1030217C, core->y2rs[0].writeInputV(IO_PARAMS)) // Y2R0_INPUT_V
                DEF_IO32(0x10332000, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332004, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332008, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033200C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332010, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332014, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332018, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033201C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332020, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332024, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332028, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033202C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332030, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332034, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332038, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033203C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332040, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332044, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332048, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033204C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332050, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332054, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332058, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033205C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332060, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332064, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332068, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033206C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332070, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332074, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332078, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x1033207C, core->y2rs[1].writeInputY(IO_PARAMS)) // Y2R1_INPUT_Y
                DEF_IO32(0x10332080, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332084, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332088, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x1033208C, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332090, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332094, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332098, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x1033209C, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320A0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320A4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320A8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320AC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320B0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320B4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320B8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320BC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320C0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320C4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320C8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320CC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320D0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320D4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320D8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320DC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320E0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320E4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320E8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320EC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320F0, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320F4, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320F8, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x103320FC, core->y2rs[1].writeInputU(IO_PARAMS)) // Y2R1_INPUT_U
                DEF_IO32(0x10332100, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332104, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332108, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033210C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332110, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332114, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332118, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033211C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332120, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332124, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332128, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033212C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332130, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332134, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332138, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033213C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332140, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332144, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332148, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033214C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332150, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332154, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332158, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033215C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332160, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332164, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332168, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033216C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332170, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332174, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10332178, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x1033217C, core->y2rs[1].writeInputV(IO_PARAMS)) // Y2R1_INPUT_V
                DEF_IO32(0x10322000, core->wifi.writeData32Fifo(IO_PARAMS)) // WIFI_DATA32_FIFO
                DEF_IO32(0x10400010, core->gpu.writeFillDstAddr(0, IO_PARAMS)) // GPU_FILL0_DST_ADDR
                DEF_IO32(0x10400014, core->gpu.writeFillDstEnd(0, IO_PARAMS)) // GPU_FILL0_DST_END
                DEF_IO32(0x10400018, core->gpu.writeFillData(0, IO_PARAMS)) // GPU_FILL0_DATA
                DEF_IO32(0x1040001C, core->gpu.writeFillCnt(0, IO_PARAMS)) // GPU_FILL0_CNT
                DEF_IO32(0x10400020, core->gpu.writeFillDstAddr(1, IO_PARAMS)) // GPU_FILL1_DST_ADDR
                DEF_IO32(0x10400024, core->gpu.writeFillDstEnd(1, IO_PARAMS)) // GPU_FILL1_DST_END
                DEF_IO32(0x10400028, core->gpu.writeFillData(1, IO_PARAMS)) // GPU_FILL1_DATA
                DEF_IO32(0x1040002C, core->gpu.writeFillCnt(1, IO_PARAMS)) // GPU_FILL1_CNT
                DEF_IO32(0x10400468, core->pdc.writeFramebufLt0(0, IO_PARAMS)) // PDC0_FRAMEBUF_LT0
                DEF_IO32(0x1040046C, core->pdc.writeFramebufLt1(0, IO_PARAMS)) // PDC0_FRAMEBUF_LT1
                DEF_IO32(0x10400470, core->pdc.writeFramebufFormat(0, IO_PARAMS)) // PDC0_FRAMEBUF_FORMAT
                DEF_IO32(0x10400474, core->pdc.writeInterruptType(0, IO_PARAMS)) // PDC0_INTERRUPT_TYPE
                DEF_IO32(0x10400478, core->pdc.writeFramebufSelAck(0, IO_PARAMS)) // PDC0_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400490, core->pdc.writeFramebufStep(0, IO_PARAMS)) // PDC0_FRAMEBUF_STEP
                DEF_IO32(0x10400568, core->pdc.writeFramebufLt0(1, IO_PARAMS)) // PDC1_FRAMEBUF_LT0
                DEF_IO32(0x1040056C, core->pdc.writeFramebufLt1(1, IO_PARAMS)) // PDC1_FRAMEBUF_LT1
                DEF_IO32(0x10400570, core->pdc.writeFramebufFormat(1, IO_PARAMS)) // PDC1_FRAMEBUF_FORMAT
                DEF_IO32(0x10400574, core->pdc.writeInterruptType(1, IO_PARAMS)) // PDC1_INTERRUPT_TYPE
                DEF_IO32(0x10400578, core->pdc.writeFramebufSelAck(1, IO_PARAMS)) // PDC1_FRAMEBUF_SEL_ACK
                DEF_IO32(0x10400590, core->pdc.writeFramebufStep(1, IO_PARAMS)) // PDC1_FRAMEBUF_STEP
                DEF_IO32(0x10400C00, core->gpu.writeCopySrcAddr(IO_PARAMS)) // GPU_COPY_SRC_ADDR
                DEF_IO32(0x10400C04, core->gpu.writeCopyDstAddr(IO_PARAMS)) // GPU_COPY_DST_ADDR
                DEF_IO32(0x10400C08, core->gpu.writeCopyDispDstSize(IO_PARAMS)) // GPU_COPY_DISP_DST_SIZE
                DEF_IO32(0x10400C0C, core->gpu.writeCopyDispSrcSize(IO_PARAMS)) // GPU_COPY_DISP_SRC_SIZE
                DEF_IO32(0x10400C10, core->gpu.writeCopyFlags(IO_PARAMS)) // GPU_COPY_FLAGS
                DEF_IO32(0x10400C18, core->gpu.writeCopyCnt(IO_PARAMS)) // GPU_COPY_CNT
                DEF_IO32(0x10400C20, core->gpu.writeCopyTexSize(IO_PARAMS)) // GPU_COPY_TEX_SIZE
                DEF_IO32(0x10400C24, core->gpu.writeCopyTexSrcWidth(IO_PARAMS)) // GPU_COPY_TEX_SRC_WIDTH
                DEF_IO32(0x10400C28, core->gpu.writeCopyTexDstWidth(IO_PARAMS)) // GPU_COPY_TEX_DST_WIDTH
                DEF_IO32(0x10401000, core->gpu.writeIrqAck(0, IO_PARAMS)) // GPU_IRQ_ACK0
                DEF_IO32(0x10401004, core->gpu.writeIrqAck(1, IO_PARAMS)) // GPU_IRQ_ACK1
                DEF_IO32(0x10401008, core->gpu.writeIrqAck(2, IO_PARAMS)) // GPU_IRQ_ACK2
                DEF_IO32(0x1040100C, core->gpu.writeIrqAck(3, IO_PARAMS)) // GPU_IRQ_ACK3
                DEF_IO32(0x10401010, core->gpu.writeIrqAck(4, IO_PARAMS)) // GPU_IRQ_ACK4
                DEF_IO32(0x10401014, core->gpu.writeIrqAck(5, IO_PARAMS)) // GPU_IRQ_ACK5
                DEF_IO32(0x10401018, core->gpu.writeIrqAck(6, IO_PARAMS)) // GPU_IRQ_ACK6
                DEF_IO32(0x1040101C, core->gpu.writeIrqAck(7, IO_PARAMS)) // GPU_IRQ_ACK7
                DEF_IO32(0x10401020, core->gpu.writeIrqAck(8, IO_PARAMS)) // GPU_IRQ_ACK8
                DEF_IO32(0x10401024, core->gpu.writeIrqAck(9, IO_PARAMS)) // GPU_IRQ_ACK9
                DEF_IO32(0x10401028, core->gpu.writeIrqAck(10, IO_PARAMS)) // GPU_IRQ_ACK10
                DEF_IO32(0x1040102C, core->gpu.writeIrqAck(11, IO_PARAMS)) // GPU_IRQ_ACK11
                DEF_IO32(0x10401030, core->gpu.writeIrqAck(12, IO_PARAMS)) // GPU_IRQ_ACK12
                DEF_IO32(0x10401034, core->gpu.writeIrqAck(13, IO_PARAMS)) // GPU_IRQ_ACK13
                DEF_IO32(0x10401038, core->gpu.writeIrqAck(14, IO_PARAMS)) // GPU_IRQ_ACK14
                DEF_IO32(0x1040103C, core->gpu.writeIrqAck(15, IO_PARAMS)) // GPU_IRQ_ACK15
                DEF_IO32(0x10401040, core->gpu.writeIrqReq<0>(IO_PARAMS)) // GPU_IRQ_REQ0
                DEF_IO32(0x10401044, core->gpu.writeIrqReq<1>(IO_PARAMS)) // GPU_IRQ_REQ1
                DEF_IO32(0x10401048, core->gpu.writeIrqReq<2>(IO_PARAMS)) // GPU_IRQ_REQ2
                DEF_IO32(0x1040104C, core->gpu.writeIrqReq<3>(IO_PARAMS)) // GPU_IRQ_REQ3
                DEF_IO32(0x10401050, core->gpu.writeIrqReq<4>(IO_PARAMS)) // GPU_IRQ_REQ4
                DEF_IO32(0x10401054, core->gpu.writeIrqReq<5>(IO_PARAMS)) // GPU_IRQ_REQ5
                DEF_IO32(0x10401058, core->gpu.writeIrqReq<6>(IO_PARAMS)) // GPU_IRQ_REQ6
                DEF_IO32(0x1040105C, core->gpu.writeIrqReq<7>(IO_PARAMS)) // GPU_IRQ_REQ7
                DEF_IO32(0x10401060, core->gpu.writeIrqReq<8>(IO_PARAMS)) // GPU_IRQ_REQ8
                DEF_IO32(0x10401064, core->gpu.writeIrqReq<9>(IO_PARAMS)) // GPU_IRQ_REQ9
                DEF_IO32(0x10401068, core->gpu.writeIrqReq<10>(IO_PARAMS)) // GPU_IRQ_REQ10
                DEF_IO32(0x1040106C, core->gpu.writeIrqReq<11>(IO_PARAMS)) // GPU_IRQ_REQ11
                DEF_IO32(0x10401070, core->gpu.writeIrqReq<12>(IO_PARAMS)) // GPU_IRQ_REQ12
                DEF_IO32(0x10401074, core->gpu.writeIrqReq<13>(IO_PARAMS)) // GPU_IRQ_REQ13
                DEF_IO32(0x10401078, core->gpu.writeIrqReq<14>(IO_PARAMS)) // GPU_IRQ_REQ14
                DEF_IO32(0x1040107C, core->gpu.writeIrqReq<15>(IO_PARAMS)) // GPU_IRQ_REQ15
                DEF_IO32(0x10401080, core->gpu.writeIrqCmp(0, IO_PARAMS)) // GPU_IRQ_CMP0
                DEF_IO32(0x10401084, core->gpu.writeIrqCmp(1, IO_PARAMS)) // GPU_IRQ_CMP1
                DEF_IO32(0x10401088, core->gpu.writeIrqCmp(2, IO_PARAMS)) // GPU_IRQ_CMP2
                DEF_IO32(0x1040108C, core->gpu.writeIrqCmp(3, IO_PARAMS)) // GPU_IRQ_CMP3
                DEF_IO32(0x10401090, core->gpu.writeIrqCmp(4, IO_PARAMS)) // GPU_IRQ_CMP4
                DEF_IO32(0x10401094, core->gpu.writeIrqCmp(5, IO_PARAMS)) // GPU_IRQ_CMP5
                DEF_IO32(0x10401098, core->gpu.writeIrqCmp(6, IO_PARAMS)) // GPU_IRQ_CMP6
                DEF_IO32(0x1040109C, core->gpu.writeIrqCmp(7, IO_PARAMS)) // GPU_IRQ_CMP7
                DEF_IO32(0x104010A0, core->gpu.writeIrqCmp(8, IO_PARAMS)) // GPU_IRQ_CMP8
                DEF_IO32(0x104010A4, core->gpu.writeIrqCmp(9, IO_PARAMS)) // GPU_IRQ_CMP9
                DEF_IO32(0x104010A8, core->gpu.writeIrqCmp(10, IO_PARAMS)) // GPU_IRQ_CMP10
                DEF_IO32(0x104010AC, core->gpu.writeIrqCmp(11, IO_PARAMS)) // GPU_IRQ_CMP11
                DEF_IO32(0x104010B0, core->gpu.writeIrqCmp(12, IO_PARAMS)) // GPU_IRQ_CMP12
                DEF_IO32(0x104010B4, core->gpu.writeIrqCmp(13, IO_PARAMS)) // GPU_IRQ_CMP13
                DEF_IO32(0x104010B8, core->gpu.writeIrqCmp(14, IO_PARAMS)) // GPU_IRQ_CMP14
                DEF_IO32(0x104010BC, core->gpu.writeIrqCmp(15, IO_PARAMS)) // GPU_IRQ_CMP15
                DEF_IO32(0x104010C0, core->gpu.writeIrqMaskL(IO_PARAMS)) // GPU_IRQ_MASK_L
                DEF_IO32(0x104010C4, core->gpu.writeIrqMaskH(IO_PARAMS)) // GPU_IRQ_MASK_H
                DEF_IO32(0x104010D0, core->gpu.writeIrqAutostop(IO_PARAMS)) // GPU_IRQ_AUTOSTOP
                DEF_IO32(0x10401100, core->gpu.writeFaceCulling(IO_PARAMS)) // GPU_FACE_CULLING
                DEF_IO32(0x10401104, core->gpu.writeViewScaleH(IO_PARAMS)) // GPU_VIEW_SCALE_H
                DEF_IO32(0x10401108, core->gpu.writeViewStepH(IO_PARAMS)) // GPU_VIEW_STEP_H
                DEF_IO32(0x1040110C, core->gpu.writeViewScaleV(IO_PARAMS)) // GPU_VIEW_SCALE_V
                DEF_IO32(0x10401110, core->gpu.writeViewStepV(IO_PARAMS)) // GPU_VIEW_STEP_V
                DEF_IO32(0x1040113C, core->gpu.writeShdOutTotal(IO_PARAMS)) // GPU_SHD_OUT_TOTAL
                DEF_IO32(0x10401140, core->gpu.writeShdOutMap<0>(IO_PARAMS)) // GPU_SHD_OUT_MAP0
                DEF_IO32(0x10401144, core->gpu.writeShdOutMap<1>(IO_PARAMS)) // GPU_SHD_OUT_MAP1
                DEF_IO32(0x10401148, core->gpu.writeShdOutMap<2>(IO_PARAMS)) // GPU_SHD_OUT_MAP2
                DEF_IO32(0x1040114C, core->gpu.writeShdOutMap<3>(IO_PARAMS)) // GPU_SHD_OUT_MAP3
                DEF_IO32(0x10401150, core->gpu.writeShdOutMap<4>(IO_PARAMS)) // GPU_SHD_OUT_MAP4
                DEF_IO32(0x10401154, core->gpu.writeShdOutMap<5>(IO_PARAMS)) // GPU_SHD_OUT_MAP5
                DEF_IO32(0x10401158, core->gpu.writeShdOutMap<6>(IO_PARAMS)) // GPU_SHD_OUT_MAP6
                DEF_IO32(0x10401204, core->gpu.writeTexBorder<0>(IO_PARAMS)) // GPU_TEX0_BORDER
                DEF_IO32(0x10401208, core->gpu.writeTexDim<0>(IO_PARAMS)) // GPU_TEX0_DIM
                DEF_IO32(0x1040120C, core->gpu.writeTexParam<0>(IO_PARAMS)) // GPU_TEX0_PARAM
                DEF_IO32(0x10401214, core->gpu.writeTexAddr1<0>(IO_PARAMS)) // GPU_TEX0_ADDR1
                DEF_IO32(0x10401238, core->gpu.writeTexType<0>(IO_PARAMS)) // GPU_TEX0_TYPE
                DEF_IO32(0x10401244, core->gpu.writeTexBorder<1>(IO_PARAMS)) // GPU_TEX1_BORDER
                DEF_IO32(0x10401248, core->gpu.writeTexDim<1>(IO_PARAMS)) // GPU_TEX1_DIM
                DEF_IO32(0x1040124C, core->gpu.writeTexParam<1>(IO_PARAMS)) // GPU_TEX1_PARAM
                DEF_IO32(0x10401254, core->gpu.writeTexAddr1<1>(IO_PARAMS)) // GPU_TEX1_ADDR
                DEF_IO32(0x10401258, core->gpu.writeTexType<1>(IO_PARAMS)) // GPU_TEX1_TYPE
                DEF_IO32(0x10401264, core->gpu.writeTexBorder<2>(IO_PARAMS)) // GPU_TEX2_BORDER
                DEF_IO32(0x10401268, core->gpu.writeTexDim<2>(IO_PARAMS)) // GPU_TEX2_DIM
                DEF_IO32(0x1040126C, core->gpu.writeTexParam<2>(IO_PARAMS)) // GPU_TEX2_PARAM
                DEF_IO32(0x10401274, core->gpu.writeTexAddr1<2>(IO_PARAMS)) // GPU_TEX2_ADDR
                DEF_IO32(0x10401278, core->gpu.writeTexType<2>(IO_PARAMS)) // GPU_TEX2_TYPE
                DEF_IO32(0x10401300, core->gpu.writeCombSrc<0>(IO_PARAMS)) // GPU_COMB0_SRC
                DEF_IO32(0x10401304, core->gpu.writeCombOper<0>(IO_PARAMS)) // GPU_COMB0_OPER
                DEF_IO32(0x10401308, core->gpu.writeCombMode<0>(IO_PARAMS)) // GPU_COMB0_MODE
                DEF_IO32(0x1040130C, core->gpu.writeCombColor<0>(IO_PARAMS)) // GPU_COMB0_COLOR
                DEF_IO32(0x10401320, core->gpu.writeCombSrc<1>(IO_PARAMS)) // GPU_COMB1_SRC
                DEF_IO32(0x10401324, core->gpu.writeCombOper<1>(IO_PARAMS)) // GPU_COMB1_OPER
                DEF_IO32(0x10401328, core->gpu.writeCombMode<1>(IO_PARAMS)) // GPU_COMB1_MODE
                DEF_IO32(0x1040132C, core->gpu.writeCombColor<1>(IO_PARAMS)) // GPU_COMB1_COLOR
                DEF_IO32(0x10401340, core->gpu.writeCombSrc<2>(IO_PARAMS)) // GPU_COMB2_SRC
                DEF_IO32(0x10401344, core->gpu.writeCombOper<2>(IO_PARAMS)) // GPU_COMB2_OPER
                DEF_IO32(0x10401348, core->gpu.writeCombMode<2>(IO_PARAMS)) // GPU_COMB2_MODE
                DEF_IO32(0x1040134C, core->gpu.writeCombColor<2>(IO_PARAMS)) // GPU_COMB2_COLOR
                DEF_IO32(0x10401360, core->gpu.writeCombSrc<3>(IO_PARAMS)) // GPU_COMB3_SRC
                DEF_IO32(0x10401364, core->gpu.writeCombOper<3>(IO_PARAMS)) // GPU_COMB3_OPER
                DEF_IO32(0x10401368, core->gpu.writeCombMode<3>(IO_PARAMS)) // GPU_COMB3_MODE
                DEF_IO32(0x1040136C, core->gpu.writeCombColor<3>(IO_PARAMS)) // GPU_COMB3_COLOR
                DEF_IO32(0x10401380, core->gpu.writeCombBufUpd(IO_PARAMS)) // GPU_COMB_BUF_UPD
                DEF_IO32(0x104013C0, core->gpu.writeCombSrc<4>(IO_PARAMS)) // GPU_COMB4_SRC
                DEF_IO32(0x104013C4, core->gpu.writeCombOper<4>(IO_PARAMS)) // GPU_COMB4_OPER
                DEF_IO32(0x104013C8, core->gpu.writeCombMode<4>(IO_PARAMS)) // GPU_COMB4_MODE
                DEF_IO32(0x104013CC, core->gpu.writeCombColor<4>(IO_PARAMS)) // GPU_COMB4_COLOR
                DEF_IO32(0x104013E0, core->gpu.writeCombSrc<5>(IO_PARAMS)) // GPU_COMB5_SRC
                DEF_IO32(0x104013E4, core->gpu.writeCombOper<5>(IO_PARAMS)) // GPU_COMB5_OPER
                DEF_IO32(0x104013E8, core->gpu.writeCombMode<5>(IO_PARAMS)) // GPU_COMB5_MODE
                DEF_IO32(0x104013EC, core->gpu.writeCombColor<5>(IO_PARAMS)) // GPU_COMB5_COLOR
                DEF_IO32(0x104013F4, core->gpu.writeCombBufCol(IO_PARAMS)) // GPU_COMB_BUF_COL
                DEF_IO32(0x10401404, core->gpu.writeBlendFunc(IO_PARAMS)) // GPU_BLEND_FUNC
                DEF_IO32(0x1040140C, core->gpu.writeBlendColor(IO_PARAMS)) // GPU_BLEND_COLOR
                DEF_IO32(0x10401410, core->gpu.writeAlphaTest(IO_PARAMS)) // GPU_ALPHA_TEST
                DEF_IO32(0x10401414, core->gpu.writeStencilTest(IO_PARAMS)) // GPU_STENCIL_TEST
                DEF_IO32(0x10401418, core->gpu.writeStencilOp(IO_PARAMS)) // GPU_STENCIL_OP
                DEF_IO32(0x1040141C, core->gpu.writeDepcolMask(IO_PARAMS)) // GPU_DEPCOL_MASK
                DEF_IO32(0x1040144C, core->gpu.writeColbufWrite(IO_PARAMS)) // GPU_COLBUF_WRITE
                DEF_IO32(0x10401454, core->gpu.writeDepbufWrite(IO_PARAMS)) // GPU_DEPBUF_WRITE
                DEF_IO32(0x10401458, core->gpu.writeDepbufFmt(IO_PARAMS)) // GPU_DEPBUF_FMT
                DEF_IO32(0x1040145C, core->gpu.writeColbufFmt(IO_PARAMS)) // GPU_COLBUF_FMT
                DEF_IO32(0x10401470, core->gpu.writeDepbufLoc(IO_PARAMS)) // GPU_DEPBUF_LOC
                DEF_IO32(0x10401474, core->gpu.writeColbufLoc(IO_PARAMS)) // GPU_COLBUF_LOC
                DEF_IO32(0x10401478, core->gpu.writeBufferDim(IO_PARAMS)) // GPU_BUFFER_DIM
                DEF_IO32(0x10401800, core->gpu.writeAttrBase(IO_PARAMS)) // GPU_ATTR_BASE
                DEF_IO32(0x10401804, core->gpu.writeAttrFmtL(IO_PARAMS)) // GPU_ATTR_FMT_L
                DEF_IO32(0x10401808, core->gpu.writeAttrFmtH(IO_PARAMS)) // GPU_ATTR_FMT_H
                DEF_IO32(0x1040180C, core->gpu.writeAttrOfs<0>(IO_PARAMS)) // GPU_ATTR_OFS0
                DEF_IO32(0x10401810, core->gpu.writeAttrCfgL<0>(IO_PARAMS)) // GPU_ATTR_CFG0_L
                DEF_IO32(0x10401814, core->gpu.writeAttrCfgH<0>(IO_PARAMS)) // GPU_ATTR_CFG0_H
                DEF_IO32(0x10401818, core->gpu.writeAttrOfs<1>(IO_PARAMS)) // GPU_ATTR_OFS1
                DEF_IO32(0x1040181C, core->gpu.writeAttrCfgL<1>(IO_PARAMS)) // GPU_ATTR_CFG1_L
                DEF_IO32(0x10401820, core->gpu.writeAttrCfgH<1>(IO_PARAMS)) // GPU_ATTR_CFG1_H
                DEF_IO32(0x10401824, core->gpu.writeAttrOfs<2>(IO_PARAMS)) // GPU_ATTR_OFS2
                DEF_IO32(0x10401828, core->gpu.writeAttrCfgL<2>(IO_PARAMS)) // GPU_ATTR_CFG2_L
                DEF_IO32(0x1040182C, core->gpu.writeAttrCfgH<2>(IO_PARAMS)) // GPU_ATTR_CFG2_H
                DEF_IO32(0x10401830, core->gpu.writeAttrOfs<3>(IO_PARAMS)) // GPU_ATTR_OFS3
                DEF_IO32(0x10401834, core->gpu.writeAttrCfgL<3>(IO_PARAMS)) // GPU_ATTR_CFG3_L
                DEF_IO32(0x10401838, core->gpu.writeAttrCfgH<3>(IO_PARAMS)) // GPU_ATTR_CFG3_H
                DEF_IO32(0x1040183C, core->gpu.writeAttrOfs<4>(IO_PARAMS)) // GPU_ATTR_OFS4
                DEF_IO32(0x10401840, core->gpu.writeAttrCfgL<4>(IO_PARAMS)) // GPU_ATTR_CFG4_L
                DEF_IO32(0x10401844, core->gpu.writeAttrCfgH<4>(IO_PARAMS)) // GPU_ATTR_CFG4_H
                DEF_IO32(0x10401848, core->gpu.writeAttrOfs<5>(IO_PARAMS)) // GPU_ATTR_OFS5
                DEF_IO32(0x1040184C, core->gpu.writeAttrCfgL<5>(IO_PARAMS)) // GPU_ATTR_CFG5_L
                DEF_IO32(0x10401850, core->gpu.writeAttrCfgH<5>(IO_PARAMS)) // GPU_ATTR_CFG5_H
                DEF_IO32(0x10401854, core->gpu.writeAttrOfs<6>(IO_PARAMS)) // GPU_ATTR_OFS6
                DEF_IO32(0x10401858, core->gpu.writeAttrCfgL<6>(IO_PARAMS)) // GPU_ATTR_CFG6_L
                DEF_IO32(0x1040185C, core->gpu.writeAttrCfgH<6>(IO_PARAMS)) // GPU_ATTR_CFG6_H
                DEF_IO32(0x10401860, core->gpu.writeAttrOfs<7>(IO_PARAMS)) // GPU_ATTR_OFS7
                DEF_IO32(0x10401864, core->gpu.writeAttrCfgL<7>(IO_PARAMS)) // GPU_ATTR_CFG7_L
                DEF_IO32(0x10401868, core->gpu.writeAttrCfgH<7>(IO_PARAMS)) // GPU_ATTR_CFG7_H
                DEF_IO32(0x1040186C, core->gpu.writeAttrOfs<8>(IO_PARAMS)) // GPU_ATTR_OFS8
                DEF_IO32(0x10401870, core->gpu.writeAttrCfgL<8>(IO_PARAMS)) // GPU_ATTR_CFG8_L
                DEF_IO32(0x10401874, core->gpu.writeAttrCfgH<8>(IO_PARAMS)) // GPU_ATTR_CFG8_H
                DEF_IO32(0x10401878, core->gpu.writeAttrOfs<9>(IO_PARAMS)) // GPU_ATTR_OFS9
                DEF_IO32(0x1040187C, core->gpu.writeAttrCfgL<9>(IO_PARAMS)) // GPU_ATTR_CFG9_L
                DEF_IO32(0x10401880, core->gpu.writeAttrCfgH<9>(IO_PARAMS)) // GPU_ATTR_CFG9_H
                DEF_IO32(0x10401884, core->gpu.writeAttrOfs<10>(IO_PARAMS)) // GPU_ATTR_OFS10
                DEF_IO32(0x10401888, core->gpu.writeAttrCfgL<10>(IO_PARAMS)) // GPU_ATTR_CFG10_L
                DEF_IO32(0x1040188C, core->gpu.writeAttrCfgH<10>(IO_PARAMS)) // GPU_ATTR_CFG10_H
                DEF_IO32(0x10401890, core->gpu.writeAttrOfs<11>(IO_PARAMS)) // GPU_ATTR_OFS11
                DEF_IO32(0x10401894, core->gpu.writeAttrCfgL<11>(IO_PARAMS)) // GPU_ATTR_CFG11_L
                DEF_IO32(0x10401898, core->gpu.writeAttrCfgH<11>(IO_PARAMS)) // GPU_ATTR_CFG11_H
                DEF_IO32(0x1040189C, core->gpu.writeAttrIdxList(IO_PARAMS)) // GPU_ATTR_IDX_LIST
                DEF_IO32(0x104018A0, core->gpu.writeAttrNumVerts(IO_PARAMS)) // GPU_ATTR_NUM_VERTS
                DEF_IO32(0x104018A4, core->gpu.writeGshConfig(IO_PARAMS)) // GPU_GSH_CONFIG
                DEF_IO32(0x104018A8, core->gpu.writeAttrFirstIdx(IO_PARAMS)) // GPU_ATTR_FIRST_IDX
                DEF_IO32(0x104018B8, core->gpu.writeAttrDrawArrays(IO_PARAMS)) // GPU_ATTR_DRAW_ARRAYS
                DEF_IO32(0x104018BC, core->gpu.writeAttrDrawElems(IO_PARAMS)) // GPU_ATTR_DRAW_ELEMS
                DEF_IO32(0x104018C8, core->gpu.writeAttrFixedIdx(IO_PARAMS)) // GPU_ATTR_FIXED_IDX
                DEF_IO32(0x104018CC, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018D0, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018D4, core->gpu.writeAttrFixedData(IO_PARAMS)) // GPU_ATTR_FIXED_DATA
                DEF_IO32(0x104018E0, core->gpu.writeCmdSize<0>(IO_PARAMS)) // GPU_CMD_SIZE0
                DEF_IO32(0x104018E4, core->gpu.writeCmdSize<1>(IO_PARAMS)) // GPU_CMD_SIZE1
                DEF_IO32(0x104018E8, core->gpu.writeCmdAddr<0>(IO_PARAMS)) // GPU_CMD_ADDR0
                DEF_IO32(0x104018EC, core->gpu.writeCmdAddr<1>(IO_PARAMS)) // GPU_CMD_ADDR1
                DEF_IO32(0x104018F0, core->gpu.writeCmdJump<0>(IO_PARAMS)) // GPU_CMD_JUMP0
                DEF_IO32(0x104018F4, core->gpu.writeCmdJump<1>(IO_PARAMS)) // GPU_CMD_JUMP1
                DEF_IO32(0x10401908, core->gpu.writeVshNumAttr(IO_PARAMS)) // GPU_VSH_NUM_ATTR
                DEF_IO32(0x10401928, core->gpu.writeVshOutTotal(IO_PARAMS)) // GPU_VSH_OUT_TOTAL
                DEF_IO32(0x10401978, core->gpu.writePrimConfig(IO_PARAMS)) // GPU_PRIM_CONFIG
                DEF_IO32(0x1040197C, core->gpu.writePrimRestart(IO_PARAMS)) // GPU_PRIM_RESTART
                DEF_IO32(0x10401A00, core->gpu.writeGshBools(IO_PARAMS)) // GPU_GSH_BOOLS
                DEF_IO32(0x10401A04, core->gpu.writeGshInts<0>(IO_PARAMS)) // GPU_GSH_INTS0
                DEF_IO32(0x10401A08, core->gpu.writeGshInts<1>(IO_PARAMS)) // GPU_GSH_INTS1
                DEF_IO32(0x10401A0C, core->gpu.writeGshInts<2>(IO_PARAMS)) // GPU_GSH_INTS2
                DEF_IO32(0x10401A10, core->gpu.writeGshInts<3>(IO_PARAMS)) // GPU_GSH_INTS3
                DEF_IO32(0x10401A24, core->gpu.writeGshInputCfg(IO_PARAMS)) // GPU_GSH_INPUT_CFG
                DEF_IO32(0x10401A28, core->gpu.writeGshEntry(IO_PARAMS)) // GPU_GSH_ENTRY
                DEF_IO32(0x10401A2C, core->gpu.writeGshAttrIdsL(IO_PARAMS)) // GPU_GSH_ATTR_IDS_L
                DEF_IO32(0x10401A30, core->gpu.writeGshAttrIdsH(IO_PARAMS)) // GPU_GSH_ATTR_IDS_H
                DEF_IO32(0x10401A34, core->gpu.writeGshOutMask(IO_PARAMS)) // GPU_GSH_OUT_MASK
                DEF_IO32(0x10401A40, core->gpu.writeGshFloatIdx(IO_PARAMS)) // GPU_GSH_FLOAT_IDX
                DEF_IO32(0x10401A44, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A48, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A4C, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A50, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A54, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A58, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A5C, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A60, core->gpu.writeGshFloatData(IO_PARAMS)) // GPU_GSH_FLOAT_DATA
                DEF_IO32(0x10401A6C, core->gpu.writeGshCodeIdx(IO_PARAMS)) // GPU_GSH_CODE_IDX
                DEF_IO32(0x10401A70, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A74, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A78, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A7C, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A80, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A84, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A88, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A8C, core->gpu.writeGshCodeData(IO_PARAMS)) // GPU_GSH_CODE_DATA
                DEF_IO32(0x10401A94, core->gpu.writeGshDescIdx(IO_PARAMS)) // GPU_GSH_DESC_IDX
                DEF_IO32(0x10401A98, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401A9C, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AA0, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AA4, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AA8, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AAC, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AB0, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AB4, core->gpu.writeGshDescData(IO_PARAMS)) // GPU_GSH_DESC_DATA
                DEF_IO32(0x10401AC0, core->gpu.writeVshBools(IO_PARAMS)) // GPU_VSH_BOOLS
                DEF_IO32(0x10401AC4, core->gpu.writeVshInts<0>(IO_PARAMS)) // GPU_VSH_INTS0
                DEF_IO32(0x10401AC8, core->gpu.writeVshInts<1>(IO_PARAMS)) // GPU_VSH_INTS1
                DEF_IO32(0x10401ACC, core->gpu.writeVshInts<2>(IO_PARAMS)) // GPU_VSH_INTS2
                DEF_IO32(0x10401AD0, core->gpu.writeVshInts<3>(IO_PARAMS)) // GPU_VSH_INTS3
                DEF_IO32(0x10401AE8, core->gpu.writeVshEntry(IO_PARAMS)) // GPU_VSH_ENTRY
                DEF_IO32(0x10401AEC, core->gpu.writeVshAttrIdsL(IO_PARAMS)) // GPU_VSH_ATTR_IDS_L
                DEF_IO32(0x10401AF0, core->gpu.writeVshAttrIdsH(IO_PARAMS)) // GPU_VSH_ATTR_IDS_H
                DEF_IO32(0x10401AF4, core->gpu.writeVshOutMask(IO_PARAMS)) // GPU_VSH_OUT_MASK
                DEF_IO32(0x10401B00, core->gpu.writeVshFloatIdx(IO_PARAMS)) // GPU_VSH_FLOAT_IDX
                DEF_IO32(0x10401B04, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B08, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B0C, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B10, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B14, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B18, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B1C, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B20, core->gpu.writeVshFloatData(IO_PARAMS)) // GPU_VSH_FLOAT_DATA
                DEF_IO32(0x10401B2C, core->gpu.writeVshCodeIdx(IO_PARAMS)) // GPU_VSH_CODE_IDX
                DEF_IO32(0x10401B30, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B34, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B38, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B3C, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B40, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B44, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B48, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B4C, core->gpu.writeVshCodeData(IO_PARAMS)) // GPU_VSH_CODE_DATA
                DEF_IO32(0x10401B54, core->gpu.writeVshDescIdx(IO_PARAMS)) // GPU_VSH_DESC_IDX
                DEF_IO32(0x10401B58, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B5C, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B60, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B64, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B68, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B6C, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B70, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x10401B74, core->gpu.writeVshDescData(IO_PARAMS)) // GPU_VSH_DESC_DATA
                DEF_IO32(0x17E00100, core->interrupts.writeMpIle(id, IO_PARAMS)) // MP_ILE
                DEF_IO32(0x17E00104, core->interrupts.writeMpPrioMask(id, IO_PARAMS)) // MP_PRIO_MASK
                DEF_IO32(0x17E00110, core->interrupts.writeMpEoi(id, IO_PARAMS)) // MP_EOI
                DEF_IO32(0x17E00600, core->timers.writeMpReload(id, 0, IO_PARAMS)) // MP_RELOAD0
                DEF_IO32(0x17E00604, core->timers.writeMpCounter(id, 0, IO_PARAMS)) // MP_COUNTER0
                DEF_IO32(0x17E00608, core->timers.writeMpTmcnt(id, 0, IO_PARAMS)) // MP_TMCNT0
                DEF_IO32(0x17E0060C, core->timers.writeMpTmirq(id, 0, IO_PARAMS)) // MP_TMIRQ0
                DEF_IO32(0x17E00620, core->timers.writeMpReload(id, 1, IO_PARAMS)) // MP_RELOAD1
                DEF_IO32(0x17E00624, core->timers.writeMpCounter(id, 1, IO_PARAMS)) // MP_COUNTER1
                DEF_IO32(0x17E00628, core->timers.writeMpTmcnt(id, 1, IO_PARAMS)) // MP_TMCNT1
                DEF_IO32(0x17E0062C, core->timers.writeMpTmirq(id, 1, IO_PARAMS)) // MP_TMIRQ1
                DEF_IO32(0x17E00700, core->timers.writeMpReload(ARM11A, 0, IO_PARAMS)) // MP0_RELOAD0
                DEF_IO32(0x17E00704, core->timers.writeMpCounter(ARM11A, 0, IO_PARAMS)) // MP0_COUNTER0
                DEF_IO32(0x17E00708, core->timers.writeMpTmcnt(ARM11A, 0, IO_PARAMS)) // MP0_TMCNT0
                DEF_IO32(0x17E0070C, core->timers.writeMpTmirq(ARM11A, 0, IO_PARAMS)) // MP0_TMIRQ0
                DEF_IO32(0x17E00720, core->timers.writeMpReload(ARM11A, 1, IO_PARAMS)) // MP0_RELOAD1
                DEF_IO32(0x17E00724, core->timers.writeMpCounter(ARM11A, 1, IO_PARAMS)) // MP0_COUNTER1
                DEF_IO32(0x17E00728, core->timers.writeMpTmcnt(ARM11A, 1, IO_PARAMS)) // MP0_TMCNT1
                DEF_IO32(0x17E0072C, core->timers.writeMpTmirq(ARM11A, 1, IO_PARAMS)) // MP0_TMIRQ1
                DEF_IO32(0x17E00800, core->timers.writeMpReload(ARM11B, 0, IO_PARAMS)) // MP1_RELOAD0
                DEF_IO32(0x17E00804, core->timers.writeMpCounter(ARM11B, 0, IO_PARAMS)) // MP1_COUNTER0
                DEF_IO32(0x17E00808, core->timers.writeMpTmcnt(ARM11B, 0, IO_PARAMS)) // MP1_TMCNT0
                DEF_IO32(0x17E0080C, core->timers.writeMpTmirq(ARM11B, 0, IO_PARAMS)) // MP1_TMIRQ0
                DEF_IO32(0x17E00820, core->timers.writeMpReload(ARM11B, 1, IO_PARAMS)) // MP1_RELOAD1
                DEF_IO32(0x17E00824, core->timers.writeMpCounter(ARM11B, 1, IO_PARAMS)) // MP1_COUNTER1
                DEF_IO32(0x17E00828, core->timers.writeMpTmcnt(ARM11B, 1, IO_PARAMS)) // MP1_TMCNT1
                DEF_IO32(0x17E0082C, core->timers.writeMpTmirq(ARM11B, 1, IO_PARAMS)) // MP1_TMIRQ1
                DEF_IO32(0x17E00900, core->timers.writeMpReload(ARM11C, 0, IO_PARAMS)) // MP2_RELOAD0
                DEF_IO32(0x17E00904, core->timers.writeMpCounter(ARM11C, 0, IO_PARAMS)) // MP2_COUNTER0
                DEF_IO32(0x17E00908, core->timers.writeMpTmcnt(ARM11C, 0, IO_PARAMS)) // MP2_TMCNT0
                DEF_IO32(0x17E0090C, core->timers.writeMpTmirq(ARM11C, 0, IO_PARAMS)) // MP2_TMIRQ0
                DEF_IO32(0x17E00920, core->timers.writeMpReload(ARM11C, 1, IO_PARAMS)) // MP2_RELOAD1
                DEF_IO32(0x17E00924, core->timers.writeMpCounter(ARM11C, 1, IO_PARAMS)) // MP2_COUNTER1
                DEF_IO32(0x17E00928, core->timers.writeMpTmcnt(ARM11C, 1, IO_PARAMS)) // MP2_TMCNT1
                DEF_IO32(0x17E0092C, core->timers.writeMpTmirq(ARM11C, 1, IO_PARAMS)) // MP2_TMIRQ1
                DEF_IO32(0x17E00A00, core->timers.writeMpReload(ARM11D, 0, IO_PARAMS)) // MP3_RELOAD0
                DEF_IO32(0x17E00A04, core->timers.writeMpCounter(ARM11D, 0, IO_PARAMS)) // MP3_COUNTER0
                DEF_IO32(0x17E00A08, core->timers.writeMpTmcnt(ARM11D, 0, IO_PARAMS)) // MP3_TMCNT0
                DEF_IO32(0x17E00A0C, core->timers.writeMpTmirq(ARM11D, 0, IO_PARAMS)) // MP3_TMIRQ0
                DEF_IO32(0x17E00A20, core->timers.writeMpReload(ARM11D, 1, IO_PARAMS)) // MP3_RELOAD1
                DEF_IO32(0x17E00A24, core->timers.writeMpCounter(ARM11D, 1, IO_PARAMS)) // MP3_COUNTER1
                DEF_IO32(0x17E00A28, core->timers.writeMpTmcnt(ARM11D, 1, IO_PARAMS)) // MP3_TMCNT1
                DEF_IO32(0x17E00A2C, core->timers.writeMpTmirq(ARM11D, 1, IO_PARAMS)) // MP3_TMIRQ1
                DEF_IO32(0x17E01000, core->interrupts.writeMpIge(IO_PARAMS)) // MP_IGE
                DEF_IO32(0x17E01100, core->interrupts.writeMpIeSet(0, IO_PARAMS)) // MP_IE0_SET
                DEF_IO32(0x17E01104, core->interrupts.writeMpIeSet(1, IO_PARAMS)) // MP_IE1_SET
                DEF_IO32(0x17E01108, core->interrupts.writeMpIeSet(2, IO_PARAMS)) // MP_IE2_SET
                DEF_IO32(0x17E0110C, core->interrupts.writeMpIeSet(3, IO_PARAMS)) // MP_IE3_SET
                DEF_IO32(0x17E01180, core->interrupts.writeMpIeClear(0, IO_PARAMS)) // MP_IE0_CLEAR
                DEF_IO32(0x17E01184, core->interrupts.writeMpIeClear(1, IO_PARAMS)) // MP_IE1_CLEAR
                DEF_IO32(0x17E01188, core->interrupts.writeMpIeClear(2, IO_PARAMS)) // MP_IE2_CLEAR
                DEF_IO32(0x17E0118C, core->interrupts.writeMpIeClear(3, IO_PARAMS)) // MP_IE3_CLEAR
                DEF_IO32(0x17E01200, core->interrupts.writeMpIpSet(0, IO_PARAMS)) // MP_IP0_SET
                DEF_IO32(0x17E01204, core->interrupts.writeMpIpSet(1, IO_PARAMS)) // MP_IP1_SET
                DEF_IO32(0x17E01208, core->interrupts.writeMpIpSet(2, IO_PARAMS)) // MP_IP2_SET
                DEF_IO32(0x17E0120C, core->interrupts.writeMpIpSet(3, IO_PARAMS)) // MP_IP3_SET
                DEF_IO32(0x17E01280, core->interrupts.writeMpIpClear(0, IO_PARAMS)) // MP_IP0_CLEAR
                DEF_IO32(0x17E01284, core->interrupts.writeMpIpClear(1, IO_PARAMS)) // MP_IP1_CLEAR
                DEF_IO32(0x17E01288, core->interrupts.writeMpIpClear(2, IO_PARAMS)) // MP_IP2_CLEAR
                DEF_IO32(0x17E0128C, core->interrupts.writeMpIpClear(3, IO_PARAMS)) // MP_IP3_CLEAR
                DEF_IO08(0x17E01400, core->interrupts.writeMpPriorityL(id, 0, IO_PARAMS8)) // MP_PRIORITY0
                DEF_IO08(0x17E01401, core->interrupts.writeMpPriorityL(id, 1, IO_PARAMS8)) // MP_PRIORITY1
                DEF_IO08(0x17E01402, core->interrupts.writeMpPriorityL(id, 2, IO_PARAMS8)) // MP_PRIORITY2
                DEF_IO08(0x17E01403, core->interrupts.writeMpPriorityL(id, 3, IO_PARAMS8)) // MP_PRIORITY3
                DEF_IO08(0x17E01404, core->interrupts.writeMpPriorityL(id, 4, IO_PARAMS8)) // MP_PRIORITY4
                DEF_IO08(0x17E01405, core->interrupts.writeMpPriorityL(id, 5, IO_PARAMS8)) // MP_PRIORITY5
                DEF_IO08(0x17E01406, core->interrupts.writeMpPriorityL(id, 6, IO_PARAMS8)) // MP_PRIORITY6
                DEF_IO08(0x17E01407, core->interrupts.writeMpPriorityL(id, 7, IO_PARAMS8)) // MP_PRIORITY7
                DEF_IO08(0x17E01408, core->interrupts.writeMpPriorityL(id, 8, IO_PARAMS8)) // MP_PRIORITY8
                DEF_IO08(0x17E01409, core->interrupts.writeMpPriorityL(id, 9, IO_PARAMS8)) // MP_PRIORITY9
                DEF_IO08(0x17E0140A, core->interrupts.writeMpPriorityL(id, 10, IO_PARAMS8)) // MP_PRIORITY10
                DEF_IO08(0x17E0140B, core->interrupts.writeMpPriorityL(id, 11, IO_PARAMS8)) // MP_PRIORITY11
                DEF_IO08(0x17E0140C, core->interrupts.writeMpPriorityL(id, 12, IO_PARAMS8)) // MP_PRIORITY12
                DEF_IO08(0x17E0140D, core->interrupts.writeMpPriorityL(id, 13, IO_PARAMS8)) // MP_PRIORITY13
                DEF_IO08(0x17E0140E, core->interrupts.writeMpPriorityL(id, 14, IO_PARAMS8)) // MP_PRIORITY14
                DEF_IO08(0x17E0140F, core->interrupts.writeMpPriorityL(id, 15, IO_PARAMS8)) // MP_PRIORITY15
                DEF_IO08(0x17E01410, core->interrupts.writeMpPriorityL(id, 16, IO_PARAMS8)) // MP_PRIORITY16
                DEF_IO08(0x17E01411, core->interrupts.writeMpPriorityL(id, 17, IO_PARAMS8)) // MP_PRIORITY17
                DEF_IO08(0x17E01412, core->interrupts.writeMpPriorityL(id, 18, IO_PARAMS8)) // MP_PRIORITY18
                DEF_IO08(0x17E01413, core->interrupts.writeMpPriorityL(id, 19, IO_PARAMS8)) // MP_PRIORITY19
                DEF_IO08(0x17E01414, core->interrupts.writeMpPriorityL(id, 20, IO_PARAMS8)) // MP_PRIORITY20
                DEF_IO08(0x17E01415, core->interrupts.writeMpPriorityL(id, 21, IO_PARAMS8)) // MP_PRIORITY21
                DEF_IO08(0x17E01416, core->interrupts.writeMpPriorityL(id, 22, IO_PARAMS8)) // MP_PRIORITY22
                DEF_IO08(0x17E01417, core->interrupts.writeMpPriorityL(id, 23, IO_PARAMS8)) // MP_PRIORITY23
                DEF_IO08(0x17E01418, core->interrupts.writeMpPriorityL(id, 24, IO_PARAMS8)) // MP_PRIORITY24
                DEF_IO08(0x17E01419, core->interrupts.writeMpPriorityL(id, 25, IO_PARAMS8)) // MP_PRIORITY25
                DEF_IO08(0x17E0141A, core->interrupts.writeMpPriorityL(id, 26, IO_PARAMS8)) // MP_PRIORITY26
                DEF_IO08(0x17E0141B, core->interrupts.writeMpPriorityL(id, 27, IO_PARAMS8)) // MP_PRIORITY27
                DEF_IO08(0x17E0141C, core->interrupts.writeMpPriorityL(id, 28, IO_PARAMS8)) // MP_PRIORITY28
                DEF_IO08(0x17E0141D, core->interrupts.writeMpPriorityL(id, 29, IO_PARAMS8)) // MP_PRIORITY29
                DEF_IO08(0x17E0141E, core->interrupts.writeMpPriorityL(id, 30, IO_PARAMS8)) // MP_PRIORITY30
                DEF_IO08(0x17E0141F, core->interrupts.writeMpPriorityL(id, 31, IO_PARAMS8)) // MP_PRIORITY31
                DEF_IO08(0x17E01420, core->interrupts.writeMpPriorityG(32, IO_PARAMS8)) // MP_PRIORITY32
                DEF_IO08(0x17E01421, core->interrupts.writeMpPriorityG(33, IO_PARAMS8)) // MP_PRIORITY33
                DEF_IO08(0x17E01422, core->interrupts.writeMpPriorityG(34, IO_PARAMS8)) // MP_PRIORITY34
                DEF_IO08(0x17E01423, core->interrupts.writeMpPriorityG(35, IO_PARAMS8)) // MP_PRIORITY35
                DEF_IO08(0x17E01424, core->interrupts.writeMpPriorityG(36, IO_PARAMS8)) // MP_PRIORITY36
                DEF_IO08(0x17E01425, core->interrupts.writeMpPriorityG(37, IO_PARAMS8)) // MP_PRIORITY37
                DEF_IO08(0x17E01426, core->interrupts.writeMpPriorityG(38, IO_PARAMS8)) // MP_PRIORITY38
                DEF_IO08(0x17E01427, core->interrupts.writeMpPriorityG(39, IO_PARAMS8)) // MP_PRIORITY39
                DEF_IO08(0x17E01428, core->interrupts.writeMpPriorityG(40, IO_PARAMS8)) // MP_PRIORITY40
                DEF_IO08(0x17E01429, core->interrupts.writeMpPriorityG(41, IO_PARAMS8)) // MP_PRIORITY41
                DEF_IO08(0x17E0142A, core->interrupts.writeMpPriorityG(42, IO_PARAMS8)) // MP_PRIORITY42
                DEF_IO08(0x17E0142B, core->interrupts.writeMpPriorityG(43, IO_PARAMS8)) // MP_PRIORITY43
                DEF_IO08(0x17E0142C, core->interrupts.writeMpPriorityG(44, IO_PARAMS8)) // MP_PRIORITY44
                DEF_IO08(0x17E0142D, core->interrupts.writeMpPriorityG(45, IO_PARAMS8)) // MP_PRIORITY45
                DEF_IO08(0x17E0142E, core->interrupts.writeMpPriorityG(46, IO_PARAMS8)) // MP_PRIORITY46
                DEF_IO08(0x17E0142F, core->interrupts.writeMpPriorityG(47, IO_PARAMS8)) // MP_PRIORITY47
                DEF_IO08(0x17E01430, core->interrupts.writeMpPriorityG(48, IO_PARAMS8)) // MP_PRIORITY48
                DEF_IO08(0x17E01431, core->interrupts.writeMpPriorityG(49, IO_PARAMS8)) // MP_PRIORITY49
                DEF_IO08(0x17E01432, core->interrupts.writeMpPriorityG(50, IO_PARAMS8)) // MP_PRIORITY50
                DEF_IO08(0x17E01433, core->interrupts.writeMpPriorityG(51, IO_PARAMS8)) // MP_PRIORITY51
                DEF_IO08(0x17E01434, core->interrupts.writeMpPriorityG(52, IO_PARAMS8)) // MP_PRIORITY52
                DEF_IO08(0x17E01435, core->interrupts.writeMpPriorityG(53, IO_PARAMS8)) // MP_PRIORITY53
                DEF_IO08(0x17E01436, core->interrupts.writeMpPriorityG(54, IO_PARAMS8)) // MP_PRIORITY54
                DEF_IO08(0x17E01437, core->interrupts.writeMpPriorityG(55, IO_PARAMS8)) // MP_PRIORITY55
                DEF_IO08(0x17E01438, core->interrupts.writeMpPriorityG(56, IO_PARAMS8)) // MP_PRIORITY56
                DEF_IO08(0x17E01439, core->interrupts.writeMpPriorityG(57, IO_PARAMS8)) // MP_PRIORITY57
                DEF_IO08(0x17E0143A, core->interrupts.writeMpPriorityG(58, IO_PARAMS8)) // MP_PRIORITY58
                DEF_IO08(0x17E0143B, core->interrupts.writeMpPriorityG(59, IO_PARAMS8)) // MP_PRIORITY59
                DEF_IO08(0x17E0143C, core->interrupts.writeMpPriorityG(60, IO_PARAMS8)) // MP_PRIORITY60
                DEF_IO08(0x17E0143D, core->interrupts.writeMpPriorityG(61, IO_PARAMS8)) // MP_PRIORITY61
                DEF_IO08(0x17E0143E, core->interrupts.writeMpPriorityG(62, IO_PARAMS8)) // MP_PRIORITY62
                DEF_IO08(0x17E0143F, core->interrupts.writeMpPriorityG(63, IO_PARAMS8)) // MP_PRIORITY63
                DEF_IO08(0x17E01440, core->interrupts.writeMpPriorityG(64, IO_PARAMS8)) // MP_PRIORITY64
                DEF_IO08(0x17E01441, core->interrupts.writeMpPriorityG(65, IO_PARAMS8)) // MP_PRIORITY65
                DEF_IO08(0x17E01442, core->interrupts.writeMpPriorityG(66, IO_PARAMS8)) // MP_PRIORITY66
                DEF_IO08(0x17E01443, core->interrupts.writeMpPriorityG(67, IO_PARAMS8)) // MP_PRIORITY67
                DEF_IO08(0x17E01444, core->interrupts.writeMpPriorityG(68, IO_PARAMS8)) // MP_PRIORITY68
                DEF_IO08(0x17E01445, core->interrupts.writeMpPriorityG(69, IO_PARAMS8)) // MP_PRIORITY69
                DEF_IO08(0x17E01446, core->interrupts.writeMpPriorityG(70, IO_PARAMS8)) // MP_PRIORITY70
                DEF_IO08(0x17E01447, core->interrupts.writeMpPriorityG(71, IO_PARAMS8)) // MP_PRIORITY71
                DEF_IO08(0x17E01448, core->interrupts.writeMpPriorityG(72, IO_PARAMS8)) // MP_PRIORITY72
                DEF_IO08(0x17E01449, core->interrupts.writeMpPriorityG(73, IO_PARAMS8)) // MP_PRIORITY73
                DEF_IO08(0x17E0144A, core->interrupts.writeMpPriorityG(74, IO_PARAMS8)) // MP_PRIORITY74
                DEF_IO08(0x17E0144B, core->interrupts.writeMpPriorityG(75, IO_PARAMS8)) // MP_PRIORITY75
                DEF_IO08(0x17E0144C, core->interrupts.writeMpPriorityG(76, IO_PARAMS8)) // MP_PRIORITY76
                DEF_IO08(0x17E0144D, core->interrupts.writeMpPriorityG(77, IO_PARAMS8)) // MP_PRIORITY77
                DEF_IO08(0x17E0144E, core->interrupts.writeMpPriorityG(78, IO_PARAMS8)) // MP_PRIORITY78
                DEF_IO08(0x17E0144F, core->interrupts.writeMpPriorityG(79, IO_PARAMS8)) // MP_PRIORITY79
                DEF_IO08(0x17E01450, core->interrupts.writeMpPriorityG(80, IO_PARAMS8)) // MP_PRIORITY80
                DEF_IO08(0x17E01451, core->interrupts.writeMpPriorityG(81, IO_PARAMS8)) // MP_PRIORITY81
                DEF_IO08(0x17E01452, core->interrupts.writeMpPriorityG(82, IO_PARAMS8)) // MP_PRIORITY82
                DEF_IO08(0x17E01453, core->interrupts.writeMpPriorityG(83, IO_PARAMS8)) // MP_PRIORITY83
                DEF_IO08(0x17E01454, core->interrupts.writeMpPriorityG(84, IO_PARAMS8)) // MP_PRIORITY84
                DEF_IO08(0x17E01455, core->interrupts.writeMpPriorityG(85, IO_PARAMS8)) // MP_PRIORITY85
                DEF_IO08(0x17E01456, core->interrupts.writeMpPriorityG(86, IO_PARAMS8)) // MP_PRIORITY86
                DEF_IO08(0x17E01457, core->interrupts.writeMpPriorityG(87, IO_PARAMS8)) // MP_PRIORITY87
                DEF_IO08(0x17E01458, core->interrupts.writeMpPriorityG(88, IO_PARAMS8)) // MP_PRIORITY88
                DEF_IO08(0x17E01459, core->interrupts.writeMpPriorityG(89, IO_PARAMS8)) // MP_PRIORITY89
                DEF_IO08(0x17E0145A, core->interrupts.writeMpPriorityG(90, IO_PARAMS8)) // MP_PRIORITY90
                DEF_IO08(0x17E0145B, core->interrupts.writeMpPriorityG(91, IO_PARAMS8)) // MP_PRIORITY91
                DEF_IO08(0x17E0145C, core->interrupts.writeMpPriorityG(92, IO_PARAMS8)) // MP_PRIORITY92
                DEF_IO08(0x17E0145D, core->interrupts.writeMpPriorityG(93, IO_PARAMS8)) // MP_PRIORITY93
                DEF_IO08(0x17E0145E, core->interrupts.writeMpPriorityG(94, IO_PARAMS8)) // MP_PRIORITY94
                DEF_IO08(0x17E0145F, core->interrupts.writeMpPriorityG(95, IO_PARAMS8)) // MP_PRIORITY95
                DEF_IO08(0x17E01460, core->interrupts.writeMpPriorityG(96, IO_PARAMS8)) // MP_PRIORITY96
                DEF_IO08(0x17E01461, core->interrupts.writeMpPriorityG(97, IO_PARAMS8)) // MP_PRIORITY97
                DEF_IO08(0x17E01462, core->interrupts.writeMpPriorityG(98, IO_PARAMS8)) // MP_PRIORITY98
                DEF_IO08(0x17E01463, core->interrupts.writeMpPriorityG(99, IO_PARAMS8)) // MP_PRIORITY99
                DEF_IO08(0x17E01464, core->interrupts.writeMpPriorityG(100, IO_PARAMS8)) // MP_PRIORITY100
                DEF_IO08(0x17E01465, core->interrupts.writeMpPriorityG(101, IO_PARAMS8)) // MP_PRIORITY101
                DEF_IO08(0x17E01466, core->interrupts.writeMpPriorityG(102, IO_PARAMS8)) // MP_PRIORITY102
                DEF_IO08(0x17E01467, core->interrupts.writeMpPriorityG(103, IO_PARAMS8)) // MP_PRIORITY103
                DEF_IO08(0x17E01468, core->interrupts.writeMpPriorityG(104, IO_PARAMS8)) // MP_PRIORITY104
                DEF_IO08(0x17E01469, core->interrupts.writeMpPriorityG(105, IO_PARAMS8)) // MP_PRIORITY105
                DEF_IO08(0x17E0146A, core->interrupts.writeMpPriorityG(106, IO_PARAMS8)) // MP_PRIORITY106
                DEF_IO08(0x17E0146B, core->interrupts.writeMpPriorityG(107, IO_PARAMS8)) // MP_PRIORITY107
                DEF_IO08(0x17E0146C, core->interrupts.writeMpPriorityG(108, IO_PARAMS8)) // MP_PRIORITY108
                DEF_IO08(0x17E0146D, core->interrupts.writeMpPriorityG(109, IO_PARAMS8)) // MP_PRIORITY109
                DEF_IO08(0x17E0146E, core->interrupts.writeMpPriorityG(110, IO_PARAMS8)) // MP_PRIORITY110
                DEF_IO08(0x17E0146F, core->interrupts.writeMpPriorityG(111, IO_PARAMS8)) // MP_PRIORITY111
                DEF_IO08(0x17E01470, core->interrupts.writeMpPriorityG(112, IO_PARAMS8)) // MP_PRIORITY112
                DEF_IO08(0x17E01471, core->interrupts.writeMpPriorityG(113, IO_PARAMS8)) // MP_PRIORITY113
                DEF_IO08(0x17E01472, core->interrupts.writeMpPriorityG(114, IO_PARAMS8)) // MP_PRIORITY114
                DEF_IO08(0x17E01473, core->interrupts.writeMpPriorityG(115, IO_PARAMS8)) // MP_PRIORITY115
                DEF_IO08(0x17E01474, core->interrupts.writeMpPriorityG(116, IO_PARAMS8)) // MP_PRIORITY116
                DEF_IO08(0x17E01475, core->interrupts.writeMpPriorityG(117, IO_PARAMS8)) // MP_PRIORITY117
                DEF_IO08(0x17E01476, core->interrupts.writeMpPriorityG(118, IO_PARAMS8)) // MP_PRIORITY118
                DEF_IO08(0x17E01477, core->interrupts.writeMpPriorityG(119, IO_PARAMS8)) // MP_PRIORITY119
                DEF_IO08(0x17E01478, core->interrupts.writeMpPriorityG(120, IO_PARAMS8)) // MP_PRIORITY120
                DEF_IO08(0x17E01479, core->interrupts.writeMpPriorityG(121, IO_PARAMS8)) // MP_PRIORITY121
                DEF_IO08(0x17E0147A, core->interrupts.writeMpPriorityG(122, IO_PARAMS8)) // MP_PRIORITY122
                DEF_IO08(0x17E0147B, core->interrupts.writeMpPriorityG(123, IO_PARAMS8)) // MP_PRIORITY123
                DEF_IO08(0x17E0147C, core->interrupts.writeMpPriorityG(124, IO_PARAMS8)) // MP_PRIORITY124
                DEF_IO08(0x17E0147D, core->interrupts.writeMpPriorityG(125, IO_PARAMS8)) // MP_PRIORITY125
                DEF_IO08(0x17E0147E, core->interrupts.writeMpPriorityG(126, IO_PARAMS8)) // MP_PRIORITY126
                DEF_IO08(0x17E0147F, core->interrupts.writeMpPriorityG(127, IO_PARAMS8)) // MP_PRIORITY127
                DEF_IO08(0x17E01800, core->interrupts.writeMpTarget(0, IO_PARAMS8)) // MP_TARGET0
                DEF_IO08(0x17E01801, core->interrupts.writeMpTarget(1, IO_PARAMS8)) // MP_TARGET1
                DEF_IO08(0x17E01802, core->interrupts.writeMpTarget(2, IO_PARAMS8)) // MP_TARGET2
                DEF_IO08(0x17E01803, core->interrupts.writeMpTarget(3, IO_PARAMS8)) // MP_TARGET3
                DEF_IO08(0x17E01804, core->interrupts.writeMpTarget(4, IO_PARAMS8)) // MP_TARGET4
                DEF_IO08(0x17E01805, core->interrupts.writeMpTarget(5, IO_PARAMS8)) // MP_TARGET5
                DEF_IO08(0x17E01806, core->interrupts.writeMpTarget(6, IO_PARAMS8)) // MP_TARGET6
                DEF_IO08(0x17E01807, core->interrupts.writeMpTarget(7, IO_PARAMS8)) // MP_TARGET7
                DEF_IO08(0x17E01808, core->interrupts.writeMpTarget(8, IO_PARAMS8)) // MP_TARGET8
                DEF_IO08(0x17E01809, core->interrupts.writeMpTarget(9, IO_PARAMS8)) // MP_TARGET9
                DEF_IO08(0x17E0180A, core->interrupts.writeMpTarget(10, IO_PARAMS8)) // MP_TARGET10
                DEF_IO08(0x17E0180B, core->interrupts.writeMpTarget(11, IO_PARAMS8)) // MP_TARGET11
                DEF_IO08(0x17E0180C, core->interrupts.writeMpTarget(12, IO_PARAMS8)) // MP_TARGET12
                DEF_IO08(0x17E0180D, core->interrupts.writeMpTarget(13, IO_PARAMS8)) // MP_TARGET13
                DEF_IO08(0x17E0180E, core->interrupts.writeMpTarget(14, IO_PARAMS8)) // MP_TARGET14
                DEF_IO08(0x17E0180F, core->interrupts.writeMpTarget(15, IO_PARAMS8)) // MP_TARGET15
                DEF_IO08(0x17E01810, core->interrupts.writeMpTarget(16, IO_PARAMS8)) // MP_TARGET16
                DEF_IO08(0x17E01811, core->interrupts.writeMpTarget(17, IO_PARAMS8)) // MP_TARGET17
                DEF_IO08(0x17E01812, core->interrupts.writeMpTarget(18, IO_PARAMS8)) // MP_TARGET18
                DEF_IO08(0x17E01813, core->interrupts.writeMpTarget(19, IO_PARAMS8)) // MP_TARGET19
                DEF_IO08(0x17E01814, core->interrupts.writeMpTarget(20, IO_PARAMS8)) // MP_TARGET20
                DEF_IO08(0x17E01815, core->interrupts.writeMpTarget(21, IO_PARAMS8)) // MP_TARGET21
                DEF_IO08(0x17E01816, core->interrupts.writeMpTarget(22, IO_PARAMS8)) // MP_TARGET22
                DEF_IO08(0x17E01817, core->interrupts.writeMpTarget(23, IO_PARAMS8)) // MP_TARGET23
                DEF_IO08(0x17E01818, core->interrupts.writeMpTarget(24, IO_PARAMS8)) // MP_TARGET24
                DEF_IO08(0x17E01819, core->interrupts.writeMpTarget(25, IO_PARAMS8)) // MP_TARGET25
                DEF_IO08(0x17E0181A, core->interrupts.writeMpTarget(26, IO_PARAMS8)) // MP_TARGET26
                DEF_IO08(0x17E0181B, core->interrupts.writeMpTarget(27, IO_PARAMS8)) // MP_TARGET27
                DEF_IO08(0x17E0181C, core->interrupts.writeMpTarget(28, IO_PARAMS8)) // MP_TARGET28
                DEF_IO08(0x17E0181D, core->interrupts.writeMpTarget(29, IO_PARAMS8)) // MP_TARGET29
                DEF_IO08(0x17E0181E, core->interrupts.writeMpTarget(30, IO_PARAMS8)) // MP_TARGET30
                DEF_IO08(0x17E0181F, core->interrupts.writeMpTarget(31, IO_PARAMS8)) // MP_TARGET31
                DEF_IO08(0x17E01820, core->interrupts.writeMpTarget(32, IO_PARAMS8)) // MP_TARGET32
                DEF_IO08(0x17E01821, core->interrupts.writeMpTarget(33, IO_PARAMS8)) // MP_TARGET33
                DEF_IO08(0x17E01822, core->interrupts.writeMpTarget(34, IO_PARAMS8)) // MP_TARGET34
                DEF_IO08(0x17E01823, core->interrupts.writeMpTarget(35, IO_PARAMS8)) // MP_TARGET35
                DEF_IO08(0x17E01824, core->interrupts.writeMpTarget(36, IO_PARAMS8)) // MP_TARGET36
                DEF_IO08(0x17E01825, core->interrupts.writeMpTarget(37, IO_PARAMS8)) // MP_TARGET37
                DEF_IO08(0x17E01826, core->interrupts.writeMpTarget(38, IO_PARAMS8)) // MP_TARGET38
                DEF_IO08(0x17E01827, core->interrupts.writeMpTarget(39, IO_PARAMS8)) // MP_TARGET39
                DEF_IO08(0x17E01828, core->interrupts.writeMpTarget(40, IO_PARAMS8)) // MP_TARGET40
                DEF_IO08(0x17E01829, core->interrupts.writeMpTarget(41, IO_PARAMS8)) // MP_TARGET41
                DEF_IO08(0x17E0182A, core->interrupts.writeMpTarget(42, IO_PARAMS8)) // MP_TARGET42
                DEF_IO08(0x17E0182B, core->interrupts.writeMpTarget(43, IO_PARAMS8)) // MP_TARGET43
                DEF_IO08(0x17E0182C, core->interrupts.writeMpTarget(44, IO_PARAMS8)) // MP_TARGET44
                DEF_IO08(0x17E0182D, core->interrupts.writeMpTarget(45, IO_PARAMS8)) // MP_TARGET45
                DEF_IO08(0x17E0182E, core->interrupts.writeMpTarget(46, IO_PARAMS8)) // MP_TARGET46
                DEF_IO08(0x17E0182F, core->interrupts.writeMpTarget(47, IO_PARAMS8)) // MP_TARGET47
                DEF_IO08(0x17E01830, core->interrupts.writeMpTarget(48, IO_PARAMS8)) // MP_TARGET48
                DEF_IO08(0x17E01831, core->interrupts.writeMpTarget(49, IO_PARAMS8)) // MP_TARGET49
                DEF_IO08(0x17E01832, core->interrupts.writeMpTarget(50, IO_PARAMS8)) // MP_TARGET50
                DEF_IO08(0x17E01833, core->interrupts.writeMpTarget(51, IO_PARAMS8)) // MP_TARGET51
                DEF_IO08(0x17E01834, core->interrupts.writeMpTarget(52, IO_PARAMS8)) // MP_TARGET52
                DEF_IO08(0x17E01835, core->interrupts.writeMpTarget(53, IO_PARAMS8)) // MP_TARGET53
                DEF_IO08(0x17E01836, core->interrupts.writeMpTarget(54, IO_PARAMS8)) // MP_TARGET54
                DEF_IO08(0x17E01837, core->interrupts.writeMpTarget(55, IO_PARAMS8)) // MP_TARGET55
                DEF_IO08(0x17E01838, core->interrupts.writeMpTarget(56, IO_PARAMS8)) // MP_TARGET56
                DEF_IO08(0x17E01839, core->interrupts.writeMpTarget(57, IO_PARAMS8)) // MP_TARGET57
                DEF_IO08(0x17E0183A, core->interrupts.writeMpTarget(58, IO_PARAMS8)) // MP_TARGET58
                DEF_IO08(0x17E0183B, core->interrupts.writeMpTarget(59, IO_PARAMS8)) // MP_TARGET59
                DEF_IO08(0x17E0183C, core->interrupts.writeMpTarget(60, IO_PARAMS8)) // MP_TARGET60
                DEF_IO08(0x17E0183D, core->interrupts.writeMpTarget(61, IO_PARAMS8)) // MP_TARGET61
                DEF_IO08(0x17E0183E, core->interrupts.writeMpTarget(62, IO_PARAMS8)) // MP_TARGET62
                DEF_IO08(0x17E0183F, core->interrupts.writeMpTarget(63, IO_PARAMS8)) // MP_TARGET63
                DEF_IO08(0x17E01840, core->interrupts.writeMpTarget(64, IO_PARAMS8)) // MP_TARGET64
                DEF_IO08(0x17E01841, core->interrupts.writeMpTarget(65, IO_PARAMS8)) // MP_TARGET65
                DEF_IO08(0x17E01842, core->interrupts.writeMpTarget(66, IO_PARAMS8)) // MP_TARGET66
                DEF_IO08(0x17E01843, core->interrupts.writeMpTarget(67, IO_PARAMS8)) // MP_TARGET67
                DEF_IO08(0x17E01844, core->interrupts.writeMpTarget(68, IO_PARAMS8)) // MP_TARGET68
                DEF_IO08(0x17E01845, core->interrupts.writeMpTarget(69, IO_PARAMS8)) // MP_TARGET69
                DEF_IO08(0x17E01846, core->interrupts.writeMpTarget(70, IO_PARAMS8)) // MP_TARGET70
                DEF_IO08(0x17E01847, core->interrupts.writeMpTarget(71, IO_PARAMS8)) // MP_TARGET71
                DEF_IO08(0x17E01848, core->interrupts.writeMpTarget(72, IO_PARAMS8)) // MP_TARGET72
                DEF_IO08(0x17E01849, core->interrupts.writeMpTarget(73, IO_PARAMS8)) // MP_TARGET73
                DEF_IO08(0x17E0184A, core->interrupts.writeMpTarget(74, IO_PARAMS8)) // MP_TARGET74
                DEF_IO08(0x17E0184B, core->interrupts.writeMpTarget(75, IO_PARAMS8)) // MP_TARGET75
                DEF_IO08(0x17E0184C, core->interrupts.writeMpTarget(76, IO_PARAMS8)) // MP_TARGET76
                DEF_IO08(0x17E0184D, core->interrupts.writeMpTarget(77, IO_PARAMS8)) // MP_TARGET77
                DEF_IO08(0x17E0184E, core->interrupts.writeMpTarget(78, IO_PARAMS8)) // MP_TARGET78
                DEF_IO08(0x17E0184F, core->interrupts.writeMpTarget(79, IO_PARAMS8)) // MP_TARGET79
                DEF_IO08(0x17E01850, core->interrupts.writeMpTarget(80, IO_PARAMS8)) // MP_TARGET80
                DEF_IO08(0x17E01851, core->interrupts.writeMpTarget(81, IO_PARAMS8)) // MP_TARGET81
                DEF_IO08(0x17E01852, core->interrupts.writeMpTarget(82, IO_PARAMS8)) // MP_TARGET82
                DEF_IO08(0x17E01853, core->interrupts.writeMpTarget(83, IO_PARAMS8)) // MP_TARGET83
                DEF_IO08(0x17E01854, core->interrupts.writeMpTarget(84, IO_PARAMS8)) // MP_TARGET84
                DEF_IO08(0x17E01855, core->interrupts.writeMpTarget(85, IO_PARAMS8)) // MP_TARGET85
                DEF_IO08(0x17E01856, core->interrupts.writeMpTarget(86, IO_PARAMS8)) // MP_TARGET86
                DEF_IO08(0x17E01857, core->interrupts.writeMpTarget(87, IO_PARAMS8)) // MP_TARGET87
                DEF_IO08(0x17E01858, core->interrupts.writeMpTarget(88, IO_PARAMS8)) // MP_TARGET88
                DEF_IO08(0x17E01859, core->interrupts.writeMpTarget(89, IO_PARAMS8)) // MP_TARGET89
                DEF_IO08(0x17E0185A, core->interrupts.writeMpTarget(90, IO_PARAMS8)) // MP_TARGET90
                DEF_IO08(0x17E0185B, core->interrupts.writeMpTarget(91, IO_PARAMS8)) // MP_TARGET91
                DEF_IO08(0x17E0185C, core->interrupts.writeMpTarget(92, IO_PARAMS8)) // MP_TARGET92
                DEF_IO08(0x17E0185D, core->interrupts.writeMpTarget(93, IO_PARAMS8)) // MP_TARGET93
                DEF_IO08(0x17E0185E, core->interrupts.writeMpTarget(94, IO_PARAMS8)) // MP_TARGET94
                DEF_IO08(0x17E0185F, core->interrupts.writeMpTarget(95, IO_PARAMS8)) // MP_TARGET95
                DEF_IO08(0x17E01860, core->interrupts.writeMpTarget(96, IO_PARAMS8)) // MP_TARGET96
                DEF_IO08(0x17E01861, core->interrupts.writeMpTarget(97, IO_PARAMS8)) // MP_TARGET97
                DEF_IO08(0x17E01862, core->interrupts.writeMpTarget(98, IO_PARAMS8)) // MP_TARGET98
                DEF_IO08(0x17E01863, core->interrupts.writeMpTarget(99, IO_PARAMS8)) // MP_TARGET99
                DEF_IO08(0x17E01864, core->interrupts.writeMpTarget(100, IO_PARAMS8)) // MP_TARGET100
                DEF_IO08(0x17E01865, core->interrupts.writeMpTarget(101, IO_PARAMS8)) // MP_TARGET101
                DEF_IO08(0x17E01866, core->interrupts.writeMpTarget(102, IO_PARAMS8)) // MP_TARGET102
                DEF_IO08(0x17E01867, core->interrupts.writeMpTarget(103, IO_PARAMS8)) // MP_TARGET103
                DEF_IO08(0x17E01868, core->interrupts.writeMpTarget(104, IO_PARAMS8)) // MP_TARGET104
                DEF_IO08(0x17E01869, core->interrupts.writeMpTarget(105, IO_PARAMS8)) // MP_TARGET105
                DEF_IO08(0x17E0186A, core->interrupts.writeMpTarget(106, IO_PARAMS8)) // MP_TARGET106
                DEF_IO08(0x17E0186B, core->interrupts.writeMpTarget(107, IO_PARAMS8)) // MP_TARGET107
                DEF_IO08(0x17E0186C, core->interrupts.writeMpTarget(108, IO_PARAMS8)) // MP_TARGET108
                DEF_IO08(0x17E0186D, core->interrupts.writeMpTarget(109, IO_PARAMS8)) // MP_TARGET109
                DEF_IO08(0x17E0186E, core->interrupts.writeMpTarget(110, IO_PARAMS8)) // MP_TARGET110
                DEF_IO08(0x17E0186F, core->interrupts.writeMpTarget(111, IO_PARAMS8)) // MP_TARGET111
                DEF_IO08(0x17E01870, core->interrupts.writeMpTarget(112, IO_PARAMS8)) // MP_TARGET112
                DEF_IO08(0x17E01871, core->interrupts.writeMpTarget(113, IO_PARAMS8)) // MP_TARGET113
                DEF_IO08(0x17E01872, core->interrupts.writeMpTarget(114, IO_PARAMS8)) // MP_TARGET114
                DEF_IO08(0x17E01873, core->interrupts.writeMpTarget(115, IO_PARAMS8)) // MP_TARGET115
                DEF_IO08(0x17E01874, core->interrupts.writeMpTarget(116, IO_PARAMS8)) // MP_TARGET116
                DEF_IO08(0x17E01875, core->interrupts.writeMpTarget(117, IO_PARAMS8)) // MP_TARGET117
                DEF_IO08(0x17E01876, core->interrupts.writeMpTarget(118, IO_PARAMS8)) // MP_TARGET118
                DEF_IO08(0x17E01877, core->interrupts.writeMpTarget(119, IO_PARAMS8)) // MP_TARGET119
                DEF_IO08(0x17E01878, core->interrupts.writeMpTarget(120, IO_PARAMS8)) // MP_TARGET120
                DEF_IO08(0x17E01879, core->interrupts.writeMpTarget(121, IO_PARAMS8)) // MP_TARGET121
                DEF_IO08(0x17E0187A, core->interrupts.writeMpTarget(122, IO_PARAMS8)) // MP_TARGET122
                DEF_IO08(0x17E0187B, core->interrupts.writeMpTarget(123, IO_PARAMS8)) // MP_TARGET123
                DEF_IO08(0x17E0187C, core->interrupts.writeMpTarget(124, IO_PARAMS8)) // MP_TARGET124
                DEF_IO08(0x17E0187D, core->interrupts.writeMpTarget(125, IO_PARAMS8)) // MP_TARGET125
                DEF_IO08(0x17E0187E, core->interrupts.writeMpTarget(126, IO_PARAMS8)) // MP_TARGET126
                DEF_IO08(0x17E0187F, core->interrupts.writeMpTarget(127, IO_PARAMS8)) // MP_TARGET127
                DEF_IO32(0x17E01F00, core->interrupts.writeMpSoftIrq(id, IO_PARAMS)) // MP_SOFT_IRQ
            }
        }
        else { // ARM9
            switch (base) {
                DEF_IO08(0x10000000, writeCfg9Sysprot9(IO_PARAMS8)) // CFG9_SYSPROT9
                DEF_IO08(0x10000001, writeCfg9Sysprot11(IO_PARAMS8)) // CFG9_SYSPROT11
                DEF_IO16(0x10000010, core->cartridge.writeCfg9CardPower(IO_PARAMS)) // CFG9_CARD_POWER
                DEF_IO32(0x10000200, writeCfg9Extmemcnt9(IO_PARAMS)) // CFG9_EXTMEMCNT9
                DEF_IO32(0x10001000, core->interrupts.writeIrqIe(IO_PARAMS)) // IRQ_IE
                DEF_IO32(0x10001004, core->interrupts.writeIrqIf(IO_PARAMS)) // IRQ_IF
                DEF_IO32(0x10002004, core->ndma.writeSad(0, IO_PARAMS)) // NDMA0SAD
                DEF_IO32(0x10002008, core->ndma.writeDad(0, IO_PARAMS)) // NDMA0DAD
                DEF_IO32(0x1000200C, core->ndma.writeTcnt(0, IO_PARAMS)) // NDMA0TCNT
                DEF_IO32(0x10002010, core->ndma.writeWcnt(0, IO_PARAMS)) // NDMA0WCNT
                DEF_IO32(0x10002018, core->ndma.writeFdata(0, IO_PARAMS)) // NDMA0FDATA
                DEF_IO32(0x1000201C, core->ndma.writeCnt(0, IO_PARAMS)) // NDMA0CNT
                DEF_IO32(0x10002020, core->ndma.writeSad(1, IO_PARAMS)) // NDMA1SAD
                DEF_IO32(0x10002024, core->ndma.writeDad(1, IO_PARAMS)) // NDMA1DAD
                DEF_IO32(0x10002028, core->ndma.writeTcnt(1, IO_PARAMS)) // NDMA1TCNT
                DEF_IO32(0x1000202C, core->ndma.writeWcnt(1, IO_PARAMS)) // NDMA1WCNT
                DEF_IO32(0x10002034, core->ndma.writeFdata(1, IO_PARAMS)) // NDMA1FDATA
                DEF_IO32(0x10002038, core->ndma.writeCnt(1, IO_PARAMS)) // NDMA1CNT
                DEF_IO32(0x1000203C, core->ndma.writeSad(2, IO_PARAMS)) // NDMA2SAD
                DEF_IO32(0x10002040, core->ndma.writeDad(2, IO_PARAMS)) // NDMA2DAD
                DEF_IO32(0x10002044, core->ndma.writeTcnt(2, IO_PARAMS)) // NDMA2TCNT
                DEF_IO32(0x10002048, core->ndma.writeWcnt(2, IO_PARAMS)) // NDMA2WCNT
                DEF_IO32(0x10002050, core->ndma.writeFdata(2, IO_PARAMS)) // NDMA2FDATA
                DEF_IO32(0x10002054, core->ndma.writeCnt(2, IO_PARAMS)) // NDMA2CNT
                DEF_IO32(0x10002058, core->ndma.writeSad(3, IO_PARAMS)) // NDMA3SAD
                DEF_IO32(0x1000205C, core->ndma.writeDad(3, IO_PARAMS)) // NDMA3DAD
                DEF_IO32(0x10002060, core->ndma.writeTcnt(3, IO_PARAMS)) // NDMA3TCNT
                DEF_IO32(0x10002064, core->ndma.writeWcnt(3, IO_PARAMS)) // NDMA3WCNT
                DEF_IO32(0x1000206C, core->ndma.writeFdata(3, IO_PARAMS)) // NDMA3FDATA
                DEF_IO32(0x10002070, core->ndma.writeCnt(3, IO_PARAMS)) // NDMA3CNT
                DEF_IO32(0x10002074, core->ndma.writeSad(4, IO_PARAMS)) // NDMA4SAD
                DEF_IO32(0x10002078, core->ndma.writeDad(4, IO_PARAMS)) // NDMA4DAD
                DEF_IO32(0x1000207C, core->ndma.writeTcnt(4, IO_PARAMS)) // NDMA4TCNT
                DEF_IO32(0x10002080, core->ndma.writeWcnt(4, IO_PARAMS)) // NDMA4WCNT
                DEF_IO32(0x10002088, core->ndma.writeFdata(4, IO_PARAMS)) // NDMA4FDATA
                DEF_IO32(0x1000208C, core->ndma.writeCnt(4, IO_PARAMS)) // NDMA4CNT
                DEF_IO32(0x10002090, core->ndma.writeSad(5, IO_PARAMS)) // NDMA5SAD
                DEF_IO32(0x10002094, core->ndma.writeDad(5, IO_PARAMS)) // NDMA5DAD
                DEF_IO32(0x10002098, core->ndma.writeTcnt(5, IO_PARAMS)) // NDMA5TCNT
                DEF_IO32(0x1000209C, core->ndma.writeWcnt(5, IO_PARAMS)) // NDMA5WCNT
                DEF_IO32(0x100020A4, core->ndma.writeFdata(5, IO_PARAMS)) // NDMA5FDATA
                DEF_IO32(0x100020A8, core->ndma.writeCnt(5, IO_PARAMS)) // NDMA5CNT
                DEF_IO32(0x100020AC, core->ndma.writeSad(6, IO_PARAMS)) // NDMA6SAD
                DEF_IO32(0x100020B0, core->ndma.writeDad(6, IO_PARAMS)) // NDMA6DAD
                DEF_IO32(0x100020B4, core->ndma.writeTcnt(6, IO_PARAMS)) // NDMA6TCNT
                DEF_IO32(0x100020B8, core->ndma.writeWcnt(6, IO_PARAMS)) // NDMA6WCNT
                DEF_IO32(0x100020C0, core->ndma.writeFdata(6, IO_PARAMS)) // NDMA6FDATA
                DEF_IO32(0x100020C4, core->ndma.writeCnt(6, IO_PARAMS)) // NDMA6CNT
                DEF_IO32(0x100020C8, core->ndma.writeSad(7, IO_PARAMS)) // NDMA7SAD
                DEF_IO32(0x100020CC, core->ndma.writeDad(7, IO_PARAMS)) // NDMA7DAD
                DEF_IO32(0x100020D0, core->ndma.writeTcnt(7, IO_PARAMS)) // NDMA7TCNT
                DEF_IO32(0x100020D4, core->ndma.writeWcnt(7, IO_PARAMS)) // NDMA7WCNT
                DEF_IO32(0x100020DC, core->ndma.writeFdata(7, IO_PARAMS)) // NDMA7FDATA
                DEF_IO32(0x100020E0, core->ndma.writeCnt(7, IO_PARAMS)) // NDMA7CNT
                DEF_IO16(0x10003000, core->timers.writeTmCntL(0, IO_PARAMS)) // TM0CNT_L
                DEF_IO16(0x10003002, core->timers.writeTmCntH(0, IO_PARAMS)) // TM0CNT_H
                DEF_IO16(0x10003004, core->timers.writeTmCntL(1, IO_PARAMS)) // TM1CNT_L
                DEF_IO16(0x10003006, core->timers.writeTmCntH(1, IO_PARAMS)) // TM1CNT_H
                DEF_IO16(0x10003008, core->timers.writeTmCntL(2, IO_PARAMS)) // TM2CNT_L
                DEF_IO16(0x1000300A, core->timers.writeTmCntH(2, IO_PARAMS)) // TM2CNT_H
                DEF_IO16(0x1000300C, core->timers.writeTmCntL(3, IO_PARAMS)) // TM3CNT_L
                DEF_IO16(0x1000300E, core->timers.writeTmCntH(3, IO_PARAMS)) // TM3CNT_H
                DEF_IO32(0x10004000, core->cartridge.writeCtrCnt(IO_PARAMS)) // CTRCARD_CNT
                DEF_IO32(0x10004004, core->cartridge.writeCtrBlkcnt(IO_PARAMS)) // CTRCARD_BLKCNT
                DEF_IO32(0x10004008, core->cartridge.writeCtrSeccnt(IO_PARAMS)) // CTRCARD_SECCNT
                DEF_IO32(0x10004020, core->cartridge.writeCtrCmd(0, IO_PARAMS)) // CTRCARD_CMD0
                DEF_IO32(0x10004024, core->cartridge.writeCtrCmd(1, IO_PARAMS)) // CTRCARD_CMD1
                DEF_IO32(0x10004028, core->cartridge.writeCtrCmd(2, IO_PARAMS)) // CTRCARD_CMD2
                DEF_IO32(0x1000402C, core->cartridge.writeCtrCmd(3, IO_PARAMS)) // CTRCARD_CMD3
                DEF_IO32(0x10004030, core->cartridge.writeCtrFifo(IO_PARAMS)) // CTRCARD_FIFO
                DEF_IO16(0x10006000, core->sdMmcs[0].writeCmd(IO_PARAMS)) // SD0_CMD
                DEF_IO16(0x10006002, core->sdMmcs[0].writePortSelect(IO_PARAMS)) // SD0_PORT_SELECT
                DEF_IO32(0x10006004, core->sdMmcs[0].writeCmdParam(IO_PARAMS)) // SD0_CMD_PARAM
                DEF_IO16(0x1000600A, core->sdMmcs[0].writeData16Blkcnt(IO_PARAMS)) // SD0_DATA16_BLKCNT
                DEF_IO32(0x1000601C, core->sdMmcs[0].writeIrqStatus(IO_PARAMS)) // SD0_IRQ_STATUS
                DEF_IO32(0x10006020, core->sdMmcs[0].writeIrqMask(IO_PARAMS)) // SD0_IRQ_MASK
                DEF_IO16(0x10006026, core->sdMmcs[0].writeData16Blklen(IO_PARAMS)) // SD0_DATA16_BLKLEN
                DEF_IO16(0x10006030, core->sdMmcs[0].writeData16Fifo(IO_PARAMS)) // SD0_DATA16_FIFO
                DEF_IO16(0x100060D8, core->sdMmcs[0].writeDataCtl(IO_PARAMS)) // SD0_DATA_CTL
                DEF_IO16(0x10006100, core->sdMmcs[0].writeData32Irq(IO_PARAMS)) // SD0_DATA32_IRQ
                DEF_IO16(0x10006104, core->sdMmcs[0].writeData32Blklen(IO_PARAMS)) // SD0_DATA32_BLKLEN
                DEF_IO32(0x1000610C, core->sdMmcs[0].writeData32Fifo(IO_PARAMS)) // SD0_DATA32_FIFO
                DEF_IO16(0x10007000, core->sdMmcs[1].writeCmd(IO_PARAMS)) // SD1_CMD
                DEF_IO16(0x10007002, core->sdMmcs[1].writePortSelect(IO_PARAMS)) // SD1_PORT_SELECT
                DEF_IO32(0x10007004, core->sdMmcs[1].writeCmdParam(IO_PARAMS)) // SD1_CMD_PARAM
                DEF_IO16(0x1000700A, core->sdMmcs[1].writeData16Blkcnt(IO_PARAMS)) // SD1_DATA16_BLKCNT
                DEF_IO32(0x1000701C, core->sdMmcs[1].writeIrqStatus(IO_PARAMS)) // SD1_IRQ_STATUS
                DEF_IO32(0x10007020, core->sdMmcs[1].writeIrqMask(IO_PARAMS)) // SD1_IRQ_MASK
                DEF_IO16(0x10007026, core->sdMmcs[1].writeData16Blklen(IO_PARAMS)) // SD1_DATA16_BLKLEN
                DEF_IO16(0x10007030, core->sdMmcs[1].writeData16Fifo(IO_PARAMS)) // SD1_DATA16_FIFO
                DEF_IO16(0x100070D8, core->sdMmcs[1].writeDataCtl(IO_PARAMS)) // SD1_DATA_CTL
                DEF_IO16(0x10007100, core->sdMmcs[1].writeData32Irq(IO_PARAMS)) // SD1_DATA32_IRQ
                DEF_IO16(0x10007104, core->sdMmcs[1].writeData32Blklen(IO_PARAMS)) // SD1_DATA32_BLKLEN
                DEF_IO32(0x1000710C, core->sdMmcs[1].writeData32Fifo(IO_PARAMS)) // SD1_DATA32_FIFO
                DEF_IO32(0x10008000, core->pxi.writeSync(1, IO_PARAMS)) // PXI_SYNC9
                DEF_IO32(0x10008004, core->pxi.writeCnt(1, IO_PARAMS)) // PXI_CNT9
                DEF_IO32(0x10008008, core->pxi.writeSend(1, IO_PARAMS)) // PXI_SEND9
                DEF_IO32(0x10009000, core->aes.writeCnt(IO_PARAMS)) // AES_CNT
                DEF_IO32(0x10009004, core->aes.writeBlkcnt(IO_PARAMS)) // AES_BLKCNT
                DEF_IO32(0x10009008, core->aes.writeWrfifo(IO_PARAMS)) // AES_WRFIFO
                DEF_IO08(0x10009010, core->aes.writeKeysel(IO_PARAMS8)) // AES_KEYSEL
                DEF_IO08(0x10009011, core->aes.writeKeycnt(IO_PARAMS8)) // AES_KEYCNT
                DEF_IO32(0x10009020, core->aes.writeIv(0, IO_PARAMS)) // AES_IV0
                DEF_IO32(0x10009024, core->aes.writeIv(1, IO_PARAMS)) // AES_IV1
                DEF_IO32(0x10009028, core->aes.writeIv(2, IO_PARAMS)) // AES_IV2
                DEF_IO32(0x1000902C, core->aes.writeIv(3, IO_PARAMS)) // AES_IV3
                DEF_IO32(0x10009030, core->aes.writeMac(0, IO_PARAMS)) // AES_MAC0
                DEF_IO32(0x10009034, core->aes.writeMac(1, IO_PARAMS)) // AES_MAC1
                DEF_IO32(0x10009038, core->aes.writeMac(2, IO_PARAMS)) // AES_MAC2
                DEF_IO32(0x1000903C, core->aes.writeMac(3, IO_PARAMS)) // AES_MAC3
                DEF_IO32(0x10009040, core->aes.writeKey(0, 0, IO_PARAMS)) // AES_KEY0_0
                DEF_IO32(0x10009044, core->aes.writeKey(0, 1, IO_PARAMS)) // AES_KEY0_1
                DEF_IO32(0x10009048, core->aes.writeKey(0, 2, IO_PARAMS)) // AES_KEY0_2
                DEF_IO32(0x1000904C, core->aes.writeKey(0, 3, IO_PARAMS)) // AES_KEY0_3
                DEF_IO32(0x10009050, core->aes.writeKeyx(0, 0, IO_PARAMS)) // AES_KEYX0_0
                DEF_IO32(0x10009054, core->aes.writeKeyx(0, 1, IO_PARAMS)) // AES_KEYX0_1
                DEF_IO32(0x10009058, core->aes.writeKeyx(0, 2, IO_PARAMS)) // AES_KEYX0_2
                DEF_IO32(0x1000905C, core->aes.writeKeyx(0, 3, IO_PARAMS)) // AES_KEYX0_3
                DEF_IO32(0x10009060, core->aes.writeKeyy(0, 0, IO_PARAMS)) // AES_KEYY0_0
                DEF_IO32(0x10009064, core->aes.writeKeyy(0, 1, IO_PARAMS)) // AES_KEYY0_1
                DEF_IO32(0x10009068, core->aes.writeKeyy(0, 2, IO_PARAMS)) // AES_KEYY0_2
                DEF_IO32(0x1000906C, core->aes.writeKeyy(0, 3, IO_PARAMS)) // AES_KEYY0_3
                DEF_IO32(0x10009070, core->aes.writeKey(1, 0, IO_PARAMS)) // AES_KEY1_0
                DEF_IO32(0x10009074, core->aes.writeKey(1, 1, IO_PARAMS)) // AES_KEY1_1
                DEF_IO32(0x10009078, core->aes.writeKey(1, 2, IO_PARAMS)) // AES_KEY1_2
                DEF_IO32(0x1000907C, core->aes.writeKey(1, 3, IO_PARAMS)) // AES_KEY1_3
                DEF_IO32(0x10009080, core->aes.writeKeyx(1, 0, IO_PARAMS)) // AES_KEYX1_0
                DEF_IO32(0x10009084, core->aes.writeKeyx(1, 1, IO_PARAMS)) // AES_KEYX1_1
                DEF_IO32(0x10009088, core->aes.writeKeyx(1, 2, IO_PARAMS)) // AES_KEYX1_2
                DEF_IO32(0x1000908C, core->aes.writeKeyx(1, 3, IO_PARAMS)) // AES_KEYX1_3
                DEF_IO32(0x10009090, core->aes.writeKeyy(1, 0, IO_PARAMS)) // AES_KEYY1_0
                DEF_IO32(0x10009094, core->aes.writeKeyy(1, 1, IO_PARAMS)) // AES_KEYY1_1
                DEF_IO32(0x10009098, core->aes.writeKeyy(1, 2, IO_PARAMS)) // AES_KEYY1_2
                DEF_IO32(0x1000909C, core->aes.writeKeyy(1, 3, IO_PARAMS)) // AES_KEYY1_3
                DEF_IO32(0x100090A0, core->aes.writeKey(2, 0, IO_PARAMS)) // AES_KEY2_0
                DEF_IO32(0x100090A4, core->aes.writeKey(2, 1, IO_PARAMS)) // AES_KEY2_1
                DEF_IO32(0x100090A8, core->aes.writeKey(2, 2, IO_PARAMS)) // AES_KEY2_2
                DEF_IO32(0x100090AC, core->aes.writeKey(2, 3, IO_PARAMS)) // AES_KEY2_3
                DEF_IO32(0x100090B0, core->aes.writeKeyx(2, 0, IO_PARAMS)) // AES_KEYX2_0
                DEF_IO32(0x100090B4, core->aes.writeKeyx(2, 1, IO_PARAMS)) // AES_KEYX2_1
                DEF_IO32(0x100090B8, core->aes.writeKeyx(2, 2, IO_PARAMS)) // AES_KEYX2_2
                DEF_IO32(0x100090BC, core->aes.writeKeyx(2, 3, IO_PARAMS)) // AES_KEYX2_3
                DEF_IO32(0x100090C0, core->aes.writeKeyy(2, 0, IO_PARAMS)) // AES_KEYY2_0
                DEF_IO32(0x100090C4, core->aes.writeKeyy(2, 1, IO_PARAMS)) // AES_KEYY2_1
                DEF_IO32(0x100090C8, core->aes.writeKeyy(2, 2, IO_PARAMS)) // AES_KEYY2_2
                DEF_IO32(0x100090CC, core->aes.writeKeyy(2, 3, IO_PARAMS)) // AES_KEYY2_3
                DEF_IO32(0x100090D0, core->aes.writeKey(3, 0, IO_PARAMS)) // AES_KEY3_0
                DEF_IO32(0x100090D4, core->aes.writeKey(3, 1, IO_PARAMS)) // AES_KEY3_1
                DEF_IO32(0x100090D8, core->aes.writeKey(3, 2, IO_PARAMS)) // AES_KEY3_2
                DEF_IO32(0x100090DC, core->aes.writeKey(3, 3, IO_PARAMS)) // AES_KEY3_3
                DEF_IO32(0x100090E0, core->aes.writeKeyx(3, 0, IO_PARAMS)) // AES_KEYX3_0
                DEF_IO32(0x100090E4, core->aes.writeKeyx(3, 1, IO_PARAMS)) // AES_KEYX3_1
                DEF_IO32(0x100090E8, core->aes.writeKeyx(3, 2, IO_PARAMS)) // AES_KEYX3_2
                DEF_IO32(0x100090EC, core->aes.writeKeyx(3, 3, IO_PARAMS)) // AES_KEYX3_3
                DEF_IO32(0x100090F0, core->aes.writeKeyy(3, 0, IO_PARAMS)) // AES_KEYY3_0
                DEF_IO32(0x100090F4, core->aes.writeKeyy(3, 1, IO_PARAMS)) // AES_KEYY3_1
                DEF_IO32(0x100090F8, core->aes.writeKeyy(3, 2, IO_PARAMS)) // AES_KEYY3_2
                DEF_IO32(0x100090FC, core->aes.writeKeyy(3, 3, IO_PARAMS)) // AES_KEYY3_3
                DEF_IO32(0x10009100, core->aes.writeKeyfifo(IO_PARAMS)) // AES_KEYFIFO
                DEF_IO32(0x10009104, core->aes.writeKeyxfifo(IO_PARAMS)) // AES_KEYXFIFO
                DEF_IO32(0x10009108, core->aes.writeKeyyfifo(IO_PARAMS)) // AES_KEYYFIFO
                DEF_IO32(0x1000A000, core->shas[1].writeCnt(IO_PARAMS)) // SHA1_CNT
                DEF_IO32(0x1000A004, core->shas[1].writeBlkcnt(IO_PARAMS)) // SHA1_BLKCNT
                DEF_IO32(0x1000A040, core->shas[1].writeHash(0, IO_PARAMS)) // SHA1_HASH0
                DEF_IO32(0x1000A044, core->shas[1].writeHash(1, IO_PARAMS)) // SHA1_HASH1
                DEF_IO32(0x1000A048, core->shas[1].writeHash(2, IO_PARAMS)) // SHA1_HASH2
                DEF_IO32(0x1000A04C, core->shas[1].writeHash(3, IO_PARAMS)) // SHA1_HASH3
                DEF_IO32(0x1000A050, core->shas[1].writeHash(4, IO_PARAMS)) // SHA1_HASH4
                DEF_IO32(0x1000A054, core->shas[1].writeHash(5, IO_PARAMS)) // SHA1_HASH5
                DEF_IO32(0x1000A058, core->shas[1].writeHash(6, IO_PARAMS)) // SHA1_HASH6
                DEF_IO32(0x1000A05C, core->shas[1].writeHash(7, IO_PARAMS)) // SHA1_HASH7
                DEF_IO32(0x1000A080, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A084, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A088, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A08C, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A090, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A094, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A098, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A09C, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0A0, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0A4, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0A8, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0AC, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0B0, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0B4, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0B8, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000A0BC, core->shas[1].writeFifo(IO_PARAMS)) // SHA1_FIFO
                DEF_IO32(0x1000B000, core->rsa.writeCnt(IO_PARAMS)) // RSA_CNT
                DEF_IO32(0x1000B100, core->rsa.writeSlotcnt(0, IO_PARAMS)) // RSA_SLOTCNT0
                DEF_IO32(0x1000B110, core->rsa.writeSlotcnt(1, IO_PARAMS)) // RSA_SLOTCNT1
                DEF_IO32(0x1000B120, core->rsa.writeSlotcnt(2, IO_PARAMS)) // RSA_SLOTCNT2
                DEF_IO32(0x1000B130, core->rsa.writeSlotcnt(3, IO_PARAMS)) // RSA_SLOTCNT3
                DEF_IO32(0x1000B200, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B204, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B208, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B20C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B210, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B214, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B218, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B21C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B220, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B224, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B228, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B22C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B230, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B234, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B238, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B23C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B240, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B244, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B248, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B24C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B250, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B254, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B258, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B25C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B260, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B264, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B268, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B26C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B270, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B274, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B278, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B27C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B280, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B284, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B288, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B28C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B290, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B294, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B298, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B29C, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2A8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2AC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2B8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2BC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2C8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2CC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2D8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2DC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2E8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2EC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F0, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F4, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2F8, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B2FC, core->rsa.writeExpfifo(IO_PARAMS)) // RSA_EXPFIFO
                DEF_IO32(0x1000B400, core->rsa.writeMod(0, IO_PARAMS)) // RSA_MOD0
                DEF_IO32(0x1000B404, core->rsa.writeMod(1, IO_PARAMS)) // RSA_MOD1
                DEF_IO32(0x1000B408, core->rsa.writeMod(2, IO_PARAMS)) // RSA_MOD2
                DEF_IO32(0x1000B40C, core->rsa.writeMod(3, IO_PARAMS)) // RSA_MOD3
                DEF_IO32(0x1000B410, core->rsa.writeMod(4, IO_PARAMS)) // RSA_MOD4
                DEF_IO32(0x1000B414, core->rsa.writeMod(5, IO_PARAMS)) // RSA_MOD5
                DEF_IO32(0x1000B418, core->rsa.writeMod(6, IO_PARAMS)) // RSA_MOD6
                DEF_IO32(0x1000B41C, core->rsa.writeMod(7, IO_PARAMS)) // RSA_MOD7
                DEF_IO32(0x1000B420, core->rsa.writeMod(8, IO_PARAMS)) // RSA_MOD8
                DEF_IO32(0x1000B424, core->rsa.writeMod(9, IO_PARAMS)) // RSA_MOD9
                DEF_IO32(0x1000B428, core->rsa.writeMod(10, IO_PARAMS)) // RSA_MOD10
                DEF_IO32(0x1000B42C, core->rsa.writeMod(11, IO_PARAMS)) // RSA_MOD11
                DEF_IO32(0x1000B430, core->rsa.writeMod(12, IO_PARAMS)) // RSA_MOD12
                DEF_IO32(0x1000B434, core->rsa.writeMod(13, IO_PARAMS)) // RSA_MOD13
                DEF_IO32(0x1000B438, core->rsa.writeMod(14, IO_PARAMS)) // RSA_MOD14
                DEF_IO32(0x1000B43C, core->rsa.writeMod(15, IO_PARAMS)) // RSA_MOD15
                DEF_IO32(0x1000B440, core->rsa.writeMod(16, IO_PARAMS)) // RSA_MOD16
                DEF_IO32(0x1000B444, core->rsa.writeMod(17, IO_PARAMS)) // RSA_MOD17
                DEF_IO32(0x1000B448, core->rsa.writeMod(18, IO_PARAMS)) // RSA_MOD18
                DEF_IO32(0x1000B44C, core->rsa.writeMod(19, IO_PARAMS)) // RSA_MOD19
                DEF_IO32(0x1000B450, core->rsa.writeMod(20, IO_PARAMS)) // RSA_MOD20
                DEF_IO32(0x1000B454, core->rsa.writeMod(21, IO_PARAMS)) // RSA_MOD21
                DEF_IO32(0x1000B458, core->rsa.writeMod(22, IO_PARAMS)) // RSA_MOD22
                DEF_IO32(0x1000B45C, core->rsa.writeMod(23, IO_PARAMS)) // RSA_MOD23
                DEF_IO32(0x1000B460, core->rsa.writeMod(24, IO_PARAMS)) // RSA_MOD24
                DEF_IO32(0x1000B464, core->rsa.writeMod(25, IO_PARAMS)) // RSA_MOD25
                DEF_IO32(0x1000B468, core->rsa.writeMod(26, IO_PARAMS)) // RSA_MOD26
                DEF_IO32(0x1000B46C, core->rsa.writeMod(27, IO_PARAMS)) // RSA_MOD27
                DEF_IO32(0x1000B470, core->rsa.writeMod(28, IO_PARAMS)) // RSA_MOD28
                DEF_IO32(0x1000B474, core->rsa.writeMod(29, IO_PARAMS)) // RSA_MOD29
                DEF_IO32(0x1000B478, core->rsa.writeMod(30, IO_PARAMS)) // RSA_MOD30
                DEF_IO32(0x1000B47C, core->rsa.writeMod(31, IO_PARAMS)) // RSA_MOD31
                DEF_IO32(0x1000B480, core->rsa.writeMod(32, IO_PARAMS)) // RSA_MOD32
                DEF_IO32(0x1000B484, core->rsa.writeMod(33, IO_PARAMS)) // RSA_MOD33
                DEF_IO32(0x1000B488, core->rsa.writeMod(34, IO_PARAMS)) // RSA_MOD34
                DEF_IO32(0x1000B48C, core->rsa.writeMod(35, IO_PARAMS)) // RSA_MOD35
                DEF_IO32(0x1000B490, core->rsa.writeMod(36, IO_PARAMS)) // RSA_MOD36
                DEF_IO32(0x1000B494, core->rsa.writeMod(37, IO_PARAMS)) // RSA_MOD37
                DEF_IO32(0x1000B498, core->rsa.writeMod(38, IO_PARAMS)) // RSA_MOD38
                DEF_IO32(0x1000B49C, core->rsa.writeMod(39, IO_PARAMS)) // RSA_MOD39
                DEF_IO32(0x1000B4A0, core->rsa.writeMod(40, IO_PARAMS)) // RSA_MOD40
                DEF_IO32(0x1000B4A4, core->rsa.writeMod(41, IO_PARAMS)) // RSA_MOD41
                DEF_IO32(0x1000B4A8, core->rsa.writeMod(42, IO_PARAMS)) // RSA_MOD42
                DEF_IO32(0x1000B4AC, core->rsa.writeMod(43, IO_PARAMS)) // RSA_MOD43
                DEF_IO32(0x1000B4B0, core->rsa.writeMod(44, IO_PARAMS)) // RSA_MOD44
                DEF_IO32(0x1000B4B4, core->rsa.writeMod(45, IO_PARAMS)) // RSA_MOD45
                DEF_IO32(0x1000B4B8, core->rsa.writeMod(46, IO_PARAMS)) // RSA_MOD46
                DEF_IO32(0x1000B4BC, core->rsa.writeMod(47, IO_PARAMS)) // RSA_MOD47
                DEF_IO32(0x1000B4C0, core->rsa.writeMod(48, IO_PARAMS)) // RSA_MOD48
                DEF_IO32(0x1000B4C4, core->rsa.writeMod(49, IO_PARAMS)) // RSA_MOD49
                DEF_IO32(0x1000B4C8, core->rsa.writeMod(50, IO_PARAMS)) // RSA_MOD50
                DEF_IO32(0x1000B4CC, core->rsa.writeMod(51, IO_PARAMS)) // RSA_MOD51
                DEF_IO32(0x1000B4D0, core->rsa.writeMod(52, IO_PARAMS)) // RSA_MOD52
                DEF_IO32(0x1000B4D4, core->rsa.writeMod(53, IO_PARAMS)) // RSA_MOD53
                DEF_IO32(0x1000B4D8, core->rsa.writeMod(54, IO_PARAMS)) // RSA_MOD54
                DEF_IO32(0x1000B4DC, core->rsa.writeMod(55, IO_PARAMS)) // RSA_MOD55
                DEF_IO32(0x1000B4E0, core->rsa.writeMod(56, IO_PARAMS)) // RSA_MOD56
                DEF_IO32(0x1000B4E4, core->rsa.writeMod(57, IO_PARAMS)) // RSA_MOD57
                DEF_IO32(0x1000B4E8, core->rsa.writeMod(58, IO_PARAMS)) // RSA_MOD58
                DEF_IO32(0x1000B4EC, core->rsa.writeMod(59, IO_PARAMS)) // RSA_MOD59
                DEF_IO32(0x1000B4F0, core->rsa.writeMod(60, IO_PARAMS)) // RSA_MOD60
                DEF_IO32(0x1000B4F4, core->rsa.writeMod(61, IO_PARAMS)) // RSA_MOD61
                DEF_IO32(0x1000B4F8, core->rsa.writeMod(62, IO_PARAMS)) // RSA_MOD62
                DEF_IO32(0x1000B4FC, core->rsa.writeMod(63, IO_PARAMS)) // RSA_MOD63
                DEF_IO32(0x1000B800, core->rsa.writeData(0, IO_PARAMS)) // RSA_DATA0
                DEF_IO32(0x1000B804, core->rsa.writeData(1, IO_PARAMS)) // RSA_DATA1
                DEF_IO32(0x1000B808, core->rsa.writeData(2, IO_PARAMS)) // RSA_DATA2
                DEF_IO32(0x1000B80C, core->rsa.writeData(3, IO_PARAMS)) // RSA_DATA3
                DEF_IO32(0x1000B810, core->rsa.writeData(4, IO_PARAMS)) // RSA_DATA4
                DEF_IO32(0x1000B814, core->rsa.writeData(5, IO_PARAMS)) // RSA_DATA5
                DEF_IO32(0x1000B818, core->rsa.writeData(6, IO_PARAMS)) // RSA_DATA6
                DEF_IO32(0x1000B81C, core->rsa.writeData(7, IO_PARAMS)) // RSA_DATA7
                DEF_IO32(0x1000B820, core->rsa.writeData(8, IO_PARAMS)) // RSA_DATA8
                DEF_IO32(0x1000B824, core->rsa.writeData(9, IO_PARAMS)) // RSA_DATA9
                DEF_IO32(0x1000B828, core->rsa.writeData(10, IO_PARAMS)) // RSA_DATA10
                DEF_IO32(0x1000B82C, core->rsa.writeData(11, IO_PARAMS)) // RSA_DATA11
                DEF_IO32(0x1000B830, core->rsa.writeData(12, IO_PARAMS)) // RSA_DATA12
                DEF_IO32(0x1000B834, core->rsa.writeData(13, IO_PARAMS)) // RSA_DATA13
                DEF_IO32(0x1000B838, core->rsa.writeData(14, IO_PARAMS)) // RSA_DATA14
                DEF_IO32(0x1000B83C, core->rsa.writeData(15, IO_PARAMS)) // RSA_DATA15
                DEF_IO32(0x1000B840, core->rsa.writeData(16, IO_PARAMS)) // RSA_DATA16
                DEF_IO32(0x1000B844, core->rsa.writeData(17, IO_PARAMS)) // RSA_DATA17
                DEF_IO32(0x1000B848, core->rsa.writeData(18, IO_PARAMS)) // RSA_DATA18
                DEF_IO32(0x1000B84C, core->rsa.writeData(19, IO_PARAMS)) // RSA_DATA19
                DEF_IO32(0x1000B850, core->rsa.writeData(20, IO_PARAMS)) // RSA_DATA20
                DEF_IO32(0x1000B854, core->rsa.writeData(21, IO_PARAMS)) // RSA_DATA21
                DEF_IO32(0x1000B858, core->rsa.writeData(22, IO_PARAMS)) // RSA_DATA22
                DEF_IO32(0x1000B85C, core->rsa.writeData(23, IO_PARAMS)) // RSA_DATA23
                DEF_IO32(0x1000B860, core->rsa.writeData(24, IO_PARAMS)) // RSA_DATA24
                DEF_IO32(0x1000B864, core->rsa.writeData(25, IO_PARAMS)) // RSA_DATA25
                DEF_IO32(0x1000B868, core->rsa.writeData(26, IO_PARAMS)) // RSA_DATA26
                DEF_IO32(0x1000B86C, core->rsa.writeData(27, IO_PARAMS)) // RSA_DATA27
                DEF_IO32(0x1000B870, core->rsa.writeData(28, IO_PARAMS)) // RSA_DATA28
                DEF_IO32(0x1000B874, core->rsa.writeData(29, IO_PARAMS)) // RSA_DATA29
                DEF_IO32(0x1000B878, core->rsa.writeData(30, IO_PARAMS)) // RSA_DATA30
                DEF_IO32(0x1000B87C, core->rsa.writeData(31, IO_PARAMS)) // RSA_DATA31
                DEF_IO32(0x1000B880, core->rsa.writeData(32, IO_PARAMS)) // RSA_DATA32
                DEF_IO32(0x1000B884, core->rsa.writeData(33, IO_PARAMS)) // RSA_DATA33
                DEF_IO32(0x1000B888, core->rsa.writeData(34, IO_PARAMS)) // RSA_DATA34
                DEF_IO32(0x1000B88C, core->rsa.writeData(35, IO_PARAMS)) // RSA_DATA35
                DEF_IO32(0x1000B890, core->rsa.writeData(36, IO_PARAMS)) // RSA_DATA36
                DEF_IO32(0x1000B894, core->rsa.writeData(37, IO_PARAMS)) // RSA_DATA37
                DEF_IO32(0x1000B898, core->rsa.writeData(38, IO_PARAMS)) // RSA_DATA38
                DEF_IO32(0x1000B89C, core->rsa.writeData(39, IO_PARAMS)) // RSA_DATA39
                DEF_IO32(0x1000B8A0, core->rsa.writeData(40, IO_PARAMS)) // RSA_DATA40
                DEF_IO32(0x1000B8A4, core->rsa.writeData(41, IO_PARAMS)) // RSA_DATA41
                DEF_IO32(0x1000B8A8, core->rsa.writeData(42, IO_PARAMS)) // RSA_DATA42
                DEF_IO32(0x1000B8AC, core->rsa.writeData(43, IO_PARAMS)) // RSA_DATA43
                DEF_IO32(0x1000B8B0, core->rsa.writeData(44, IO_PARAMS)) // RSA_DATA44
                DEF_IO32(0x1000B8B4, core->rsa.writeData(45, IO_PARAMS)) // RSA_DATA45
                DEF_IO32(0x1000B8B8, core->rsa.writeData(46, IO_PARAMS)) // RSA_DATA46
                DEF_IO32(0x1000B8BC, core->rsa.writeData(47, IO_PARAMS)) // RSA_DATA47
                DEF_IO32(0x1000B8C0, core->rsa.writeData(48, IO_PARAMS)) // RSA_DATA48
                DEF_IO32(0x1000B8C4, core->rsa.writeData(49, IO_PARAMS)) // RSA_DATA49
                DEF_IO32(0x1000B8C8, core->rsa.writeData(50, IO_PARAMS)) // RSA_DATA50
                DEF_IO32(0x1000B8CC, core->rsa.writeData(51, IO_PARAMS)) // RSA_DATA51
                DEF_IO32(0x1000B8D0, core->rsa.writeData(52, IO_PARAMS)) // RSA_DATA52
                DEF_IO32(0x1000B8D4, core->rsa.writeData(53, IO_PARAMS)) // RSA_DATA53
                DEF_IO32(0x1000B8D8, core->rsa.writeData(54, IO_PARAMS)) // RSA_DATA54
                DEF_IO32(0x1000B8DC, core->rsa.writeData(55, IO_PARAMS)) // RSA_DATA55
                DEF_IO32(0x1000B8E0, core->rsa.writeData(56, IO_PARAMS)) // RSA_DATA56
                DEF_IO32(0x1000B8E4, core->rsa.writeData(57, IO_PARAMS)) // RSA_DATA57
                DEF_IO32(0x1000B8E8, core->rsa.writeData(58, IO_PARAMS)) // RSA_DATA58
                DEF_IO32(0x1000B8EC, core->rsa.writeData(59, IO_PARAMS)) // RSA_DATA59
                DEF_IO32(0x1000B8F0, core->rsa.writeData(60, IO_PARAMS)) // RSA_DATA60
                DEF_IO32(0x1000B8F4, core->rsa.writeData(61, IO_PARAMS)) // RSA_DATA61
                DEF_IO32(0x1000B8F8, core->rsa.writeData(62, IO_PARAMS)) // RSA_DATA62
                DEF_IO32(0x1000B8FC, core->rsa.writeData(63, IO_PARAMS)) // RSA_DATA63
                DEF_IO32(0x1000C020, core->cdmas[XDMA].writeInten(IO_PARAMS)) // XDMA_INTEN
                DEF_IO32(0x1000C02C, core->cdmas[XDMA].writeIntclr(IO_PARAMS)) // XDMA_INTCLR
                DEF_IO32(0x1000CD04, core->cdmas[XDMA].writeDbgcmd(IO_PARAMS)) // XDMA_DBGCMD
                DEF_IO32(0x1000CD08, core->cdmas[XDMA].writeDbginst0(IO_PARAMS)) // XDMA_DBGINST0
                DEF_IO32(0x1000CD0C, core->cdmas[XDMA].writeDbginst1(IO_PARAMS)) // XDMA_DBGINST1
                DEF_IO32(0x1000D800, core->cartridge.writeSpiFifoCnt(IO_PARAMS)) // SPICARD_FIFO_CNT
                DEF_IO32(0x1000D804, core->cartridge.writeSpiFifoSelect(IO_PARAMS)) // SPICARD_FIFO_SELECT
                DEF_IO32(0x1000D808, core->cartridge.writeSpiFifoBlklen(IO_PARAMS)) // SPICARD_FIFO_BLKLEN
                DEF_IO32(0x1000D80C, core->cartridge.writeSpiFifoData(IO_PARAMS)) // SPICARD_FIFO_DATA
                DEF_IO32(0x1000D818, core->cartridge.writeSpiFifoIntMask(IO_PARAMS)) // SPICARD_FIFO_INT_MASK
                DEF_IO32(0x1000D81C, core->cartridge.writeSpiFifoIntStat(IO_PARAMS)) // SPICARD_FIFO_INT_STAT
            }
        }

        // Catch writes to unknown I/O registers
        if (id == ARM9)
            LOG_WARN("Unknown ARM9 I/O write: 0x%X\n", base);
        else
            LOG_WARN("Unknown ARM11 core %d I/O write: 0x%X\n", id, base);
        return;

    next:
        // Loop until the full value has been written
        i += size - base;
    }
}
