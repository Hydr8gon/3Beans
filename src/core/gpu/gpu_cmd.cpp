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

#include <cstring>

#include "../core.h"
#include "gpu_render.h"

TEMPLATE4(void Gpu::writeIrqReq, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 8, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 12, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeShdOutMap, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeShdOutMap, 4, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeTexBorder, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeTexDim, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeTexParam, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeTexAddr1, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeTexType, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombSrc, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombSrc, 3, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombOper, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombOper, 3, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombMode, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombMode, 3, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombColor, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeCombColor, 3, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrOfs, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrOfs, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrOfs, 8, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgL, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgL, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgL, 8, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgH, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgH, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeAttrCfgH, 8, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdSize, 0, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdAddr, 0, uint32_t, uint32_t)
TEMPLATE2(void Gpu::writeCmdJump, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeGshInts, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeVshInts, 0, uint32_t, uint32_t)

FORCE_INLINE uint32_t Gpu::flt24e7to32e8(uint32_t value) {
    // Convert a 24-bit float with 7-bit exponent to 32-bit with 8-bit exponent
    if (!(value & 0xFFFFFF)) return 0;
    return ((value << 8) & BIT(31)) | (((value & 0x7FFFFF) + 0x400000) << 7);
}

FORCE_INLINE uint32_t Gpu::flt32e7to32e8(uint32_t value) {
    // Convert a 32-bit float with 7-bit exponent to 32-bit with 8-bit exponent
    if (!value) return 0;
    return (value & BIT(31)) | (((value & 0x7FFFFFFF) + 0x40000000) >> 1);
}

void Gpu::runCommands() {
    // Start the GPU thread if enabled but not running
    if (Settings::threadedGpu && !running.exchange(true))
        thread = new std::thread(&Gpu::runThreaded, this);

    // Execute GPU commands until the end is reached
    while (cmdAddr < cmdEnd) {
        // Decode the command header
        uint32_t header = core->memory.read<uint32_t>(ARM11, cmdAddr + 4);
        uint32_t mask = maskTable[(header >> 16) & 0xF];
        uint8_t count = (header >> 20) & 0xFF;
        curCmd = (header & 0x3FF);

        // Adjust the address for the next command with 64-bit alignment
        uint32_t address = (cmdAddr + 4);
        cmdAddr += ((count + 3) << 2) & ~0x7;

        // Forward parameters to the thread if running, except for IRQ and jump commands
        if (running.load() && (curCmd & 0x3F0) != 0x10 && (curCmd < 0x238 || curCmd > 0x23D)) {
            uint32_t *data = new uint32_t[count + 2];
            data[0] = header;
            data[1] = core->memory.read<uint32_t>(ARM11, address - 4);
            for (int i = 0; i < count; i++)
                data[i + 2] = core->memory.read<uint32_t>(ARM11, address += 4);
            mutex.lock();
            tasks.emplace(TASK_CMD, data);
            mutex.unlock();
            continue;
        }

        // Write command parameters to GPU registers, with optionally increasing ID
        (this->*cmdWrites[curCmd])(mask, core->memory.read<uint32_t>(ARM11, address - 4));
        if (header & BIT(31)) // Increasing
            for (int i = 0; i < count; i++)
                (this->*cmdWrites[++curCmd & 0x3FF])(mask, core->memory.read<uint32_t>(ARM11, address += 4));
        else // Fixed
            for (int i = 0; i < count; i++)
                (this->*cmdWrites[curCmd])(mask, core->memory.read<uint32_t>(ARM11, address += 4));
    }

    // Reset the command address to indicate being stopped
    LOG_INFO("Finished executing GPU command list\n");
    cmdAddr = -1;
}

void Gpu::drawAttrIdx(uint32_t idx) {
    // Update the base shader input list using fixed attributes if dirty
    if (fixedDirty) {
        memset(fixedBase, 0, sizeof(fixedBase));
        for (uint32_t i = 0, f; i < 12; i++) {
            if (~gpuAttrFmt & BITL(48 + i)) continue;
            uint8_t id = (gpuVshAttrIds >> (i << 2)) & 0xF;
            fixedBase[id][0] = *(float*)&(f = flt24e7to32e8(attrFixedData[i][2]));
            fixedBase[id][1] = *(float*)&(f = flt24e7to32e8((attrFixedData[i][1] << 8) | (attrFixedData[i][2] >> 24)));
            fixedBase[id][2] = *(float*)&(f = flt24e7to32e8((attrFixedData[i][0] << 16) | (attrFixedData[i][1] >> 16)));
            fixedBase[id][3] = *(float*)&(f = flt24e7to32e8(attrFixedData[i][0] >> 8));
        }
        fixedDirty = false;
    }

    // Build an input list on top of the base by parsing attribute arrays at the given index
    float input[16][4];
    memcpy(input, fixedBase, sizeof(input));
    for (int i = 0; i < 12; i++) {
        uint8_t count = std::min<uint8_t>(12, gpuAttrCfg[i] >> 60);
        uint32_t base = (gpuAttrBase << 3) + gpuAttrOfs[i] + uint8_t(gpuAttrCfg[i] >> 48) * idx;
        for (int j = 0; j < count; j++) {
            // Skip over components that are just padding
            uint8_t comp = (gpuAttrCfg[i] >> (j << 2)) & 0xF;
            if (comp > 0xB) {
                base = ((base + 2) & ~0x3) + ((comp - 0xB) << 2);
                continue;
            }

            // Fill in default values that aren't provided by the array
            uint8_t fmt = (gpuAttrFmt >> (comp << 2)) & 0xF;
            uint8_t id = (gpuVshAttrIds >> (comp << 2)) & 0xF;
            for (int k = 3; k > (fmt >> 2); k--)
                input[id][k] = (k == 3) ? 1.0f : 0.0f;

            // Handle components based on format and write them to their mapped input ID
            switch (fmt & 0x3) {
            case 0: // Signed byte
                for (int k = 0; k <= (fmt >> 2); k++)
                    input[id][k] = int8_t(core->memory.read<uint8_t>(ARM11, base++));
                continue;

            case 1: // Unsigned byte
                for (int k = 0; k <= (fmt >> 2); k++)
                    input[id][k] = core->memory.read<uint8_t>(ARM11, base++);
                continue;

            case 2: // Signed half-word
                base = (base + 1) & ~0x1;
                for (int k = 0; k <= (fmt >> 2); k++) {
                    input[id][k] = int16_t(core->memory.read<uint16_t>(ARM11, base));
                    base += 2;
                }
                continue;

            case 3: // Single float
                base = (base + 2) & ~0x3;
                for (int k = 0; k <= (fmt >> 2); k++) {
                    uint32_t value = core->memory.read<uint32_t>(ARM11, base);
                    input[id][k] = *(float*)&value;
                    base += 4;
                }
                continue;
            }
        }
    }

    // Pass the finished input to the shader
    gpuShader->processVtx(input, idx);
}

void Gpu::updateShdMaps() {
    // Build a map of shader outputs to fixed semantics for the renderer
    uint8_t outMap[0x18][2] = {};
    uint32_t outMask = (gpuGshConfig & BIT(1)) ? gpuGshOutMask : gpuVshOutMask;
    for (int t = 0, i = 0; t < gpuShdOutTotal && i < 7; t++, i++) {
        while (~outMask & BIT(i))
            if (++i >= 7) break;
        for (int j = 0; j < 4; j++) {
            uint8_t sem = (gpuShdOutMap[i] >> (j << 3));
            if (sem >= 0x18) continue;
            outMap[sem][0] = i;
            outMap[sem][1] = j;
        }
    }

    // Update the map and check if the second one is needed
    shdMapDirty = false;
    gpuShader->setOutMap(outMap);
    if (~gpuGshConfig & BIT(1)) return;

    // Build a map of vertex shader outputs to geometry shader inputs
    uint8_t gshInMap[0x10] = {};
    for (int i = 0; i <= (gpuGshInputCfg & 0xF); i++)
        gshInMap[i] = (gpuGshAttrIds >> (i << 2)) & 0xF;
    if ((gpuGshInputCfg & 0xF) < 0xF)
        gshInMap[(gpuGshInputCfg & 0xF) + 1] = -1; // End
    gpuShader->setGshInMap(gshInMap);
}

template <int i> void Gpu::writeIrqReq(uint32_t mask, uint32_t value) {
    // Write IRQ request bytes, check if they match, and stop execution on interrupt if enabled
    gpuIrqReq[i] = (gpuIrqReq[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if ((mask & (0xFF << (j * 8))) && checkInterrupt((i << 2) + j) && gpuIrqAutostop)
            cmdAddr = -1;
}

void Gpu::writeFaceCulling(uint32_t mask, uint32_t value) {
    // Write to the triangle face culling register
    mask &= 0x3;
    gpuFaceCulling = (gpuFaceCulling & ~mask) | (value & mask);

    // Set the culling mode for the renderer
    switch (gpuFaceCulling) {
        case 0x1: return gpuRender->setCullMode(CULL_FRONT);
        case 0x2: return gpuRender->setCullMode(CULL_BACK);
        default: return gpuRender->setCullMode(CULL_NONE);
    }
}

void Gpu::writeViewScaleH(uint32_t mask, uint32_t value) {
    // Write to the viewport horizontal scale and send it to the renderer
    gpuViewScaleH = (gpuViewScaleH & ~mask) | (value & mask);
    uint32_t conv = flt24e7to32e8(gpuViewScaleH);
    gpuRender->setViewScaleH(*(float*)&conv);
}

void Gpu::writeViewStepH(uint32_t mask, uint32_t value) {
    // Write to the viewport horizontal step and send it to the renderer
    gpuViewStepH = (gpuViewStepH & ~mask) | (value & mask);
    uint32_t conv = flt32e7to32e8(gpuViewStepH);
    gpuRender->setViewStepH(*(float*)&conv);
}

void Gpu::writeViewScaleV(uint32_t mask, uint32_t value) {
    // Write to the viewport vertical scale and send it to the renderer
    gpuViewScaleV = (gpuViewScaleV & ~mask) | (value & mask);
    uint32_t conv = flt24e7to32e8(gpuViewScaleV);
    gpuRender->setViewScaleV(*(float*)&conv);
}

void Gpu::writeViewStepV(uint32_t mask, uint32_t value) {
    // Write to the viewport vertical step and send it to the renderer
    gpuViewStepV = (gpuViewStepV & ~mask) | (value & mask);
    uint32_t conv = flt32e7to32e8(gpuViewStepV);
    gpuRender->setViewStepV(*(float*)&conv);
}

void Gpu::writeShdOutTotal(uint32_t mask, uint32_t value) {
    // Write to the shader output mapping total count
    mask &= 0x7;
    gpuShdOutTotal = (gpuShdOutTotal & ~mask) | (value & mask);
    shdMapDirty = true;
}

template <int i> void Gpu::writeShdOutMap(uint32_t mask, uint32_t value) {
    // Write to one of the shader output mapping registers
    mask &= 0x1F1F1F1F;
    gpuShdOutMap[i] = (gpuShdOutMap[i] & ~mask) | (value & mask);
    shdMapDirty = true;
}

template <int i> void Gpu::writeTexBorder(uint32_t mask, uint32_t value) {
    // Write to one of the texture border colors and send it to the renderer
    gpuTexBorder[i] = (gpuTexBorder[i] & ~mask) | (value & mask);
    float r = float((gpuTexBorder[i] >> 0) & 0xFF) / 0xFF;
    float g = float((gpuTexBorder[i] >> 8) & 0xFF) / 0xFF;
    float b = float((gpuTexBorder[i] >> 16) & 0xFF) / 0xFF;
    float a = float((gpuTexBorder[i] >> 24) & 0xFF) / 0xFF;
    gpuRender->setTexBorder(i, r, g, b, a);
}

template <int i> void Gpu::writeTexDim(uint32_t mask, uint32_t value) {
    // Write to one of the texture dimensions and send them to the renderer
    mask &= 0x7FFFFFF;
    gpuTexDim[i] = (gpuTexDim[i] & ~mask) | (value & mask);
    gpuRender->setTexDims(i, gpuTexDim[i] >> 16, gpuTexDim[i] & 0x7FF);
}

template <int i> void Gpu::writeTexParam(uint32_t mask, uint32_t value) {
    // Write to one of the texture parameter registers and update the renderer's state
    // TODO: use bits other than coordinate wraps
    mask &= (i ? 0x1FFFFFF : 0x71FFFFFF);
    gpuTexParam[i] = (gpuTexParam[i] & ~mask) | (value & mask);
    gpuRender->setTexWrapS(i, TexWrap((gpuTexParam[i] >> 12) & 0x3));
    gpuRender->setTexWrapT(i, TexWrap((gpuTexParam[i] >> 8) & 0x3));
}

template <int i> void Gpu::writeTexAddr1(uint32_t mask, uint32_t value) {
    // Write to one of the texture addresses and send it to the renderer
    mask &= 0xFFFFFFF;
    gpuTexAddr1[i] = (gpuTexAddr1[i] & ~mask) | (value & mask);
    gpuRender->setTexAddr(i, gpuTexAddr1[i] << 3);
}

template <int i> void Gpu::writeTexType(uint32_t mask, uint32_t value) {
    // Write to one of the texture types
    mask &= 0xF;
    gpuTexType[i] = (gpuTexType[i] & ~mask) | (value & mask);

    // Set the format for the renderer if it's valid
    if (gpuTexType[i] < 0xE)
        return gpuRender->setTexFmt(i, TexFmt(gpuTexType[i]));
    LOG_WARN("GPU texture %d set to unknown format: 0x%X\n", i, gpuTexType[i]);
    return gpuRender->setTexFmt(i, TEX_UNK);
}

template <int i> void Gpu::writeCombSrc(uint32_t mask, uint32_t value) {
    // Write to one of the texture combiner source registers
    mask &= 0xFFF0FFF;
    gpuCombSrc[i] = (gpuCombSrc[i] & ~mask) | (value & mask);

    // Set the sources for the renderer if they're valid
    for (int j = 0; j < 6; j++) {
        switch (uint8_t src = (gpuCombSrc[i] >> ((j + (j > 2)) * 4)) & 0xF) {
            case 0x0: gpuRender->setCombSrc(i, j, COMB_PRIM); continue;
            case 0x1: gpuRender->setCombSrc(i, j, COMB_FRAG0); continue;
            case 0x2: gpuRender->setCombSrc(i, j, COMB_FRAG1); continue;
            case 0x3: gpuRender->setCombSrc(i, j, COMB_TEX0); continue;
            case 0x4: gpuRender->setCombSrc(i, j, COMB_TEX1); continue;
            case 0x5: gpuRender->setCombSrc(i, j, COMB_TEX2); continue;
            case 0x6: gpuRender->setCombSrc(i, j, COMB_TEX3); continue;
            case 0xD: gpuRender->setCombSrc(i, j, COMB_PRVBUF); continue;
            case 0xE: gpuRender->setCombSrc(i, j, COMB_CONST); continue;
            case 0xF: gpuRender->setCombSrc(i, j, COMB_PREV); continue;

        default:
            // Catch unknown texture combiner source values
            LOG_WARN("GPU texture combiner %d source %d set to unknown value: 0x%X\n", i, j, src);
            gpuRender->setCombSrc(i, j, COMB_UNK);
            continue;
        }
    }
}

template <int i> void Gpu::writeCombOper(uint32_t mask, uint32_t value) {
    // Write to some of the texture combiner operands
    mask &= 0x777FFF;
    gpuCombOper[i] = (gpuCombOper[i] & ~mask) | (value & mask);

    // Set RGB operands for the renderer if they're valid
    for (int j = 0; j < 3; j++) {
        switch (uint8_t oper = (gpuCombOper[i] >> (j * 4)) & 0xF) {
            case 0x0: gpuRender->setCombOper(i, j, OPER_SRC); continue;
            case 0x1: gpuRender->setCombOper(i, j, OPER_1MSRC); continue;
            case 0x2: gpuRender->setCombOper(i, j, OPER_SRCA); continue;
            case 0x3: gpuRender->setCombOper(i, j, OPER_1MSRCA); continue;
            case 0x4: gpuRender->setCombOper(i, j, OPER_SRCR); continue;
            case 0x5: gpuRender->setCombOper(i, j, OPER_1MSRCR); continue;
            case 0x8: gpuRender->setCombOper(i, j, OPER_SRCG); continue;
            case 0x9: gpuRender->setCombOper(i, j, OPER_1MSRCG); continue;
            case 0xC: gpuRender->setCombOper(i, j, OPER_SRCB); continue;
            case 0xD: gpuRender->setCombOper(i, j, OPER_1MSRCB); continue;

        default:
            // Catch unknown RGB operand values
            LOG_WARN("GPU texture combiner %d operand %d set to unknown value: 0x%X\n", i, j, oper);
            gpuRender->setCombOper(i, j, OPER_SRC);
            continue;
        }
    }

    // Set alpha operands for the renderer
    for (int j = 3; j < 6; j++) {
        switch ((gpuCombOper[i] >> (j * 4)) & 0x7) {
            case 0x0: gpuRender->setCombOper(i, j, OPER_SRCA); continue;
            case 0x1: gpuRender->setCombOper(i, j, OPER_1MSRCA); continue;
            case 0x2: gpuRender->setCombOper(i, j, OPER_SRCR); continue;
            case 0x3: gpuRender->setCombOper(i, j, OPER_1MSRCR); continue;
            case 0x4: gpuRender->setCombOper(i, j, OPER_SRCG); continue;
            case 0x5: gpuRender->setCombOper(i, j, OPER_1MSRCG); continue;
            case 0x6: gpuRender->setCombOper(i, j, OPER_SRCB); continue;
            default: gpuRender->setCombOper(i, j, OPER_1MSRCB); continue;
        }
    }
}

template <int i> void Gpu::writeCombMode(uint32_t mask, uint32_t value) {
    // Write to one of the texture combiner mode registers
    mask &= 0xF000F;
    gpuCombMode[i] = (gpuCombMode[i] & ~mask) | (value & mask);

    // Set the modes for the renderer if they're valid
    for (int j = 0; j < 2; j++) {
        uint8_t mode = (gpuCombMode[i] >> (j * 16)) & 0xF;
        if (mode < 0xA) {
            gpuRender->setCombMode(i, j, CalcMode(mode));
            continue;
        }
        LOG_WARN("GPU texture combiner %d mode %d set to unknown value: 0x%X\n", i, j, mode);
        gpuRender->setCombMode(i, j, MODE_UNK);
    }
}

template <int i> void Gpu::writeCombColor(uint32_t mask, uint32_t value) {
    // Write to one of the texture combiner colors and send it to the renderer
    gpuCombColor[i] = (gpuCombColor[i] & ~mask) | (value & mask);
    float r = float((gpuCombColor[i] >> 0) & 0xFF) / 0xFF;
    float g = float((gpuCombColor[i] >> 8) & 0xFF) / 0xFF;
    float b = float((gpuCombColor[i] >> 16) & 0xFF) / 0xFF;
    float a = float((gpuCombColor[i] >> 24) & 0xFF) / 0xFF;
    gpuRender->setCombColor(i, r, g, b, a);
}

void Gpu::writeCombBufUpd(uint32_t mask, uint32_t value) {
    // Write to the texture combiner buffer update register and set the renderer's mask
    // TODO: use other bits (or are they just for fog?)
    mask &= 0x301FF0F;
    gpuCombBufUpd = (gpuCombBufUpd & ~mask) | (value & mask);
    gpuRender->setCombBufMask(gpuCombBufUpd >> 8);
}

void Gpu::writeCombBufCol(uint32_t mask, uint32_t value) {
    // Write to the texture combiner buffer color and send it to the renderer
    gpuCombBufCol = (gpuCombBufCol & ~mask) | (value & mask);
    float r = float((gpuCombBufCol >> 0) & 0xFF) / 0xFF;
    float g = float((gpuCombBufCol >> 8) & 0xFF) / 0xFF;
    float b = float((gpuCombBufCol >> 16) & 0xFF) / 0xFF;
    float a = float((gpuCombBufCol >> 24) & 0xFF) / 0xFF;
    gpuRender->setCombBufColor(r, g, b, a);
}

void Gpu::writeBlendFunc(uint32_t mask, uint32_t value) {
    // Write to the blender function register
    mask &= 0xFFFF0707;
    gpuBlendFunc = (gpuBlendFunc & ~mask) | (value & mask);

    // Set the blend modes for the renderer
    for (int i = 0; i < 2; i++) {
        switch ((gpuBlendFunc >> (i * 8)) & 0x7) {
            default: gpuRender->setBlendMode(i, MODE_ADD); continue;
            case 0x1: gpuRender->setBlendMode(i, MODE_SUB); continue;
            case 0x2: gpuRender->setBlendMode(i, MODE_RSUB); continue;
            case 0x3: gpuRender->setBlendMode(i, MODE_MIN); continue;
            case 0x4: gpuRender->setBlendMode(i, MODE_MAX); continue;
        }
    }

    // Set the blend operands for the renderer if they're valid
    for (int i = 0; i < 4; i++) {
        uint8_t oper = (gpuBlendFunc >> (16 + (i * 4))) & 0xF;
        if (oper < 0xE) {
            gpuRender->setBlendOper(i, BlendOper(oper));
            continue;
        }
        LOG_WARN("GPU blender operand %d set to unknown value: 0x%X\n", i, oper);
        gpuRender->setBlendOper(i, BLND_ONE);
    }
}

void Gpu::writeBlendColor(uint32_t mask, uint32_t value) {
    // Write to the blender constant color and send it to the renderer
    gpuBlendColor = (gpuBlendColor & ~mask) | (value & mask);
    float r = float((gpuBlendColor >> 0) & 0xFF) / 0xFF;
    float g = float((gpuBlendColor >> 8) & 0xFF) / 0xFF;
    float b = float((gpuBlendColor >> 16) & 0xFF) / 0xFF;
    float a = float((gpuBlendColor >> 24) & 0xFF) / 0xFF;
    gpuRender->setBlendColor(r, g, b, a);
}

void Gpu::writeAlphaTest(uint32_t mask, uint32_t value) {
    // Write to the alpha test register and update the renderer's state
    mask &= 0xFF71;
    gpuAlphaTest = (gpuAlphaTest & ~mask) | (value & mask);
    gpuRender->setAlphaFunc((gpuAlphaTest & BIT(0)) ? TestFunc((gpuAlphaTest >> 4) & 0x7) : TEST_AL);
    gpuRender->setAlphaValue(float(gpuAlphaTest >> 8) / 0xFF);
}

void Gpu::writeStencilTest(uint32_t mask, uint32_t value) {
    // Write to the stencil test register and update the renderer's state
    mask &= 0xFFFFFF71;
    gpuStencilTest = (gpuStencilTest & ~mask) | (value & mask);
    gpuRender->setStencilTest(TestFunc((gpuStencilTest >> 4) & 0x7), gpuStencilTest & BIT(0));
    gpuRender->setStencilMasks(gpuStencilTest >> 8, gpuStencilTest >> 24);
    gpuRender->setStencilValue(gpuStencilTest >> 16);
}

void Gpu::writeStencilOp(uint32_t mask, uint32_t value) {
    // Write to the stencil operation register and update the renderer's state
    mask &= 0x777;
    gpuStencilOp = (gpuStencilOp & ~mask) | (value & mask);
    gpuRender->setStencilOps(StenOper(gpuStencilOp & 0x7),
        StenOper((gpuStencilOp >> 4) & 0x7), StenOper((gpuStencilOp >> 8) & 0x7));
}

void Gpu::writeDepcolMask(uint32_t mask, uint32_t value) {
    // Write to the depth/color mask register and update the renderer's state
    mask &= 0x1F71;
    gpuDepcolMask = (gpuDepcolMask & ~mask) | (value & mask);
    gpuRender->setDepthFunc((gpuDepcolMask & BIT(0)) ? TestFunc((gpuDepcolMask >> 4) & 0x7) : TEST_AL);
    gpuRender->setColbufMask(gpuColbufWrite ? ((gpuDepcolMask >> 8) & 0xF) : 0);
    gpuRender->setDepbufMask(gpuDepbufWrite ? (((gpuDepcolMask >> 11) & 0x2) | 0x1) : 0);
}

void Gpu::writeColbufWrite(uint32_t mask, uint32_t value) {
    // Write to the color buffer write enable and update the renderer's color mask
    mask &= 0xF;
    gpuColbufWrite = (gpuColbufWrite & ~mask) | (value & mask);
    gpuRender->setColbufMask(gpuColbufWrite ? ((gpuDepcolMask >> 8) & 0xF) : 0);
}

void Gpu::writeDepbufWrite(uint32_t mask, uint32_t value) {
    // Write to the depth buffer write enable and update the renderer's depth mask
    mask &= 0x3;
    gpuDepbufWrite = (gpuDepbufWrite & ~mask) | (value & mask);
    gpuRender->setDepbufMask(gpuDepbufWrite ? ((gpuDepcolMask >> 11) & 0x2) : 0);
}

void Gpu::writeDepbufFmt(uint32_t mask, uint32_t value) {
    // Write to the depth buffer format register
    mask &= 0x3;
    gpuDepbufFmt = (gpuDepbufFmt & ~mask) | (value & mask);

    // Set the format for the renderer if it's valid
    switch (gpuDepbufFmt) {
        case 0x0: return gpuRender->setDepbufFmt(DEP_16);
        case 0x2: return gpuRender->setDepbufFmt(DEP_24);
        case 0x3: return gpuRender->setDepbufFmt(DEP_24S8);

    default:
        // Catch unknown depth buffer formats
        LOG_CRIT("Setting unknown GPU depth buffer format: 0x%X\n", gpuDepbufFmt);
        return gpuRender->setDepbufFmt(DEP_UNK);
    }
}

void Gpu::writeColbufFmt(uint32_t mask, uint32_t value) {
    // Write to the color buffer format register
    mask &= 0x70003;
    gpuColbufFmt = (gpuColbufFmt & ~mask) | (value & mask);

    // Set the format for the renderer if it's valid
    switch (gpuColbufFmt) {
        case 0x00002: return gpuRender->setColbufFmt(COL_RGBA8);
        case 0x10001: return gpuRender->setColbufFmt(COL_RGB8);
        case 0x20000: return gpuRender->setColbufFmt(COL_RGB5A1);
        case 0x30000: return gpuRender->setColbufFmt(COL_RGB565);
        case 0x40000: return gpuRender->setColbufFmt(COL_RGBA4);

    default:
        // Catch unknown color buffer formats
        LOG_CRIT("Setting unknown GPU color buffer format: 0x%X\n", gpuColbufFmt);
        return gpuRender->setColbufFmt(COL_UNK);
    }
}

void Gpu::writeDepbufLoc(uint32_t mask, uint32_t value) {
    // Write to the depth buffer location and send its address to the renderer
    mask &= 0xFFFFFFF;
    gpuDepbufLoc = (gpuDepbufLoc & ~mask) | (value & mask);
    gpuRender->setDepbufAddr((gpuDepbufLoc & ~0x7) << 3);
}

void Gpu::writeColbufLoc(uint32_t mask, uint32_t value) {
    // Write to the color buffer location and send its address to the renderer
    mask &= 0xFFFFFFF;
    gpuColbufLoc = (gpuColbufLoc & ~mask) | (value & mask);
    gpuRender->setColbufAddr((gpuColbufLoc & ~0x7) << 3);
}

void Gpu::writeBufferDim(uint32_t mask, uint32_t value) {
    // Write to the render buffer dimensions and send them to the renderer
    mask &= 0x13FF7FF;
    gpuColbufLoc = (gpuColbufLoc & ~mask) | (value & mask);
    gpuRender->setBufferDims(gpuColbufLoc & 0x7FF, ((gpuColbufLoc >> 12) & 0x3FF) + 1, gpuColbufLoc & BIT(24));
}

void Gpu::writeAttrBase(uint32_t mask, uint32_t value) {
    // Write to the attribute buffer base address
    mask &= 0x1FFFFFFE;
    gpuAttrBase = (gpuAttrBase & ~mask) | (value & mask);
}

void Gpu::writeAttrFmtL(uint32_t mask, uint32_t value) {
    // Write to the lower attribute buffer format register
    gpuAttrFmt = (gpuAttrFmt & ~mask) | (value & mask);
}

void Gpu::writeAttrFmtH(uint32_t mask, uint32_t value) {
    // Write to the upper attribute buffer format register
    gpuAttrFmt = (gpuAttrFmt & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
    fixedDirty = true;
}

template <int i> void Gpu::writeAttrOfs(uint32_t mask, uint32_t value) {
    // Write to one of the attribute buffer address offsets
    mask &= 0xFFFFFFF;
    gpuAttrOfs[i] = (gpuAttrOfs[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeAttrCfgL(uint32_t mask, uint32_t value) {
    // Write to one of the lower attribute buffer config registers
    gpuAttrCfg[i] = (gpuAttrCfg[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeAttrCfgH(uint32_t mask, uint32_t value) {
    // Write to one of the upper attribute buffer config registers
    gpuAttrCfg[i] = (gpuAttrCfg[i] & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
}

void Gpu::writeAttrIdxList(uint32_t mask, uint32_t value) {
    // Write to the attribute buffer index list register
    mask &= 0x8FFFFFFF;
    gpuAttrIdxList = (gpuAttrIdxList & ~mask) | (value & mask);
}

void Gpu::writeAttrNumVerts(uint32_t mask, uint32_t value) {
    // Write to the attribute buffer vertex render count
    gpuAttrNumVerts = (gpuAttrNumVerts & ~mask) | (value & mask);
}

void Gpu::writeGshConfig(uint32_t mask, uint32_t value) {
    // Write to the geometry config register and set the shader's enable state
    // TODO: use the other bits for something?
    mask &= 0x800F0303;
    gpuGshConfig = (gpuGshConfig & ~mask) | (value & mask);
    gpuShader->setGshInCount((gpuGshConfig & BIT(1)) ? (gpuVshOutTotal + 1) : 0);
    shdMapDirty = true;
}

void Gpu::writeAttrFirstIdx(uint32_t mask, uint32_t value) {
    // Write to the attribute buffer starting index
    gpuAttrFirstIdx = (gpuAttrFirstIdx & ~mask) | (value & mask);
}

void Gpu::writeAttrDrawArrays(uint32_t mask, uint32_t value) {
    // Update shader state before a new vertex batch
    if (shdMapDirty) updateShdMaps();
    gpuShader->startList();

    // Draw vertices from the attribute buffer using increasing indices
    LOG_INFO("GPU sending %d linear vertices to be rendered\n", gpuAttrNumVerts);
    for (uint32_t i = 0; i < gpuAttrNumVerts; i++)
        drawAttrIdx(gpuAttrFirstIdx + i);
}

void Gpu::writeAttrDrawElems(uint32_t mask, uint32_t value) {
    // Update shader state before a new vertex batch
    if (shdMapDirty) updateShdMaps();
    gpuShader->startList();

    // Draw vertices from the attribute buffer using indices from a list
    LOG_INFO("GPU sending %d indexed vertices to be rendered\n", gpuAttrNumVerts);
    uint32_t base = (gpuAttrBase << 3) + (gpuAttrIdxList & 0xFFFFFFF);
    if (gpuAttrIdxList & BIT(31)) // 16-bit
        for (uint32_t i = 0; i < gpuAttrNumVerts; i++)
            drawAttrIdx(core->memory.read<uint16_t>(ARM11, base + (i << 1)));
    else // 8-bit
        for (uint32_t i = 0; i < gpuAttrNumVerts; i++)
            drawAttrIdx(core->memory.read<uint8_t>(ARM11, base + i));
}

void Gpu::writeAttrFixedIdx(uint32_t mask, uint32_t value) {
    // Update the fixed attribute data index if written to
    if (mask & 0xFF)
        attrFixedIdx = (value & 0xF) << 2;
}

void Gpu::writeAttrFixedData(uint32_t mask, uint32_t value) {
    // Write to the selected 3-word fixed attribute data buffer
    attrFixedData[attrFixedIdx >> 2][attrFixedIdx & 0x3] = (value & mask);

    // In non-immediate mode, reset the index once the attribute is finished
    if (attrFixedIdx < (0xF << 2)) {
        if ((++attrFixedIdx & 0x3) == 0x3) attrFixedIdx &= ~0x3;
        fixedDirty = true;
        return;
    }

    // In immediate mode, increment the index and check if a full vertex has been sent
    if ((++attrFixedIdx & 0x3) == 0x3) attrFixedIdx++;
    if (attrFixedIdx != ((gpuVshNumAttr + 0x10) << 2)) return;
    attrFixedIdx = (0xF << 2); // Reset index

    // Build a shader input list using the immediate attributes
    float input[16][4] = {};
    for (uint32_t i = 0, f; i <= gpuVshNumAttr; i++) {
        uint32_t j = i + 0xF;
        input[i][0] = *(float*)&(f = flt24e7to32e8(attrFixedData[j][2]));
        input[i][1] = *(float*)&(f = flt24e7to32e8((attrFixedData[j][1] << 8) | (attrFixedData[j][2] >> 24)));
        input[i][2] = *(float*)&(f = flt24e7to32e8((attrFixedData[j][0] << 16) | (attrFixedData[j][1] >> 16)));
        input[i][3] = *(float*)&(f = flt24e7to32e8(attrFixedData[j][0] >> 8));
    }

    // Pass the finished input to the shader
    if (shdMapDirty) updateShdMaps();
    gpuShader->processVtx(input);
}

template <int i> void Gpu::writeCmdSize(uint32_t mask, uint32_t value) {
    // Write to one of the command buffer sizes
    mask &= 0x1FFFFE;
    gpuCmdSize[i] = (gpuCmdSize[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeCmdAddr(uint32_t mask, uint32_t value) {
    // Write to one of the command buffer addresses
    mask &= 0x1FFFFFFE;
    gpuCmdAddr[i] = (gpuCmdAddr[i] & ~mask) | (value & mask);
}

template <int i> void Gpu::writeCmdJump(uint32_t mask, uint32_t value) {
    // Jump to a new command buffer and start executing if stopped
    bool stopped = (cmdAddr == -1);
    cmdAddr = (gpuCmdAddr[i] << 3);
    cmdEnd = (gpuCmdAddr[i] + gpuCmdSize[i]) << 3;
    LOG_INFO("Jumping to GPU command list at 0x%X with size 0x%X\n", cmdAddr, cmdEnd - cmdAddr);
    if (stopped) runCommands();
}

void Gpu::writeVshNumAttr(uint32_t mask, uint32_t value) {
    // Write to the vertex shader input attribute count
    mask &= 0xF;
    gpuVshNumAttr = (gpuVshNumAttr & ~mask) | (value & mask);
}

void Gpu::writeVshOutTotal(uint32_t mask, uint32_t value) {
    // Write to the vertex output attribute count and send it to the shader
    mask &= 0xF;
    gpuVshOutTotal = (gpuVshOutTotal & ~mask) | (value & mask);
    gpuShader->setGshInCount((gpuGshConfig & BIT(1)) ? (gpuVshOutTotal + 1) : 0);
}

void Gpu::writePrimConfig(uint32_t mask, uint32_t value) {
    // Write to the primitive configuration register
    // TODO: use bits other than primitive mode?
    mask &= 0x1030F;
    gpuPrimConfig = (gpuPrimConfig & ~mask) | (value & mask);
}

void Gpu::writePrimRestart(uint32_t mask, uint32_t value) {
    // Write to the primitive restart register and pass new mode to the renderer
    gpuPrimRestart = (gpuPrimConfig & ~mask) | (value & mask);
    gpuRender->setPrimMode(PrimMode((gpuPrimConfig >> 8) & 0x3));
}

void Gpu::writeGshBools(uint32_t mask, uint32_t value) {
    // Write to the geometry uniform boolean register
    // TODO: use the upper bits for something?
    mask &= 0xFFFFFFF;
    gpuGshBools = (gpuGshBools & ~mask) | (value & mask);

    // Update the uniform booleans in the shader
    for (int i = 0; i < 16; i++)
        gpuShader->setGshBool(i, gpuGshBools & BIT(i));
}

template <int i> void Gpu::writeGshInts(uint32_t mask, uint32_t value) {
    // Write to one of the geometry uniform integer registers
    mask &= 0xFFFFFF;
    gpuGshInts[i] = (gpuGshInts[i] & ~mask) | (value & mask);

    // Update some of the uniform integers in the shader
    for (int j = 0; j < 3; j++)
        gpuShader->setGshInt(i, j, gpuGshInts[i] >> (j << 3));
}

void Gpu::writeGshInputCfg(uint32_t mask, uint32_t value) {
    // Write to the geometry shader input config register
    // TODO: use bits other than attribute count?
    mask &= 0xB800010F;
    gpuGshInputCfg = (gpuGshInputCfg & ~mask) | (value & mask);
    shdMapDirty = true;
}

void Gpu::writeGshEntry(uint32_t mask, uint32_t value) {
    // Write to the geometry entry/end points and send them to the shader
    mask &= 0xFFF0FFF;
    gpuGshEntry = (gpuGshEntry & ~mask) | (value & mask);
    gpuShader->setGshEntry(gpuGshEntry >> 0, gpuGshEntry >> 16);
}

void Gpu::writeGshAttrIdsL(uint32_t mask, uint32_t value) {
    // Write to the lower geometry shader attribute IDs
    gpuGshAttrIds = (gpuGshAttrIds & ~mask) | (value & mask);
    shdMapDirty = true;
}

void Gpu::writeGshAttrIdsH(uint32_t mask, uint32_t value) {
    // Write to the upper geometry shader attribute IDs
    gpuGshAttrIds = (gpuGshAttrIds & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
    shdMapDirty = true;
}

void Gpu::writeGshOutMask(uint32_t mask, uint32_t value) {
    // Write to the geometry shader output mask
    mask &= 0x7FFFFFF;
    gpuGshOutMask = (gpuGshOutMask & ~mask) | (value & mask);
    shdMapDirty = true;
}

void Gpu::writeGshFloatIdx(uint32_t mask, uint32_t value) {
    // Update the geometry uniform float index register state if written to
    if (mask & 0x000000FF) gshFloatIdx = (value & 0xFF) << 2;
    if (mask & 0xFF000000) gshFloat32 = (value & BIT(31));
}

void Gpu::writeGshFloatData(uint32_t mask, uint32_t value) {
    // Write to the geometry uniform float buffer and convert 24-bit to 32-bit if necessary
    gshFloatData[~gshFloatIdx & 0x3] = (value & mask);
    if (!gshFloat32 && (gshFloatIdx & 0x3) == 0x2) {
        gshFloatData[0] = flt24e7to32e8(gshFloatData[1]);
        gshFloatData[1] = flt24e7to32e8((gshFloatData[2] << 8) | (gshFloatData[1] >> 24));
        gshFloatData[2] = flt24e7to32e8((gshFloatData[3] << 16) | (gshFloatData[2] >> 16));
        gshFloatData[3] = flt24e7to32e8(gshFloatData[3] >> 8);
        gshFloatIdx++;
    }

    // Increment the index and update the shader once 4 floats are received
    if (gshFloatIdx++ < (96 << 2) && !(gshFloatIdx & 0x3))
        for (int i = 0; i < 4; i++)
            gpuShader->setGshFloat((gshFloatIdx >> 2) - 1, i, *(float*)&gshFloatData[i]);
}

void Gpu::writeGshCodeIdx(uint32_t mask, uint32_t value) {
    // Write to the geometry shader program data index
    gpuGshCodeIdx = (gpuGshCodeIdx & ~mask) | (value & mask);
}

void Gpu::writeGshCodeData(uint32_t mask, uint32_t value) {
    // Write to the current geometry shader program index and increment it
    gpuShader->writeGshCode(gpuGshCodeIdx++ & 0xFFF, value & mask);
}

void Gpu::writeGshDescIdx(uint32_t mask, uint32_t value) {
    // Write to the geometry operand descriptor data index
    gpuGshDescIdx = (gpuGshDescIdx & ~mask) | (value & mask);
}

void Gpu::writeGshDescData(uint32_t mask, uint32_t value) {
    // Write to the current geometry operand descriptor index and increment it
    gpuShader->writeGshDesc(gpuGshDescIdx++ & 0x7F, value & mask);
}

void Gpu::writeVshBools(uint32_t mask, uint32_t value) {
    // Write to the vertex uniform boolean register
    // TODO: use the upper bits for something?
    mask &= 0x1FFFFFF;
    gpuVshBools = (gpuVshBools & ~mask) | (value & mask);

    // Update the uniform booleans in the shader
    for (int i = 0; i < 16; i++)
        gpuShader->setVshBool(i, gpuVshBools & BIT(i));
}

template <int i> void Gpu::writeVshInts(uint32_t mask, uint32_t value) {
    // Write to one of the vertex uniform integer registers
    mask &= 0xFFFFFF;
    gpuVshInts[i] = (gpuVshInts[i] & ~mask) | (value & mask);

    // Update some of the uniform integers in the shader
    for (int j = 0; j < 3; j++)
        gpuShader->setVshInt(i, j, gpuVshInts[i] >> (j << 3));
}

void Gpu::writeVshEntry(uint32_t mask, uint32_t value) {
    // Write to the vertex entry/end points and send them to the shader
    mask &= 0x1FF01FF;
    gpuVshEntry = (gpuVshEntry & ~mask) | (value & mask);
    gpuShader->setVshEntry(gpuVshEntry >> 0, gpuVshEntry >> 16);
}

void Gpu::writeVshAttrIdsL(uint32_t mask, uint32_t value) {
    // Write to the lower vertex shader attribute IDs
    gpuVshAttrIds = (gpuVshAttrIds & ~mask) | (value & mask);
    fixedDirty = true;
}

void Gpu::writeVshAttrIdsH(uint32_t mask, uint32_t value) {
    // Write to the upper vertex shader attribute IDs
    gpuVshAttrIds = (gpuVshAttrIds & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
}

void Gpu::writeVshOutMask(uint32_t mask, uint32_t value) {
    // Write to the vertex shader output mask
    mask &= 0x7FFFFFF;
    gpuVshOutMask = (gpuVshOutMask & ~mask) | (value & mask);
    shdMapDirty = true;
}

void Gpu::writeVshFloatIdx(uint32_t mask, uint32_t value) {
    // Update the vertex uniform float index register state if written to
    if (mask & 0x000000FF) vshFloatIdx = (value & 0xFF) << 2;
    if (mask & 0xFF000000) vshFloat32 = (value & BIT(31));
}

void Gpu::writeVshFloatData(uint32_t mask, uint32_t value) {
    // Write to the vertex uniform float buffer and convert 24-bit to 32-bit if necessary
    vshFloatData[~vshFloatIdx & 0x3] = (value & mask);
    if (!vshFloat32 && (vshFloatIdx & 0x3) == 0x2) {
        vshFloatData[0] = flt24e7to32e8(vshFloatData[1]);
        vshFloatData[1] = flt24e7to32e8((vshFloatData[2] << 8) | (vshFloatData[1] >> 24));
        vshFloatData[2] = flt24e7to32e8((vshFloatData[3] << 16) | (vshFloatData[2] >> 16));
        vshFloatData[3] = flt24e7to32e8(vshFloatData[3] >> 8);
        vshFloatIdx++;
    }

    // Increment the index and update the shader once 4 floats are received
    if (vshFloatIdx++ < (96 << 2) && !(vshFloatIdx & 0x3))
        for (int i = 0; i < 4; i++)
            gpuShader->setVshFloat((vshFloatIdx >> 2) - 1, i, *(float*)&vshFloatData[i]);
}

void Gpu::writeVshCodeIdx(uint32_t mask, uint32_t value) {
    // Write to the vertex shader program data index
    gpuVshCodeIdx = (gpuVshCodeIdx & ~mask) | (value & mask);
}

void Gpu::writeVshCodeData(uint32_t mask, uint32_t value) {
    // Write to the current vertex shader program index and increment it
    gpuShader->writeVshCode(gpuVshCodeIdx++ & 0x1FF, value & mask);
}

void Gpu::writeVshDescIdx(uint32_t mask, uint32_t value) {
    // Write to the vertex operand descriptor data index
    gpuVshDescIdx = (gpuVshDescIdx & ~mask) | (value & mask);
}

void Gpu::writeVshDescData(uint32_t mask, uint32_t value) {
    // Write to the current vertex operand descriptor index and increment it
    gpuShader->writeVshDesc(gpuVshDescIdx++ & 0x7F, value & mask);
}

void Gpu::writeUnkCmd(uint32_t mask, uint32_t value) {
    // Catch unknown GPU commands, pulling ID from the thread if running
    uint16_t cmd = (tasks.empty() ? curCmd : *(uint32_t*)tasks.front().data) & 0x3FF;
    LOG_WARN("Unknown GPU command ID: 0x%X\n", cmd);
}
