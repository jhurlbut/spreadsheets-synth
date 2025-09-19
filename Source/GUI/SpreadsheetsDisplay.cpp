#include "SpreadsheetsDisplay.h"
#include "../PluginProcessor.h"
#include <JuceHeader.h>

SpreadsheetsDisplay::SpreadsheetsDisplay(SpreadsheetsSynthProcessor& p)
    : processor(p)
{
    startTimerHz(30);
}

SpreadsheetsDisplay::~SpreadsheetsDisplay()
{
    stopTimer();
}

void SpreadsheetsDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    // Use monospace font for terminal look
    if (getHeight() > 0)
    {
        letterFont = juce::Font(juce::Font::getDefaultMonospacedFontName(),
                               getHeight() * 0.7f, juce::Font::plain);
    }
    g.setFont(letterFont);

    float letterWidth = getWidth() / static_cast<float>(numLetters);

    // Draw ASCII border
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));

    // Top border
    juce::String topBorder = "┌";
    for (int i = 0; i < numLetters * 5; ++i) topBorder += "─";
    topBorder += "┐";
    g.drawText(topBorder, 0, 0, getWidth(), 15, juce::Justification::centred);

    // Bottom border
    juce::String bottomBorder = "└";
    for (int i = 0; i < numLetters * 5; ++i) bottomBorder += "─";
    bottomBorder += "┘";
    g.drawText(bottomBorder, 0, getHeight() - 15, getWidth(), 15, juce::Justification::centred);

    // Side borders
    g.drawText("│", 5, 15, 10, getHeight() - 30, juce::Justification::left);
    g.drawText("│", getWidth() - 15, 15, 10, getHeight() - 30, juce::Justification::right);

    g.setFont(letterFont);

    for (int i = 0; i < numLetters; ++i)
    {
        auto& state = letterStates[i];

        // Terminal white color only
        float alpha = 0.3f + (state.brightness * 0.7f);
        juce::Colour letterColor = juce::Colours::white.withAlpha(alpha);

        // Phosphor glow effect
        if (state.brightness > 0.5f)
        {
            float glowSize = state.brightness * 8.0f;

            // Multiple glow layers for phosphor effect
            for (int layer = 3; layer > 0; --layer)
            {
                float layerAlpha = (state.brightness * 0.1f) / layer;
                g.setColour(juce::Colours::white.withAlpha(layerAlpha));

                auto bounds = juce::Rectangle<float>(
                    i * letterWidth - (glowSize * layer),
                    getHeight() * 0.25f - (glowSize * layer),
                    letterWidth + (glowSize * layer * 2),
                    getHeight() * 0.5f + (glowSize * layer * 2));

                g.fillRect(bounds);
            }

            // Add digital noise when triggered
            if (state.brightness > 0.9f)
            {
                static juce::Random random;
                for (int n = 0; n < 5; ++n)
                {
                    float x = (i * letterWidth) + random.nextFloat() * letterWidth;
                    float y = getHeight() * 0.25f + random.nextFloat() * getHeight() * 0.5f;
                    g.setColour(juce::Colours::white.withAlpha(random.nextFloat() * 0.5f));
                    g.fillRect(x, y, 2.0f, 2.0f);
                }
            }
        }

        g.setColour(letterColor);

        // Create string from single character
        juce::String letter;
        letter << text[i];

        g.drawText(letter,
                   i * letterWidth, getHeight() * 0.25f,
                   letterWidth, getHeight() * 0.5f,
                   juce::Justification::centred);

        // Add scanline effect over letter
        if (state.brightness > 0.1f)
        {
            g.setColour(juce::Colours::black.withAlpha(0.1f));
            for (float y = getHeight() * 0.25f; y < getHeight() * 0.75f; y += 2.0f)
            {
                g.drawLine(i * letterWidth, y, (i + 1) * letterWidth, y, 0.5f);
            }
        }
    }

    // Data readout at bottom
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
    juce::String dataString = "[ACTIVE] ";
    for (int i = 0; i < numLetters; ++i)
    {
        dataString += letterStates[i].brightness > 0.01f ? "■" : "□";
    }
    g.drawText(dataString, 10, getHeight() - 12, getWidth() - 20, 12, juce::Justification::left);
}

void SpreadsheetsDisplay::resized()
{
}

void SpreadsheetsDisplay::timerCallback()
{
    int currentIndex = processor.getCurrentLetterIndex();

    if (currentIndex != lastLetterIndex && currentIndex >= 0 && currentIndex < numLetters)
    {
        triggerLetter(currentIndex);
        lastLetterIndex = currentIndex;
    }

    bool needsRepaint = false;

    for (auto& state : letterStates)
    {
        if (state.brightness > 0.01f)
        {
            state.brightness *= 0.92f;
            needsRepaint = true;

            if (state.brightness < 0.01f)
            {
                state.brightness = 0.0f;
            }
        }
    }

    if (needsRepaint)
    {
        repaint();
    }
}

void SpreadsheetsDisplay::triggerLetter(int index)
{
    if (index >= 0 && index < numLetters)
    {
        letterStates[index].brightness = 1.0f;
        repaint();
    }
}