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

#pragma once

#include <cstdint>

#include "gpu.h"
#include "gpu_shader_interp.h"

class GpuRender {
public:
    virtual ~GpuRender() {}

    virtual void submitVertex(SoftVertex &vertex) = 0;
    virtual void flushBuffers(uint32_t mod = 0) = 0;

    virtual void setPrimMode(PrimMode mode) = 0;
    virtual void setCullMode(CullMode mode) = 0;

    virtual void setTexAddr(int i, uint32_t address) = 0;
    virtual void setTexDims(int i, uint16_t width, uint16_t height) = 0;
    virtual void setTexBorder(int i, float *color) = 0;
    virtual void setTexFmt(int i, TexFmt format) = 0;
    virtual void setTexWrap(int i, TexWrap wrapS, TexWrap wrapT) = 0;
    virtual void setCombSrcs(int i, CombSrc *srcs) = 0;
    virtual void setCombOpers(int i, CombOper *opers) = 0;
    virtual void setCombModes(int i, CalcMode *modes) = 0;
    virtual void setCombColor(int i, float *color) = 0;
    virtual void setCombBufColor(float *color) = 0;
    virtual void setCombBufMask(uint8_t mask) = 0;
    virtual void setBlendOpers(BlendOper *opers) = 0;
    virtual void setBlendModes(CalcMode *modes) = 0;
    virtual void setBlendColor(float *color) = 0;
    virtual void setAlphaTest(TestFunc func, float value) = 0;
    virtual void setStencilTest(TestFunc Func, bool enable) = 0;
    virtual void setStencilOps(StenOper fail, StenOper depFail, StenOper depPass) = 0;
    virtual void setStencilMasks(uint8_t bufMask, uint8_t refMask) = 0;
    virtual void setStencilValue(uint8_t value) = 0;

    virtual void setLightSpec0(int i, float r, float g, float b) = 0;
    virtual void setLightSpec1(int i, float r, float g, float b) = 0;
    virtual void setLightDiff(int i, float r, float g, float b) = 0;
    virtual void setLightAmb(int i, float r, float g, float b) = 0;
    virtual void setLightVector(int i, float x, float y, float z) = 0;
    virtual void setLightSpot(int i, float x, float y, float z) = 0;
    virtual void setLightAtten(int i, float bias, float scale) = 0;
    virtual void setLightType(int i, bool direction) = 0;
    virtual void setLightBaseAmb(float r, float g, float b) = 0;
    virtual void setLightLutVal(LutId id, int i, float entry, float diff) = 0;
    virtual void setLightLutMask(uint32_t mask) = 0;
    virtual void setLightLutAbs(bool *flags) = 0;
    virtual void setLightLutInps(LutInput *inputs) = 0;
    virtual void setLightLutScls(float *scales) = 0;
    virtual void setLightMap(int8_t *map) = 0;

    virtual void setViewScaleH(float scale) = 0;
    virtual void setViewStepH(float step) = 0;
    virtual void setViewScaleV(float scale) = 0;
    virtual void setViewStepV(float step) = 0;
    virtual void setViewOffset(int16_t x, int16_t y) = 0;
    virtual void setBufferDims(uint16_t width, uint16_t height, bool flip) = 0;
    virtual void setColbufAddr(uint32_t address) = 0;
    virtual void setColbufFmt(ColbufFmt format) = 0;
    virtual void setColbufMask(uint8_t mask) = 0;
    virtual void setDepbufAddr(uint32_t address) = 0;
    virtual void setDepbufFmt(DepbufFmt format) = 0;
    virtual void setDepbufMask(uint8_t mask) = 0;
    virtual void setDepthFunc(TestFunc func) = 0;

protected:
    static const int16_t etc1Tables[8][4];
};
