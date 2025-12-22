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
#include <epoxy/gl.h>

#include "gpu_render.h"

class Core;

struct TexCache {
    uint32_t addr;
    uint16_t width;
    uint16_t height;
    TexFmt fmt;
    GLuint tex;

    bool operator<(const TexCache &t) const { return addr < t.addr; }
};

class GpuRenderOgl: public GpuRender {
public:
    GpuRenderOgl(Core *core);
    ~GpuRenderOgl();

    void submitVertex(SoftVertex &vertex);
    void flushBuffers();

    void setPrimMode(PrimMode mode);
    void setCullMode(CullMode mode);

    void setTexAddr(int i, uint32_t address);
    void setTexDims(int i, uint16_t width, uint16_t height);
    void setTexBorder(int i, float r, float g, float b, float a);
    void setTexFmt(int i, TexFmt format);
    void setTexWrapS(int i, TexWrap wrap);
    void setTexWrapT(int i, TexWrap wrap);
    void setCombSrc(int i, int j, CombSrc src);
    void setCombOper(int i, int j, CombOper oper);
    void setCombMode(int i, int j, CalcMode mode);
    void setCombColor(int i, float r, float g, float b, float a);
    void setCombBufColor(float r, float g, float b, float a);
    void setCombBufMask(uint8_t mask);
    void setBlendOper(int i, BlendOper oper);
    void setBlendMode(int i, CalcMode mode);
    void setBlendColor(float r, float g, float b, float a);
    void setAlphaFunc(TestFunc func);
    void setAlphaValue(float value);
    void setStencilTest(TestFunc Func, bool enable);
    void setStencilOps(StenOper fail, StenOper depFail, StenOper depPass);
    void setStencilMasks(uint8_t bufMask, uint8_t refMask);
    void setStencilValue(uint8_t value);

    void setViewScaleH(float scale);
    void setViewStepH(float step) {}
    void setViewScaleV(float scale);
    void setViewStepV(float step) {}
    void setBufferDims(uint16_t width, uint16_t height, bool flip);
    void setColbufAddr(uint32_t address);
    void setColbufFmt(ColbufFmt format);
    void setColbufMask(uint8_t mask);
    void setDepbufAddr(uint32_t address) {}
    void setDepbufFmt(DepbufFmt format) {}
    void setDepbufMask(uint8_t mask);
    void setDepthFunc(TestFunc func);

private:
    Core *core;
    GLuint program;
    GLuint vao, vbo;
    GLuint colBuf, depBuf;
    GLuint texture;

    GLint posScaleLoc;
    GLint combSrcLocs[6][6];
    GLint combOperLocs[6][6];
    GLint combModeLocs[6][2];
    GLint combColorLocs[6];
    GLint combBufColorLoc;
    GLint combBufMaskLoc;
    GLint alphaFuncLoc;
    GLint alphaValueLoc;

    static const char *vtxCode;
    static const char *fragCode;

    std::vector<SoftVertex> vertices;
    std::vector<TexCache> texCache;
    GLint primMode = GL_TRIANGLES;
    uint8_t texDirty = 0;
    bool readDirty = false;
    bool writeDirty = false;

    uint32_t texAddrs[3] = {};
    uint16_t texWidths[3] = {};
    uint16_t texHeights[3] = {};
    float texBorders[3][4] = {};
    TexFmt texFmts[3] = {};
    GLint texWrapS[3] = {};
    GLint texWrapT[3] = {};
    GLenum blendOpers[4] = {};
    GLenum blendModes[2] = {};

    GLsizei viewWidth = 0;
    GLsizei viewHeight = 0;
    bool flipY = false;
    GLsizei bufWidth = 0;
    GLsizei bufHeight = 0;
    uint32_t colbufAddr = 0;
    ColbufFmt colbufFmt = COL_UNK;
    GLboolean depbufMask = GL_FALSE;
    GLenum stencilFunc = GL_NEVER;
    GLint stencilValue = 0;
    GLuint stencilMasks[2] = {};

    static uint32_t getSwizzle(int x, int y, int width);
    template <bool alpha> uint32_t etc1Texel(int i, int x, int y);

    void flushVertices();
    void updateBuffers();
    void updateTextures();
    void updateViewport();
};
