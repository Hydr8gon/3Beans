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
#include "gpu_render_ogl.h"

const char *GpuRenderOgl::vtxCode = R"(
    #version 150

    in vec4 inPosition;
    in vec4 inColor;
    out vec4 vtxColor;
    uniform vec4 posScale;

    void main() {
        gl_Position = inPosition * posScale;
        vtxColor = inColor;
    }
)";

const char *GpuRenderOgl::fragCode = R"(
    #version 150

    in vec4 vtxColor;
    out vec4 fragColor;

    void main() {
        fragColor = vtxColor;
    }
)";

GpuRenderOgl::GpuRenderOgl(Core *core): core(core) {
    // Compile the vertex and fragment shaders
    GLint vtxShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtxShader, 1, &vtxCode, nullptr);
    glCompileShader(vtxShader);
    GLint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragCode, nullptr);
    glCompileShader(fragShader);

    // Create a program with the shaders
    GLuint program = glCreateProgram();
    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glUseProgram(program);
    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);

    // Set initial uniforms and constant parameters
    posScaleLoc = glGetUniformLocation(program, "posScale");
    glUniform4f(posScaleLoc, 1.0f, 1.0f, -1.0f, 1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glFrontFace(GL_CW);

    // Set up vertex array and buffer objects
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLint loc = glGetAttribLocation(program, "inPosition");
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(SoftVertex), (void*)offsetof(SoftVertex, x));
    glEnableVertexAttribArray(loc);
    loc = glGetAttribLocation(program, "inColor");
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(SoftVertex), (void*)offsetof(SoftVertex, r));
    glEnableVertexAttribArray(loc);

    // Create color and depth buffers for rendering
    GLuint col, dep;
    glGenFramebuffers(1, &col);
    glBindFramebuffer(GL_FRAMEBUFFER, col);
    glGenRenderbuffers(1, &dep);
    glBindRenderbuffer(GL_RENDERBUFFER, dep);

    // Create a texture to back the color buffer
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Bind everything for drawing and reading
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dep);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}

uint32_t GpuRenderOgl::getSwizzle(int x, int y) {
    // Convert buffer coordinates to a swizzled offset
    uint32_t ofs = (((y >> 3) * (bufWidth >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    ofs |= ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
    return ofs;
}

void GpuRenderOgl::submitVertex(SoftVertex &vertex) {
    // Queue a vertex to be drawn
    vertices.push_back(vertex);
}

void GpuRenderOgl::flushVertices() {
    // Draw queued vertices and clear them
    if (vertices.empty()) return;
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SoftVertex), &vertices[0], GL_DYNAMIC_DRAW);
    glDrawArrays(primMode, 0, vertices.size());
    vertices = {};
    bufDirty = true;
}

void GpuRenderOgl::flushBuffers() {
    // Check if anything has been drawn and make sure it's done
    flushVertices();
    if (!bufDirty) return;
    glFinish();

    // Read the color buffer and write it to memory based on format
    uint32_t *data;
    switch (colbufFmt) {
    case COL_RGBA8:
        data = new uint32_t[bufWidth * bufHeight];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x++)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 4, data[y * bufWidth + x]);
        break;
    case COL_RGB8:
        data = new uint32_t[bufWidth * bufHeight];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < bufHeight; y++) {
            for (int x = 0; x < bufWidth; x++) {
                uint32_t src = data[y * bufWidth + x], dst = colbufAddr + getSwizzle(x, y) * 3;
                core->memory.write<uint8_t>(ARM11, dst + 0, src >> 8);
                core->memory.write<uint8_t>(ARM11, dst + 1, src >> 16);
                core->memory.write<uint8_t>(ARM11, dst + 2, src >> 24);
            }
        }
        break;
    case COL_RGB565:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    case COL_RGB5A1:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    case COL_RGBA4:
        data = new uint32_t[bufWidth * bufHeight / 2];
        glReadPixels(0, 0, bufWidth, bufHeight, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
        for (int y = 0; y < bufHeight; y++)
            for (int x = 0; x < bufWidth; x += 2)
                core->memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y) * 2, data[(y * bufWidth + x) / 2]);
        break;
    }

    // Clean up and finish
    delete[] data;
    bufDirty = false;
}

void GpuRenderOgl::setPrimMode(PrimMode mode) {
    // Flush old vertices and set a new primitive mode
    flushVertices();
    switch (mode) {
        case TRIANGLES: primMode = GL_TRIANGLES; return;
        case TRI_STRIPS: primMode = GL_TRIANGLE_STRIP; return;
        case TRI_FANS: primMode = GL_TRIANGLE_FAN; return;
        case GEO_PRIM: primMode = GL_TRIANGLES; return;
    }
}

void GpuRenderOgl::setCullMode(CullMode mode) {
    // Change or disable the culling mode
    glEnable(GL_CULL_FACE);
    switch (mode) {
        case CULL_NONE: return glDisable(GL_CULL_FACE);
        case CULL_FRONT: return glCullFace(GL_FRONT);
        case CULL_BACK: return glCullFace(GL_BACK);
    }
}

void GpuRenderOgl::setBlendOper(int i, BlendOper oper) {
    // Update one of the source or destination RGB/alpha blend functions
    switch (oper) {
        case BLND_ZERO: blendOpers[i] = GL_ZERO; break;
        case BLND_ONE: blendOpers[i] = GL_ONE; break;
        case BLND_SRC: blendOpers[i] = GL_SRC_COLOR; break;
        case BLND_1MSRC: blendOpers[i] = GL_ONE_MINUS_SRC_COLOR; break;
        case BLND_DST: blendOpers[i] = GL_DST_COLOR; break;
        case BLND_1MDST: blendOpers[i] = GL_ONE_MINUS_DST_COLOR; break;
        case BLND_SRCA: blendOpers[i] = GL_SRC_ALPHA; break;
        case BLND_1MSRCA: blendOpers[i] = GL_ONE_MINUS_SRC_ALPHA; break;
        case BLND_DSTA: blendOpers[i] = GL_DST_ALPHA; break;
        case BLND_1MDSTA: blendOpers[i] = GL_ONE_MINUS_DST_ALPHA; break;
        case BLND_CONST: blendOpers[i] = GL_CONSTANT_COLOR; break;
        case BLND_1MCON: blendOpers[i] = GL_ONE_MINUS_CONSTANT_COLOR; break;
        case BLND_CONSTA: blendOpers[i] = GL_CONSTANT_ALPHA; break;
        case BLND_1MCONA: blendOpers[i] = GL_ONE_MINUS_CONSTANT_ALPHA; break;
    }
    glBlendFuncSeparate(blendOpers[0], blendOpers[1], blendOpers[2], blendOpers[3]);
}

void GpuRenderOgl::setBlendMode(int i, CalcMode mode) {
    // Update one of the RGB or alpha blend equations
    switch (mode) {
        default: blendModes[i] = GL_FUNC_ADD; break;
        case MODE_SUB: blendModes[i] = GL_FUNC_SUBTRACT; break;
        case MODE_RSUB: blendModes[i] = GL_FUNC_REVERSE_SUBTRACT; break;
        case MODE_MIN: blendModes[i] = GL_MIN; break;
        case MODE_MAX: blendModes[i] = GL_MAX; break;
    }
    glBlendEquationSeparate(blendModes[0], blendModes[1]);
}

void GpuRenderOgl::setBlendColor(float r, float g, float b, float a) {
    // Update the blend color
    glBlendColor(r, g, b, a);
}

void GpuRenderOgl::setAlphaFunc(TestFunc func) {
    // Update the alpha test function
    switch (func) {
        case TEST_NV: alphaFunc = GL_NEVER; break;
        case TEST_AL: alphaFunc = GL_ALWAYS; break;
        case TEST_EQ: alphaFunc = GL_EQUAL; break;
        case TEST_NE: alphaFunc = GL_NOTEQUAL; break;
        case TEST_LT: alphaFunc = GL_LESS; break;
        case TEST_LE: alphaFunc = GL_LEQUAL; break;
        case TEST_GT: alphaFunc = GL_GREATER; break;
        case TEST_GE: alphaFunc = GL_GEQUAL; break;
    }
    glAlphaFunc(alphaFunc, alphaValue);
}

void GpuRenderOgl::setAlphaValue(float value) {
    // Update the alpha test reference value
    alphaValue = value;
    glAlphaFunc(alphaFunc, alphaValue);
}

void GpuRenderOgl::setStencilTest(TestFunc func, bool enable) {
    // Update the stencil test function
    switch (func) {
        case TEST_NV: stencilFunc = GL_NEVER; break;
        case TEST_AL: stencilFunc = GL_ALWAYS; break;
        case TEST_EQ: stencilFunc = GL_EQUAL; break;
        case TEST_NE: stencilFunc = GL_NOTEQUAL; break;
        case TEST_LT: stencilFunc = GL_LESS; break;
        case TEST_LE: stencilFunc = GL_LEQUAL; break;
        case TEST_GT: stencilFunc = GL_GREATER; break;
        case TEST_GE: stencilFunc = GL_GEQUAL; break;
    }

    // Toggle stencil test and set the function
    enable ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    glStencilFunc(stencilFunc, stencilValue, stencilMask);
}

void GpuRenderOgl::setStencilOps(StenOper fail, StenOper depFail, StenOper depPass) {
    // Update the stencil test conditional operations
    StenOper opers[] = { fail, depFail, depPass };
    GLenum glOps[3];
    for (int i = 0; i < 3; i++) {
        switch (opers[i]) {
            case STEN_KEEP: glOps[i] = GL_KEEP; continue;
            case STEN_ZERO: glOps[i] = GL_ZERO; continue;
            case STEN_REPLACE: glOps[i] = GL_REPLACE; continue;
            case STEN_INCR: glOps[i] = GL_INCR; continue;
            case STEN_DECR: glOps[i] = GL_DECR; continue;
            case STEN_INVERT: glOps[i] = GL_INVERT; continue;
            case STEN_INCWR: glOps[i] = GL_INCR_WRAP; continue;
            case STEN_DECWR: glOps[i] = GL_DECR_WRAP; continue;
        }
    }
    glStencilOp(glOps[0], glOps[1], glOps[2]);
}

void GpuRenderOgl::setStencilMasks(uint8_t bufMask, uint8_t refMask) {
    // Update the stencil test masks
    glStencilMask(bufMask);
    glStencilFunc(stencilFunc, stencilValue, stencilMask = refMask);
}

void GpuRenderOgl::setStencilValue(uint8_t value) {
    // Update the stencil reference value
    stencilValue = value;
    glStencilFunc(stencilFunc, stencilValue, stencilMask);
}

void GpuRenderOgl::setBufferDims(uint16_t width, uint16_t height, bool flip) {
    // Flush the old buffers and set new dimensions
    flushBuffers();
    bufWidth = width;
    bufHeight = height;

    // Update the position scale to flip the Y-axis if enabled
    glUniform4f(posScaleLoc, 1.0f, flip ? -1.0f : 1.0f, -1.0f, 1.0f);

    // Resize and clear the color and depth buffers
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufWidth, bufHeight);
    glViewport(0, 0, bufWidth, bufHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GpuRenderOgl::setColbufAddr(uint32_t address) {
    // Flush the old color buffer and set a new address
    flushBuffers();
    colbufAddr = address;
}

void GpuRenderOgl::setColbufFmt(ColbufFmt format) {
    // Flush the old color buffer and set a new format
    flushBuffers();
    colbufFmt = format;
}

void GpuRenderOgl::setColbufMask(uint8_t mask) {
    // Update the color write mask
    glColorMask(bool(mask & BIT(0)), bool(mask & BIT(1)), bool(mask & BIT(2)), bool(mask & BIT(3)));
}

void GpuRenderOgl::setDepbufMask(uint8_t mask) {
    // Update the depth write mask
    glDepthMask(bool(mask & BIT(1)));
}

void GpuRenderOgl::setDepthFunc(TestFunc func) {
    // Update the depth testing function
    switch (func) {
        case TEST_NV: return glDepthFunc(GL_NEVER);
        case TEST_AL: return glDepthFunc(GL_ALWAYS);
        case TEST_EQ: return glDepthFunc(GL_EQUAL);
        case TEST_NE: return glDepthFunc(GL_NOTEQUAL);
        case TEST_LT: return glDepthFunc(GL_LESS);
        case TEST_LE: return glDepthFunc(GL_LEQUAL);
        case TEST_GT: return glDepthFunc(GL_GREATER);
        case TEST_GE: return glDepthFunc(GL_GEQUAL);
    }
}
