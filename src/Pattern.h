#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstring>

//==============================================================================
// A step pattern: 12 lanes x up to 32 steps. Each cell stores a velocity, where
// 0 means "step off" and 1..127 means "on at that velocity". Fixed-size and
// trivially copyable, which is what makes the lock-free hand-off to the audio
// thread cheap (see the double-buffer in PluginProcessor).
//==============================================================================
struct Pattern
{
    static constexpr int kMaxLanes = 16;   // 13 in use (12 instruments + open hat) + headroom
    static constexpr int kMaxSteps = 32;

    int   numSteps = 16;     // active length (8/16/32)
    int   numLanes = 13;
    float swing    = 0.0f;   // 0..1, delays odd 16ths (de-prioritised; defaults straight)

    std::array<std::array<juce::uint8, kMaxSteps>, kMaxLanes> vel {};

    bool isOn (int lane, int step) const noexcept
    {
        return vel[(size_t) lane][(size_t) step] > 0;
    }

    void setStep (int lane, int step, juce::uint8 velocity) noexcept
    {
        vel[(size_t) lane][(size_t) step] = velocity;
    }

    void clear() noexcept
    {
        for (auto& row : vel)
            row.fill (0);
    }

    //==========================================================================
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree t ("PATTERN");
        t.setProperty ("numSteps", numSteps, nullptr);
        t.setProperty ("numLanes", numLanes, nullptr);
        t.setProperty ("swing",    swing,    nullptr);

        juce::MemoryBlock mb (vel.data(), sizeof (vel));
        t.setProperty ("grid", mb.toBase64Encoding(), nullptr);
        return t;
    }

    void fromValueTree (const juce::ValueTree& t)
    {
        if (! t.hasType ("PATTERN"))
            return;

        numSteps = juce::jlimit (1, kMaxSteps, (int) t.getProperty ("numSteps", 16));
        numLanes = juce::jlimit (1, kMaxLanes, (int) t.getProperty ("numLanes", 12));
        swing    = (float) t.getProperty ("swing", 0.0);

        juce::MemoryBlock mb;
        if (mb.fromBase64Encoding (t.getProperty ("grid", "").toString())
            && mb.getSize() == sizeof (vel))
            std::memcpy (vel.data(), mb.getData(), sizeof (vel));
    }
};
