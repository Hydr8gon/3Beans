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

#pragma once

#include <cstdint>
#include <deque>

class Core;

class GpuRenderSoft {
public:
    GpuRenderSoft(Core *core);
    void runShader(uint16_t entry, float (*input)[4]);

    void writeVshCode(uint16_t address, uint32_t value);
    void writeVshDesc(uint16_t address, uint32_t value);
    void setVshBool(int i, bool value);
    void setVshInt(int i, int j, uint8_t value);
    void setVshFloat(int i, int j, float value);

    void setOutMap(uint8_t (*map)[2]);
    void setViewScaleH(float scale);
    void setViewScaleV(float scale);
    void setBufferSize(uint16_t width, uint16_t height);
    void setColbufAddr(uint32_t address);
    void setColbufFmt(ColbufFmt format);

private:
    Core *core;
    static void (GpuRenderSoft::*vshInstrs[0x40])(uint32_t);

    float *srcRegs[0x80];
    float *dstRegs[0x20];
    uint32_t *shdDesc;
    bool *shdBools;

    float shdTmp[16][4];
    float shdOut[8][4];
    bool shdCond[2];
    uint16_t shdPc;
    bool running;

    std::deque<uint32_t> ifStack;
    std::deque<uint32_t> callStack;
    float getRegs[4][4];
    uint8_t getIdx = 0;

    uint32_t vshCode[0x200] = {};
    uint32_t vshDesc[0x80] = {};
    bool vshBools[16] = {};
    uint8_t vshInts[4][3] = {};
    float vshFloats[96][4] = {};

    uint8_t outMap[0x18][2] = {};
    float viewScaleV = 0;
    float viewScaleH = 0;
    uint16_t bufWidth = 0;
    uint16_t bufHeight = 0;
    uint32_t colbufAddr = 0;
    ColbufFmt colbufFmt = UNK_FMT;

    float *getSrc(uint8_t src, uint32_t desc, uint8_t idx);
    void setDst(uint8_t dst, uint32_t desc, float *value);

    void shdAdd(uint32_t opcode);
    void shdDp3(uint32_t opcode);
    void shdDp4(uint32_t opcode);
    void shdMul(uint32_t opcode);
    void shdRcp(uint32_t opcode);
    void shdRsq(uint32_t opcode);
    void shdMov(uint32_t opcode);
    void shdNop(uint32_t opcode);
    void shdEnd(uint32_t opcode);
    void shdCall(uint32_t opcode);
    void shdCallc(uint32_t opcode);
    void shdCallu(uint32_t opcode);
    void shdIfu(uint32_t opcode);
    void shdIfc(uint32_t opcode);
    void shdCmp(uint32_t opcode);
    void vshUnk(uint32_t opcode);
};
