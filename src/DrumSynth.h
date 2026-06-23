#pragma once

#include <JuceHeader.h>

//==============================================================================
// Procedurally synthesizes simple drum one-shots into mono AudioBuffers.
//
// This is the silent-fail fallback: a voice with no bundled or user-loaded WAV
// still makes sound, so the plugin works with ZERO external asset files. The
// synthesis here is intentionally crude — a fallback, not accurate drum modelling.
// Drop real samples into assets/factory_kit/ or load per-voice at runtime.
//==============================================================================
namespace DrumSynth
{
    inline float whiteNoise (juce::Random& rng) { return rng.nextFloat() * 2.0f - 1.0f; }

    inline juce::AudioBuffer<float> makeKick (double sr)
    {
        const int len = (int) (sr * 0.5);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        for (int i = 0; i < len; ++i)
        {
            const double t       = i / sr;
            const double amp     = std::exp (-t * 16.0);
            const double pitchHz = 48.0 + 110.0 * std::exp (-t * 32.0);
            d[i] = (float) (std::sin (2.0 * juce::MathConstants<double>::pi * pitchHz * t) * amp);
        }
        return b;
    }

    inline juce::AudioBuffer<float> makeSnare (double sr)
    {
        const int len = (int) (sr * 0.25);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        juce::Random rng (1);
        for (int i = 0; i < len; ++i)
        {
            const double t    = i / sr;
            const double amp  = std::exp (-t * 22.0);
            const double tone = std::sin (2.0 * juce::MathConstants<double>::pi * 190.0 * t);
            d[i] = (float) ((0.6 * whiteNoise (rng) + 0.4 * tone) * amp);
        }
        return b;
    }

    inline juce::AudioBuffer<float> makeHat (double sr, double seconds, double decay)
    {
        const int len = (int) (sr * seconds);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        juce::Random rng (2);
        float hp = 0.0f; // simple one-pole high-pass state
        for (int i = 0; i < len; ++i)
        {
            const double t   = i / sr;
            const double amp = std::exp (-t * decay);
            const float  n   = whiteNoise (rng);
            hp = 0.85f * (hp + n);                 // crude high-pass to brighten
            d[i] = (float) (hp * 0.5 * amp);
        }
        return b;
    }

    inline juce::AudioBuffer<float> makeClap (double sr)
    {
        const int len = (int) (sr * 0.3);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        juce::Random rng (3);
        for (int i = 0; i < len; ++i)
        {
            const double t = i / sr;
            // three quick bursts then a tail
            double env = std::exp (-t * 50.0)
                       + std::exp (-std::abs (t - 0.01) * 60.0)
                       + std::exp (-std::abs (t - 0.02) * 60.0)
                       + 0.6 * std::exp (-t * 12.0);
            d[i] = (float) (whiteNoise (rng) * juce::jmin (1.0, env) * 0.5);
        }
        return b;
    }

    inline juce::AudioBuffer<float> makeTom (double sr, double pitchHz)
    {
        const int len = (int) (sr * 0.35);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        for (int i = 0; i < len; ++i)
        {
            const double t   = i / sr;
            const double amp = std::exp (-t * 12.0);
            const double f   = pitchHz * (1.0 + 0.4 * std::exp (-t * 25.0));
            d[i] = (float) (std::sin (2.0 * juce::MathConstants<double>::pi * f * t) * amp);
        }
        return b;
    }

    inline juce::AudioBuffer<float> makeCowbell (double sr)
    {
        const int len = (int) (sr * 0.3);
        juce::AudioBuffer<float> b (1, len);
        auto* d = b.getWritePointer (0);
        for (int i = 0; i < len; ++i)
        {
            const double t   = i / sr;
            const double amp = std::exp (-t * 9.0);
            const double a   = std::sin (2.0 * juce::MathConstants<double>::pi * 540.0 * t);
            const double bb  = std::sin (2.0 * juce::MathConstants<double>::pi * 800.0 * t);
            d[i] = (float) ((a + bb) * 0.5 * amp);
        }
        return b;
    }
}
