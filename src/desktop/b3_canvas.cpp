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

#include "b3_canvas.h"
#include "b3_app.h"

#ifdef _WIN32
#include <GL/gl.h>
#include <GL/glext.h>
#endif

wxBEGIN_EVENT_TABLE(b3Canvas, wxGLCanvas)
EVT_PAINT(b3Canvas::draw)
EVT_SIZE(b3Canvas::resize)
EVT_KEY_DOWN(b3Canvas::pressKey)
EVT_KEY_UP(b3Canvas::releaseKey)
EVT_LEFT_DOWN(b3Canvas::pressScreen)
EVT_MOTION(b3Canvas::pressScreen)
EVT_LEFT_UP(b3Canvas::releaseScreen)
wxEND_EVENT_TABLE()

b3Canvas::b3Canvas(b3Frame *frame): wxGLCanvas(frame, wxID_ANY, nullptr), frame(frame) {
    // Prepare the GL context and set focus for key presses
    context = new wxGLContext(this);
    SetFocus();
}

void b3Canvas::finish() {
    // Tell the canvas to stop rendering
    finished = true;
}

void b3Canvas::updateKeyStick() {
    // Apply the base stick movement from pressed keys
    int stickX = 0, stickY = 0;
    if (stickKeys[0]) stickX -= 0x7FF;
    if (stickKeys[1]) stickX += 0x7FF;
    if (stickKeys[2]) stickY += 0x7FF;
    if (stickKeys[3]) stickY -= 0x7FF;

    // Scale diagonals to create a round boundary
    if (stickX && stickY) {
        stickX = stickX * 0x5FF / 0x7FF;
        stickY = stickY * 0x5FF / 0x7FF;
    }

    // Halve coordinates when the modifier is applied
    if (stickKeys[4]) {
        stickX /= 2;
        stickY /= 2;
    }

    // Send stick coordinates to the core
    frame->core->input.setLStick(stickX, stickY);
}

void b3Canvas::draw(wxPaintEvent &event) {
    // Set the GL context if still rendering
    if (finished) return;
    SetCurrent(*context);

    // Run initial setup once
    static bool setup = false;
    if (!setup) {
        // Prepare a texture for the framebuffer
        GLuint texture;
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Finish initial setup
        frame->SendSizeEvent();
        setup = true;
    }

    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // At the swap interval, upload a new framebuffer texture if available
    if (++frameCount >= swapInterval) {
        uint32_t *fb;
        frame->mutex.lock();
        if (frame->core) {
            fb = frame->core->pdc.getFrame();
            frame->mutex.unlock();
        }
        else {
            frame->mutex.unlock();
            fb = new uint32_t[400 * 480];
            memset(fb, 0, 400 * 480 * sizeof(uint32_t));
        }
        if (fb) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
            frameCount = 0;
            delete fb;
        }
    }

    // Submit the polygon vertices
    glBegin(GL_QUADS);
    glTexCoord2i(1, 1);
    glVertex2i(x + width, y + height);
    glTexCoord2i(0, 1);
    glVertex2i(x, y + height);
    glTexCoord2i(0, 0);
    glVertex2i(x, y);
    glTexCoord2i(1, 0);
    glVertex2i(x + width, y);
    glEnd();

    // Track the refresh rate and update the swap interval every second
    // Speed is limited by drawing, so this tries to keep it at 60 Hz
    refreshRate++;
    std::chrono::duration<double> rateTime = std::chrono::steady_clock::now() - lastRateTime;
    if (rateTime.count() >= 1.0f) {
        swapInterval = (refreshRate + 5) / 60; // Margin of 5
        refreshRate = 0;
        lastRateTime = std::chrono::steady_clock::now();
    }

    // Finish the frame
    glFinish();
    SwapBuffers();
}

void b3Canvas::resize(wxSizeEvent &event) {
    // Update the canvas dimensions
    SetCurrent(*context);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    wxSize size = GetSize();
    glOrtho(0, size.x, size.y, 0, -1, 1);
    glViewport(0, 0, size.x, size.y);

    // Set the layout to be centered and as large as possible
    if ((float(size.x) / size.y) > (400.0f / 480)) { // Wide
        width = 400 * size.y / 480;
        height = size.y;
        x = (size.x - width) / 2;
        y = 0;
    }
    else { // Tall
        width = size.x;
        height = 480 * size.x / 400;
        x = 0;
        y = (size.y - height) / 2;
    }
}

void b3Canvas::pressKey(wxKeyEvent &event) {
    // Trigger a key press if a mapped key was pressed
    frame->mutex.lock();
    if (frame->core) {
        for (int i = 0; i < 12; i++)
            if (event.GetKeyCode() == b3App::keyBinds[i])
                frame->core->input.pressKey(i);
        for (int i = 12; i < 17; i++)
            if (event.GetKeyCode() == b3App::keyBinds[i])
                stickKeys[i - 12] = true, updateKeyStick();
    }
    frame->mutex.unlock();
}

void b3Canvas::releaseKey(wxKeyEvent &event) {
    // Trigger a key release if a mapped key was released
    frame->mutex.lock();
    if (frame->core) {
        for (int i = 0; i < 12; i++)
            if (event.GetKeyCode() == b3App::keyBinds[i])
                frame->core->input.releaseKey(i);
        for (int i = 12; i < 17; i++)
            if (event.GetKeyCode() == b3App::keyBinds[i])
                stickKeys[i - 12] = false, updateKeyStick();
    }
    frame->mutex.unlock();
}

void b3Canvas::pressScreen(wxMouseEvent &event) {
    // Calculate coordinates relative to the bottom screen if clicked
    if (!event.LeftIsDown()) return;
    int touchX = (int(event.GetX()) - x) * 400 / width - 40;
    int touchY = (int(event.GetY()) - y) * 480 / height - 240;

    // Send a screen press to the core
    frame->mutex.lock();
    if (frame->core)
        frame->core->input.pressScreen(touchX, touchY);
    frame->mutex.unlock();
}

void b3Canvas::releaseScreen(wxMouseEvent &event) {
    // Send a screen release to the core
    frame->mutex.lock();
    if (frame->core)
        frame->core->input.releaseScreen();
    frame->mutex.unlock();
}
