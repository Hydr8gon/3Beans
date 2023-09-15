/*
    Copyright 2023 Hydr8gon

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

#include <atomic>
#include <functional>
#include <vector>

#include "defines.h"
#include "interpreter.h"
#include "memory.h"

enum Task
{
    RESET_CYCLES,
    END_FRAME,
    INTERRUPT_11,
    INTERRUPT_9,
    MAX_TASKS
};

struct Event
{
    std::function<void()> *task;
    uint32_t cycles;

    Event(std::function<void()> *task, uint32_t cycles): task(task), cycles(cycles) {}
    bool operator<(const Event &event) const { return cycles < event.cycles; }
};

class Core
{
    public:
        Interpreter cpus[2];
        Memory memory;

        std::atomic<bool> running;
        std::vector<Event> events;
        uint32_t globalCycles = 0;

        Core();
        void runFrame() { Interpreter::runFrame(this); }
        void schedule(Task task, uint32_t cycles);

    private:
        std::function<void()> tasks[MAX_TASKS];

        void resetCycles();
        void endFrame();
};
