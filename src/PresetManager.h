#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Saves/loads the full plugin state (params + kit + pattern bank) as
// `.lm1preset` XML files under the user's app-data dir. Message-thread only.
//
// Sample references are stored by path (factory voices by name), so a preset
// that points at a moved/missing user WAV falls back to the default for that
// voice — see ROADMAP Stage F ("portable preset" embedding is a later add).
//==============================================================================
class PresetManager
{
public:
    explicit PresetManager (LMOneAudioProcessor& p) : processor (p) {}

    static juce::File getPresetDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("LM-1").getChildFile ("Presets");
        dir.createDirectory();
        return dir;
    }

    bool save (const juce::File& file) const
    {
        const auto f = file.withFileExtension ("lm1preset");
        if (auto xml = processor.captureStateTree().createXml())
            return xml->writeTo (f);
        return false;
    }

    bool load (const juce::File& file) const
    {
        if (auto xml = juce::XmlDocument::parse (file))
        {
            processor.restoreStateTree (juce::ValueTree::fromXml (*xml));
            return true;
        }
        return false;
    }

    juce::Array<juce::File> list() const
    {
        return getPresetDir().findChildFiles (juce::File::findFiles, false, "*.lm1preset");
    }

private:
    LMOneAudioProcessor& processor;
};
