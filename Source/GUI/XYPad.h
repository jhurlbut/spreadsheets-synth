#pragma once

#include <JuceHeader.h>

class XYPad : public juce::Component
{
public:
    XYPad();
    ~XYPad() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    void setXValue(float newX);
    void setYValue(float newY);

    float getXValue() const { return xValue; }
    float getYValue() const { return yValue; }

    std::function<void(float, float)> onValueChange;

private:
    float xValue { 0.5f };
    float yValue { 0.5f };

    juce::Point<float> getThumbPosition() const;
    void updateFromMousePosition(const juce::Point<float>& mousePos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};