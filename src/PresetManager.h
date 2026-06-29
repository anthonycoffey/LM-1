#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Saves/loads the full plugin state (params + kit + pattern bank) as
// `.nixiepreset` XML files under the user's app-data dir. Message-thread only.
//
// Sample references are stored by path (factory voices by name), so a preset
// that points at a moved/missing user WAV falls back to the default for that
// voice. (Embedding the actual sample data for fully portable presets is a
// possible future add — see ROADMAP.md.)
//==============================================================================
class PresetManager
{
public:
    explicit PresetManager (NixieAudioProcessor& p) : processor (p) {}

    static juce::File getPresetDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("Nixie").getChildFile ("Presets");
        dir.createDirectory();
        return dir;
    }

    bool save (const juce::File& file) const
    {
        const auto f = file.withFileExtension ("nixiepreset");
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
        return getPresetDir().findChildFiles (juce::File::findFiles, false, "*.nixiepreset");
    }

private:
    NixieAudioProcessor& processor;
};
