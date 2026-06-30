#pragma once

#include <JuceHeader.h>
#include "DrumKit.h"

//==============================================================================
// A single drum voice: plays one mono sample buffer back at a variable rate
// (for tuning) with 4-point Catmull-Rom interpolation, equal-power pan and
// per-hit gain.
//
// The sample data lives in a DrumKit. The voice holds a DrumKit::Ptr so a note
// in flight keeps its kit alive even if the user swaps kits mid-sound. Copying
// the Ptr is a lock-free atomic refcount bump, so assign() is audio-thread safe;
// the kit is never *deleted* on the audio thread because the processor keeps a
// reference (currentKit / retiredKits) until a message-thread reap.
//
// Real-time safe: no allocation, no locks, no file I/O in render()/trigger().
//==============================================================================
class DrumVoice
{
public:
    DrumVoice() = default;

    void prepare (double hostSampleRate)
    {
        outputSampleRate = hostSampleRate;
    }

    // Point the voice at voice `voiceIndex` of `kit`, picking up its buffer,
    // source rate and trim. Call immediately before trigger().
    void assign (DrumKit* kit, int voiceIndex)
    {
        heldKit = kit;                                  // refcount bump (atomic, lock-free)

        if (kit == nullptr)
        {
            sample = nullptr;
            trimStart = trimEnd = 0;
            return;
        }

        const auto& vs = kit->voice (voiceIndex);
        sample           = &vs.buffer;
        sourceSampleRate = vs.sourceSampleRate;

        const int n = vs.buffer.getNumSamples();
        trimStart = juce::jlimit (0, juce::jmax (0, n - 1), vs.startSample);
        const int end = (vs.endSample >= 0 ? vs.endSample : n);
        trimEnd = juce::jlimit (trimStart, n, end);
    }

    // Start (or restart) the voice. tuneSemitones shifts pitch by resampling.
    void trigger (float velocity, float tuneSemitones)
    {
        if (sample == nullptr || sample->getNumSamples() < 2)
        {
            active = false;
            return;
        }

        position     = (double) trimStart;
        velocityGain = juce::jlimit (0.0f, 1.0f, velocity);

        // Base rate corrects for any sample-rate mismatch, then tuning is applied.
        const double tuneRatio = std::pow (2.0, tuneSemitones / 12.0);
        rate = (sourceSampleRate / outputSampleRate) * tuneRatio;
        active = true;
    }

    void stop() { active = false; }
    bool isActive() const noexcept { return active; }

    // Adds this voice's output into the buffer between [startSample, startSample+numSamples).
    void render (juce::AudioBuffer<float>& out, int startSample, int numSamples,
                 float level, float pan)
    {
        if (! active || sample == nullptr)
            return;

        const int    len    = sample->getNumSamples();
        const int    srcEnd = juce::jmin (trimEnd, len);
        const float* src    = sample->getReadPointer (0);

        // Equal-power pan: pan in [-1, 1]. On a mono bus pan is meaningless, so write
        // the voice at unity to the single channel (avoids the +3 dB that summing an
        // equal-power pan into one channel would add).
        const bool  mono      = out.getNumChannels() < 2;
        const float angle     = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
        const float leftGain  = mono ? level * velocityGain : level * velocityGain * std::cos (angle);
        const float rightGain = mono ? 0.0f                 : level * velocityGain * std::sin (angle);

        float* outL = out.getWritePointer (0, startSample);
        float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1, startSample) : outL;

        // 4-point Catmull-Rom (cubic Hermite). Taps are clamped to the buffer so a
        // sample's edges never read out of bounds; cleaner than linear when a voice is
        // tuned/resampled (less high-frequency aliasing and zipper on the transient).
        auto tap = [src, len] (int k) noexcept { return src[juce::jlimit (0, len - 1, k)]; };

        for (int i = 0; i < numSamples; ++i)
        {
            const int idx = (int) position;
            if (idx >= srcEnd - 1)
            {
                active = false;
                break;
            }

            const float frac = (float) (position - idx);
            const float x0 = tap (idx - 1), x1 = tap (idx), x2 = tap (idx + 1), x3 = tap (idx + 2);
            const float c1 = 0.5f * (x2 - x0);
            const float c2 = x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
            const float c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
            const float s  = ((c3 * frac + c2) * frac + c1) * frac + x1;

            outL[i] += s * leftGain;
            outR[i] += s * rightGain;

            position += rate;
        }
    }

private:
    DrumKit::Ptr heldKit;                               // keeps the kit alive while playing
    const juce::AudioBuffer<float>* sample = nullptr;   // raw view into heldKit's buffer
    double sourceSampleRate = 44100.0;
    double outputSampleRate = 44100.0;
    double position = 0.0;
    double rate = 1.0;
    int    trimStart = 0;
    int    trimEnd = 0;
    float  velocityGain = 1.0f;
    bool   active = false;
};
