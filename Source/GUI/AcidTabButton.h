#pragma once

#include <JuceHeader.h>

class AcidTabButton : public juce::Component,
                       public juce::Timer
{
public:
    AcidTabButton();
    ~AcidTabButton() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    std::function<void()> onClick;

private:
    bool isHovered { false };
    bool isPressed { false };
    bool glitchActive { false };
    float animationPhase { 0.0f };
    int glitchTimer { 0 };
    int dataOffset { 0 };
    uint8_t dataBuffer[256];
    juce::Random random;

    void drawDataCorruption(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcidTabButton)
};