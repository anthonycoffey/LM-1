#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "MidiExport.h"

//==============================================================================
// A small drag handle: drag it onto a DAW timeline to drop the current pattern
// as a .mid clip. Writes a uniquely-named temp file and starts a native external
// drag (canMoveFiles = false so the host copies it; we never hand off ownership).
//==============================================================================
class MidiDragSource : public juce::Component
{
public:
    explicit MidiDragSource (LMOneAudioProcessor& p) : processor (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::orange.withAlpha (dragging ? 0.55f : 0.22f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::orange.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (juce::String::fromUTF8 ("\xE2\x86\x93 drag MIDI"), getLocalBounds(),
                    juce::Justification::centred);
    }

    void mouseDrag (const juce::MouseEvent&) override
    {
        if (dragging)
            return;
        dragging = true;
        repaint();

        const auto mf  = MidiExport::build (processor.getPattern(), (double) processor.getSeqTempo());
        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getChildFile ("LM-1-" + juce::String (juce::Time::getMillisecondCounter())
                                            + ".mid");

        bool ok = false;
        {
            if (auto os = std::unique_ptr<juce::FileOutputStream> (tmp.createOutputStream()))
            {
                os->setPosition (0);
                os->truncate();
                ok = mf.writeTo (*os, 1);
            }
        } // stream flushes/closes here, before the OS reads the file

        if (ok)
        {
            juce::StringArray files;
            files.add (tmp.getFullPathName());
            juce::DragAndDropContainer::performExternalDragDropOfFiles (
                files, false, this, [this] { dragging = false; repaint(); });
        }
        else
        {
            dragging = false;
            repaint();
        }
    }

private:
    LMOneAudioProcessor& processor;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDragSource)
};
