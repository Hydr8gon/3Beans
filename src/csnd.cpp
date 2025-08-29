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

#include <cstring>
#include "core.h"

const int8_t Csnd::indexTable[] = { -1, -1, -1, -1, 2, 4, 6, 8 };

const int16_t Csnd::adpcmTable[] = {
    0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010, 0x0011, 0x0013, 0x0015,
    0x0017, 0x0019, 0x001C, 0x001F, 0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042,
    0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F, 0x009D, 0x00AD, 0x00BE, 0x00D1,
    0x00E6, 0x00FD, 0x0117, 0x0133, 0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292,
    0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583, 0x0610, 0x06AB, 0x0756, 0x0812,
    0x08E0, 0x09C3, 0x0ABD, 0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
    0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B, 0x3BB9, 0x41B2, 0x4844, 0x4F7E,
    0x5771, 0x602F, 0x69CE, 0x7462, 0x7FFF
};

Csnd::Csnd(Core *core): core(core) {
    // Mark the buffer as not ready
    ready.store(false);
}

Csnd::~Csnd() {
    // Free the audio buffers
    delete[] mixBuffer;
    for (int i = 0; i < 2; i++)
        delete[] csndBuffer[i];
}

uint32_t *Csnd::getSamples(uint32_t freq, uint32_t count) {
    // Check if parameters changed and update the buffer details if so
    dspSize = count * ((dspClock == CLK_32KHZ) ? 32730 : 47610) / freq;
    if (mixFreq != freq || mixSize != count) {
        mixFreq = freq, mixSize = count;
        csndSize = count * 130920 / freq;
        csndOfs = 0;

        // Initialize or resize the buffers themselves
        delete[] mixBuffer;
        mixBuffer = new uint32_t[mixSize];
        for (int i = 0; i < 2; i++) {
            delete[] csndBuffer[i];
            csndBuffer[i] = new uint32_t[csndSize];
        }
    }

    // Try to wait for buffer data, but don't starve if it takes too long
    bool wait; {
        std::unique_lock<std::mutex> lock(mutexes[1]);
        wait = !condVars[1].wait_for(lock, std::chrono::microseconds(1000000 / 60), [&]{ return ready.load(); });
    }

    // Fill the output buffer depending on if it's still waiting
    if (wait) {
        // Repeat the last sample to prevent crackles when running slow
        for (int i = 0; i < count; i++)
            mixBuffer[i] = lastSample[0];
    }
    else {
        // Resample and add CSND and DSP left/right samples together
        mutexes[2].lock();
        for (int i = 0; i < count; i++) {
            int j = i * dspSize / count;
            uint32_t csnd = csndBuffer[1][i * csndSize / count];
            uint32_t dsp = (j < dspBuffer.size()) ? dspBuffer[j] : lastSample[1];
            mixBuffer[i] = (((csnd >> 16) + (dsp >> 16)) << 16) | ((csnd + dsp) & 0xFFFF);
        }

        // Save the last sample and dequeue used DSP samples
        lastSample[0] = mixBuffer[count - 1];
        int j = std::min<int>(dspSize, dspBuffer.size());
        if (j > 0) {
            lastSample[1] = dspBuffer[j - 1];
            dspBuffer.erase(dspBuffer.begin(), dspBuffer.begin() + j);
        }
        mutexes[2].unlock();
    }

    { // Signal that the buffer was processed
        std::lock_guard<std::mutex> guard(mutexes[0]);
        ready.store(false);
        condVars[0].notify_one();
    }
    return mixBuffer;
}

void Csnd::runSample() {
    // Push a dummy sample if disabled and schedule the next one
    core->schedule(CSND_SAMPLE, 2048);
    if (csndMainCnt & BIT(0)) return sampleCsnd(0, 0);

    // Mix enabled sound channels in stereo
    int32_t sampleLeft = 0, sampleRight = 0;
    for (int i = 0; enabled >> i; i++) {
        // Skip disabled channels
        if (~enabled & BIT(i)) continue;
        uint8_t fmt = (csndChanCnt[i] >> 12) & 0x3;
        int16_t data = 0;

        // Get sample data based on the channel format
        switch (fmt) {
            case 0: data = core->memory.read<uint8_t>(ARM11, chanCurrent[i]) << 8; break; // PCM8
            case 1: data = core->memory.read<uint16_t>(ARM11, chanCurrent[i]); break; // PCM16
            case 2: data = adpcmSamples[i]; break; // ADPCM

        case 3: // Pulse/Noise
            if (i >= 8 && i <= 13) { // Pulse
                // Set the sample to low or high depending on its duty cycle position
                uint8_t duty = 7 - (csndChanCnt[i] & 0x7);
                data = ((dutyCycles[i - 8] >> 2) < duty) ? -0x7FFF : 0x7FFF;
            }
            else if (i >= 14 && i <= 15) { // Noise
                // Set the sample to low or high depending on its carry bit
                data = (noiseValues[i - 14] & BIT(15)) ? -0x7FFF : 0x7FFF;
            }
            break;
        }

        // Advance the channel timer
        chanTimers[i] += 512;
        bool overflow = (chanTimers[i] < 512);

        // Handle timer overflows
        while (overflow) {
            // Reload the timer
            chanTimers[i] += csndChanRate[i];
            overflow = (chanTimers[i] < csndChanRate[i]);

            // Handle format-specific channel updates
            switch (fmt) {
            case 0: case 1: // PCM8/PCM16
                // Increment the data pointer by the size of a sample
                chanCurrent[i] += fmt + 1;
                break;

            case 2: { // ADPCM
                // Get the next 4-bit ADPCM data value
                uint8_t value = core->memory.read<uint8_t>(ARM11, chanCurrent[i]);
                value = (value >> ((adpcmToggle & BIT(i)) ? 4 : 0)) & 0xF;

                // Increment the data pointer every other 4-bit value
                adpcmToggle ^= BIT(i);
                chanCurrent[i] += !(adpcmToggle & BIT(i));

                // Calculate the sample difference
                int32_t diff = adpcmTable[adpcmIndices[i]] >> 3;
                if (value & BIT(0)) diff += adpcmTable[adpcmIndices[i]] >> 2;
                if (value & BIT(1)) diff += adpcmTable[adpcmIndices[i]] >> 1;
                if (value & BIT(2)) diff += adpcmTable[adpcmIndices[i]] >> 0;

                // Apply the sample difference and adjust the index
                if (value & BIT(3)) // Positive
                    adpcmSamples[i] = std::min(0x7FFF, adpcmSamples[i] + diff);
                else // Negative
                    adpcmSamples[i] = std::max(-0x8000, adpcmSamples[i] - diff);
                adpcmIndices[i] = std::max(0, std::min(88, adpcmIndices[i] + indexTable[value & 0x7]));
                break;
            }

            case 3: // Pulse/Noise
                if (i >= 8 && i <= 13) { // Pulse
                    // Increment the duty cycle counter
                    dutyCycles[i - 8] = (dutyCycles[i - 8] + 1) & 0x1F;
                }
                else if (i >= 14 && i <= 15) { // Noise
                    // Advance the noise generator with bit 15 as carry
                    uint16_t &value = noiseValues[i - 14];
                    value = (((value & ~BIT(15)) >> 1) ^ ((value & BIT(0)) ? 0xE000 : 0));
                }
                break;
            }

            // Handle reaching the end of channel data
            if (fmt != 3 && chanCurrent[i] >= csndChanStart[i] + csndChanSize[i]) {
                switch ((csndChanCnt[i] >> 10) & 0x3) {
                case 1: case 3: // Loop infinite
                    // Reload the channel, including ADPCM state if enabled
                    chanCurrent[i] = csndChanLoop[i];
                    if (fmt != 2 || !(csndAdpcmLoop[i] & BIT(31))) break;
                    adpcmSamples[i] = (csndAdpcmLoop[i] & 0xFFFF);
                    adpcmIndices[i] = std::min<uint8_t>(88, (csndAdpcmLoop[i] >> 16) & 0xFF);
                    adpcmToggle &= ~BIT(i);
                    break;

                case 2: // One-shot
                    // End the channel and break out of overflow
                    csndChanCnt[i] &= ~BIT(15);
                    enabled &= ~BIT(i);
                    overflow = false;
                    break;
                }
            }
        }

        // Apply left and right volumes for the final output
        sampleLeft += (data * csndChanLvol[i]) >> 15;
        sampleRight += (data * csndChanRvol[i]) >> 15;
    }

    // Apply master volume and clip to signed 16-bit range for the sample buffer
    sampleLeft = std::max(-0x8000, std::min(0x7FFF, ((sampleLeft * csndMainVol) >> 15)));
    sampleRight = std::max(-0x8000, std::min(0x7FFF, ((sampleRight * csndMainVol) >> 15)));
    sampleCsnd(sampleLeft, sampleRight);
}

void Csnd::sampleCsnd(int16_t left, int16_t right) {
    // Write samples to the buffer and check if full
    if (!csndSize) return;
    csndBuffer[0][csndOfs++] = (right << 16) | (left & 0xFFFF);
    if (csndOfs != csndSize) return;

    // Limit FPS to 60 if enabled by waiting for the previous buffer to play
    if (Settings::fpsLimiter) {
        std::unique_lock<std::mutex> lock(mutexes[0]);
        condVars[0].wait_for(lock, std::chrono::microseconds(1000000), [&]{ return !ready.load(); });
    }

    // Swap buffers and reset the pointer
    std::swap(csndBuffer[0], csndBuffer[1]);
    csndOfs = 0;

    { // Signal that a buffer is ready to play
        std::lock_guard<std::mutex> guard(mutexes[1]);
        ready.store(true);
        condVars[1].notify_one();
    }
}

void Csnd::sampleDsp(int16_t left, int16_t right) {
    // Enqueue a volume-scaled DSP sample for mixing, up to twice the size of an output frame
    mutexes[2].lock();
    if (dspBuffer.size() < dspSize * 2) {
        int vol = std::min<int>(0x20, codecSndexcnt & 0x3F);
        dspBuffer.push_back((((right * vol) >> 5) << 16) | (((left * vol) >> 5) & 0xFFFF));
    }
    mutexes[2].unlock();
}

void Csnd::startChannel(int i) {
    // Reload and enable a sound channel
    chanCurrent[i] = csndChanStart[i];
    chanTimers[i] = csndChanRate[i];
    enabled |= BIT(i);

    // Handle special reloads depending on the format
    switch ((csndChanCnt[i] >> 12) & 0x3) { // Format
    case 2: // ADPCM
        adpcmSamples[i] = (csndAdpcmStart[i] & 0xFFFF);
        adpcmIndices[i] = std::min<uint8_t>(88, (csndAdpcmStart[i] >> 16) & 0xFF);
        adpcmToggle &= ~BIT(i);
        break;

    case 3: // Pulse/Noise
        if (i >= 8 && i <= 13) // Pulse
            dutyCycles[i - 8] = 0;
        else if (i >= 14) // Noise
            noiseValues[i - 14] = 0x7FFF;
        break;
    }
}

void Csnd::writeMainVol(uint16_t mask, uint16_t value) {
    // Write to the CSND_MAIN_VOL register, clamping the value
    csndMainVol = std::min<uint16_t>(0x8000, (csndMainVol & ~mask) | (value & mask));
}

void Csnd::writeMainCnt(uint16_t mask, uint16_t value) {
    // Write to the CSND_MAIN_CNT register
    // TODO: handle bits 30-31
    mask &= 0xC001;
    csndMainCnt = (csndMainCnt & ~mask) | (value & mask);
}

void Csnd::writeChanCnt(int i, uint16_t mask, uint16_t value) {
    // Write to one of the CSND_CHANx_CNT registers
    // TODO: handle bits 6-7 and 14
    mask &= 0xFCC7;
    bool enable = (~csndChanCnt[i] & value & mask & BIT(15));
    csndChanCnt[i] = (csndChanCnt[i] & ~mask) | (value & mask);

    // Start a channel on enable rising edge, or stop if cleared
    if (enable)
        return startChannel(i);
    else if (~csndChanCnt[i] & BIT(15))
        enabled &= ~BIT(i);
}

void Csnd::writeChanRate(int i, uint16_t mask, uint16_t value) {
    // Write to one of the CSND_CHANx_RATE registers
    csndChanRate[i] = (csndChanRate[i] & ~mask) | (value & mask);
}

void Csnd::writeChanRvol(int i, uint16_t mask, uint16_t value) {
    // Write to one of the CSND_CHANx_RVOL registers, clamping the value
    csndChanRvol[i] = std::min<uint16_t>(0x8000, (csndChanRvol[i] & ~mask) | (value & mask));
}

void Csnd::writeChanLvol(int i, uint16_t mask, uint16_t value) {
    // Write to one of the CSND_CHANx_LVOL registers, clamping the value
    csndChanLvol[i] = std::min<uint16_t>(0x8000, (csndChanLvol[i] & ~mask) | (value & mask));
}

void Csnd::writeChanStart(int i, uint32_t mask, uint32_t value) {
    // Write to one of the CSND_CHANx_START registers
    csndChanStart[i] = (csndChanStart[i] & ~mask) | (value & mask);
}

void Csnd::writeChanSize(int i, uint32_t mask, uint32_t value) {
    // Write to one of the CSND_CHANx_SIZE registers
    mask &= 0x7FFFFFF;
    csndChanSize[i] = (csndChanSize[i] & ~mask) | (value & mask);
}

void Csnd::writeChanLoop(int i, uint32_t mask, uint32_t value) {
    // Write to one of the CSND_CHANx_LOOP registers
    csndChanLoop[i] = (csndChanLoop[i] & ~mask) | (value & mask);
}

void Csnd::writeAdpcmStart(int i, uint32_t mask, uint32_t value) {
    // Write to one of the CSND_ADPCMx_START registers
    mask &= 0xFFFFFF;
    csndAdpcmStart[i] = (csndAdpcmStart[i] & ~mask) | (value & mask);
}

void Csnd::writeAdpcmLoop(int i, uint32_t mask, uint32_t value) {
    // Write to one of the CSND_ADPCMx_LOOP registers
    mask &= 0x80FFFFFF;
    csndAdpcmLoop[i] = (csndAdpcmLoop[i] & ~mask) | (value & mask);
}

void Csnd::writeSndexcnt(uint32_t mask, uint32_t value) {
    // Write to the CODEC_SNDEXCNT register with a mask based on the clock enable bit
    mask &= (codecSndexcnt & BIT(31)) ? 0x80008FFF : 0xE000FFFF;
    codecSndexcnt = (codecSndexcnt & ~mask) | (value & mask);

    // Disable or set the DSP clock speed based on register bits
    if ((codecSndexcnt & 0x80008000) != 0x80008000) // Clock/timing enable
        dspClock = CLK_OFF;
    else if (codecSndexcnt & BIT(13)) // Frequency
        dspClock = CLK_47KHZ;
    else
        dspClock = CLK_32KHZ;
    core->dsp.setAudClock(dspClock);
}
