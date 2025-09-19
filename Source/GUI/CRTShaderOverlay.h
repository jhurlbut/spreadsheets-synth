#pragma once

#include <JuceHeader.h>

class CRTShaderOverlay : public juce::Component,
                          public juce::Timer
{
public:
    CRTShaderOverlay();
    ~CRTShaderOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setGlitchIntensity(float intensity) { glitchIntensity = intensity; }
    void triggerGlitch() { glitchTimer = 10; }

private:
    float scanlineOffset { 0.0f };
    float flickerAmount { 1.0f };
    float glitchIntensity { 0.0f };
    int glitchTimer { 0 };
    juce::Random random;

    void drawScanlines(juce::Graphics& g);
    void drawPhosphorGlow(juce::Graphics& g);
    void drawScreenCurvature(juce::Graphics& g);
    void drawGlitchEffect(juce::Graphics& g);
    void drawStaticNoise(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CRTShaderOverlay)
};