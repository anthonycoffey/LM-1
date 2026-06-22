#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// One vertical mixer strip for a single voice: an audition pad, a Load… button,
// the loaded-sample name, a level fader, pan + tune knobs, and mute/solo toggles.
// All controls are attached to the processor's APVTS parameters.
//==============================================================================
class VoiceStripComponent : public juce::Component
{
public:
    VoiceStripComponent (LMOneAudioProcessor& proc, int voiceIndex);

    void resized() override;
    void paint (juce::Graphics&) override;

    // Re-read the loaded-sample name from the processor (e.g. after a state restore).
    void refreshSourceLabel() { updateSourceLabel(); }

private:
    void chooseSample();
    void updateSourceLabel();

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    LMOneAudioProcessor& processor;
    int index = 0;
    int midiNote = 0;

    juce::TextButton padButton;
    juce::TextButton loadButton { "Load" };
    juce::Label      sourceLabel;
    juce::Slider     levelSlider, panSlider, tuneSlider, swingSlider;
    juce::Label      levelCaption, panCaption, tuneCaption, swingCaption;
    juce::TextButton muteButton { "M" }, soloButton { "S" };

    std::unique_ptr<SliderAttachment> levelAtt, panAtt, tuneAtt, swingAtt;
    std::unique_ptr<ButtonAttachment> muteAtt, soloAtt;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceStripComponent)
};
