/*
    Copyright 2023-2026 Hydr8gon

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
#include "gpu_render_ogl.h"
#include "gpu_render_soft.h"
#include "gpu_shader_glsl.h"

const int16_t GpuRender::etc1Tables[][4] {
    { 2, 8, -2, -8 },
    { 5, 17, -5, -17 },
    { 9, 29, -9, -29 },
    { 13, 42, -13, -42 },
    { 18, 60, -18, -60 },
    { 24, 80, -24, -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 }
};

Gpu::Gpu(Core &core, std::function<void()> *contextFunc): core(core), contextFunc(contextFunc) {
    // Initialize the renderer
    createRender();
}

Gpu::~Gpu() {
    // Finish and clean up
    syncRender();
    destroyRender();
}

void Gpu::createRender() {
    // Initialize a new renderer of the current type
    if (Settings::gpuRenderer == 1) (*contextFunc)();
    switch (renderType = Settings::gpuRenderer) {
        default: gpuRender = new GpuRenderSoft(core); break;
        case 1: gpuRender = new GpuRenderOgl(core); break;
    }

    // Initialize a new shader of the current type
    switch (shaderType = Settings::gpuShader) {
        default: gpuShader = new GpuShaderInterp(*gpuRender, shdInput); break;
        case 1: gpuShader = new GpuShaderGlsl(*(GpuRenderOgl*)gpuRender, shdInput); break;
    }
    if (renderType == 1) (*contextFunc)();
}

void Gpu::destroyRender() {
    // Clean up the initialized renderer
    if (renderType == 1) (*contextFunc)();
    delete gpuShader, delete gpuRender;
    if (renderType == 1) (*contextFunc)();
}

void Gpu::syncRender() {
    // Stop the GPU thread or release context on this thread depending on settings
    if (running.exchange(false)) {
        if (thread) {
            thread->join();
            delete thread;
            thread = nullptr;
        }
        else if (renderType == 1) {
            (*contextFunc)();
        }
    }

    // Reset the renderer if it was changed
    if (renderType == Settings::gpuRenderer && shaderType == Settings::gpuShader) return;
    destroyRender();
    createRender();

    // Restore the renderer base state
    if (renderType == 1) (*contextFunc)();
    writeFaceCulling(0xFFFFFFFF, gpuFaceCulling);
    writeViewScaleH(0xFFFFFFFF, gpuViewScaleH);
    writeViewStepH(0xFFFFFFFF, gpuViewStepH);
    writeViewScaleV(0xFFFFFFFF, gpuViewScaleV);
    writeViewStepV(0xFFFFFFFF, gpuViewStepV);
    writeTexBorder<0>(0xFFFFFFFF, gpuTexBorder[0]);
    writeTexBorder<1>(0xFFFFFFFF, gpuTexBorder[1]);
    writeTexBorder<2>(0xFFFFFFFF, gpuTexBorder[2]);
    writeTexDim<0>(0xFFFFFFFF, gpuTexDim[0]);
    writeTexDim<1>(0xFFFFFFFF, gpuTexDim[1]);
    writeTexDim<2>(0xFFFFFFFF, gpuTexDim[2]);
    writeTexParam<0>(0xFFFFFFFF, gpuTexParam[0]);
    writeTexParam<1>(0xFFFFFFFF, gpuTexParam[1]);
    writeTexParam<2>(0xFFFFFFFF, gpuTexParam[2]);
    writeTexAddr1<0>(0xFFFFFFFF, gpuTexAddr1[0]);
    writeTexAddr1<1>(0xFFFFFFFF, gpuTexAddr1[1]);
    writeTexAddr1<2>(0xFFFFFFFF, gpuTexAddr1[2]);
    writeTexType<0>(0xFFFFFFFF, gpuTexType[0]);
    writeTexType<1>(0xFFFFFFFF, gpuTexType[1]);
    writeTexType<2>(0xFFFFFFFF, gpuTexType[2]);
    writeCombSrc<0>(0xFFFFFFFF, gpuCombSrc[0]);
    writeCombSrc<1>(0xFFFFFFFF, gpuCombSrc[1]);
    writeCombSrc<2>(0xFFFFFFFF, gpuCombSrc[2]);
    writeCombSrc<3>(0xFFFFFFFF, gpuCombSrc[3]);
    writeCombSrc<4>(0xFFFFFFFF, gpuCombSrc[4]);
    writeCombSrc<5>(0xFFFFFFFF, gpuCombSrc[5]);
    writeCombOper<0>(0xFFFFFFFF, gpuCombOper[0]);
    writeCombOper<1>(0xFFFFFFFF, gpuCombOper[1]);
    writeCombOper<2>(0xFFFFFFFF, gpuCombOper[2]);
    writeCombOper<3>(0xFFFFFFFF, gpuCombOper[3]);
    writeCombOper<4>(0xFFFFFFFF, gpuCombOper[4]);
    writeCombOper<5>(0xFFFFFFFF, gpuCombOper[5]);
    writeCombMode<0>(0xFFFFFFFF, gpuCombMode[0]);
    writeCombMode<1>(0xFFFFFFFF, gpuCombMode[1]);
    writeCombMode<2>(0xFFFFFFFF, gpuCombMode[2]);
    writeCombMode<3>(0xFFFFFFFF, gpuCombMode[3]);
    writeCombMode<4>(0xFFFFFFFF, gpuCombMode[4]);
    writeCombMode<5>(0xFFFFFFFF, gpuCombMode[5]);
    writeCombColor<0>(0xFFFFFFFF, gpuCombColor[0]);
    writeCombColor<1>(0xFFFFFFFF, gpuCombColor[1]);
    writeCombColor<2>(0xFFFFFFFF, gpuCombColor[2]);
    writeCombColor<3>(0xFFFFFFFF, gpuCombColor[3]);
    writeCombColor<4>(0xFFFFFFFF, gpuCombColor[4]);
    writeCombColor<5>(0xFFFFFFFF, gpuCombColor[5]);
    writeCombBufUpd(0xFFFFFFFF, gpuCombBufUpd);
    writeCombBufCol(0xFFFFFFFF, gpuCombBufCol);
    writeBlendFunc(0xFFFFFFFF, gpuBlendFunc);
    writeBlendColor(0xFFFFFFFF, gpuBlendColor);
    writeAlphaTest(0xFFFFFFFF, gpuAlphaTest);
    writeStencilTest(0xFFFFFFFF, gpuStencilTest);
    writeStencilOp(0xFFFFFFFF, gpuStencilOp);
    writeDepcolMask(0xFFFFFFFF, gpuDepcolMask);
    writeColbufWrite(0xFFFFFFFF, gpuColbufWrite);
    writeDepbufWrite(0xFFFFFFFF, gpuDepbufWrite);
    writeDepbufFmt(0xFFFFFFFF, gpuDepbufFmt);
    writeColbufFmt(0xFFFFFFFF, gpuColbufFmt);
    writeDepbufLoc(0xFFFFFFFF, gpuDepbufLoc);
    writeColbufLoc(0xFFFFFFFF, gpuColbufLoc);
    writeBufferDim(0xFFFFFFFF, gpuBufferDim);
    writePrimRestart(0xFFFFFFFF, gpuPrimRestart);

    // Restore the renderer light state
    writeLightSpec0<0>(0xFFFFFFFF, gpuLightSpec0[0]);
    writeLightSpec0<1>(0xFFFFFFFF, gpuLightSpec0[1]);
    writeLightSpec0<2>(0xFFFFFFFF, gpuLightSpec0[2]);
    writeLightSpec0<3>(0xFFFFFFFF, gpuLightSpec0[3]);
    writeLightSpec0<4>(0xFFFFFFFF, gpuLightSpec0[4]);
    writeLightSpec0<5>(0xFFFFFFFF, gpuLightSpec0[5]);
    writeLightSpec0<6>(0xFFFFFFFF, gpuLightSpec0[6]);
    writeLightSpec0<7>(0xFFFFFFFF, gpuLightSpec0[7]);
    writeLightSpec1<0>(0xFFFFFFFF, gpuLightSpec1[0]);
    writeLightSpec1<1>(0xFFFFFFFF, gpuLightSpec1[1]);
    writeLightSpec1<2>(0xFFFFFFFF, gpuLightSpec1[2]);
    writeLightSpec1<3>(0xFFFFFFFF, gpuLightSpec1[3]);
    writeLightSpec1<4>(0xFFFFFFFF, gpuLightSpec1[4]);
    writeLightSpec1<5>(0xFFFFFFFF, gpuLightSpec1[5]);
    writeLightSpec1<6>(0xFFFFFFFF, gpuLightSpec1[6]);
    writeLightSpec1<7>(0xFFFFFFFF, gpuLightSpec1[7]);
    writeLightDiff<0>(0xFFFFFFFF, gpuLightDiff[0]);
    writeLightDiff<1>(0xFFFFFFFF, gpuLightDiff[1]);
    writeLightDiff<2>(0xFFFFFFFF, gpuLightDiff[2]);
    writeLightDiff<3>(0xFFFFFFFF, gpuLightDiff[3]);
    writeLightDiff<4>(0xFFFFFFFF, gpuLightDiff[4]);
    writeLightDiff<5>(0xFFFFFFFF, gpuLightDiff[5]);
    writeLightDiff<6>(0xFFFFFFFF, gpuLightDiff[6]);
    writeLightDiff<7>(0xFFFFFFFF, gpuLightDiff[7]);
    writeLightAmb<0>(0xFFFFFFFF, gpuLightAmb[0]);
    writeLightAmb<1>(0xFFFFFFFF, gpuLightAmb[1]);
    writeLightAmb<2>(0xFFFFFFFF, gpuLightAmb[2]);
    writeLightAmb<3>(0xFFFFFFFF, gpuLightAmb[3]);
    writeLightAmb<4>(0xFFFFFFFF, gpuLightAmb[4]);
    writeLightAmb<5>(0xFFFFFFFF, gpuLightAmb[5]);
    writeLightAmb<6>(0xFFFFFFFF, gpuLightAmb[6]);
    writeLightAmb<7>(0xFFFFFFFF, gpuLightAmb[7]);
    writeLightVecL<0>(0xFFFFFFFF, gpuLightVecL[0]);
    writeLightVecL<1>(0xFFFFFFFF, gpuLightVecL[1]);
    writeLightVecL<2>(0xFFFFFFFF, gpuLightVecL[2]);
    writeLightVecL<3>(0xFFFFFFFF, gpuLightVecL[3]);
    writeLightVecL<4>(0xFFFFFFFF, gpuLightVecL[4]);
    writeLightVecL<5>(0xFFFFFFFF, gpuLightVecL[5]);
    writeLightVecL<6>(0xFFFFFFFF, gpuLightVecL[6]);
    writeLightVecL<7>(0xFFFFFFFF, gpuLightVecL[7]);
    writeLightVecH<0>(0xFFFFFFFF, gpuLightVecH[0]);
    writeLightVecH<1>(0xFFFFFFFF, gpuLightVecH[1]);
    writeLightVecH<2>(0xFFFFFFFF, gpuLightVecH[2]);
    writeLightVecH<3>(0xFFFFFFFF, gpuLightVecH[3]);
    writeLightVecH<4>(0xFFFFFFFF, gpuLightVecH[4]);
    writeLightVecH<5>(0xFFFFFFFF, gpuLightVecH[5]);
    writeLightVecH<6>(0xFFFFFFFF, gpuLightVecH[6]);
    writeLightVecH<7>(0xFFFFFFFF, gpuLightVecH[7]);
    writeLightSpotL<0>(0xFFFFFFFF, gpuLightSpotL[0]);
    writeLightSpotL<1>(0xFFFFFFFF, gpuLightSpotL[1]);
    writeLightSpotL<2>(0xFFFFFFFF, gpuLightSpotL[2]);
    writeLightSpotL<3>(0xFFFFFFFF, gpuLightSpotL[3]);
    writeLightSpotL<4>(0xFFFFFFFF, gpuLightSpotL[4]);
    writeLightSpotL<5>(0xFFFFFFFF, gpuLightSpotL[5]);
    writeLightSpotL<6>(0xFFFFFFFF, gpuLightSpotL[6]);
    writeLightSpotL<7>(0xFFFFFFFF, gpuLightSpotL[7]);
    writeLightSpotH<0>(0xFFFFFFFF, gpuLightSpotH[0]);
    writeLightSpotH<1>(0xFFFFFFFF, gpuLightSpotH[1]);
    writeLightSpotH<2>(0xFFFFFFFF, gpuLightSpotH[2]);
    writeLightSpotH<3>(0xFFFFFFFF, gpuLightSpotH[3]);
    writeLightSpotH<4>(0xFFFFFFFF, gpuLightSpotH[4]);
    writeLightSpotH<5>(0xFFFFFFFF, gpuLightSpotH[5]);
    writeLightSpotH<6>(0xFFFFFFFF, gpuLightSpotH[6]);
    writeLightSpotH<7>(0xFFFFFFFF, gpuLightSpotH[7]);
    writeLightConfig<0>(0xFFFFFFFF, gpuLightConfig[0]);
    writeLightConfig<1>(0xFFFFFFFF, gpuLightConfig[1]);
    writeLightConfig<2>(0xFFFFFFFF, gpuLightConfig[2]);
    writeLightConfig<3>(0xFFFFFFFF, gpuLightConfig[3]);
    writeLightConfig<4>(0xFFFFFFFF, gpuLightConfig[4]);
    writeLightConfig<5>(0xFFFFFFFF, gpuLightConfig[5]);
    writeLightConfig<6>(0xFFFFFFFF, gpuLightConfig[6]);
    writeLightConfig<7>(0xFFFFFFFF, gpuLightConfig[7]);
    writeLightAtnBias<0>(0xFFFFFFFF, gpuLightAtnBias[0]);
    writeLightAtnBias<1>(0xFFFFFFFF, gpuLightAtnBias[1]);
    writeLightAtnBias<2>(0xFFFFFFFF, gpuLightAtnBias[2]);
    writeLightAtnBias<3>(0xFFFFFFFF, gpuLightAtnBias[3]);
    writeLightAtnBias<4>(0xFFFFFFFF, gpuLightAtnBias[4]);
    writeLightAtnBias<5>(0xFFFFFFFF, gpuLightAtnBias[5]);
    writeLightAtnBias<6>(0xFFFFFFFF, gpuLightAtnBias[6]);
    writeLightAtnBias<7>(0xFFFFFFFF, gpuLightAtnBias[7]);
    writeLightAtnScl<0>(0xFFFFFFFF, gpuLightAtnScl[0]);
    writeLightAtnScl<1>(0xFFFFFFFF, gpuLightAtnScl[1]);
    writeLightAtnScl<2>(0xFFFFFFFF, gpuLightAtnScl[2]);
    writeLightAtnScl<3>(0xFFFFFFFF, gpuLightAtnScl[3]);
    writeLightAtnScl<4>(0xFFFFFFFF, gpuLightAtnScl[4]);
    writeLightAtnScl<5>(0xFFFFFFFF, gpuLightAtnScl[5]);
    writeLightAtnScl<6>(0xFFFFFFFF, gpuLightAtnScl[6]);
    writeLightAtnScl<7>(0xFFFFFFFF, gpuLightAtnScl[7]);
    writeLightBaseAmb(0xFFFFFFFF, gpuLightBaseAmb);
    writeLightTotal(0xFFFFFFFF, gpuLightTotal);
    writeLightConfig0(0xFFFFFFFF, gpuLightConfig0);
    writeLightConfig1(0xFFFFFFFF, gpuLightConfig1);
    writeLightLutAbs(0xFFFFFFFF, gpuLightLutAbs);
    writeLightLutSel(0xFFFFFFFF, gpuLightLutSel);
    writeLightLutScl(0xFFFFFFFF, gpuLightLutScl);
    writeLightIds(0xFFFFFFFF, gpuLightIds);

    // Restore the renderer light tables
    uint32_t backup = gpuLightLutIdx;
    writeLightLutIdx(0xFFFFFFFF, LUT_D0 << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutD0[i]);
    writeLightLutIdx(0xFFFFFFFF, LUT_D1 << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutD1[i]);
    writeLightLutIdx(0xFFFFFFFF, LUT_FR << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutFr[i]);
    writeLightLutIdx(0xFFFFFFFF, LUT_RB << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutRb[i]);
    writeLightLutIdx(0xFFFFFFFF, LUT_RG << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutRg[i]);
    writeLightLutIdx(0xFFFFFFFF, LUT_RR << 8);
    for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutRr[i]);
    for (int t = 0; t < 8; t++) {
        writeLightLutIdx(0xFFFFFFFF, (LUT_SP0 + t) << 8);
        for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutSp[t][i]);
        writeLightLutIdx(0xFFFFFFFF, (LUT_DA0 + t) << 8);
        for (int i = 0; i < 0x100; i++) writeLightLutData(0xFFFFFFFF, gpuLightLutDa[t][i]);
    }
    writeLightLutIdx(0xFFFFFFFF, backup);

    // Restore the shader base state
    writeGshConfig(0xFFFFFFFF, gpuGshConfig);
    writeVshOutTotal(0xFFFFFFFF, gpuVshOutTotal);
    writeGshBools(0xFFFFFFFF, gpuGshBools);
    writeGshInts<0>(0xFFFFFFFF, gpuGshInts[0]);
    writeGshInts<1>(0xFFFFFFFF, gpuGshInts[1]);
    writeGshInts<2>(0xFFFFFFFF, gpuGshInts[2]);
    writeGshEntry(0xFFFFFFFF, gpuGshEntry);
    writeVshBools(0xFFFFFFFF, gpuVshBools);
    writeVshInts<0>(0xFFFFFFFF, gpuVshInts[0]);
    writeVshInts<1>(0xFFFFFFFF, gpuVshInts[1]);
    writeVshInts<2>(0xFFFFFFFF, gpuVshInts[2]);
    writeVshEntry(0xFFFFFFFF, gpuVshEntry);
    shdMapDirty = true;

    // Restore the shader array state
    for (int i = 0; i < 96; i++) gpuShader->setGshFloats(i, &gshFloats[i * 4]);
    for (int i = 0; i < 0x1000; i++) gpuShader->setGshCode(i, gshCode[i]);
    for (int i = 0; i < 0x80; i++) gpuShader->setGshDesc(i, gshDesc[i]);
    for (int i = 0; i < 96; i++) gpuShader->setVshFloats(i, &vshFloats[i * 4]);
    for (int i = 0; i < 0x200; i++) gpuShader->setVshCode(i, vshCode[i]);
    for (int i = 0; i < 0x80; i++) gpuShader->setVshDesc(i, vshDesc[i]);
    if (renderType == 1) (*contextFunc)();
}

void Gpu::runThreaded() {
    // Set the OpenGL context on this thread if needed
    if (renderType == 1)
        (*contextFunc)();

    // Process GPU thread tasks
    while (true) {
        // Wait for more tasks or finish and release context if stopped
        while (tasks.empty()) {
            if (!running.load()) {
                if (renderType == 1) (*contextFunc)();
                return;
            }
            std::this_thread::yield();
        }

        // Get and pop the next task from the queue
        mutex.lock();
        GpuThreadTask task = tasks.front();
        tasks.pop();
        mutex.unlock();

        // Handle the task based on its type
        switch (task.type) {
        case TASK_CMD: {
            // Decode a GPU command header
            uint32_t *cmds = (uint32_t*)task.data;
            uint8_t count = (cmds[0] >> 20) & 0xFF;
            uint32_t mask = maskTable[(cmds[0] >> 16) & 0xF];
            uint16_t cmd = (cmds[0] & 0x3FF);

            // Write command parameters to GPU registers, with optionally increasing ID
            (this->*cmdWrites[cmd])(mask, cmds[1]);
            if (cmds[0] & BIT(31)) // Increasing
                for (int i = 0; i < count; i++)
                    (this->*cmdWrites[++cmd & 0x3FF])(mask, cmds[i + 2]);
            else // Fixed
                for (int i = 0; i < count; i++)
                    (this->*cmdWrites[cmd])(mask, cmds[i + 2]);
            delete[] cmds;
            continue;
        }

        case TASK_FILL: {
            // Start a GPU fill using saved register values
            GpuFillRegs *regs = (GpuFillRegs*)task.data;
            startFill(*regs);
            delete regs;
            continue;
        }

        case TASK_COPY: {
            // Start a GPU copy using saved register values
            GpuCopyRegs *regs = (GpuCopyRegs*)task.data;
            startCopy(*regs);
            delete regs;
            continue;
        }}
    }
}

bool Gpu::checkInterrupt(int i) {
    // Trigger a GPU interrupt if enabled and its request/compare bytes match
    if ((gpuIrqMask & BITL(i)) || ((gpuIrqCmp[i >> 2] ^ gpuIrqReq[i >> 2]) & (0xFF << ((i & 0x3) * 8))))
        return false;
    gpuIrqStat |= BITL(i);
    core.interrupts.sendInterrupt(ARM11, 0x2D);
    return true;
}

uint32_t Gpu::getDispSrcOfs(uint32_t x, uint32_t y, uint32_t width) {
    // Get a linear source offset in linear-to-linear and linear-to-tiled display copy modes
    if (gpuCopy.flags & (BIT(5) | BIT(1)))
        return (y * width) + x;

    // Get a swizzled source offset in tiled-to-linear display copy mode
    uint32_t ofs = (((y >> 3) * (width >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    return ofs | ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
}

uint32_t Gpu::getDispDstOfs(uint32_t x, uint32_t y, uint32_t width) {
    // Get a linear destination offset in linear-to-linear and tiled-to-linear display copy modes
    if ((gpuCopy.flags & (BIT(5) | BIT(1))) != BIT(1))
        return (y * width) + x;

    // Get a swizzled destination offset in linear-to-tiled display copy mode
    uint32_t ofs = (((y >> 3) * (width >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    return ofs | ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
}

void Gpu::startFill(GpuFillRegs &regs) {
    // Get the start and end addresses for a GPU fill
    uint32_t start = (regs.dstAddr << 3), end = (regs.dstEnd << 3);
    LOG_INFO("Performing GPU memory fill at 0x%X with size 0x%X\n", start, end - start);
    gpuRender->flushBuffers(start);

    // Perform a memory fill using the selected data width
    switch ((regs.cnt >> 8) & 0x3) {
    case 0: // 16-bit
        for (uint32_t addr = start; addr < end; addr += 2)
            core.memory.write<uint16_t>(ARM11, addr, regs.data);
        return;
    case 1: case 3: // 24-bit
        for (uint32_t addr = start, val = (regs.data & 0xFFFFFF) | (regs.data << 24); addr < end; addr += 2)
            core.memory.write<uint16_t>(ARM11, addr, val >> (((addr - start) * 8) % 24));
        return;
    case 2: // 32-bit
        for (uint32_t addr = start; addr < end; addr += 4)
            core.memory.write<uint32_t>(ARM11, addr, regs.data);
        return;
    }
}

void Gpu::startCopy(GpuCopyRegs &regs) {
    // Get the source and destination addresses for a GPU copy
    uint32_t srcAddr = (regs.srcAddr << 3);
    uint32_t dstAddr = (regs.dstAddr << 3);
    gpuRender->flushBuffers(dstAddr);

    // Perform a texture copy if enabled, which ignores most settings
    if (regs.flags & BIT(3)) {
        // Get the source and destination parameters for a texture copy
        uint32_t srcWidth = (regs.texSrcWidth << 4) & 0xFFFF0;
        uint32_t srcGap = (regs.texSrcWidth >> 12) & 0xFFFF0;
        uint32_t dstWidth = (regs.texDstWidth << 4) & 0xFFFF0;
        uint32_t dstGap = (regs.texDstWidth >> 12) & 0xFFFF0;
        LOG_INFO("Performing GPU texture copy from 0x%X to 0x%X with size 0x%X\n", srcAddr, dstAddr, gpuCopy.texSize);

        // Perform a texture copy, applying address gaps when widths are reached
        for (uint32_t i = 0; i < regs.texSize; i += 4) {
            core.memory.write<uint32_t>(ARM11, dstAddr + i, core.memory.read<uint32_t>(ARM11, srcAddr + i));
            if (srcWidth && !((i + 4) % srcWidth)) srcAddr += srcGap;
            if (dstWidth && !((i + 4) % dstWidth)) dstAddr += dstGap;
        }
        return;
    }

    // Get the source and destination parameters for a display copy
    uint8_t srcFmt = (regs.flags >> 8) & 0x7;
    uint16_t srcWidth = (regs.flags & BIT(2)) ? regs.dispSrcSize : regs.dispDstSize;
    uint8_t dstFmt = (regs.flags >> 12) & 0x7;
    uint16_t dstWidth = (regs.dispDstSize >> 0);
    uint16_t dstHeight = (regs.dispDstSize >> 16);
    uint8_t scaleType = (regs.flags >> 24) & 0x3;

    // Scale the destination dimensions based on scale type
    dstWidth >>= (scaleType == 0x1 || scaleType == 0x2);
    dstHeight >>= (scaleType == 0x2);

    // Adjust the Y order based on the vertical flip bit
    int yStart, yInc;
    if (regs.flags & BIT(0)) // Flipped
        yStart = dstHeight - 1, yInc = -1;
    else // Normal
        yStart = 0, yInc = 1;

    // Log the display copy and check for unimplemented flags
    LOG_INFO("Performing GPU display copy from 0x%X to 0x%X with width %d and height %d\n",
        srcAddr, dstAddr, dstWidth, dstHeight);
    if (regs.flags & BIT(16))
        LOG_CRIT("Unhandled GPU display copy tile size: 32x32\n");

    // Perform an 8x8 tiled to linear display copy based on format and scale settings
    uint8_t r, g, b, a;
    uint32_t ofs, c0, c1, c2, c3;
    switch (srcFmt) {
    case 0x0: // RGBA8
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = srcAddr + getDispSrcOfs(x, y0, srcWidth) * 4;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs);
                    r = ((c0 >> 24) & 0xFF);
                    g = ((c0 >> 16) & 0xFF);
                    b = ((c0 >> 8) & 0xFF);
                    a = ((c0 >> 0) & 0xFF);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0, srcWidth) * 4;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint32_t>(ARM11, ofs + 0x4);
                    r = (((c0 >> 24) & 0xFF) + ((c1 >> 24) & 0xFF)) / 2;
                    g = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF)) / 2;
                    b = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF)) / 2;
                    a = (((c0 >> 0) & 0xFF) + ((c1 >> 0) & 0xFF)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 4;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint32_t>(ARM11, ofs + 0x4);
                    c2 = core.memory.read<uint32_t>(ARM11, ofs + 0x8);
                    c3 = core.memory.read<uint32_t>(ARM11, ofs + 0xC);
                    r = (((c0 >> 24) & 0xFF) + ((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF) + ((c3 >> 24) & 0xFF)) / 4;
                    g = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF) + ((c3 >> 16) & 0xFF)) / 4;
                    b = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF) + ((c2 >> 8) & 0xFF) + ((c3 >> 8) & 0xFF)) / 4;
                    a = (((c0 >> 0) & 0xFF) + ((c1 >> 0) & 0xFF) + ((c2 >> 0) & 0xFF) + ((c3 >> 0) & 0xFF)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 4;
                    c0 = (r << 24) | (g << 16) | (b << 8) | a;
                    core.memory.write<uint32_t>(ARM11, ofs, c0);
                    continue;
                case 0x1: // RGB8
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 3;
                    core.memory.write<uint8_t>(ARM11, ofs + 2, r);
                    core.memory.write<uint8_t>(ARM11, ofs + 1, g);
                    core.memory.write<uint8_t>(ARM11, ofs + 0, b);
                    continue;
                case 0x2: // RGB565
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 255) << 11) | ((g * 63 / 255) << 5) | (b * 31 / 255);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 255) << 11) | ((g * 31 / 255) << 6) | ((b * 31 / 255) << 1) | bool(a);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 255) << 12) | ((g * 15 / 255) << 8) | ((b * 15 / 255) << 4) | (a * 15 / 255);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                }
            }
        }
        return;

    case 0x1: // RGB8
        if (dstFmt != 0x1) { // RGB8
            LOG_CRIT("Invalid destination format for RGB8 display copy: 0x%X\n", dstFmt);
            return;
        }

        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = srcAddr + getDispSrcOfs(x, y0, srcWidth) * 3;
                    r = core.memory.read<uint8_t>(ARM11, ofs + 2);
                    g = core.memory.read<uint8_t>(ARM11, ofs + 1);
                    b = core.memory.read<uint8_t>(ARM11, ofs + 0);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0, srcWidth) * 3;
                    r = (core.memory.read<uint8_t>(ARM11, ofs + 2) + core.memory.read<uint8_t>(ARM11, ofs + 5)) / 2;
                    g = (core.memory.read<uint8_t>(ARM11, ofs + 1) + core.memory.read<uint8_t>(ARM11, ofs + 4)) / 2;
                    b = (core.memory.read<uint8_t>(ARM11, ofs + 0) + core.memory.read<uint8_t>(ARM11, ofs + 3)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 3;
                    r = (core.memory.read<uint8_t>(ARM11, ofs + 2) + core.memory.read<uint8_t>(ARM11, ofs + 5) +
                        core.memory.read<uint8_t>(ARM11, ofs + 8) + core.memory.read<uint8_t>(ARM11, ofs + 11)) / 4;
                    g = (core.memory.read<uint8_t>(ARM11, ofs + 1) + core.memory.read<uint8_t>(ARM11, ofs + 4) +
                        core.memory.read<uint8_t>(ARM11, ofs + 7) + core.memory.read<uint8_t>(ARM11, ofs + 10)) / 4;
                    b = (core.memory.read<uint8_t>(ARM11, ofs + 0) + core.memory.read<uint8_t>(ARM11, ofs + 3) +
                        core.memory.read<uint8_t>(ARM11, ofs + 6) + core.memory.read<uint8_t>(ARM11, ofs + 9)) / 4;
                    break;
                }

                ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 3;
                core.memory.write<uint8_t>(ARM11, ofs + 2, r);
                core.memory.write<uint8_t>(ARM11, ofs + 1, g);
                core.memory.write<uint8_t>(ARM11, ofs + 0, b);
            }
        }
        return;

    case 0x2: // RGB565
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = srcAddr + getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs);
                    r = ((c0 >> 11) & 0x1F);
                    g = ((c0 >> 5) & 0x3F);
                    b = ((c0 >> 0) & 0x1F);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint16_t>(ARM11, ofs + 0x2);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F)) / 2;
                    g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F)) / 2;
                    b = (((c0 >> 0) & 0x1F) + ((c1 >> 0) & 0x1F)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint32_t>(ARM11, ofs + 0x2);
                    c2 = core.memory.read<uint32_t>(ARM11, ofs + 0x4);
                    c3 = core.memory.read<uint32_t>(ARM11, ofs + 0x6);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + ((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 4;
                    g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + ((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) / 4;
                    b = (((c0 >> 0) & 0x1F) + ((c1 >> 0) & 0x1F) + ((c2 >> 0) & 0x1F) + ((c3 >> 0) & 0x1F)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGB565 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | (g << 5) | b;
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | ((g * 31 / 63) << 6) | (b << 1) | 0x1;
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 31) << 12) | ((g * 15 / 63) << 8) | ((b * 15 / 31) << 4) | 0xF;
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                }
            }
        }
        return;

    case 0x3: // RGB5A1
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = srcAddr + getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs);
                    r = ((c0 >> 11) & 0x1F);
                    g = ((c0 >> 6) & 0x1F);
                    b = ((c0 >> 1) & 0x1F);
                    a = (c0 & BIT(0)) * 4;
                    break;
                case 0x1: // 2x1 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint16_t>(ARM11, ofs + 0x2);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F)) / 2;
                    g = (((c0 >> 6) & 0x1F) + ((c1 >> 6) & 0x1F)) / 2;
                    b = (((c0 >> 1) & 0x1F) + ((c1 >> 1) & 0x1F)) / 2;
                    a = ((c0 & BIT(0)) + (c1 & BIT(0))) * 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint32_t>(ARM11, ofs + 0x2);
                    c2 = core.memory.read<uint32_t>(ARM11, ofs + 0x4);
                    c3 = core.memory.read<uint32_t>(ARM11, ofs + 0x6);
                    r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + ((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 4;
                    g = (((c0 >> 6) & 0x1F) + ((c1 >> 6) & 0x1F) + ((c2 >> 6) & 0x1F) + ((c3 >> 6) & 0x1F)) / 4;
                    b = (((c0 >> 1) & 0x1F) + ((c1 >> 1) & 0x1F) + ((c2 >> 1) & 0x1F) + ((c3 >> 1) & 0x1F)) / 4;
                    a = ((c0 & BIT(0)) + (c1 & BIT(0)) + (c2 & BIT(0)) + (c3 & BIT(0)));
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGB5A1 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | ((g * 63 / 31) << 5) | b;
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 11) | (g << 6) | (b << 1) | bool(a);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 15 / 31) << 12) | ((g * 15 / 31) << 8) | ((b * 15 / 31) << 4) | (a * 15 / 4);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                }
            }
        }
        return;

    default: // RGBA4
        for (int y0 = yStart, y1 = 0; y1 < dstHeight; y0 += yInc, y1++) {
            for (int x = 0; x < dstWidth; x++) {
                switch (scaleType) {
                default: // No downscale
                    ofs = srcAddr + getDispSrcOfs(x, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs);
                    r = ((c0 >> 12) & 0xF);
                    g = ((c0 >> 8) & 0xF);
                    b = ((c0 >> 4) & 0xF);
                    a = ((c0 >> 0) & 0xF);
                    break;
                case 0x1: // 2x1 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0, srcWidth) * 2;
                    c0 = core.memory.read<uint16_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint16_t>(ARM11, ofs + 0x2);
                    r = (((c0 >> 12) & 0xF) + ((c1 >> 12) & 0xF)) / 2;
                    g = (((c0 >> 8) & 0xF) + ((c1 >> 8) & 0xF)) / 2;
                    b = (((c0 >> 4) & 0xF) + ((c1 >> 4) & 0xF)) / 2;
                    a = (((c0 >> 0) & 0xF) + ((c1 >> 0) & 0xF)) / 2;
                    break;
                case 0x2: // 2x2 downscale
                    ofs = srcAddr + getDispSrcOfs(x << 1, y0 << 1, srcWidth) * 2;
                    c0 = core.memory.read<uint32_t>(ARM11, ofs + 0x0);
                    c1 = core.memory.read<uint32_t>(ARM11, ofs + 0x2);
                    c2 = core.memory.read<uint32_t>(ARM11, ofs + 0x4);
                    c3 = core.memory.read<uint32_t>(ARM11, ofs + 0x6);
                    r = (((c0 >> 12) & 0xF) + ((c1 >> 12) & 0xF) + ((c2 >> 12) & 0xF) + ((c3 >> 12) & 0xF)) / 4;
                    g = (((c0 >> 8) & 0xF) + ((c1 >> 8) & 0xF) + ((c2 >> 8) & 0xF) + ((c3 >> 8) & 0xF)) / 4;
                    b = (((c0 >> 4) & 0xF) + ((c1 >> 4) & 0xF) + ((c2 >> 4) & 0xF) + ((c3 >> 4) & 0xF)) / 4;
                    a = (((c0 >> 0) & 0xF) + ((c1 >> 0) & 0xF) + ((c2 >> 0) & 0xF) + ((c3 >> 0) & 0xF)) / 4;
                    break;
                }

                switch (dstFmt) {
                case 0x0: // RGBA8
                case 0x1: // RGB8
                    LOG_CRIT("Invalid destination format for RGBA4 display copy: 0x%X\n", dstFmt);
                    return;
                case 0x2: // RGB565
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 15) << 11) | ((g * 63 / 15) << 5) | (b * 31 / 15);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                case 0x3: // RGB5A1
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = ((r * 31 / 15) << 11) | ((g * 31 / 15) << 6) | ((b * 31 / 15) << 1) | bool(a);
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                default: // RGBA4
                    ofs = dstAddr + getDispDstOfs(x, y1, dstWidth) * 2;
                    c0 = (r << 12) | (g << 8) | (b << 4) | a;
                    core.memory.write<uint16_t>(ARM11, ofs, c0);
                    continue;
                }
            }
        }
        return;
    }
}

void Gpu::endFill(int i) {
    // Trigger a GPU fill end interrupt after some time
    gpuFill[i].cnt |= BIT(1);
    core.interrupts.sendInterrupt(ARM11, 0x28 + i);
}

void Gpu::endCopy() {
    // Trigger a GPU copy ehd interrupt after some time
    gpuCopy.cnt |= BIT(8);
    core.interrupts.sendInterrupt(ARM11, 0x2C);
}

void Gpu::writeCfg11GpuCnt(uint32_t mask, uint32_t value) {
    // Write to the CFG11_GPU_CNT register
    mask &= 0x1007F;
    cfg11GpuCnt = (cfg11GpuCnt & ~mask) | (value & mask);
}

void Gpu::writeFillDstAddr(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_FILL_DST_ADDR registers
    mask &= 0x1FFFFFFE;
    gpuFill[i].dstAddr = (gpuFill[i].dstAddr & ~mask) | (value & mask);
}

void Gpu::writeFillDstEnd(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_FILL_DST_END registers
    mask &= 0x1FFFFFFE;
    gpuFill[i].dstEnd = (gpuFill[i].dstEnd & ~mask) | (value & mask);
}

void Gpu::writeFillData(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_FILL_DATA registers
    gpuFill[i].data = (gpuFill[i].data & ~mask) | (value & mask);
}

void Gpu::writeFillCnt(int i, uint32_t mask, uint32_t value) {
    // Write to one of the GPU_FILL_CNT registers
    uint32_t mask2 = (mask & 0x1F0300);
    gpuFill[i].cnt = (gpuFill[i].cnt & ~mask2) | (value & mask2);

    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(1)) && !(value & BIT(1)))
        gpuFill[i].cnt &= ~BIT(1);

    // Check if a fill should start and schedule the end signal based on size
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(1))) return;
    core.schedule(Task(GPU_END_FILL0 + i), gpuFill[i].dstEnd - gpuFill[i].dstAddr);

    // Start the fill now or forward it to the thread if running
    if (!thread) return startFill(gpuFill[i]);
    GpuFillRegs *regs = new GpuFillRegs(gpuFill[i]);
    mutex.lock();
    tasks.emplace(TASK_FILL, regs);
    mutex.unlock();
}

void Gpu::writeCopySrcAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_SRC_ADDR register
    mask &= 0x1FFFFFFE;
    gpuCopy.srcAddr = (gpuCopy.srcAddr & ~mask) | (value & mask);
}

void Gpu::writeCopyDstAddr(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_DST_ADDR register
    mask &= 0x1FFFFFFE;
    gpuCopy.dstAddr = (gpuCopy.dstAddr & ~mask) | (value & mask);
}

void Gpu::writeCopyDispDstSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_DISP_DST_SIZE register
    mask &= 0xFFF8FFF8;
    gpuCopy.dispDstSize = (gpuCopy.dispDstSize & ~mask) | (value & mask);
}

void Gpu::writeCopyDispSrcSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_DISP_SRC_SIZE register
    mask &= 0xFFF8FFF8;
    gpuCopy.dispSrcSize = (gpuCopy.dispSrcSize & ~mask) | (value & mask);
}

void Gpu::writeCopyFlags(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_FLAGS register
    mask &= 0x301772F;
    gpuCopy.flags = (gpuCopy.flags & ~mask) | (value & mask);
}

void Gpu::writeCopyCnt(uint32_t mask, uint32_t value) {
    // Allow clearing the interrupt bit but not setting it
    if ((mask & BIT(8)) && !(value & BIT(8)))
        gpuCopy.cnt &= ~BIT(8);

    // Check if a copy should start and schedule the end signal based on type and size
    if (!(value & mask & BIT(0)) || !(cfg11GpuCnt & BIT(4))) return;
    core.schedule(GPU_END_COPY, (gpuCopy.flags & BIT(3)) ? (gpuCopy.texSize / 4)
        : ((gpuCopy.dispDstSize & 0xFFFF) * (gpuCopy.dispDstSize >> 16)));

    // Start the copy now or forward it to the thread if running
    if (!thread) return startCopy(gpuCopy);
    GpuCopyRegs *regs = new GpuCopyRegs(gpuCopy);
    mutex.lock();
    tasks.emplace(TASK_COPY, regs);
    mutex.unlock();
}

void Gpu::writeCopyTexSize(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_TEX_SIZE register
    mask &= 0xFFFFFFF0;
    gpuCopy.texSize = (gpuCopy.texSize & ~mask) | (value & mask);
}

void Gpu::writeCopyTexSrcWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_TEX_SRC_WIDTH register
    gpuCopy.texSrcWidth = (gpuCopy.texSrcWidth & ~mask) | (value & mask);
}

void Gpu::writeCopyTexDstWidth(uint32_t mask, uint32_t value) {
    // Write to the GPU_COPY_TEX_DST_WIDTH register
    gpuCopy.texDstWidth = (gpuCopy.texDstWidth & ~mask) | (value & mask);
}

void Gpu::writeIrqAck(int i, uint32_t mask, uint32_t value) {
    // Write to some of the GPU_IRQ_REQ registers and acknowledge if they don't match
    gpuIrqReq[i] = (gpuIrqReq[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if ((mask & (0xFF << (j * 8))) && !checkInterrupt((i << 2) + j))
            gpuIrqStat &= ~BITL((i << 2) + j);
}

void Gpu::writeIrqCmp(int i, uint32_t mask, uint32_t value) {
    // Write to some of the GPU_IRQ_CMP registers and check if they match
    gpuIrqCmp[i] = (gpuIrqCmp[i] & ~mask) | (value & mask);
    for (int j = 0; j < 4; j++)
        if (mask & (0xFF << (j * 8)))
            checkInterrupt((i << 2) + j);
}

void Gpu::writeIrqMaskL(uint32_t mask, uint32_t value) {
    // Write to the low part of the GPU_IRQ_MASK register
    gpuIrqMask = (gpuIrqMask & ~mask) | (value & mask);
}

void Gpu::writeIrqMaskH(uint32_t mask, uint32_t value) {
    // Write to the high part of the GPU_IRQ_MASK register
    gpuIrqMask = (gpuIrqMask & ~(uint64_t(mask) << 32)) | (uint64_t(value & mask) << 32);
}

void Gpu::writeIrqAutostop(uint32_t mask, uint32_t value) {
    // Write to the GPU_IRQ_AUTOSTOP register
    mask &= 0x1;
    gpuIrqAutostop = (gpuIrqAutostop & ~mask) | (value & mask);
}
