#include "AcidTabButton.h"

AcidTabButton::AcidTabButton()
{
    setOpaque(false);
    startTimerHz(10); // Animation timer for glitch effects

    // Initialize data buffer with pseudo-random values
    for (int i = 0; i < 256; ++i)
    {
        dataBuffer[i] = (i * 37 + 113) % 256;
    }
}

AcidTabButton::~AcidTabButton()
{
    stopTimer();
}

void AcidTabButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Black background
    g.fillAll(juce::Colours::black);

    // Draw terminal border
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawRect(bounds, 1.0f);

    // Data corruption background pattern
    drawDataCorruption(g, bounds);

    // Draw glitch blocks
    if (isHovered || glitchActive)
    {
        for (int i = 0; i < 15; ++i)
        {
            float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
            float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
            float w = 5.0f + random.nextFloat() * 40.0f;
            float h = 2.0f + random.nextFloat() * 20.0f;

            g.setColour(random.nextBool() ? juce::Colours::white.withAlpha(random.nextFloat())
                                         : juce::Colours::black);
            g.fillRect(x, y, w, h);

            // Add inverted blocks
            if (random.nextFloat() < 0.3f)
            {
                // Create inverted appearance by drawing white overlay
                g.setColour(juce::Colours::white.withAlpha(0.8f));
                g.fillRect(x, y, w, h);
            }
        }
    }

    // Draw binary/hex data stream
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
    g.setColour(juce::Colours::white.withAlpha(0.4f));

    for (int row = 0; row < 6; ++row)
    {
        juce::String hexString;
        for (int col = 0; col < 8; ++col)
        {
            int value = (dataBuffer[(row * 8 + col + dataOffset) % 256] + (int)(animationPhase * 255)) % 256;
            hexString += juce::String::toHexString(value).paddedLeft('0', 2) + " ";
        }
        g.drawText(hexString, bounds.getX() + 2, bounds.getY() + row * 10 + 5,
                   bounds.getWidth() - 4, 10, juce::Justification::left);
    }

    // Central command prompt cursor
    auto center = bounds.getCentre();
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::plain));

    // Blinking cursor effect
    if ((int)(animationPhase * 10) % 2 == 0 || isHovered)
    {
        g.drawText("[RND]", bounds, juce::Justification::centred);

        // Draw glitch effect on hover
        if (isHovered)
        {
            // RGB separation effect
            g.setColour(juce::Colour(0xFF0000).withAlpha(0.2f));
            g.drawText("[RND]", bounds.translated(-1, 0), juce::Justification::centred);

            g.setColour(juce::Colour(0x0000FF).withAlpha(0.2f));
            g.drawText("[RND]", bounds.translated(1, 0), juce::Justification::centred);
        }
    }
    else
    {
        g.drawText("[___]", bounds, juce::Justification::centred);
    }

    // Status indicator
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.0f, juce::Font::plain));
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(isHovered ? ">>EXEC READY" : ">>STANDBY",
               bounds.getX() + 2, bounds.getBottom() - 12,
               bounds.getWidth() - 4, 10, juce::Justification::left);

    // Scanline effect
    g.setColour(juce::Colours::black.withAlpha(0.1f));
    for (float y = 0; y < bounds.getHeight(); y += 2)
    {
        g.drawLine(bounds.getX(), bounds.getY() + y,
                   bounds.getRight(), bounds.getY() + y, 0.5f);
    }

    // Press effect
    if (isPressed)
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(bounds);

        // Trigger more intense glitch
        glitchActive = true;
    }
}

void AcidTabButton::drawDataCorruption(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // ASCII art pattern
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 6.0f, juce::Font::plain));
    g.setColour(juce::Colours::white.withAlpha(0.15f));

    juce::String patterns[4] = { "▓", "▒", "░", "█" };

    for (int y = 0; y < bounds.getHeight(); y += 8)
    {
        for (int x = 0; x < bounds.getWidth(); x += 6)
        {
            int patternIndex = (x / 6 + y / 8 + (int)(animationPhase * 10)) % 4;
            g.drawText(patterns[patternIndex],
                      bounds.getX() + x, bounds.getY() + y, 6, 8,
                      juce::Justification::centred);
        }
    }
}

void AcidTabButton::timerCallback()
{
    animationPhase += 0.05f;
    if (animationPhase > 1.0f)
        animationPhase = 0.0f;

    // Update data buffer with "random" data
    dataOffset = (dataOffset + 1) % 256;

    // Decay glitch effect
    if (glitchActive)
    {
        glitchTimer++;
        if (glitchTimer > 10)
        {
            glitchActive = false;
            glitchTimer = 0;
        }
    }

    repaint();
}

void AcidTabButton::resized()
{
}

void AcidTabButton::mouseDown(const juce::MouseEvent& event)
{
    isPressed = true;
    repaint();

    if (onClick)
    {
        onClick();
    }

    // Trigger glitch effect
    glitchActive = true;
    glitchTimer = 0;

    juce::Timer::callAfterDelay(100, [this]()
    {
        isPressed = false;
        repaint();
    });
}

void AcidTabButton::mouseEnter(const juce::MouseEvent& event)
{
    isHovered = true;
    repaint();
}

void AcidTabButton::mouseExit(const juce::MouseEvent& event)
{
    isHovered = false;
    repaint();
}