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

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>

class Core;

enum DspClock {
    CLK_OFF,
    CLK_32KHZ,
    CLK_47KHZ
};

class Csnd {
public:
    Csnd(Core *core);
    ~Csnd();

    uint32_t *getSamples(uint32_t freq, uint32_t count);
    void runSample();
    void sampleDsp(int16_t left, int16_t right);

    uint16_t readMainVol() { return csndMainVol; }
    uint16_t readMainCnt() { return csndMainCnt; }
    uint16_t readChanCnt(int i) { return csndChanCnt[i]; }
    uint16_t readChanRvol(int i) { return csndChanRvol[i]; }
    uint16_t readChanLvol(int i) { return csndChanLvol[i]; }
    uint32_t readChanStart(int i) { return csndChanStart[i] & ~0x3; }
    uint16_t readSndexcnt() { return codecSndexcnt; }

    void writeMainVol(uint16_t mask, uint16_t value);
    void writeMainCnt(uint16_t mask, uint16_t value);
    void writeChanCnt(int i, uint16_t mask, uint16_t value);
    void writeChanRate(int i, uint16_t mask, uint16_t value);
    void writeChanRvol(int i, uint16_t mask, uint16_t value);
    void writeChanLvol(int i, uint16_t mask, uint16_t value);
    void writeChanStart(int i, uint32_t mask, uint32_t value);
    void writeChanSize(int i, uint32_t mask, uint32_t value);
    void writeChanLoop(int i, uint32_t mask, uint32_t value);
    void writeAdpcmStart(int i, uint32_t mask, uint32_t value);
    void writeAdpcmLoop(int i, uint32_t mask, uint32_t value);
    void writeSndexcnt(uint32_t mask, uint32_t value);

private:
    Core *core;
    std::condition_variable condVars[2];
    std::mutex mutexes[3];
    std::atomic<bool> ready;

    uint32_t *mixBuffer = nullptr;
    uint32_t *csndBuffer[2] = {};
    std::deque<uint32_t> dspBuffer;
    uint32_t lastSample[2] = {};
    uint32_t mixFreq = 0, mixSize = 0;
    uint32_t csndSize = 0, csndOfs = 0;
    uint32_t dspSize = 0;
    DspClock dspClock = CLK_OFF;

    static const int8_t indexTable[8];
    static const int16_t adpcmTable[89];

    int8_t adpcmIndices[32] = {};
    int16_t adpcmSamples[32] = {};
    uint32_t adpcmToggle = 0;

    uint32_t enabled = 0;
    uint8_t dutyCycles[6] = {};
    uint16_t noiseValues[2] = {};
    uint32_t chanCurrent[32] = {};
    uint16_t chanTimers[32] = {};

    uint16_t csndMainVol = 0;
    uint16_t csndMainCnt = 0x1;
    uint16_t csndChanCnt[32] = {};
    uint16_t csndChanRate[32] = {};
    uint16_t csndChanRvol[32] = {};
    uint16_t csndChanLvol[32] = {};
    uint32_t csndChanStart[32] = {};
    uint32_t csndChanSize[32] = {};
    uint32_t csndChanLoop[32] = {};
    uint32_t csndAdpcmStart[32] = {};
    uint32_t csndAdpcmLoop[32] = {};
    uint32_t codecSndexcnt = 0;

    void sampleCsnd(int16_t left, int16_t right);
    void startChannel(int i);
};
