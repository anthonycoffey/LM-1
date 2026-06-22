#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// The step grid: 12 lanes (one per voice) x N steps. Click a cell to toggle it,
// drag to paint, and use the mouse wheel over a lit cell to set its velocity
// (shown as brightness). A column highlight tracks the playing step.
//
// Holds a local mirror of the pattern; edits are pushed to the processor via
// setStep(), and reloadFromProcessor() pulls it back (e.g. after a state load).
//==============================================================================
class StepGridComponent : public juce::Component
{
public:
    explicit StepGridComponent (LMOneAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void reloadFromProcessor();      // pull the pattern from the processor
    void setPlayingStep (int step);  // playhead column (-1 = idle)

private:
    struct Cell
    {
        int lane = -1, step = -1;
        bool valid() const noexcept { return lane >= 0 && step >= 0; }
    };

    Cell cellAt (juce::Point<int>) const;
    void applyCell (int lane, int step, juce::uint8 velocity);

    LMOneAudioProcessor& processor;
    Pattern     pattern;             // local mirror
    int         playingStep = -1;
    juce::uint8 paintValue  = 100;   // value being painted during a drag
    bool        painting    = false;

    static constexpr int kLabelW     = 78;
    static constexpr int kDefaultVel = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridComponent)
};
