#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/SpreadsheetsDisplay.h"
#include "GUI/XYPad.h"
#include "GUI/AcidTabButton.h"
#include "GUI/CRTShaderOverlay.h"

class StepButton : public juce::TextButton
{
public:
    StepButton(int stepIndex) : stepNum(stepIndex) {}

    void updateState(bool active, bool slide, bool accent, bool chain)
    {
        isActive = active;
        hasSlide = slide;
        hasAccent = accent;
        isChained = chain;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Black background
        g.fillAll(juce::Colours::black);

        // Terminal white color scheme
        float brightness = 0.3f;
        if (isActive) brightness = hasAccent ? 1.0f : 0.8f;

        juce::Colour terminalWhite = juce::Colours::white.withAlpha(brightness);

        // Draw border with ASCII characters
        g.setColour(terminalWhite);
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));

        // Determine ASCII representation based on state
        juce::String asciiChar;
        if (!isActive) asciiChar = "[ ]";
        else if (hasSlide) asciiChar = "[>]";
        else if (isChained) asciiChar = "[=]";
        else asciiChar = "[■]";

        // Draw ASCII border
        g.drawRect(bounds, 1.0f);

        // Draw state indicator
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
        g.drawText(asciiChar, bounds.reduced(2), juce::Justification::centred);

        // Show step number
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        g.setColour(terminalWhite.withAlpha(0.5f));
        juce::String stepHex = juce::String::toHexString(stepNum + 1).paddedLeft('0', 2);
        g.drawText(stepHex, bounds.reduced(2), juce::Justification::topRight);

        // Current step indicator
        if (isCurrent)
        {
            // Flashing cursor effect
            static int flashCounter = 0;
            flashCounter++;
            if ((flashCounter / 15) % 2 == 0)
            {
                g.setColour(juce::Colours::white);
                g.drawRect(bounds.reduced(1), 2.0f);

                // Add glow effect
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.drawRect(bounds, 3.0f);
            }
        }

        // Add scanline effect
        g.setColour(juce::Colours::black.withAlpha(0.1f));
        for (float y = 0; y < bounds.getHeight(); y += 2)
        {
            g.drawLine(0, y, bounds.getWidth(), y, 0.5f);
        }
    }

    void setIsCurrent(bool current) { isCurrent = current; repaint(); }

private:
    int stepNum;
    bool isActive = false;
    bool hasSlide = false;
    bool hasAccent = false;
    bool isChained = false;
    bool isCurrent = false;
};

class SpreadsheetsSynthEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer,
                                 public juce::Slider::Listener,
                                 public juce::Button::Listener
{
public:
    SpreadsheetsSynthEditor (SpreadsheetsSynthProcessor&);
    ~SpreadsheetsSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    SpreadsheetsSynthProcessor& audioProcessor;

    SpreadsheetsDisplay spreadsheetsDisplay;
    CRTShaderOverlay crtOverlay;

    juce::ComboBox waveformSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    juce::Slider cutoffKnob;
    juce::Slider resonanceKnob;
    juce::Slider decayKnob;
    juce::Slider accentKnob;
    juce::Slider overdriveKnob;

    juce::Slider delayTimeKnob;
    juce::Slider delayFeedbackKnob;
    juce::Slider delayMixKnob;

    juce::Slider phaserRateKnob;
    juce::Slider phaserDepthKnob;
    juce::Slider phaserFeedbackKnob;
    juce::Slider phaserMixKnob;

    XYPad combFilterPad;  // Now controls harmonics/subharmonics

    juce::Slider masterVolumeKnob;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    std::vector<std::unique_ptr<StepButton>> stepButtons;
    std::vector<std::unique_ptr<juce::Slider>> stepCutoffSliders;

    juce::TextButton playButton {"Play"};
    juce::TextButton stopButton {"Stop"};
    juce::TextButton clearButton {"Clear"};
    AcidTabButton randomButton;

    juce::Slider tempoSlider;
    juce::Label tempoLabel;

    juce::Label statusLabel;
    juce::Label debugLabel;

    void setupSlider(juce::Slider& slider, const juce::String& paramID);
    void updateStepButtons();
    void randomizePattern();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpreadsheetsSynthEditor)
};