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
#include <vector>

#include "gpu_render.h"

class Core;

struct SoftColor {
    float r, g, b, a;
};

struct SoftLight {
    float specular0[3];
    float specular1[3];
    float diffuse[3];
    float ambient[3];
    float x, y, z;
    float px, py, pz;
    float atnBias;
    float atnScale;
    bool direction;
};

struct CombParam {
    SoftColor *color;
    CombOper oper;
};

struct CombOpcode {
    CombParam params[3];
    CalcMode mode;
    uint8_t id;
};

class GpuRenderSoft: public GpuRender {
public:
    GpuRenderSoft(Core &core): core(core) {}

    void submitVertex(SoftVertex &vertex);
    void flushBuffers(uint32_t mod = 0) {}

    void setPrimMode(PrimMode mode);
    void setCullMode(CullMode mode) { cullMode = mode; }

    void setTexAddr(int i, uint32_t address);
    void setTexDims(int i, uint16_t width, uint16_t height);
    void setTexBorder(int i, float *color);
    void setTexFmt(int i, TexFmt format);
    void setTexWrap(int i, TexWrap wrapS, TexWrap wrapT);
    void setCombSrcs(int i, CombSrc *srcs);
    void setCombOpers(int i, CombOper *opers);
    void setCombModes(int i, CalcMode *modes);
    void setCombColor(int i, float *color);
    void setCombBufColor(float *color);
    void setCombBufMask(uint8_t mask);
    void setBlendOpers(BlendOper *opers);
    void setBlendModes(CalcMode *modes);
    void setBlendColor(float *color);
    void setAlphaTest(TestFunc func, float value);
    void setStencilTest(TestFunc Func, bool enable);
    void setStencilOps(StenOper fail, StenOper depFail, StenOper depPass);
    void setStencilMasks(uint8_t bufMask, uint8_t refMask);
    void setStencilValue(uint8_t value) { stencilValue = value; }

    void setLightSpec0(int i, float r, float g, float b);
    void setLightSpec1(int i, float r, float g, float b);
    void setLightDiff(int i, float r, float g, float b);
    void setLightAmb(int i, float r, float g, float b);
    void setLightVector(int i, float x, float y, float z);
    void setLightSpot(int i, float x, float y, float z);
    void setLightAtten(int i, float bias, float scale);
    void setLightType(int i, bool direction) { lights[i].direction = direction; }
    void setLightBaseAmb(float r, float g, float b);
    void setLightLutVal(LutId id, int i, float entry, float diff);
    void setLightLutMask(uint32_t mask) { lutMask = mask; }
    void setLightLutAbs(bool *flags);
    void setLightLutInps(LutInput *inputs);
    void setLightLutScls(float *scales);
    void setLightMap(int8_t *map);

    void setViewScaleH(float scale) { viewScaleH = scale; }
    void setViewStepH(float step) { viewStepH = step; }
    void setViewScaleV(float scale) { viewScaleV = scale; }
    void setViewStepV(float step) { viewStepV = step; }
    void setViewOffset(int16_t x, int16_t y) { viewX = x, viewY = y; }
    void setBufferDims(uint16_t width, uint16_t height, bool flip);
    void setColbufAddr(uint32_t address) { colbufAddr = address; }
    void setColbufFmt(ColbufFmt format) { colbufFmt = format; }
    void setColbufMask(uint8_t mask) { colbufMask = mask; }
    void setDepbufAddr(uint32_t address) { depbufAddr = address; }
    void setDepbufFmt(DepbufFmt format) { depbufFmt = format; }
    void setDepbufMask(uint8_t mask) { depbufMask = mask; }
    void setDepthFunc(TestFunc func) { depthFunc = func; }

private:
    Core &core;

    static const uint8_t paramCounts[MODE_UNK + 1];
    static SoftColor zeroColor, oneColor;

    std::vector<CombOpcode> combCache;
    SoftColor combBuffer[6] = {};
    SoftColor texColors[3] = {};
    SoftColor fragColors[2] = {};
    SoftColor primColor = {};
    int64_t lastU[3] = { -1, -1, -1 };
    int64_t lastV[3] = { -1, -1, -1 };
    uint16_t paramMask = 0;
    uint16_t combMask = 0;
    uint8_t combEnd = -1;

    SoftVertex vertices[3] = {};
    uint32_t vtxCount = 0;
    PrimMode primMode = TRIANGLES;
    CullMode cullMode = CULL_NONE;

    uint32_t texAddrs[3] = {};
    uint16_t texWidths[3] = {};
    uint16_t texHeights[3] = {};
    SoftColor texBorders[3] = {};
    TexFmt texFmts[3] = {};
    TexWrap texWrapS[3] = {};
    TexWrap texWrapT[3] = {};
    CombSrc combSrcs[6][6] = {};
    CombOper combOpers[6][6] = {};
    CalcMode combModes[6][2] = {};
    SoftColor combColors[6] = {};
    SoftColor combBufColor = {};
    uint8_t combBufMask = 0;
    BlendOper blendOpers[4] = {};
    CalcMode blendModes[2] = {};
    SoftColor blendColor = {};
    TestFunc alphaFunc = TEST_AL;
    float alphaValue = 0;

    SoftLight lights[8] = {};
    float baseAmbient[3] = {};
    float lutD0[0x100][2] = {};
    float lutD1[0x100][2] = {};
    float lutFr[0x100][2] = {};
    float lutRb[0x100][2] = {};
    float lutRg[0x100][2] = {};
    float lutRr[0x100][2] = {};
    float lutSp[8][0x100][2] = {};
    float lutDa[8][0x100][2] = {};
    uint32_t lutMask = 0;
    bool lutAbsFlags[7] = {};
    LutInput lutInputs[7] = {};
    float lutScales[7] = {};
    SoftLight *lightMap[9] = {};

    float viewScaleH = 0;
    float viewStepH = 0;
    float viewScaleV = 0;
    float viewStepV = 0;
    int16_t viewX = 0, viewY = 0;
    bool flipY = false;
    uint16_t bufWidth = 0;
    uint16_t bufHeight = 0;
    uint32_t colbufAddr = 0;
    ColbufFmt colbufFmt = COL_UNK;
    uint8_t colbufMask = 0;
    uint32_t depbufAddr = 0;
    DepbufFmt depbufFmt = DEP_UNK;
    uint8_t depbufMask = 0;
    TestFunc depthFunc = TEST_AL;
    TestFunc stencilFunc = TEST_NV;
    StenOper stencilFail = STEN_KEEP;
    StenOper stenDepFail = STEN_KEEP;
    StenOper stenDepPass = STEN_KEEP;
    uint8_t stencilMasks[2] = {};
    uint8_t stencilValue = 0;
    bool stencilEnable = false;

    template <bool doX> static SoftVertex interpolate(SoftVertex &v1, SoftVertex &v2, float x1, float x, float x2);
    static SoftVertex intersect(SoftVertex &v1, SoftVertex &v2, float x1, float x2);
    static int32_t procTexCoord(float c, uint16_t size, TexWrap wrap);
    static void normalize(float &x, float &y, float &z);

    float readLut(float (*lut)[2], float *inp, int i);
    uint8_t stencilOp(uint8_t value, StenOper oper);

    void updateTexel(int i, float s, float t);
    void updateFrag(float qx, float qy, float qz, float qw, float vx, float vy, float vz, float z);
    void updateCombine(SoftVertex &v);
    CombParam cacheParam(int i, int j);
    void cacheCombRgb(int i);
    void cacheCombA(int i);

    void drawPixel(SoftVertex &p);
    void drawTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c);
    void clipTriangle(SoftVertex &a, SoftVertex &b, SoftVertex &c);
};
