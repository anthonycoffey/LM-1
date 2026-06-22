#pragma once

#include <JuceHeader.h>
#include "LMOneLookAndFeel.h"

//==============================================================================
// A small red 7-segment LED readout (vintage LM-1 style). Right-aligns its text
// into a fixed number of digit cells. Accepts 0-9, '-', '.', and space.
//==============================================================================
class LedDisplay : public juce::Component
{
public:
    explicit LedDisplay (int numDigits = 3) : digits (juce::jmax (1, numDigits)) {}

    void setText (const juce::String& t)
    {
        if (t != text) { text = t; repaint(); }
    }

    void setNumDigits (int n) { digits = juce::jmax (1, n); repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // Inset bezel + dark glass.
        g.setColour (juce::Colour (0xff160a08));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

        auto inner = r.reduced (5.0f, 3.0f);
        const int  n   = digits;
        const float gap = 3.0f;
        const float dw  = (inner.getWidth() - gap * (float) (n - 1)) / (float) n;

        // Right-align into the cells.
        juce::String s = text.substring (0, n);
        while (s.length() < n) s = " " + s;

        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<float> cell (inner.getX() + (float) i * (dw + gap), inner.getY(),
                                         dw, inner.getHeight());
            drawDigit (g, s[i], cell);
        }
    }

private:
    // segment bits: a=1, b=2, c=4, d=8, e=16, f=32, g=64
    static int segmentsFor (juce::juce_wchar c)
    {
        switch (c)
        {
            case '0': return 0b0111111;
            case '1': return 0b0000110;
            case '2': return 0b1011011;
            case '3': return 0b1001111;
            case '4': return 0b1100110;
            case '5': return 0b1101101;
            case '6': return 0b1111101;
            case '7': return 0b0000111;
            case '8': return 0b1111111;
            case '9': return 0b1101111;
            case '-': return 0b1000000;
            default:  return 0;            // space / unknown -> all off
        }
    }

    void drawDigit (juce::Graphics& g, juce::juce_wchar c, juce::Rectangle<float> cell)
    {
        const int   segs = segmentsFor (c);
        const float t    = juce::jmin (cell.getWidth(), cell.getHeight()) * 0.16f;
        const float x = cell.getX(), y = cell.getY(), w = cell.getWidth(), h = cell.getHeight();
        const float midY = y + h * 0.5f;
        const float vH   = h * 0.5f - 1.5f * t;   // vertical-segment length

        const juce::Colour onCol  (0xffff3322);   // lit red
        const juce::Colour offCol (0xff2a0b07);   // unlit

        auto seg = [&] (int bit, juce::Rectangle<float> rect)
        {
            g.setColour ((segs & bit) != 0 ? onCol : offCol);
            g.fillRoundedRectangle (rect, t * 0.35f);
        };

        seg (1,  { x + t,     y,             w - 2 * t, t });   // a  top
        seg (64, { x + t,     midY - t*0.5f, w - 2 * t, t });   // g  middle
        seg (8,  { x + t,     y + h - t,     w - 2 * t, t });   // d  bottom
        seg (32, { x,         y + t,         t, vH });          // f  upper-left
        seg (2,  { x + w - t, y + t,         t, vH });          // b  upper-right
        seg (16, { x,         midY + 0.5f*t, t, vH });          // e  lower-left
        seg (4,  { x + w - t, midY + 0.5f*t, t, vH });          // c  lower-right
    }

    juce::String text { "--" };
    int digits = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedDisplay)
};
