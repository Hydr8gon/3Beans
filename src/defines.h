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

#include <cstdio>

enum CpuId {
    ARM11A,
    ARM11B,
    ARM11C,
    ARM11D,
    ARM9,
    MAX_CPUS,
    ARM11 = ARM11A
};

// Simple bit macro
#define BIT(i) (1 << (i))

// Macro to swap two values
#define SWAP(a, b) { auto c = a; a = b; b = c; }

// Macros to perform byteswaps
#define BSWAP32(v) __builtin_bswap32(v)
#define BSWAP64(v) __builtin_bswap64(v)

// Macros to perform bit rotations
#define ROL8(v, s) ((v << s) | (v >> (8 - s)))
#define ROL32(v, s) ((v << s) | (v >> (32 - s)))
#define ROR32(v, s) ((v >> s) | (v << (32 - s)))

// Macro to handle differing mkdir arguments on Windows
#ifdef WINDOWS
#define MKDIR_ARGS
#else
#define MKDIR_ARGS , 0777
#endif

// Macro to force inlining
#define FORCE_INLINE inline __attribute__((always_inline))

// If enabled, print critical logs in red
#if LOG_LEVEL > 0
#define LOG_CRIT(...) printf("\x1b[31m" __VA_ARGS__)
#else
#define LOG_CRIT(...) (0)
#endif

// If enabled, print warning logs in yellow
#if LOG_LEVEL > 1
#define LOG_WARN(...) printf("\x1b[33m" __VA_ARGS__)
#else
#define LOG_WARN(...) (0)
#endif

// If enabled, print info logs normally
#if LOG_LEVEL > 2
#define LOG_INFO(...) printf("\x1b[0m" __VA_ARGS__)
#else
#define LOG_INFO(...) (0)
#endif

// If enabled, print OS logs in green
#if LOG_LEVEL > 3
#define LOG_OS(...) printf("\x1b[32m" __VA_ARGS__)
#else
#define LOG_OS(...) (0)
#endif
