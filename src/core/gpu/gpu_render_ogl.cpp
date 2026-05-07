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

#include <algorithm>
#include <cstring>

#include "../core.h"
#include "gpu_render_ogl.h"

enum RenderLoc {
    LOC_IN_POS = 0,
    LOC_IN_COL = 1,
    LOC_IN_CRDS = 2,
    LOC_IN_CRDT = 3,
    LOC_IN_QUAT = 4,
    LOC_IN_VIEW = 5
};

enum TexSlot {
    TEX_BUFFER = 0,
    TEX_LUTD0,
    TEX_LUTD1,
    TEX_LUTFR,
    TEX_LUTRB,
    TEX_LUTRG,
    TEX_LUTRR,
    TEX_LUTSP,
    TEX_LUTDA,
    TEX_UNIT0,
    TEX_UNIT1,
    TEX_UNIT2,
    TEX_UNIT3
};

const char *GpuRenderOgl::vtxCodeSoft = R"(
    #version 330

    layout(location = 0) in vec4 inPosition;
    layout(location = 1) in vec4 inColor;
    layout(location = 2) in vec3 inCoordsS;
    layout(location = 3) in vec3 inCoordsT;
    layout(location = 4) in vec4 inNormQuat;
    layout(location = 5) in vec3 inViewVec;

    out vec4 vtxColor;
    out vec3 vtxCoordsS;
    out vec3 vtxCoordsT;
    out vec4 vtxNormQuat;
    out vec3 vtxViewVec;
    uniform vec4 posScale;

    void main() {
        gl_Position = inPosition * posScale;
        vtxColor = inColor;
        vtxCoordsS = inCoordsS;
        vtxCoordsT = inCoordsT;
        vtxNormQuat = inNormQuat;
        vtxViewVec = inViewVec;
    }
)";

const char *GpuRenderOgl::fragCode = R"(
    #version 330

    in vec4 vtxColor;
    in vec3 vtxCoordsS;
    in vec3 vtxCoordsT;
    in vec4 vtxNormQuat;
    in vec3 vtxViewVec;
    out vec4 fragColor;

    uniform ivec3 combSrcs[12];
    uniform ivec3 combOpers[12];
    uniform ivec2 combModes[6];
    uniform vec4 combColors[6];
    uniform vec4 combBufColor;
    uniform int combBufMask;
    uniform int alphaFunc;
    uniform float alphaValue;

    uniform vec3 lightSpec0[8];
    uniform vec3 lightSpec1[8];
    uniform vec3 lightDiff[8];
    uniform vec3 lightAmb[8];
    uniform vec3 lightVector[8];
    uniform vec3 lightSpot[8];
    uniform vec2 lightAtten[8];
    uniform int lightTypes[8];
    uniform vec3 lightBaseAmb;
    uniform int lutMask;
    uniform int lutAbsFlags[7];
    uniform int lutInputs[7];
    uniform float lutScales[7];
    uniform int lightMap[9];

    uniform sampler1D lutD0;
    uniform sampler1D lutD1;
    uniform sampler1D lutFr;
    uniform sampler1D lutRb;
    uniform sampler1D lutRg;
    uniform sampler1D lutRr;
    uniform sampler1DArray lutSp;
    uniform sampler1DArray lutDa;
    uniform sampler2D texUnits[3];

    vec4 prevColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 combBuffer = combBufColor;
    vec4 fragColors[2];
    bool fragDone = false;

    float dot3(vec3 c0, vec3 c1) {
        return 4.0 * c0.r - 0.5 * c1.r - 0.5 + c0.g - 0.5 * c1.g - 0.5 + c0.b - 0.5 * c1.b - 0.5;
    }

    void updateFrag() {
        float n = 2.0f / dot(vtxNormQuat, vtxNormQuat);
        float x = (vtxNormQuat.x * vtxNormQuat.z - vtxNormQuat.y * vtxNormQuat.w) * n;
        float y = (vtxNormQuat.y * vtxNormQuat.z + vtxNormQuat.x * vtxNormQuat.w) * n;
        float z = 1.0f - (vtxNormQuat.x * vtxNormQuat.x - vtxNormQuat.y * vtxNormQuat.y) * n;
        vec3 normalVec = normalize(vec3(x, y, z));
        vec3 viewVec = normalize(vtxViewVec);
        fragColors[0] = vec4(lightBaseAmb, 0.0);
        fragColors[1] = vec4(0.0, 0.0, 0.0, 0.0);

        for (int i = 0; lightMap[i] >= 0; i++) {
            int id = lightMap[i];
            vec3 lightVec = lightVector[id] + (lightTypes[id] == 0 ? vtxViewVec : vec3(0.0));
            vec3 halfVec = normalize((lightVec + vtxViewVec) / 2);
            vec3 spotVec = normalize(lightSpot[id]);
            lightVec = normalize(lightVec);

            float inp[7];
            inp[0] = dot(normalVec, halfVec);
            inp[1] = dot(viewVec, halfVec);
            inp[2] = dot(normalVec, viewVec);
            inp[3] = dot(lightVec, normalVec);
            inp[4] = dot(-lightVec, spotVec);
            inp[5] = inp[6] = 0.0;

            float idx = (lutAbsFlags[0] != 0) ? abs(inp[lutInputs[0]]) : inp[lutInputs[0]];
            float d0 = ((lutMask & (1 << 0)) != 0) ? texture(lutD0, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[0] : 1.0;
            idx = (lutAbsFlags[1] != 0) ? abs(inp[lutInputs[1]]) : inp[lutInputs[1]];
            float d1 = ((lutMask & (1 << 1)) != 0) ? texture(lutD1, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[1] : 1.0;
            idx = (lutAbsFlags[3] != 0) ? abs(inp[lutInputs[3]]) : inp[lutInputs[3]];
            float fr = ((lutMask & (1 << 3)) != 0) ? texture(lutFr, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[3] : 1.0;
            idx = (lutAbsFlags[6] != 0) ? abs(inp[lutInputs[6]]) : inp[lutInputs[6]];
            float rr = ((lutMask & (1 << 6)) != 0) ? texture(lutRr, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[6] : 1.0;
            idx = (lutAbsFlags[5] != 0) ? abs(inp[lutInputs[5]]) : inp[lutInputs[5]];
            float rg = ((lutMask & (1 << 5)) != 0) ? texture(lutRg, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[5] : rr;
            idx = (lutAbsFlags[4] != 0) ? abs(inp[lutInputs[4]]) : inp[lutInputs[4]];
            float rb = ((lutMask & (1 << 4)) != 0) ? texture(lutRb, (idx * 0x7F + 0x80) / 0xFF).r * lutScales[4] : rr;
            vec3 r = vec3(rr, rg, rb);

            float sp = 1.0;
            if ((lutMask & (1 << (8 + id))) != 0) {
                idx = (lutAbsFlags[2] != 0) ? abs(inp[lutInputs[2]]) : inp[lutInputs[2]];
                sp = texture(lutSp, vec2((idx * 0x7F + 0x80) / 0xFF, id)).r * lutScales[2];
            }
            if ((lutMask & (1 << (16 + id))) != 0) {
                idx = lightAtten[id][0] + gl_FragCoord.z * lightAtten[id][1];
                sp *= texture(lutDa, vec2(idx, id)).r;
            }

            fragColors[0] += vec4(sp * (lightAmb[id] + lightDiff[id] * inp[3]), fr);
            fragColors[1] += vec4(sp * (lightSpec0[id] * d0 + lightSpec1[id] * d1 * r), fr);
        }
        fragColors[0] = min(max(fragColors[0], 0.0), 1.0);
        fragColors[1] = min(max(fragColors[1], 0.0), 1.0);
        fragDone = true;
    }

    vec4 getSrc(int i, int j) {
        vec4 color;
        switch (combSrcs[i * 2 + ((j > 2) ? 1 : 0)][j % 3]) {
            case 0: color = vtxColor; break;
            case 1: if (!fragDone) updateFrag(); color = fragColors[0]; break;
            case 2: if (!fragDone) updateFrag(); color = fragColors[1]; break;
            case 3: color = texture(texUnits[0], vec2(vtxCoordsS[0], vtxCoordsT[0])); break;
            case 4: color = texture(texUnits[1], vec2(vtxCoordsS[1], vtxCoordsT[1])); break;
            case 5: color = texture(texUnits[2], vec2(vtxCoordsS[2], vtxCoordsT[2])); break;
            case 6: color = vec4(1.0, 1.0, 1.0, 1.0); break;
            case 7: color = combBuffer; break;
            case 8: color = combColors[i]; break;
            case 9: color = prevColor; break;
            default: color = vec4(0.0, 0.0, 0.0, 0.0); break;
        }

        switch (combOpers[i * 2 + ((j > 2) ? 1 : 0)][j % 3]) {
            default: return color;
            case 1: return 1.0 - color;
            case 2: return color.aaaa;
            case 3: return 1.0 - color.aaaa;
            case 4: return color.rrrr;
            case 5: return 1.0 - color.rrrr;
            case 6: return color.gggg;
            case 7: return 1.0 - color.gggg;
            case 8: return color.bbbb;
            case 9: return 1.0 - color.bbbb;
        }
    }

    void main() {
        vec4 color;
        for (int i = 0; i < 6; i++) {
            switch (combModes[i][0]) {
                case 0: color.rgb = getSrc(i, 0).rgb; break;
                case 1: color.rgb = getSrc(i, 0).rgb * getSrc(i, 1).rgb; break;
                case 2: color.rgb = getSrc(i, 0).rgb + getSrc(i, 1).rgb; break;
                case 3: color.rgb = getSrc(i, 0).rgb + getSrc(i, 1).rgb - 0.5; break;
                case 4: color.rgb = mix(getSrc(i, 1).rgb, getSrc(i, 0).rgb, getSrc(i, 2).rgb); break;
                case 5: color.rgb = getSrc(i, 0).rgb - getSrc(i, 1).rgb; break;
                case 6: color.rgb = vec3(dot3(getSrc(i, 0).rgb, getSrc(i, 1).rgb)); break;
                case 7: color.rgb = vec3(dot3(getSrc(i, 0).rgb, getSrc(i, 1).rgb)); break;
                case 8: color.rgb = (getSrc(i, 0).rgb * getSrc(i, 1).rgb) + getSrc(i, 2).rgb; break;
                case 9: color.rgb = (getSrc(i, 0).rgb + getSrc(i, 1).rgb) * getSrc(i, 2).rgb; break;
                default: color.rgb = vec3(0.0, 0.0, 0.0); break;
            }

            switch (combModes[i][1]) {
                case 0: color.a = getSrc(i, 3).a; break;
                case 1: color.a = getSrc(i, 3).a * getSrc(i, 4).a; break;
                case 2: color.a = getSrc(i, 3).a + getSrc(i, 4).a; break;
                case 3: color.a = getSrc(i, 3).a + getSrc(i, 4).a - 0.5; break;
                case 4: color.a = mix(getSrc(i, 4).a, getSrc(i, 3).a, getSrc(i, 5).a); break;
                case 5: color.a = getSrc(i, 3).a - getSrc(i, 4).a; break;
                case 6: color.a = 1.0; break;
                case 7: color.a = dot3(getSrc(i, 3).aaa, getSrc(i, 4).aaa); break;
                case 8: color.a = (getSrc(i, 3).a * getSrc(i, 4).a) + getSrc(i, 5).a; break;
                case 9: color.a = (getSrc(i, 3).a + getSrc(i, 4).a) * getSrc(i, 5).a; break;
                default: color.a = 1.0; break;
            }

            prevColor = color;
            if (i >= 4) continue;
            if ((combBufMask & (0x01 << i)) != 0) combBuffer.rgb = color.rgb;
            if ((combBufMask & (0x10 << i)) != 0) combBuffer.a = color.a;
        }

        switch (alphaFunc) {
            case 0: discard;
            case 1: break;
            case 2: if (color.a == alphaValue) break; discard;
            case 3: if (color.a != alphaValue) break; discard;
            case 4: if (color.a < alphaValue) break; discard;
            case 5: if (color.a <= alphaValue) break; discard;
            case 6: if (color.a > alphaValue) break; discard;
            case 7: if (color.a >= alphaValue) break; discard;
        }
        fragColor = color;
    }
)";

GpuRenderOgl::GpuRenderOgl(Core &core): core(core) {
    // Compile the default shader program for software vertices
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragCode, nullptr);
    glCompileShader(fragShader);
    softProgram = makeProgram(vtxCodeSoft);
    setProgram(softProgram);

    // Check for fragment compilation errors and log them
    GLint res, size;
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &res);
    if (res == GL_FALSE) {
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &size);
        GLchar *log = new GLchar[size];
        glGetShaderInfoLog(fragShader, size, &size, log);
        LOG_CRIT("Fragment shader GLSL compilation error: %s", log);
        delete[] log;
    }

    // Create array and buffer objects for the soft vertex shader
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Configure input attributes for the soft vertex shader
    glVertexAttribPointer(LOC_IN_POS, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, x));
    glEnableVertexAttribArray(LOC_IN_POS);
    glVertexAttribPointer(LOC_IN_COL, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, r));
    glEnableVertexAttribArray(LOC_IN_COL);
    glVertexAttribPointer(LOC_IN_CRDS, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, s0));
    glEnableVertexAttribArray(LOC_IN_CRDS);
    glVertexAttribPointer(LOC_IN_CRDT, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, t0));
    glEnableVertexAttribArray(LOC_IN_CRDT);
    glVertexAttribPointer(LOC_IN_QUAT, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, qx));
    glEnableVertexAttribArray(LOC_IN_QUAT);
    glVertexAttribPointer(LOC_IN_VIEW, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInput), (void*)offsetof(SoftVertex, vx));
    glEnableVertexAttribArray(LOC_IN_VIEW);

    // Set some state that only has to be done once
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClearStencil(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glFrontFace(GL_CW);

    // Create color and depth buffers for rendering
    glGenFramebuffers(1, &colBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, colBuf);
    glGenRenderbuffers(1, &depBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depBuf);

    // Create a texture to back the color buffer
    glGenTextures(9, textures);
    glActiveTexture(GL_TEXTURE0 + TEX_BUFFER);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Bind everything for drawing and reading
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures[0], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depBuf);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Initialize and allocate 1D textures for interpolated light LUTs
    for (int i = 0; i < 8; i++) {
        glActiveTexture(GL_TEXTURE0 + LUT_D0 + i);
        GLenum type = (i < 6) ? GL_TEXTURE_1D : GL_TEXTURE_1D_ARRAY;
        glBindTexture(type, textures[LUT_D0 + i]);
        glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        if (i < 6) glTexImage1D(type, 0, GL_RED, 0x100, 0, GL_RED, GL_FLOAT, nullptr); // D0, D1, FR, RB, RG, RR
        else glTexImage2D(type, 0, GL_RED, 0x100, 8, 0, GL_RED, GL_FLOAT, nullptr); // SP0-7, DA0-7
    }
}

GpuRenderOgl::~GpuRenderOgl() {
    // Clean up resources allocated in the texture cache
    for (int i = 0; i < texCache.size(); i++) {
        glDeleteTextures(1, &texCache[i].tex);
        delete[] texCache[i].tags;
    }

    // Clean up everything else that was generated
    glDeleteTextures(9, textures);
    glDeleteRenderbuffers(1, &depBuf);
    glDeleteFramebuffers(1, &colBuf);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(softProgram);
    glDeleteShader(fragShader);
}

GLuint GpuRenderOgl::makeProgram(const char *vtxCode) {
    // Compile the provided vertex shader code
    GLint vtxShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtxShader, 1, &vtxCode, nullptr);
    glCompileShader(vtxShader);

    // Check for vertex compilation errors and log them
    GLint res, size;
    glGetShaderiv(vtxShader, GL_COMPILE_STATUS, &res);
    if (res == GL_FALSE) {
        glGetShaderiv(vtxShader, GL_INFO_LOG_LENGTH, &size);
        GLchar *log = new GLchar[size];
        glGetShaderInfoLog(vtxShader, size, &size, log);
        LOG_CRIT("Vertex shader GLSL compilation error: %s", log);
        delete[] log;
    }

    // Link a program using the shared fragment shader
    GLuint program = glCreateProgram();
    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    // Clean up the shader and return the program
    glDeleteShader(vtxShader);
    return program;
}

void GpuRenderOgl::setProgram(GLuint program) {
    // Update the program and its uniform locations
    glUseProgram(program);
    posScaleLoc = glGetUniformLocation(program, "posScale");
    combSrcsLoc = glGetUniformLocation(program, "combSrcs");
    combOpersLoc = glGetUniformLocation(program, "combOpers");
    combModesLoc = glGetUniformLocation(program, "combModes");
    combColorsLoc = glGetUniformLocation(program, "combColors");
    combBufColorLoc = glGetUniformLocation(program, "combBufColor");
    combBufMaskLoc = glGetUniformLocation(program, "combBufMask");
    alphaFuncLoc = glGetUniformLocation(program, "alphaFunc");
    alphaValueLoc = glGetUniformLocation(program, "alphaValue");
    lightSpec0Loc = glGetUniformLocation(program, "lightSpec0");
    lightSpec1Loc = glGetUniformLocation(program, "lightSpec1");
    lightDiffLoc = glGetUniformLocation(program, "lightDiff");
    lightAmbLoc = glGetUniformLocation(program, "lightAmb");
    lightVectorLoc = glGetUniformLocation(program, "lightVector");
    lightSpotLoc = glGetUniformLocation(program, "lightSpot");
    lightAttenLoc = glGetUniformLocation(program, "lightAtten");
    lightTypesLoc = glGetUniformLocation(program, "lightTypes");
    lightBaseAmbLoc = glGetUniformLocation(program, "lightBaseAmb");
    lutMaskLoc = glGetUniformLocation(program, "lutMask");
    lutAbsFlagsLoc = glGetUniformLocation(program, "lutAbsFlags");
    lutInputsLoc = glGetUniformLocation(program, "lutInputs");
    lutScalesLoc = glGetUniformLocation(program, "lutScales");
    lightMapLoc = glGetUniformLocation(program, "lightMap");

    // Restore uniform values for the new program
    glUniform4f(posScaleLoc, 1.0f, flipY ? -1.0f : 1.0f, -1.0f, 1.0f);
    glUniform3iv(combSrcsLoc, 12, combSrcs[0]);
    glUniform3iv(combOpersLoc, 12, combOpers[0]);
    glUniform2iv(combModesLoc, 6, combModes[0]);
    glUniform4fv(combColorsLoc, 6, combColors[0]);
    glUniform4fv(combBufColorLoc, 1, combBufColor);
    glUniform1i(combBufMaskLoc, combBufMask);
    glUniform1i(alphaFuncLoc, alphaFunc);
    glUniform1f(alphaValueLoc, alphaValue);
    glUniform3fv(lightSpec0Loc, 8, lightSpec0[0]);
    glUniform3fv(lightSpec1Loc, 8, lightSpec1[0]);
    glUniform3fv(lightDiffLoc, 8, lightDiff[0]);
    glUniform3fv(lightAmbLoc, 8, lightAmb[0]);
    glUniform3fv(lightVectorLoc, 8, lightVector[0]);
    glUniform3fv(lightSpotLoc, 8, lightSpot[0]);
    glUniform2fv(lightAttenLoc, 8, lightAtten[0]);
    glUniform1iv(lightTypesLoc, 8, lightTypes);
    glUniform3fv(lightBaseAmbLoc, 1, lightBaseAmb);
    glUniform1i(lutMaskLoc, lutMask);
    glUniform1iv(lutAbsFlagsLoc, 7, lutAbsFlags);
    glUniform1iv(lutInputsLoc, 7, lutInputs);
    glUniform1fv(lutScalesLoc, 7, lutScales);
    glUniform1iv(lightMapLoc, 9, lightMap);

    // Map texture slots for the new program
    glUniform1i(glGetUniformLocation(program, "lutD0"), TEX_LUTD0);
    glUniform1i(glGetUniformLocation(program, "lutD1"), TEX_LUTD1);
    glUniform1i(glGetUniformLocation(program, "lutFr"), TEX_LUTFR);
    glUniform1i(glGetUniformLocation(program, "lutRb"), TEX_LUTRB);
    glUniform1i(glGetUniformLocation(program, "lutRg"), TEX_LUTRG);
    glUniform1i(glGetUniformLocation(program, "lutRr"), TEX_LUTRR);
    glUniform1i(glGetUniformLocation(program, "lutSp"), TEX_LUTSP);
    glUniform1i(glGetUniformLocation(program, "lutDa"), TEX_LUTDA);
    GLint loc = glGetUniformLocation(program, "texUnits");
    for (int i = 0; i < 3; i++) glUniform1i(loc + i, TEX_UNIT0 + i);
}

uint32_t GpuRenderOgl::getSwizzle(int x, int y, int width) {
    // Convert buffer coordinates to a swizzled offset
    uint32_t ofs = (((y >> 3) * (width >> 3) + (x >> 3)) << 6);
    ofs |= ((y << 3) & 0x20) | ((y << 2) & 0x8) | ((y << 1) & 0x2);
    ofs |= ((x << 2) & 0x10) | ((x << 1) & 0x4) | (x & 0x1);
    return ofs;
}

template <bool alpha> uint32_t GpuRenderOgl::etc1Texel(int i, int x, int y) {
    // Convert coordinates to an offset and index
    uint32_t ofs = getSwizzle(x, y, texWidths[i]), value;
    uint8_t idx = (x & 0x3) * 4 + (y & 0x3);
    int r, g, b, a;

    // Adjust the offset for 4x4 ETC1 tiles and read alpha if provided
    if (alpha) {
        ofs = (ofs & ~0xF) + 8;
        value = core.memory.read<uint8_t>(ARM11, texAddrs[i] + ofs - 8 + idx / 2);
        a = ((value >> ((idx & 0x1) * 4)) & 0xF) * 0xFF / 0xF;
    }
    else {
        ofs = (ofs & ~0xF) >> 1;
        a = 0xFF;
    }

    // Decode an ETC1 texel based on the block it falls in and the base color mode
    int32_t val1 = core.memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 0);
    int32_t val2 = core.memory.read<uint32_t>(ARM11, texAddrs[i] + ofs + 4);
    if ((((val2 & BIT(0)) ? y : x) & 0x3) < 2) { // Block 1
        int16_t tbl = etc1Tables[(val2 >> 5) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
        if (val2 & BIT(1)) { // Differential
            r = ((val2 >> 27) & 0x1F) * 0x21 / 4 + tbl;
            g = ((val2 >> 19) & 0x1F) * 0x21 / 4 + tbl;
            b = ((val2 >> 11) & 0x1F) * 0x21 / 4 + tbl;
        }
        else { // Individual
            r = ((val2 >> 28) & 0xF) * 0x11 + tbl;
            g = ((val2 >> 20) & 0xF) * 0x11 + tbl;
            b = ((val2 >> 12) & 0xF) * 0x11 + tbl;
        }
    }
    else { // Block 2
        int16_t tbl = etc1Tables[(val2 >> 2) & 0x7][((val1 >> (idx + 15)) & 0x2) | ((val1 >> idx) & 0x1)];
        if (val2 & BIT(1)) { // Differential
            r = (((val2 >> 27) & 0x1F) + (int8_t(val2 >> 19) >> 5)) * 0x21 / 4 + tbl;
            g = (((val2 >> 19) & 0x1F) + (int8_t(val2 >> 11) >> 5)) * 0x21 / 4 + tbl;
            b = (((val2 >> 11) & 0x1F) + (int8_t(val2 >> 3) >> 5)) * 0x21 / 4 + tbl;
        }
        else { // Individual
            r = ((val2 >> 24) & 0xF) * 0x11 + tbl;
            g = ((val2 >> 16) & 0xF) * 0x11 + tbl;
            b = ((val2 >> 8) & 0xF) * 0x11 + tbl;
        }
    }

    // Clamp and return the final color values
    r = std::min(0xFF, std::max(0, r));
    g = std::min(0xFF, std::max(0, g));
    b = std::min(0xFF, std::max(0, b));
    return (r << 24) | (g << 16) | (b << 8) | a;
}

void GpuRenderOgl::submitInput(float (*input)[4]) {
    // Queue raw shader input to be drawn
    VertexInput vi;
    memcpy(vi.input, input, sizeof(vi.input));
    vertices.push_back(vi);
}

void GpuRenderOgl::submitVertex(SoftVertex &vertex) {
    // Queue a software vertex to be drawn
    VertexInput vi;
    vi.vertex = vertex;
    vertices.push_back(vi);
}

void GpuRenderOgl::flushVertices() {
    // Update state and draw queued vertices
    if (vertices.empty()) return;
    if (readDirty) updateBuffers();
    if (texDirty) updateTextures();
    if (lutDirty) updateLuts();
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexInput), &vertices[0], GL_DYNAMIC_DRAW);
    glDrawArrays(primMode, 0, vertices.size());
    vertices = {};
    writeDirty = true;
}

void GpuRenderOgl::flushBuffers(uint32_t mod) {
    // Finish drawing and update dirty state if a buffer is being modified
    flushVertices();
    readDirty |= (mod == colbufAddr) | ((mod == depbufAddr) << 1);
    if (!writeDirty) return;
    glFinish();

    // Copy data from the color buffer to memory based on format
    uint32_t *data = nullptr;
    uint16_t w = bufWidth, h = bufHeight;
    switch (colbufFmt) {
    case COL_RGBA8:
        data = new uint32_t[w * h];
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                core.memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 4, data[y * w + x]);
        break;
    case COL_RGB8:
        data = new uint32_t[w * h];
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                uint32_t src = data[y * w + x], dst = colbufAddr + getSwizzle(x, y, w) * 3;
                core.memory.write<uint8_t>(ARM11, dst + 0, src >> 8);
                core.memory.write<uint8_t>(ARM11, dst + 1, src >> 16);
                core.memory.write<uint8_t>(ARM11, dst + 2, src >> 24);
            }
        }
        break;
    case COL_RGB565:
        data = new uint32_t[w * h / 2];
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                core.memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2, data[(y * w + x) / 2]);
        break;
    case COL_RGB5A1:
        data = new uint32_t[w * h / 2];
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                core.memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2, data[(y * w + x) / 2]);
        break;
    case COL_RGBA4:
        data = new uint32_t[w * h / 2];
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                core.memory.write<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2, data[(y * w + x) / 2]);
        break;
    }
    delete[] data;
    writeDirty = false;
}

void GpuRenderOgl::updateBuffers() {
    // Resize and clear the depth/stencil buffer if dirty, restoring state after
    if (readDirty & BIT(1)) {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufWidth, bufHeight);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glViewport(0, 0, bufWidth, bufHeight);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDepthMask(depbufMask);
        glStencilMask(stencilMasks[0]);
        updateViewport();
        readDirty &= ~BIT(1);
    }

    // Copy data from memory to the color buffer based on format if dirty
    if (~readDirty & BIT(0)) return;
    uint32_t *data = nullptr;
    uint16_t w = bufWidth, h = bufHeight;
    glActiveTexture(GL_TEXTURE0 + TEX_BUFFER);
    switch (colbufFmt) {
    case COL_RGBA8:
        data = new uint32_t[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                data[y * w + x] = core.memory.read<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        break;
    case COL_RGB8:
        data = new uint32_t[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                data[y * w + x] = core.memory.read<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 3 - 1) | 0xFF;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
        break;
    case COL_RGB565:
        data = new uint32_t[w * h / 2];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                data[(y * w + x) / 2] = core.memory.read<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
        break;
    case COL_RGB5A1:
        data = new uint32_t[w * h / 2];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                data[(y * w + x) / 2] = core.memory.read<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
        break;
    case COL_RGBA4:
        data = new uint32_t[w * h / 2];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x += 2)
                data[(y * w + x) / 2] = core.memory.read<uint32_t>(ARM11, colbufAddr + getSwizzle(x, y, w) * 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufWidth, bufHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, data);
        break;
    }
    delete[] data;
    readDirty &= ~BIT(0);
}

void GpuRenderOgl::updateTextures() {
    // Update any textures that are dirty
    for (int i = 0; texDirty >> i; i++) {
        if (~texDirty & BIT(i)) continue;
        glActiveTexture(GL_TEXTURE0 + TEX_UNIT0 + i);

        // Check for a matching texture in the cache
        const TexCache *cache = nullptr;
        TexCache cmp; cmp.addr = texAddrs[i];
        auto it = std::lower_bound(texCache.cbegin(), texCache.cend(), cmp);
        while (it < texCache.cend() && it->addr == texAddrs[i]) {
            if (it->width == texWidths[i] && it->height == texHeights[i] && it->fmt == texFmts[i]) {
                cache = &*it;
                break;
            }
            it++;
        }

        // Process the cache entry or create it if missing
        if (cache) {
            // Bind an existing texture from the cache
            glBindTexture(GL_TEXTURE_2D, cache->tex);
            const TexCache *c = cache;

            // Verify memory tags and invalidate the cache if they changed
            for (int j = 0; j < c->size; j++) {
                uint32_t tag = core.memory.memMap11[(c->addr >> 12) + j].tag;
                if (c->tags[j] == tag) continue;
                c->tags[j] = tag;
                cache = nullptr;
            }
        }
        else {
            // Create a new texture with current tags for the memory it uses
            TexCache tex = { texAddrs[i], texWidths[i], texHeights[i], texFmts[i] };
            static const uint8_t nybs[] = { 8, 6, 4, 4, 4, 4, 4, 2, 2, 2, 1, 1, 1, 2, 1 };
            tex.size = (tex.width * tex.height * nybs[tex.fmt] / 2 + 0xFFF) >> 12;
            tex.tags = new uint32_t[tex.size];
            for (int j = 0; j < tex.size; j++)
                tex.tags[j] = core.memory.memMap11[(tex.addr >> 12) + j].tag;

            // Bind the new texture and add it to the cache
            glGenTextures(1, &tex.tex);
            glBindTexture(GL_TEXTURE_2D, tex.tex);
            it = std::upper_bound(texCache.cbegin(), texCache.cend(), tex);
            texCache.insert(it, tex);
        }

        // Update texture parameters and finish if already cached
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapS[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapT[i]);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, texBorders[i]);
        if (cache) continue;

        // Set texture swizzling based on format
        switch (texFmts[i]) {
        case TEX_RG8:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        case TEX_LA8: case TEX_LA4:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
            break;
        case TEX_L8: case TEX_L4:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            break;
        case TEX_A8: case TEX_A4:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
            break;
        default:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);
            break;
        }

        // Decode and upload a texture based on format
        uint16_t w = texWidths[i], h = texHeights[i], y1 = (h - 1);
        uint32_t *dat32; uint16_t *dat16;
        switch (texFmts[i]) {
        case TEX_RGBA8:
            dat32 = new uint32_t[w * h];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x++)
                    dat32[y1 * w + x] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, dat32);
            delete[] dat32;
            continue;
        case TEX_RGB8:
            dat32 = new uint32_t[w * h];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x++)
                    dat32[y1 * w + x] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 3 - 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, dat32);
            delete[] dat32;
            continue;
        case TEX_RGB5A1:
            dat32 = new uint32_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x += 2)
                    dat32[(y1 * w + x) / 2] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, dat32);
            delete[] dat32;
            continue;
        case TEX_RGB565:
            dat32 = new uint32_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x += 2)
                    dat32[(y1 * w + x) / 2] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, dat32);
            delete[] dat32;
            continue;
        case TEX_RGBA4:
            dat32 = new uint32_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x += 2)
                    dat32[(y1 * w + x) / 2] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, dat32);
            delete[] dat32;
            continue;
        case TEX_LA8: case TEX_RG8:
            dat32 = new uint32_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x += 2)
                    dat32[(y1 * w + x) / 2] = core.memory.read<uint32_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) * 2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w, h, 0, GL_RG, GL_UNSIGNED_BYTE, dat32);
            delete[] dat32;
            continue;
        case TEX_L8: case TEX_A8:
            dat16 = new uint16_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x += 2)
                    dat16[(y1 * w + x) / 2] = core.memory.read<uint16_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, dat16);
            delete[] dat16;
            continue;
        case TEX_LA4:
            dat16 = new uint16_t[w * h];
            for (int y = 0; y < h; y++, y1--) {
                for (int x = 0; x < w; x++) {
                    uint8_t val = core.memory.read<uint8_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w));
                    dat16[y1 * w + x] = (((val >> 4) * 0xFF / 0xF) << 8) | ((val & 0xF) * 0xFF / 0xF);
                }
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w, h, 0, GL_RG, GL_UNSIGNED_BYTE, dat16);
            delete[] dat16;
            continue;
        case TEX_L4: case TEX_A4:
            dat16 = new uint16_t[w * h / 2];
            for (int y = 0; y < h; y++, y1--) {
                for (int x = 0; x < w; x += 2) {
                    uint8_t val = core.memory.read<uint8_t>(ARM11, texAddrs[i] + getSwizzle(x, y, w) / 2);
                    dat16[(y1 * w + x) / 2] = (((val >> 4) * 0xFF / 0xF) << 8) | ((val & 0xF) * 0xFF / 0xF);
                }
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, dat16);
            delete[] dat16;
            continue;
        case TEX_ETC1:
            dat32 = new uint32_t[w * h];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x++)
                    dat32[y1 * w + x] = etc1Texel<false>(i, x, y);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, dat32);
            delete[] dat32;
            continue;
        case TEX_ETC1A4:
            dat32 = new uint32_t[w * h];
            for (int y = 0; y < h; y++, y1--)
                for (int x = 0; x < w; x++)
                    dat32[y1 * w + x] = etc1Texel<true>(i, x, y);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, dat32);
            delete[] dat32;
            continue;
        default:
            w = 0xFFFF;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, &w);
            continue;
        }
    }
    texDirty = 0;
}

void GpuRenderOgl::updateLuts() {
    // Update any light LUT textures that are dirty
    for (int i = 0; lutDirty != 0; i++) {
        if (~lutDirty & BIT(i)) continue;
        lutDirty &= ~BIT(i);

        // Replace light LUT data based on ID without reallocating
        switch (i) {
        case LUT_D0:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTD0);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutD0);
            continue;
        case LUT_D1:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTD1);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutD1);
            continue;
        case LUT_FR:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTFR);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutFr);
            continue;
        case LUT_RB:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTRB);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutRb);
            continue;
        case LUT_RG:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTRG);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutRg);
            continue;
        case LUT_RR:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTRR);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 0x100, GL_RED, GL_FLOAT, lutRr);
            continue;
        case LUT_SP0: case LUT_SP1: case LUT_SP2: case LUT_SP3:
        case LUT_SP4: case LUT_SP5: case LUT_SP6: case LUT_SP7:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTSP);
            glTexSubImage2D(GL_TEXTURE_1D_ARRAY, 0, 0, i - LUT_SP0, 0x100, 1, GL_RED, GL_FLOAT, lutSp[i - LUT_SP0]);
            continue;
        case LUT_DA0: case LUT_DA1: case LUT_DA2: case LUT_DA3:
        case LUT_DA4: case LUT_DA5: case LUT_DA6: case LUT_DA7:
            glActiveTexture(GL_TEXTURE0 + TEX_LUTDA);
            glTexSubImage2D(GL_TEXTURE_1D_ARRAY, 0, 0, i - LUT_DA0, 0x100, 1, GL_RED, GL_FLOAT, lutDa[i - LUT_DA0]);
            continue;
        }
    }
}

void GpuRenderOgl::updateViewport() {
    // Update the viewport, adjusting Y-position when flipped
    GLint y = flipY ? (bufHeight - viewHeight) : 0;
    glViewport(0 - viewX, y - viewY, viewWidth, viewHeight);
}

void GpuRenderOgl::setPrimMode(PrimMode mode) {
    // Set a new primitive mode
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
    flushVertices();
    glEnable(GL_CULL_FACE);
    switch (mode) {
        case CULL_NONE: return glDisable(GL_CULL_FACE);
        case CULL_FRONT: return glCullFace(GL_FRONT);
        case CULL_BACK: return glCullFace(GL_BACK);
    }
}

void GpuRenderOgl::setTexAddr(int i, uint32_t address) {
    // Set a texture unit's address and mark it as dirty
    flushVertices();
    texAddrs[i] = address;
    texDirty |= BIT(i);
}

void GpuRenderOgl::setTexDims(int i, uint16_t width, uint16_t height) {
    // Set a texture unit's dimensions and mark it as dirty
    flushVertices();
    texWidths[i] = width;
    texHeights[i] = height;
    texDirty |= BIT(i);
}

void GpuRenderOgl::setTexBorder(int i, float *color) {
    // Set a texture unit's border and mark it as dirty
    flushVertices();
    for (int j = 0; j < 4; j++)
        texBorders[i][j] = color[j];
    texDirty |= BIT(i);
}

void GpuRenderOgl::setTexFmt(int i, TexFmt format) {
    // Set a texture unit's format and mark it as dirty
    flushVertices();
    texFmts[i] = format;
    texDirty |= BIT(i);
}

void GpuRenderOgl::setTexWrap(int i, TexWrap wrapS, TexWrap wrapT) {
    // Set a texture unit's S-wrap and mark it as dirty
    flushVertices();
    texDirty |= BIT(i);
    switch (wrapS) {
        case WRAP_CLAMP: texWrapS[i] = GL_CLAMP_TO_EDGE; break;
        case WRAP_BORDER: texWrapS[i] = GL_CLAMP_TO_BORDER; break;
        case WRAP_REPEAT: texWrapS[i] = GL_REPEAT; break;
        case WRAP_MIRROR: texWrapS[i] = GL_MIRRORED_REPEAT; break;
    }

    // Set a texture unit's T-wrap
    switch (wrapT) {
        case WRAP_CLAMP: texWrapT[i] = GL_CLAMP_TO_EDGE; return;
        case WRAP_BORDER: texWrapT[i] = GL_CLAMP_TO_BORDER; return;
        case WRAP_REPEAT: texWrapT[i] = GL_REPEAT; return;
        case WRAP_MIRROR: texWrapT[i] = GL_MIRRORED_REPEAT; return;
    }
}

void GpuRenderOgl::setCombSrcs(int i, CombSrc *srcs) {
    // Update a group of texture combiner source uniforms
    flushVertices();
    for (int j = 0; j < 6; j++)
        combSrcs[i * 2 + (j > 2)][j % 3] = srcs[j];
    glUniform3iv(combSrcsLoc + i * 2, 2, combSrcs[i * 2]);
}

void GpuRenderOgl::setCombOpers(int i, CombOper *opers) {
    // Update a group of texture combiner operand uniforms
    flushVertices();
    for (int j = 0; j < 6; j++)
        combOpers[i * 2 + (j > 2)][j % 3] = opers[j];
    glUniform3iv(combOpersLoc + i * 2, 2, combOpers[i * 2]);
}

void GpuRenderOgl::setCombModes(int i, CalcMode *modes) {
    // Update a group of texture combiner mode uniforms
    flushVertices();
    for (int j = 0; j < 2; j++)
        combModes[i][j] = modes[j];
    glUniform2iv(combModesLoc + i, 1, combModes[i]);
}

void GpuRenderOgl::setCombColor(int i, float *color) {
    // Update one of the texture combiner color uniforms
    flushVertices();
    for (int j = 0; j < 4; j++)
        combColors[i][j] = color[j];
    glUniform4fv(combColorsLoc + i, 1, combColors[i]);
}

void GpuRenderOgl::setCombBufColor(float *color) {
    // Update the texture combiner buffer color uniform
    flushVertices();
    for (int i = 0; i < 4; i++)
        combBufColor[i] = color[i];
    glUniform4fv(combBufColorLoc, 1, combBufColor);
}

void GpuRenderOgl::setCombBufMask(uint8_t mask) {
    // Update the texture combiner buffer mask uniform
    flushVertices();
    glUniform1i(combBufMaskLoc, combBufMask = mask);
}

void GpuRenderOgl::setBlendOpers(BlendOper *opers) {
    // Update the source and destination RGB/alpha blend functions
    flushVertices();
    GLenum blendOpers[4];
    for (int i = 0; i < 4; i++) {
        switch (opers[i]) {
            case BLND_ZERO: blendOpers[i] = GL_ZERO; continue;
            case BLND_ONE: blendOpers[i] = GL_ONE; continue;
            case BLND_SRC: blendOpers[i] = GL_SRC_COLOR; continue;
            case BLND_1MSRC: blendOpers[i] = GL_ONE_MINUS_SRC_COLOR; continue;
            case BLND_DST: blendOpers[i] = GL_DST_COLOR; continue;
            case BLND_1MDST: blendOpers[i] = GL_ONE_MINUS_DST_COLOR; continue;
            case BLND_SRCA: blendOpers[i] = GL_SRC_ALPHA; continue;
            case BLND_1MSRCA: blendOpers[i] = GL_ONE_MINUS_SRC_ALPHA; continue;
            case BLND_DSTA: blendOpers[i] = GL_DST_ALPHA; continue;
            case BLND_1MDSTA: blendOpers[i] = GL_ONE_MINUS_DST_ALPHA; continue;
            case BLND_CONST: blendOpers[i] = GL_CONSTANT_COLOR; continue;
            case BLND_1MCON: blendOpers[i] = GL_ONE_MINUS_CONSTANT_COLOR; continue;
            case BLND_CONSTA: blendOpers[i] = GL_CONSTANT_ALPHA; continue;
            case BLND_1MCONA: blendOpers[i] = GL_ONE_MINUS_CONSTANT_ALPHA; continue;
            case BLND_ALPHSAT: blendOpers[i] = GL_SRC_ALPHA_SATURATE; continue;
        }
    }
    glBlendFuncSeparate(blendOpers[0], blendOpers[1], blendOpers[2], blendOpers[3]);
}

void GpuRenderOgl::setBlendModes(CalcMode *modes) {
    // Update the RGB and alpha blend equations
    flushVertices();
    GLenum blendModes[2];
    for (int i = 0; i < 2; i++) {
        switch (modes[i]) {
            default: blendModes[i] = GL_FUNC_ADD; continue;
            case MODE_SUB: blendModes[i] = GL_FUNC_SUBTRACT; continue;
            case MODE_RSUB: blendModes[i] = GL_FUNC_REVERSE_SUBTRACT; continue;
            case MODE_MIN: blendModes[i] = GL_MIN; continue;
            case MODE_MAX: blendModes[i] = GL_MAX; continue;
        }
    }
    glBlendEquationSeparate(blendModes[0], blendModes[1]);
}

void GpuRenderOgl::setBlendColor(float *color) {
    // Update the blend color
    flushVertices();
    glBlendColor(color[0], color[1], color[2], color[3]);
}

void GpuRenderOgl::setAlphaTest(TestFunc func, float value) {
    // Update the alpha test function and value uniforms
    flushVertices();
    glUniform1i(alphaFuncLoc, alphaFunc = func);
    glUniform1f(alphaValueLoc, alphaValue = value);
}

void GpuRenderOgl::setStencilTest(TestFunc func, bool enable) {
    // Update the stencil test function
    flushVertices();
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
    glStencilFunc(stencilFunc, stencilValue, stencilMasks[1]);
}

void GpuRenderOgl::setStencilOps(StenOper fail, StenOper depFail, StenOper depPass) {
    // Update the stencil test conditional operations
    flushVertices();
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
    flushVertices();
    glStencilMask(stencilMasks[0] = bufMask);
    glStencilFunc(stencilFunc, stencilValue, stencilMasks[1] = refMask);
}

void GpuRenderOgl::setStencilValue(uint8_t value) {
    // Update the stencil test reference value
    flushVertices();
    stencilValue = value;
    glStencilFunc(stencilFunc, stencilValue, stencilMasks[1]);
}

void GpuRenderOgl::setLightSpec0(int i, float r, float g, float b) {
    // Update one of the first light specular colors
    flushVertices();
    lightSpec0[i][0] = r, lightSpec0[i][1] = g, lightSpec0[i][2] = b;
    glUniform3fv(lightSpec0Loc + i, 1, lightSpec0[i]);
}

void GpuRenderOgl::setLightSpec1(int i, float r, float g, float b) {
    // Update one of the second light specular colors
    flushVertices();
    lightSpec1[i][0] = r, lightSpec1[i][1] = g, lightSpec1[i][2] = b;
    glUniform3fv(lightSpec1Loc + i, 1, lightSpec1[i]);
}

void GpuRenderOgl::setLightDiff(int i, float r, float g, float b) {
    // Update one of the light diffuse colors
    flushVertices();
    lightDiff[i][0] = r, lightDiff[i][1] = g, lightDiff[i][2] = b;
    glUniform3fv(lightDiffLoc + i, 1, lightDiff[i]);
}

void GpuRenderOgl::setLightAmb(int i, float r, float g, float b) {
    // Update one of the light ambient colors
    flushVertices();
    lightAmb[i][0] = r, lightAmb[i][1] = g, lightAmb[i][2] = b;
    glUniform3fv(lightAmbLoc + i, 1, lightAmb[i]);
}

void GpuRenderOgl::setLightVector(int i, float x, float y, float z) {
    // Update one of the light position/direction vectors
    flushVertices();
    lightVector[i][0] = x, lightVector[i][1] = y, lightVector[i][2] = z;
    glUniform3fv(lightVectorLoc + i, 1, lightVector[i]);
}

void GpuRenderOgl::setLightSpot(int i, float x, float y, float z) {
    // Update one of the spotlight vectors
    flushVertices();
    lightSpot[i][0] = x, lightSpot[i][1] = y, lightSpot[i][2] = z;
    glUniform3fv(lightSpotLoc + i, 1, lightSpot[i]);
}

void GpuRenderOgl::setLightAtten(int i, float bias, float scale) {
    // Update one of the light depth attenuation parameters
    flushVertices();
    lightAtten[i][0] = bias, lightAtten[i][1] = scale;
    glUniform2fv(lightAttenLoc + i, 1, lightAtten[i]);
}

void GpuRenderOgl::setLightType(int i, bool direction) {
    // Update one of the light type values
    flushVertices();
    glUniform1i(lightTypesLoc + i, lightTypes[i] = direction);
}

void GpuRenderOgl::setLightBaseAmb(float r, float g, float b) {
    // Update the light base ambient color
    flushVertices();
    lightBaseAmb[0] = r, lightBaseAmb[1] = g, lightBaseAmb[2] = b;
    glUniform3fv(lightBaseAmbLoc, 1, lightBaseAmb);
}

void GpuRenderOgl::setLightLutVal(LutId id, int i, float entry, float diff) {
    // Get a LUT pointer based on its ID
    float *lut;
    switch (id) {
        case LUT_D0: lut = lutD0; break;
        case LUT_D1: lut = lutD1; break;
        case LUT_FR: lut = lutFr; break;
        case LUT_RB: lut = lutRb; break;
        case LUT_RG: lut = lutRg; break;
        case LUT_RR: lut = lutRr; break;
        case LUT_SP0: lut = lutSp[0]; break;
        case LUT_SP1: lut = lutSp[1]; break;
        case LUT_SP2: lut = lutSp[2]; break;
        case LUT_SP3: lut = lutSp[3]; break;
        case LUT_SP4: lut = lutSp[4]; break;
        case LUT_SP5: lut = lutSp[5]; break;
        case LUT_SP6: lut = lutSp[6]; break;
        case LUT_SP7: lut = lutSp[7]; break;
        case LUT_DA0: lut = lutDa[0]; break;
        case LUT_DA1: lut = lutDa[1]; break;
        case LUT_DA2: lut = lutDa[2]; break;
        case LUT_DA3: lut = lutDa[3]; break;
        case LUT_DA4: lut = lutDa[4]; break;
        case LUT_DA5: lut = lutDa[5]; break;
        case LUT_DA6: lut = lutDa[6]; break;
        case LUT_DA7: lut = lutDa[7]; break;
        default: return;
    }

    // Set a LUT entry and mark its table as dirty
    // TODO: use the difference values
    lut[i] = entry;
    lutDirty |= BIT(id);
}

void GpuRenderOgl::setLightLutMask(uint32_t mask) {
    // Update the light LUT enable mask
    flushVertices();
    glUniform1i(lutMaskLoc, lutMask = mask);
}

void GpuRenderOgl::setLightLutAbs(bool *flags) {
    // Update the light LUT absolute flags
    flushVertices();
    for (int i = 0; i < 7; i++) lutAbsFlags[i] = flags[i];
    glUniform1iv(lutAbsFlagsLoc, 7, lutAbsFlags);
}

void GpuRenderOgl::setLightLutInps(LutInput *inputs) {
    // Update the light LUT input selectors
    flushVertices();
    for (int i = 0; i < 7; i++) lutInputs[i] = inputs[i];
    glUniform1iv(lutInputsLoc, 7, lutInputs);
}

void GpuRenderOgl::setLightLutScls(float *scales) {
    // Update the light LUT scale values
    flushVertices();
    memcpy(lutScales, scales, sizeof(lutScales));
    glUniform1fv(lutScalesLoc, 7, lutScales);
}

void GpuRenderOgl::setLightMap(int8_t *map) {
    // Update the map of enabled light IDs
    flushVertices();
    for (int i = 0; i < 9; i++) lightMap[i] = map[i];
    glUniform1iv(lightMapLoc, 9, lightMap);
}

void GpuRenderOgl::setViewScaleH(float scale) {
    // Update the viewport width
    flushVertices();
    viewWidth = scale * 2;
    updateViewport();
}

void GpuRenderOgl::setViewScaleV(float scale) {
    // Update the viewport height
    flushVertices();
    viewHeight = scale * 2;
    updateViewport();
}

void GpuRenderOgl::setViewOffset(int16_t x, int16_t y) {
    // Update the viewport X/Y offsets
    flushVertices();
    viewX = x, viewY = y;
    updateViewport();
}

void GpuRenderOgl::setBufferDims(uint16_t width, uint16_t height, bool flip) {
    // Set new buffer dimensions and mark them as dirty
    flushBuffers();
    bufWidth = width;
    bufHeight = height;
    readDirty |= BIT(0) | BIT(1);

    // Update the position scale to flip the Y-axis if enabled
    glUniform4f(posScaleLoc, 1.0f, flip ? -1.0f : 1.0f, -1.0f, 1.0f);
    flipY = flip;
}

void GpuRenderOgl::setColbufAddr(uint32_t address) {
    // Set a new color buffer address and mark it as dirty
    flushBuffers();
    colbufAddr = address;
    readDirty |= BIT(0);
}

void GpuRenderOgl::setColbufFmt(ColbufFmt format) {
    // Set a new color buffer format and mark it as dirty
    flushBuffers();
    colbufFmt = format;
    readDirty |= BIT(0);
}

void GpuRenderOgl::setColbufMask(uint8_t mask) {
    // Update the color write mask
    flushVertices();
    glColorMask(bool(mask & BIT(0)), bool(mask & BIT(1)), bool(mask & BIT(2)), bool(mask & BIT(3)));
}

void GpuRenderOgl::setDepbufAddr(uint32_t address) {
    // Set a new depth buffer address and mark it as dirty
    flushBuffers();
    depbufAddr = address;
    readDirty |= BIT(1);
}

void GpuRenderOgl::setDepbufMask(uint8_t mask) {
    // Update the depth write mask
    flushVertices();
    depbufMask = (mask & BIT(1)) ? GL_TRUE : GL_FALSE;
    glDepthMask(depbufMask);
}

void GpuRenderOgl::setDepthFunc(TestFunc func) {
    // Update the depth testing function
    flushVertices();
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
