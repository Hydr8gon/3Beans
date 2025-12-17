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

#include "gpu.h"
#include "gpu_shader_interp.h"

class GpuRender {
public:
    virtual void submitVertex(SoftVertex &vertex) = 0;
    virtual void flushBuffers() = 0;

    virtual void setPrimMode(PrimMode mode) = 0;
    virtual void setCullMode(CullMode mode) = 0;

    virtual void setTexAddr(int i, uint32_t address) = 0;
    virtual void setTexDims(int i, uint16_t width, uint16_t height) = 0;
    virtual void setTexBorder(int i, float r, float g, float b, float a) = 0;
    virtual void setTexFmt(int i, TexFmt format) = 0;
    virtual void setTexWrapS(int i, TexWrap wrap) = 0;
    virtual void setTexWrapT(int i, TexWrap wrap) = 0;
    virtual void setCombSrc(int i, int j, CombSrc src) = 0;
    virtual void setCombOper(int i, int j, CombOper oper) = 0;
    virtual void setCombMode(int i, int j, CalcMode mode) = 0;
    virtual void setCombColor(int i, float r, float g, float b, float a) = 0;
    virtual void setCombBufColor(float r, float g, float b, float a) = 0;
    virtual void setCombBufMask(uint8_t mask) = 0;
    virtual void setBlendOper(int i, BlendOper oper) = 0;
    virtual void setBlendMode(int i, CalcMode mode) = 0;
    virtual void setBlendColor(float r, float g, float b, float a) = 0;
    virtual void setAlphaFunc(TestFunc func) = 0;
    virtual void setAlphaValue(float value) = 0;
    virtual void setStencilTest(TestFunc Func, bool enable) = 0;
    virtual void setStencilOps(StenOper fail, StenOper depFail, StenOper depPass) = 0;
    virtual void setStencilMasks(uint8_t bufMask, uint8_t refMask) = 0;
    virtual void setStencilValue(uint8_t value) = 0;

    virtual void setViewScaleH(float scale) = 0;
    virtual void setViewStepH(float step) = 0;
    virtual void setViewScaleV(float scale) = 0;
    virtual void setViewStepV(float step) = 0;
    virtual void setBufferDims(uint16_t width, uint16_t height, bool flip) = 0;
    virtual void setColbufAddr(uint32_t address) = 0;
    virtual void setColbufFmt(ColbufFmt format) = 0;
    virtual void setColbufMask(uint8_t mask) = 0;
    virtual void setDepbufAddr(uint32_t address) = 0;
    virtual void setDepbufFmt(DepbufFmt format) = 0;
    virtual void setDepbufMask(uint8_t mask) = 0;
    virtual void setDepthFunc(TestFunc func) = 0;
};
