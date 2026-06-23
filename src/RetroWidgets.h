#pragma once

#include <JuceHeader.h>

//==============================================================================
// StepArrow — a small, centered, equilateral triangle button (◀ / ▶). The
// triangle is drawn at a fixed size regardless of the button bounds, so it never
// stretches. Used for bank navigation and the shuffle steppers.
//==============================================================================
class StepArrow : public juce::Button
{
public:
    StepArrow (bool pointsLeft, juce::Colour c)
        : juce::Button ("arrow"), left (pointsLeft), colour (c) {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        const float h = juce::jmin (b.getHeight() - 2.0f, 11.0f); // triangle base
        const float w = h * 0.86f;                                // equilateral extent
        const auto  c = b.getCentre();

        juce::Path tri;
        if (left)
        {
            tri.startNewSubPath (c.x - w * 0.5f, c.y);
            tri.lineTo          (c.x + w * 0.5f, c.y - h * 0.5f);
            tri.lineTo          (c.x + w * 0.5f, c.y + h * 0.5f);
        }
        else
        {
            tri.startNewSubPath (c.x + w * 0.5f, c.y);
            tri.lineTo          (c.x - w * 0.5f, c.y - h * 0.5f);
            tri.lineTo          (c.x - w * 0.5f, c.y + h * 0.5f);
        }
        tri.closeSubPath();

        auto col = colour;
        if (down)      col = col.darker   (0.25f);
        else if (over) col = col.brighter (0.25f);
        g.setColour (col);
        g.fillPath (tri);
    }

private:
    bool         left;
    juce::Colour colour;
};

//==============================================================================
// LedText — a small LED-style text readout that matches the 7-segment LEDs
// (lit red on dark glass). For short words like the shuffle setting names.
//==============================================================================
class LedText : public juce::Component
{
public:
    void setText (const juce::String& t)   { if (t != text) { text = t; repaint(); } }
    void setFontHeight (float h)           { fontHeight = h; }

    std::function<void()> onDoubleClick;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff160a08));            // dark glass
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

        g.setColour (juce::Colour (0xffff3322));            // lit red
        g.setFont (juce::FontOptions (fontHeight, juce::Font::bold));
        g.drawFittedText (text.toUpperCase(), getLocalBounds().reduced (4, 0),   // scale long text to fit
                          juce::Justification::centred, 1, 0.6f);
    }

private:
    juce::String text;
    float        fontHeight = 9.0f;
};

//==============================================================================
// XButton — an orange "X" in an orange-bordered square. Hover/press inverts it
// (orange fill, dark X). Used for Clear, to drop the white "Clear" text.
//==============================================================================
class XButton : public juce::Button
{
public:
    XButton() : juce::Button ("clear") {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        // Flat background, matching the mute/solo buttons (subtle hover/press).
        getLookAndFeel().drawButtonBackground (g, *this,
            findColour (juce::TextButton::buttonColourId), over, down);

        // A small orange X.
        auto r = getLocalBounds().toFloat();
        const float s = juce::jmin (r.getWidth(), r.getHeight()) * 0.42f;
        juce::Rectangle<float> x (0.0f, 0.0f, s, s);
        x.setCentre (r.getCentre());
        g.setColour (juce::Colour (0xfffc5824));
        g.drawLine (x.getX(), x.getY(),      x.getRight(), x.getBottom(), 1.7f);
        g.drawLine (x.getX(), x.getBottom(), x.getRight(), x.getY(),      1.7f);
    }
};

//==============================================================================
// Helpers to step / read an AudioParameterChoice from arrow buttons.
//==============================================================================
namespace ChoiceParam
{
    inline void step (juce::RangedAudioParameter* p, int delta)
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (p))
        {
            const int n   = cp->choices.size();
            const int idx = juce::jlimit (0, n - 1, cp->getIndex() + delta);
            cp->setValueNotifyingHost (n > 1 ? (float) idx / (float) (n - 1) : 0.0f);
        }
    }

    inline juce::String name (juce::RangedAudioParameter* p)
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (p))
            return cp->getCurrentChoiceName();
        return {};
    }
}
