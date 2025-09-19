#include "CRTShaderOverlay.h"

CRTShaderOverlay::CRTShaderOverlay()
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(60); // 60 FPS for smooth CRT effects
}

CRTShaderOverlay::~CRTShaderOverlay()
{
    stopTimer();
}

void CRTShaderOverlay::paint(juce::Graphics& g)
{
    // Draw effects in order (back to front)
    drawScreenCurvature(g);
    drawPhosphorGlow(g);
    drawScanlines(g);
    drawStaticNoise(g);

    if (glitchTimer > 0)
    {
        drawGlitchEffect(g);
    }
}

void CRTShaderOverlay::drawScanlines(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Horizontal scanlines
    g.setColour(juce::Colour(0x00FF00).withAlpha(0.05f * flickerAmount));

    float lineHeight = 2.0f;
    float spacing = 3.0f;

    for (float y = scanlineOffset; y < bounds.getHeight(); y += spacing)
    {
        // Varying intensity for more realistic CRT look
        float intensity = 0.03f + (0.02f * sin(y * 0.1f));
        g.setColour(juce::Colours::black.withAlpha(intensity));
        g.fillRect(0.0f, y, bounds.getWidth(), lineHeight);
    }

    // Subtle vertical lines for pixel grid
    for (float x = 0; x < bounds.getWidth(); x += 3.0f)
    {
        g.setColour(juce::Colours::black.withAlpha(0.02f));
        g.drawLine(x, 0, x, bounds.getHeight(), 0.5f);
    }
}

void CRTShaderOverlay::drawPhosphorGlow(juce::Graphics& g)
{
    // Add green phosphor glow to edges
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient edgeGlow;

    // Top glow
    edgeGlow = juce::ColourGradient(
        juce::Colour(0x00FF00).withAlpha(0.1f),
        0, 0,
        juce::Colours::transparentBlack,
        0, 30,
        false);
    g.setGradientFill(edgeGlow);
    g.fillRect(0.0f, 0.0f, bounds.getWidth(), 30.0f);

    // Bottom glow
    edgeGlow = juce::ColourGradient(
        juce::Colour(0x00FF00).withAlpha(0.1f),
        0, bounds.getHeight(),
        juce::Colours::transparentBlack,
        0, bounds.getHeight() - 30,
        false);
    g.setGradientFill(edgeGlow);
    g.fillRect(0.0f, bounds.getHeight() - 30, bounds.getWidth(), 30.0f);

    // Left glow
    edgeGlow = juce::ColourGradient(
        juce::Colour(0x00FF00).withAlpha(0.1f),
        0, 0,
        juce::Colours::transparentBlack,
        30, 0,
        false);
    g.setGradientFill(edgeGlow);
    g.fillRect(0.0f, 0.0f, 30.0f, bounds.getHeight());

    // Right glow
    edgeGlow = juce::ColourGradient(
        juce::Colour(0x00FF00).withAlpha(0.1f),
        bounds.getWidth(), 0,
        juce::Colours::transparentBlack,
        bounds.getWidth() - 30, 0,
        false);
    g.setGradientFill(edgeGlow);
    g.fillRect(bounds.getWidth() - 30, 0.0f, 30.0f, bounds.getHeight());
}

void CRTShaderOverlay::drawScreenCurvature(juce::Graphics& g)
{
    // Simulate CRT screen curvature with vignette effect
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();

    juce::ColourGradient vignette(
        juce::Colours::transparentBlack,
        center,
        juce::Colours::black.withAlpha(0.3f),
        bounds.getTopLeft(),
        true);

    g.setGradientFill(vignette);
    g.fillAll();

    // Corner darkening for curved screen effect
    float cornerSize = 50.0f;
    juce::Path corners;

    // Top-left corner
    corners.addPieSegment(0, 0, cornerSize * 2, cornerSize * 2,
                          juce::MathConstants<float>::pi,
                          juce::MathConstants<float>::pi * 1.5f, 0);

    // Top-right corner
    corners.addPieSegment(bounds.getWidth() - cornerSize * 2, 0,
                          cornerSize * 2, cornerSize * 2,
                          juce::MathConstants<float>::pi * 1.5f,
                          juce::MathConstants<float>::twoPi, 0);

    // Bottom-left corner
    corners.addPieSegment(0, bounds.getHeight() - cornerSize * 2,
                          cornerSize * 2, cornerSize * 2,
                          juce::MathConstants<float>::halfPi,
                          juce::MathConstants<float>::pi, 0);

    // Bottom-right corner
    corners.addPieSegment(bounds.getWidth() - cornerSize * 2,
                          bounds.getHeight() - cornerSize * 2,
                          cornerSize * 2, cornerSize * 2,
                          0, juce::MathConstants<float>::halfPi, 0);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillPath(corners);
}

void CRTShaderOverlay::drawGlitchEffect(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Random horizontal displacement lines
    for (int i = 0; i < 5; ++i)
    {
        float y = random.nextFloat() * bounds.getHeight();
        float height = 2.0f + random.nextFloat() * 10.0f;
        float displacement = (random.nextFloat() - 0.5f) * 20.0f;

        // Create RGB separation effect
        g.setColour(juce::Colour(0xFF0000).withAlpha(0.3f));
        g.fillRect(displacement - 2, y, bounds.getWidth(), height);

        g.setColour(juce::Colour(0x00FF00).withAlpha(0.3f));
        g.fillRect(displacement, y, bounds.getWidth(), height);

        g.setColour(juce::Colour(0x0000FF).withAlpha(0.3f));
        g.fillRect(displacement + 2, y, bounds.getWidth(), height);
    }

    // Data corruption blocks
    for (int i = 0; i < 3; ++i)
    {
        float x = random.nextFloat() * bounds.getWidth();
        float y = random.nextFloat() * bounds.getHeight();
        float w = 10.0f + random.nextFloat() * 50.0f;
        float h = 10.0f + random.nextFloat() * 30.0f;

        g.setColour(random.nextBool() ? juce::Colour(0x00FF00) : juce::Colours::black);
        g.fillRect(x, y, w, h);

        // Add some ASCII noise
        g.setColour(juce::Colour(0x00FF00));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));

        for (int j = 0; j < 5; ++j)
        {
            char randomChar = 33 + random.nextInt(94); // Random ASCII character
            g.drawText(juce::String::charToString(randomChar),
                      x + random.nextFloat() * w,
                      y + random.nextFloat() * h,
                      10, 10,
                      juce::Justification::centred);
        }
    }
}

void CRTShaderOverlay::drawStaticNoise(juce::Graphics& g)
{
    // Add subtle static noise
    auto bounds = getLocalBounds();

    for (int i = 0; i < 100; ++i)
    {
        float x = random.nextFloat() * bounds.getWidth();
        float y = random.nextFloat() * bounds.getHeight();
        float brightness = random.nextFloat();

        g.setColour(juce::Colour(0x00FF00).withAlpha(brightness * 0.05f));
        g.fillRect(x, y, 1.0f, 1.0f);
    }
}

void CRTShaderOverlay::resized()
{
}

void CRTShaderOverlay::timerCallback()
{
    // Animate scanlines
    scanlineOffset += 0.5f;
    if (scanlineOffset >= 3.0f)
        scanlineOffset = 0.0f;

    // Subtle flicker effect
    flickerAmount = 0.95f + (0.05f * sin(random.nextFloat() * juce::MathConstants<float>::twoPi));

    // Decrease glitch timer
    if (glitchTimer > 0)
    {
        glitchTimer--;
    }

    // Occasional random glitch
    if (random.nextFloat() < 0.001f) // 0.1% chance per frame
    {
        triggerGlitch();
    }

    repaint();
}