#include "XYPad.h"
#include <cmath>

XYPad::XYPad()
{
}

XYPad::~XYPad()
{
}

void XYPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Black background
    g.fillAll(juce::Colours::black);

    // Terminal green border
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.7f));
    g.drawRect(bounds, 1.0f);

    // Grid lines - dot matrix style
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.2f));

    // Dot grid pattern
    for (int x = 10; x < bounds.getWidth(); x += 20)
    {
        for (int y = 10; y < bounds.getHeight(); y += 20)
        {
            g.fillRect(x - 0.5f, y - 0.5f, 1.0f, 1.0f);
        }
    }

    // Major grid lines
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.3f));
    for (int i = 1; i < 4; ++i)
    {
        float x = bounds.getWidth() * (i / 4.0f);
        g.drawLine(x, 0, x, bounds.getHeight(), 0.5f);

        float y = bounds.getHeight() * (i / 4.0f);
        g.drawLine(0, y, bounds.getWidth(), y, 0.5f);
    }

    // Center crosshair - terminal style
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.5f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::plain));

    // Draw crosshair with ASCII characters
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();

    // Horizontal line with dashes
    for (float x = 0; x < bounds.getWidth(); x += 10)
    {
        if (std::abs(x - centerX) > 20)
            g.drawText("-", x, centerY - 8, 10, 16, juce::Justification::centred);
    }

    // Vertical line with pipes
    for (float y = 0; y < bounds.getHeight(); y += 10)
    {
        if (std::abs(y - centerY) > 20)
            g.drawText("|", centerX - 5, y - 8, 10, 16, juce::Justification::centred);
    }

    // Center marker
    g.drawText("+", centerX - 8, centerY - 8, 16, 16, juce::Justification::centred);

    // Thumb position
    auto thumbPos = getThumbPosition();

    // Tracking lines to thumb
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.4f));

    // Vertical tracking line
    for (float y = 0; y < bounds.getHeight(); y += 4)
    {
        g.fillRect(thumbPos.x - 0.5f, y, 1.0f, 2.0f);
    }

    // Horizontal tracking line
    for (float x = 0; x < bounds.getWidth(); x += 4)
    {
        g.fillRect(x, thumbPos.y - 0.5f, 2.0f, 1.0f);
    }

    // Thumb cursor - terminal block style
    g.setColour(juce::Colour(0x00FF00));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    g.drawText("█", thumbPos.x - 7, thumbPos.y - 7, 14, 14, juce::Justification::centred);

    // Glow effect around thumb
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.3f));
    g.drawRect(thumbPos.x - 8.0f, thumbPos.y - 8.0f, 16.0f, 16.0f, 2.0f);
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.1f));
    g.drawRect(thumbPos.x - 12.0f, thumbPos.y - 12.0f, 24.0f, 24.0f, 1.0f);

    // Coordinate display - terminal style
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.8f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));

    // Display coordinates as ASCII
    juce::String coordText = "[X:" + juce::String(xValue, 2) + " Y:" + juce::String(yValue, 2) + "]";
    g.drawText(coordText, bounds.reduced(5), juce::Justification::topRight);

    // Labels with terminal prompt style
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.6f));
    g.drawText(">DELAY_T", 5, bounds.getBottom() - 15, 60, 15, juce::Justification::left);
    g.drawText(">FEEDBK", 5, 5, 60, 15, juce::Justification::left);

    // Add data readout at bottom
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    juce::String hexX = juce::String::toHexString((int)(xValue * 255)).paddedLeft('0', 2);
    juce::String hexY = juce::String::toHexString((int)(yValue * 255)).paddedLeft('0', 2);
    g.drawText("0x" + hexX + " 0x" + hexY, bounds.reduced(5), juce::Justification::bottomLeft);

    // Scanline effect
    g.setColour(juce::Colours::black.withAlpha(0.05f));
    for (float y = 0; y < bounds.getHeight(); y += 2)
    {
        g.drawLine(0, y, bounds.getWidth(), y, 0.5f);
    }
}

void XYPad::resized()
{
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    updateFromMousePosition(event.position);
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    updateFromMousePosition(event.position);
}

void XYPad::setXValue(float newX)
{
    xValue = juce::jlimit(0.0f, 1.0f, newX);
    repaint();
}

void XYPad::setYValue(float newY)
{
    yValue = juce::jlimit(0.0f, 1.0f, newY);
    repaint();
}

juce::Point<float> XYPad::getThumbPosition() const
{
    return juce::Point<float>(
        xValue * getWidth(),
        (1.0f - yValue) * getHeight()
    );
}

void XYPad::updateFromMousePosition(const juce::Point<float>& mousePos)
{
    xValue = juce::jlimit(0.0f, 1.0f, mousePos.x / getWidth());
    yValue = juce::jlimit(0.0f, 1.0f, 1.0f - (mousePos.y / getHeight()));

    if (onValueChange)
    {
        onValueChange(xValue, yValue);
    }

    repaint();
}