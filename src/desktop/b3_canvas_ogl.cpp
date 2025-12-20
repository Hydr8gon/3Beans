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

#include <epoxy/gl.h>
#include "b3_canvas_ogl.h"
#include "b3_app.h"

wxBEGIN_EVENT_TABLE(b3CanvasOgl, wxGLCanvas)
EVT_PAINT(b3CanvasOgl::draw)
EVT_SIZE(b3CanvasOgl::resize)
EVT_KEY_DOWN(b3CanvasOgl::pressKey)
EVT_KEY_UP(b3CanvasOgl::releaseKey)
EVT_LEFT_DOWN(b3CanvasOgl::pressScreen)
EVT_MOTION(b3CanvasOgl::pressScreen)
EVT_LEFT_UP(b3CanvasOgl::releaseScreen)
wxEND_EVENT_TABLE()

const char *b3CanvasOgl::vtxCode = R"(
    #version 330

    in vec2 inPosition;
    in vec2 inTexCoord;
    out vec2 vtxTexCoord;
    uniform vec2 winSize;

    void main() {
        gl_Position = vec4(inPosition.x / winSize.x * 2 - 1, inPosition.y / winSize.y * -2 + 1, 0, 1);
        vtxTexCoord = inTexCoord;
    }
)";

const char *b3CanvasOgl::fragCode = R"(
    #version 330

    in vec2 vtxTexCoord;
    out vec4 fragColor;
    uniform sampler2D texUnit;

    void main() {
        fragColor = texture(texUnit, vtxTexCoord);
    }
)";

b3CanvasOgl::b3CanvasOgl(b3Frame *frame): wxGLCanvas(frame), frame(frame) {
    // Create a canvas for the core and bind the context function
    coreCanvas = new wxGLCanvas(frame);
    coreCanvas->SetSize(0, 0);
    contextFunc = std::bind(&b3CanvasOgl::coreContext, this);

    // Prepare two OpenGL 3.3 contexts if they're supported
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
    for (int i = 0; i < 2; i++) {
        contexts[i] = new wxGLContext(i ? coreCanvas : this, nullptr, &ctxAttrs);
        if (!contexts[i]->IsOK()) throw OPENGL_FAIL;
    }

    // Set focus for key presses
    SetFocus();
}

void b3CanvasOgl::coreContext() {
    // Toggle the core's context on a thread
    if (toggle = !toggle)
        coreCanvas->SetCurrent(*contexts[1]);
    else
        wxGLContext::ClearCurrent();
}

void b3CanvasOgl::glInit() {
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

    // Set up vertex array and buffer objects
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLint loc = glGetAttribLocation(program, "inPosition");
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(CanvasVtx), (void*)offsetof(CanvasVtx, x));
    glEnableVertexAttribArray(loc);
    loc = glGetAttribLocation(program, "inTexCoord");
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(CanvasVtx), (void*)offsetof(CanvasVtx, s));
    glEnableVertexAttribArray(loc);

    // Prepare a texture for the framebuffer
    GLuint texture;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Trigger an initial resize to set the window size
    winSizeLoc = glGetUniformLocation(program, "winSize");
    frame->SendSizeEvent();
}

void b3CanvasOgl::draw(wxPaintEvent &event) {
    // Initialize GL here to ensure the canvas is ready
    SetCurrent(*contexts[0]);
    if (!inited) {
        glInit();
        inited = true;
    }

    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Upload a new framebuffer texture if one is available
    if (uint32_t *fb = frame->getFrame()) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
        delete[] fb;
    }

    // Define vertices for the screen
    CanvasVtx vertices[] = {
        { scrX + scrW, scrY + scrH, 1.0f, 1.0f },
        { scrX, scrY + scrH, 0.0f, 1.0f },
        { scrX, scrY, 0.0f, 0.0f },
        { scrX + scrW, scrY, 1.0f, 0.0f }
    };

    // Submit vertices and finish the frame
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glFinish();
    SwapBuffers();
}

void b3CanvasOgl::resize(wxSizeEvent &event) {
    // Update the canvas dimensions
    SetCurrent(*contexts[0]);
    wxSize size = GetSize();
    glViewport(0, 0, size.x, size.y);
    glUniform2f(winSizeLoc, size.x, size.y);

    // Set the layout to be centered and as large as possible
    if ((float(size.x) / size.y) > (400.0f / 480)) { // Wide
        scrW = (400.0f * size.y) / 480;
        scrH = size.y;
        scrX = (size.x - scrW) / 2;
        scrY = 0;
    }
    else { // Tall
        scrW = size.x;
        scrH = 480 * size.x / 400;
        scrX = 0;
        scrY = (size.y - scrH) / 2;
    }
}

void b3CanvasOgl::pressKey(wxKeyEvent &event) {
    // Trigger a key press if a mapped key was pressed
    for (int i = 0; i < MAX_KEYS; i++)
        if (event.GetKeyCode() == b3App::keyBinds[i])
            return frame->pressKey(i);
}

void b3CanvasOgl::releaseKey(wxKeyEvent &event) {
    // Trigger a key release if a mapped key was released
    for (int i = 0; i < MAX_KEYS; i++)
        if (event.GetKeyCode() == b3App::keyBinds[i])
            return frame->releaseKey(i);
}

void b3CanvasOgl::pressScreen(wxMouseEvent &event) {
    // Trigger a screen touch relative to the bottom screen if clicked
    if (!event.LeftIsDown()) return;
    int x = (event.GetX() - scrX) * 400 / scrW - 40;
    int y = (event.GetY() - scrY) * 480 / scrH - 240;
    frame->pressScreen(x, y);
}

void b3CanvasOgl::releaseScreen(wxMouseEvent &event) {
    // Trigger a screen release
    frame->releaseScreen();
}
