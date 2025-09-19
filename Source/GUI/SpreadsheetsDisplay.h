#pragma once

#include <JuceHeader.h>

class SpreadsheetsSynthProcessor;

class SpreadsheetsDisplay : public juce::Component,
                             public juce::Timer
{
public:
    SpreadsheetsDisplay(SpreadsheetsSynthProcessor& processor);
    ~SpreadsheetsDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    void triggerLetter(int index);

private:
    SpreadsheetsSynthProcessor& processor;

    static constexpr const char* text = "SPREADSHEETS";
    static constexpr int numLetters = 12;

    struct LetterState
    {
        float brightness = 0.0f;
        juce::Colour currentColor = juce::Colour(0x00FF00);
    };

    std::array<LetterState, numLetters> letterStates;

    int lastLetterIndex = -1;

    juce::Font letterFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpreadsheetsDisplay)
};