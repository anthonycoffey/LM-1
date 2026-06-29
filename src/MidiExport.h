#pragma once

#include <JuceHeader.h>
#include "Pattern.h"
#include "DrumKit.h"   // Nixie::kVoiceDefs (lane -> GM note)

//==============================================================================
// Builds a Standard MIDI File from a Pattern: one track, GM drum channel (10),
// each lit step a note-on/off at the right tick, per-step velocity preserved,
// looped to the pattern's bar length. Used by both "Export MIDI" and drag-to-DAW.
//==============================================================================
namespace MidiExport
{
    inline juce::MidiFile build (const Pattern& p, double bpm)
    {
        constexpr int tpqn         = 96;            // ticks per quarter note
        const int     ticksPerStep = tpqn / 4;      // 16th note = 24 ticks
        const double  gateTicks    = ticksPerStep * 0.5;

        juce::MidiMessageSequence track;
        track.addEvent (juce::MidiMessage::tempoMetaEvent (
            (int) std::round (60000000.0 / juce::jmax (1.0, bpm))));   // microseconds / quarter
        track.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4));

        const int numSteps = juce::jlimit (1, Pattern::kMaxSteps, p.numSteps);
        const int numLanes = juce::jlimit (1, Pattern::kMaxLanes, p.numLanes);

        for (int lane = 0; lane < numLanes; ++lane)
        {
            const int note = Nixie::kVoiceDefs[(size_t) lane].midiNote;
            for (int step = 0; step < numSteps; ++step)
            {
                const juce::uint8 v = p.vel[(size_t) lane][(size_t) step];
                if (v == 0)
                    continue;

                const double onTick = (double) step * ticksPerStep;
                track.addEvent (juce::MidiMessage::noteOn  (10, note, v), onTick);
                track.addEvent (juce::MidiMessage::noteOff (10, note, (juce::uint8) 0), onTick + gateTicks);
            }
        }

        track.updateMatchedPairs();
        track.addEvent (juce::MidiMessage::endOfTrack(), (double) numSteps * ticksPerStep);

        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (tpqn);
        mf.addTrack (track);
        return mf;
    }
}
