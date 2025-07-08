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

TEMPLATE4(void Gpu::writeIrqReq, 0, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 4, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 8, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeIrqReq, 12, uint32_t, uint32_t)
TEMPLATE4(void Gpu::writeShdOutMap, 0, uint32_t, uint32_t)
TEMPLATE3(void Gpu::writeShdOutMap, 4, uint32_t, uint32_t)
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
TEMPLATE4(void Gpu::writeVshInts, 0, uint32_t, uint32_t)

FORCE_INLINE uint32_t Gpu::flt24e7to32e8(uint32_t value) {
    // Convert a 24-bit float with 7-bit exponent to 32-bit with 8-bit exponent
    return ((value << 8) & BIT(31)) | (((value & 0x7FFFFF) + 0x400000) << 7);
}

FORCE_INLINE uint32_t Gpu::flt32e7to32e8(uint32_t value) {
    // Convert a 32-bit float with 7-bit exponent to 32-bit with 8-bit exponent
    return (value & BIT(31)) | (((value & 0x7FFFFFFF) + 0x40000000) >> 1);
}

void Gpu::runCommands() {
    // Execute GPU commands until the end is reached
    while (cmdAddr < cmdEnd) {
        // Decode the command header
        uint32_t header = core->memory.read<uint32_t>(ARM11, cmdAddr + 4);
        uint32_t mask = maskTable[(header >> 16) & 0xF];
        uint8_t count = (header >> 20) & 0xFF;
        curCmd = (header & 0x3FF);

        // Adjust the address and write the first parameter
        uint32_t address = (cmdAddr + 4);
        cmdAddr += ((count + 3) << 2) & ~0x7;
        (this->*cmdWrites[curCmd])(mask, core->memory.read<uint32_t>(ARM11, address - 4));

        // Write the remaining parameters if any, with optionally increasing command ID
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
    // Build a shader input list by parsing attribute arrays at the given index
    float input[16][4] = {};
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

            // Handle components based on format and write them to their mapped input ID
            uint8_t fmt = (gpuAttrFmt >> (comp << 2)) & 0xF;
            uint8_t id = (gpuVshAttrIds >> (comp << 2)) & 0xF;
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

    // Run the finished input through the shader and handle primitive restarts
    core->gpuRender.runShader(input, restart ? PrimMode(((gpuPrimConfig >> 8) & 0x3) + 1) : SAME_PRIM);
    restart = false;
}

void Gpu::updateOutMap() {
    // Build a map of shader outputs to fixed semantics for the renderer
    uint8_t outMap[0x18][2] = {};
    for (int i = 0; i < gpuShdOutTotal; i++) {
        for (int j = 0; j < 4; j++) {
            uint8_t sem = (gpuShdOutMap[i] >> (j << 3));
            if (sem >= 0x18) continue;
            outMap[sem][0] = i;
            outMap[sem][1] = j;
        }
    }
    core->gpuRender.setOutMap(outMap);
}

template <int i> void Gpu::writeIrqReq(uint32_t mask, uint32_t value) {
    // Write IRQ request bytes, check if they match, and stop execution on interrupt if enabled
    gpuIrqReq[i] = (gpuIrqReq[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if ((mask & (0xFF << (j * 8))) && checkInterrupt((i << 2) + j) && gpuIrqAutostop)
            cmdAddr = -1;
}

void Gpu::writeViewScaleH(uint32_t mask, uint32_t value) {
    // Write to the viewport horizontal scale and send it to the renderer
    gpuViewScaleH = (gpuViewScaleH & ~mask) | (value & mask);
    uint32_t conv = flt24e7to32e8(gpuViewScaleH);
    core->gpuRender.setViewScaleH(*(float*)&conv);
}

void Gpu::writeViewStepH(uint32_t mask, uint32_t value) {
    // Write to the viewport horizontal step and send it to the renderer
    gpuViewStepH = (gpuViewStepH & ~mask) | (value & mask);
    uint32_t conv = flt32e7to32e8(gpuViewStepH);
    core->gpuRender.setViewStepH(*(float*)&conv);
}

void Gpu::writeViewScaleV(uint32_t mask, uint32_t value) {
    // Write to the viewport vertical scale and send it to the renderer
    gpuViewScaleV = (gpuViewScaleV & ~mask) | (value & mask);
    uint32_t conv = flt24e7to32e8(gpuViewScaleV);
    core->gpuRender.setViewScaleV(*(float*)&conv);
}

void Gpu::writeViewStepV(uint32_t mask, uint32_t value) {
    // Write to the viewport vertical step and send it to the renderer
    gpuViewStepV = (gpuViewStepV & ~mask) | (value & mask);
    uint32_t conv = flt32e7to32e8(gpuViewStepV);
    core->gpuRender.setViewStepV(*(float*)&conv);
}

void Gpu::writeShdOutTotal(uint32_t mask, uint32_t value) {
    // Write to the shader output mapping total count
    mask &= 0x7;
    gpuShdOutTotal = (gpuShdOutTotal & ~mask) | (value & mask);
    updateOutMap();
}

template <int i> void Gpu::writeShdOutMap(uint32_t mask, uint32_t value) {
    // Write to one of the shader output mapping registers
    mask &= 0x1F1F1F1F;
    gpuShdOutMap[i] = (gpuShdOutMap[i] & ~mask) | (value & mask);
    updateOutMap();
}

void Gpu::writeDepcolMask(uint32_t mask, uint32_t value) {
    // Write to the depth/color mask register and update the renderer's state
    mask &= 0x1F71;
    gpuDepcolMask = (gpuDepcolMask & ~mask) | (value & mask);
    core->gpuRender.setDepthFunc((gpuDepcolMask & BIT(0)) ? DepthFunc((gpuDepcolMask >> 4) & 0x7) : DEPTH_AL);
    core->gpuRender.setColbufMask(gpuColbufWrite & (gpuDepcolMask >> 8) & 0xF);
    core->gpuRender.setDepbufMask(gpuDepbufWrite & (((gpuDepcolMask >> 12) & 0x1) | ((gpuDepcolMask >> 11) & 0x2)));
}

void Gpu::writeColbufWrite(uint32_t mask, uint32_t value) {
    // Write to the color buffer write enable and update the renderer's color mask
    mask &= 0xF;
    gpuColbufWrite = (gpuColbufWrite & ~mask) | (value & mask);
    core->gpuRender.setColbufMask(gpuColbufWrite & (gpuDepcolMask >> 8) & 0xF);
}

void Gpu::writeDepbufWrite(uint32_t mask, uint32_t value) {
    // Write to the depth buffer write enable and update the renderer's depth mask
    mask &= 0x3;
    gpuDepbufWrite = (gpuDepbufWrite & ~mask) | (value & mask);
    core->gpuRender.setDepbufMask(gpuDepbufWrite & (((gpuDepcolMask >> 12) & 0x1) | ((gpuDepcolMask >> 11) & 0x2)));
}

void Gpu::writeDepbufFmt(uint32_t mask, uint32_t value) {
    // Write to the depth buffer format register
    mask &= 0x3;
    gpuDepbufFmt = (gpuDepbufFmt & ~mask) | (value & mask);

    // Set the format for the renderer if it's valid
    switch (gpuDepbufFmt) {
        case 0x0: return core->gpuRender.setDepbufFmt(DEP16);
        case 0x2: return core->gpuRender.setDepbufFmt(DEP24);
        case 0x3: return core->gpuRender.setDepbufFmt(DEP24STN8);

    default:
        // Catch unknown depth buffer formats
        LOG_CRIT("Setting unknown GPU depth buffer format: 0x%X\n", gpuDepbufFmt);
        return core->gpuRender.setDepbufFmt(DEP_UNK);
    }
}

void Gpu::writeColbufFmt(uint32_t mask, uint32_t value) {
    // Write to the color buffer format register
    mask &= 0x70003;
    gpuColbufFmt = (gpuColbufFmt & ~mask) | (value & mask);

    // Set the format for the renderer if it's valid
    switch (gpuColbufFmt) {
        case 0x00002: return core->gpuRender.setColbufFmt(RGBA8);
        case 0x20000: return core->gpuRender.setColbufFmt(RGB5A1);
        case 0x30000: return core->gpuRender.setColbufFmt(RGB565);
        case 0x40000: return core->gpuRender.setColbufFmt(RGBA4);

    default:
        // Catch unknown color buffer formats
        LOG_CRIT("Setting unknown GPU color buffer format: 0x%X\n", gpuColbufFmt);
        return core->gpuRender.setColbufFmt(COL_UNK);
    }
}

void Gpu::writeDepbufLoc(uint32_t mask, uint32_t value) {
    // Write to the depth buffer location and send its address to the renderer
    mask &= 0xFFFFFFF;
    gpuDepbufLoc = (gpuDepbufLoc & ~mask) | (value & mask);
    core->gpuRender.setDepbufAddr((gpuDepbufLoc & ~0x7) << 3);
}

void Gpu::writeColbufLoc(uint32_t mask, uint32_t value) {
    // Write to the color buffer location and send its address to the renderer
    mask &= 0xFFFFFFF;
    gpuColbufLoc = (gpuColbufLoc & ~mask) | (value & mask);
    core->gpuRender.setColbufAddr((gpuColbufLoc & ~0x7) << 3);
}

void Gpu::writeBufferDim(uint32_t mask, uint32_t value) {
    // Write to the render buffer dimensions and send them to the renderer
    mask &= 0x13FF7FF;
    gpuColbufLoc = (gpuColbufLoc & ~mask) | (value & mask);
    core->gpuRender.setBufferDims(gpuColbufLoc & 0x7FF, ((gpuColbufLoc >> 12) & 0x3FF) + 1, gpuColbufLoc & BIT(24));
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

void Gpu::writeAttrFirstIdx(uint32_t mask, uint32_t value) {
    // Write to the attribute buffer starting index
    gpuAttrFirstIdx = (gpuAttrFirstIdx & ~mask) | (value & mask);
}

void Gpu::writeAttrDrawArrays(uint32_t mask, uint32_t value) {
    // Draw vertices from the attribute buffer using increasing indices
    LOG_INFO("GPU sending %d linear vertices to be rendered\n", gpuAttrNumVerts);
    for (uint32_t i = 0; i < gpuAttrNumVerts; i++)
        drawAttrIdx(gpuAttrFirstIdx + i);
}

void Gpu::writeAttrDrawElems(uint32_t mask, uint32_t value) {
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

void Gpu::writePrimConfig(uint32_t mask, uint32_t value) {
    // Write to the primitive configuration register
    // TODO: use bits other than primitive mode?
    mask &= 0x1030F;
    gpuPrimConfig = (gpuPrimConfig & ~mask) | (value & mask);
}

void Gpu::writePrimRestart(uint32_t mask, uint32_t value) {
    // Write to the primitive restart register and restart on the next vertex
    gpuPrimRestart = (gpuPrimConfig & ~mask) | (value & mask);
    restart = true;
}

void Gpu::writeVshBools(uint32_t mask, uint32_t value) {
    // Write to the vertex uniform boolean register
    // TODO: use the upper bits for something?
    mask &= 0x1FFFFFF;
    gpuVshBools = (gpuVshBools & ~mask) | (value & mask);

    // Update the uniform booleans in the renderer
    for (int i = 0; i < 16; i++)
        core->gpuRender.setVshBool(i, gpuVshBools & BIT(i));
}

template <int i> void Gpu::writeVshInts(uint32_t mask, uint32_t value) {
    // Write to one of the vertex uniform integer registers
    mask &= 0xFFFFFF;
    gpuVshInts[i] = (gpuVshInts[i] & ~mask) | (value & mask);

    // Update some of the uniform integers in the renderer
    for (int j = 0; j < 3; j++)
        core->gpuRender.setVshInt(i, j, gpuVshInts[i] >> (j << 3));
}

void Gpu::writeVshEntry(uint32_t mask, uint32_t value) {
    // Write to the vertex shader entry/end and send them to the renderer
    mask &= 0x1FF01FF;
    gpuVshEntry = (gpuVshEntry & ~mask) | (value & mask);
    core->gpuRender.setVshEntry(gpuVshEntry >> 0, gpuVshEntry >> 16);
}

void Gpu::writeVshAttrIdsL(uint32_t mask, uint32_t value) {
    // Write to the lower vertex shader attribute IDs
    gpuVshAttrIds = (gpuVshAttrIds & ~mask) | (value & mask);
}

void Gpu::writeVshAttrIdsH(uint32_t mask, uint32_t value) {
    // Write to the upper vertex shader attribute IDs
    gpuVshAttrIds = (gpuVshAttrIds & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
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

    // Increment the index and update the renderer once 4 floats are received
    if (vshFloatIdx++ < (96 << 2) && !(vshFloatIdx & 0x3))
        for (int i = 0; i < 4; i++)
            core->gpuRender.setVshFloat((vshFloatIdx >> 2) - 1, i, *(float*)&vshFloatData[i]);
}

void Gpu::writeVshCodeIdx(uint32_t mask, uint32_t value) {
    // Write to the vertex shader program data index
    gpuVshCodeIdx = (gpuVshCodeIdx & ~mask) | (value & mask);
}

void Gpu::writeVshCodeData(uint32_t mask, uint32_t value) {
    // Write to the current vertex shader program index and increment it
    core->gpuRender.writeVshCode(gpuVshCodeIdx++ & 0x1FF, value & mask);
}

void Gpu::writeVshDescIdx(uint32_t mask, uint32_t value) {
    // Write to the vertex operand descriptor data index
    gpuVshDescIdx = (gpuVshDescIdx & ~mask) | (value & mask);
}

void Gpu::writeVshDescData(uint32_t mask, uint32_t value) {
    // Write to the current vertex operand descriptor index and increment it
    core->gpuRender.writeVshDesc(gpuVshDescIdx++ & 0x7F, value & mask);
}

void Gpu::writeUnkCmd(uint32_t mask, uint32_t value) {
    // Catch unknown GPU command IDs
    LOG_WARN("Unknown GPU command ID: 0x%X\n", curCmd & 0x3FF);
}
