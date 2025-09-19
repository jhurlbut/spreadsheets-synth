#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GUI/CRTShaderOverlay.h"

SpreadsheetsSynthEditor::SpreadsheetsSynthEditor (SpreadsheetsSynthProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      spreadsheetsDisplay(p)
{
    addAndMakeVisible(spreadsheetsDisplay);

    waveformSelector.addItem("Sawtooth", 1);
    waveformSelector.addItem("Square", 2);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "waveform", waveformSelector);
    waveformSelector.setTooltip("Oscillator wave shape - Saw for bright, Square for hollow");
    addAndMakeVisible(waveformSelector);

    setupSlider(cutoffKnob, "cutoff");
    setupSlider(resonanceKnob, "resonance");
    setupSlider(decayKnob, "decay");
    setupSlider(accentKnob, "accent");
    setupSlider(overdriveKnob, "overdrive");

    setupSlider(delayTimeKnob, "delayTime");
    setupSlider(delayFeedbackKnob, "delayFeedback");
    setupSlider(delayMixKnob, "delayMix");

    setupSlider(phaserRateKnob, "phaserRate");
    setupSlider(phaserDepthKnob, "phaserDepth");
    setupSlider(phaserFeedbackKnob, "phaserFeedback");
    setupSlider(phaserMixKnob, "phaserMix");

    // Setup comb filter XY pad
    combFilterPad.onValueChange = [this](float x, float y)
    {
        auto* xParam = audioProcessor.getAPVTS().getParameter("combFilterX");
        auto* yParam = audioProcessor.getAPVTS().getParameter("combFilterY");

        if (xParam) xParam->setValueNotifyingHost(xParam->convertTo0to1(x));
        if (yParam) yParam->setValueNotifyingHost(yParam->convertTo0to1(y));
    };
    // combFilterPad.setTooltip("X: Delay scaling | Y: Feedback amount - Creates metallic textures");
    addAndMakeVisible(combFilterPad);

    setupSlider(combFilterMixKnob, "combFilterMix");
    combFilterMixKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    combFilterMixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

    setupSlider(masterVolumeKnob, "masterVolume");

    for (int i = 0; i < 16; ++i)
    {
        auto button = std::make_unique<StepButton>(i);
        button->addListener(this);
        // button->setTooltip("Click to cycle: OFF → Normal (blue) → Slide (yellow) → Accent (bright) → Chain (green)");
        addAndMakeVisible(button.get());
        stepButtons.push_back(std::move(button));

        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::LinearVertical);
        slider->setRange(20.0, 20000.0, 1.0);
        slider->setSkewFactorFromMidPoint(1000.0);
        slider->setValue(1000.0);
        slider->setTooltip("Per-step filter cutoff - Adjust brightness for this step");
        slider->addListener(this);
        addAndMakeVisible(slider.get());
        stepCutoffSliders.push_back(std::move(slider));
    }

    playButton.addListener(this);
    stopButton.addListener(this);
    clearButton.addListener(this);
    playButton.setTooltip("Start sequencer playback");
    stopButton.setTooltip("Stop sequencer playback");
    clearButton.setTooltip("Clear all sequence steps");
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(clearButton);

    // Setup random button
    randomButton.onClick = [this]() { randomizePattern(); };
    // randomButton.setTooltip("Randomize pattern - Generates new acid sequence with random notes, slides, and parameters");
    addAndMakeVisible(randomButton);

    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setRange(60.0, 200.0, 1.0);
    tempoSlider.setValue(120.0);
    tempoSlider.setTooltip("Playback speed in BPM");
    tempoSlider.addListener(this);
    addAndMakeVisible(tempoSlider);

    tempoLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);

    statusLabel.setText("[STATUS:STOPPED]", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    addAndMakeVisible(statusLabel);

    debugLabel.setText("[STEP:00/16]", juce::dontSendNotification);
    debugLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    debugLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    addAndMakeVisible(debugLabel);

    // Add CRT shader overlay on top
    addAndMakeVisible(crtOverlay);
    crtOverlay.toFront(false);
    crtOverlay.setInterceptsMouseClicks(false, false);

    setSize (800, 700);

    startTimerHz(15);
}

SpreadsheetsSynthEditor::~SpreadsheetsSynthEditor()
{
    stopTimer();
}

void SpreadsheetsSynthEditor::setupSlider(juce::Slider& slider, const juce::String& paramID)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

    // Add tooltips based on parameter ID
    if (paramID == "cutoff")
        slider.setTooltip("Filter frequency - Lower = darker bass sound");
    else if (paramID == "resonance")
        slider.setTooltip("Filter emphasis - Higher = more acid squelch");
    else if (paramID == "decay")
        slider.setTooltip("Note length - How long each note takes to fade");
    else if (paramID == "accent")
        slider.setTooltip("Velocity boost - Makes accented notes louder/brighter");
    else if (paramID == "overdrive")
        slider.setTooltip("Distortion amount - Adds grit and aggression");
    else if (paramID == "delayTime")
        slider.setTooltip("Echo delay in seconds");
    else if (paramID == "delayFeedback")
        slider.setTooltip("Number of echo repeats");
    else if (paramID == "delayMix")
        slider.setTooltip("Wet/dry balance for delay");
    else if (paramID == "phaserRate")
        slider.setTooltip("Speed of phaser sweep");
    else if (paramID == "phaserDepth")
        slider.setTooltip("Intensity of phaser effect");
    else if (paramID == "phaserFeedback")
        slider.setTooltip("Resonance of phaser");
    else if (paramID == "phaserMix")
        slider.setTooltip("Wet/dry balance for phaser");
    else if (paramID == "combFilterMix")
        slider.setTooltip("Wet/dry balance for comb filter");
    else if (paramID == "masterVolume")
        slider.setTooltip("Overall output volume");

    addAndMakeVisible(slider);

    auto attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramID, slider);
    sliderAttachments.push_back(std::move(attachment));
}

void SpreadsheetsSynthEditor::paint (juce::Graphics& g)
{
    // Black background
    g.fillAll(juce::Colours::black);

    // Terminal white text with monospace font
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));

    // Parameter labels in terminal style
    g.drawText("[WAVE]", 10, 90, 80, 20, juce::Justification::centred);
    g.drawText("[CUTOFF]", 100, 90, 80, 20, juce::Justification::centred);
    g.drawText("[RESO]", 190, 90, 80, 20, juce::Justification::centred);
    g.drawText("[DECAY]", 280, 90, 80, 20, juce::Justification::centred);
    g.drawText("[ACCENT]", 370, 90, 80, 20, juce::Justification::centred);
    g.drawText("[DRIVE]", 460, 90, 80, 20, juce::Justification::centred);

    g.drawText("[DLY_T]", 100, 390, 80, 20, juce::Justification::centred);
    g.drawText("[DLY_FB]", 190, 390, 80, 20, juce::Justification::centred);
    g.drawText("[DLY_MX]", 280, 390, 80, 20, juce::Justification::centred);

    g.drawText("[PH_RT]", 400, 390, 80, 20, juce::Justification::centred);
    g.drawText("[PH_DP]", 490, 390, 80, 20, juce::Justification::centred);
    g.drawText("[PH_FB]", 580, 390, 80, 20, juce::Justification::centred);
    g.drawText("[PH_MX]", 670, 390, 80, 20, juce::Justification::centred);

    g.drawText("[MASTER]", 710, 90, 80, 20, juce::Justification::centred);

    // ASCII box drawing for sections
    g.setColour(juce::Colours::white.withAlpha(0.5f));

    // Draw section dividers with ASCII characters
    for (int x = 0; x < getWidth(); x += 8)
    {
        g.drawText("-", x, 210, 8, 10, juce::Justification::centred);
        g.drawText("-", x, 380, 8, 10, juce::Justification::centred);
        g.drawText("-", x, 500, 8, 10, juce::Justification::centred);
    }

    // Section labels
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText(">SYNTH_PARAMS", 10, 195, 150, 15, juce::Justification::left);
    g.drawText(">SEQ_MATRIX", 10, 365, 150, 15, juce::Justification::left);
    g.drawText(">COMB_FILTER", 10, 510, 100, 20, juce::Justification::left);
    g.drawText("[MIX]", 200, 570, 50, 20, juce::Justification::centred);

    // Draw corner brackets for terminal window effect
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    g.drawText("┌", 0, 0, 20, 20, juce::Justification::topLeft);
    g.drawText("┐", getWidth() - 20, 0, 20, 20, juce::Justification::topRight);
    g.drawText("└", 0, getHeight() - 20, 20, 20, juce::Justification::bottomLeft);
    g.drawText("┘", getWidth() - 20, getHeight() - 20, 20, 20, juce::Justification::bottomRight);
}

void SpreadsheetsSynthEditor::resized()
{
    spreadsheetsDisplay.setBounds(10, 10, getWidth() - 20, 70);

    waveformSelector.setBounds(10, 110, 80, 30);
    cutoffKnob.setBounds(100, 110, 80, 80);
    resonanceKnob.setBounds(190, 110, 80, 80);
    decayKnob.setBounds(280, 110, 80, 80);
    accentKnob.setBounds(370, 110, 80, 80);
    overdriveKnob.setBounds(460, 110, 80, 80);
    masterVolumeKnob.setBounds(710, 110, 80, 80);

    int stepButtonY = 220;
    int stepButtonWidth = 40;
    int stepButtonHeight = 40;
    int stepSpacing = 48;

    for (int i = 0; i < 16; ++i)
    {
        stepButtons[i]->setBounds(10 + i * stepSpacing, stepButtonY,
                                   stepButtonWidth, stepButtonHeight);

        stepCutoffSliders[i]->setBounds(10 + i * stepSpacing, stepButtonY + 45,
                                         stepButtonWidth, 80);  // Made taller: 60 -> 80
    }

    playButton.setBounds(10, 330, 60, 30);
    stopButton.setBounds(75, 330, 60, 30);
    clearButton.setBounds(140, 330, 60, 30);
    randomButton.setBounds(210, 315, 50, 50);

    tempoLabel.setBounds(270, 330, 40, 30);
    tempoSlider.setBounds(310, 330, 120, 30);

    statusLabel.setBounds(440, 330, 150, 30);
    debugLabel.setBounds(600, 330, 150, 30);

    delayTimeKnob.setBounds(100, 410, 80, 80);
    delayFeedbackKnob.setBounds(190, 410, 80, 80);
    delayMixKnob.setBounds(280, 410, 80, 80);

    phaserRateKnob.setBounds(400, 410, 80, 80);
    phaserDepthKnob.setBounds(490, 410, 80, 80);
    phaserFeedbackKnob.setBounds(580, 410, 80, 80);
    phaserMixKnob.setBounds(670, 410, 80, 80);

    combFilterPad.setBounds(10, 535, 150, 150);
    combFilterMixKnob.setBounds(180, 590, 80, 80);

    // CRT overlay covers entire window
    crtOverlay.setBounds(getLocalBounds());
}

void SpreadsheetsSynthEditor::timerCallback()
{
    updateStepButtons();

    // Update status display
    bool isPlaying = audioProcessor.getSequencer().isPlaying();
    statusLabel.setText(isPlaying ? "[STATUS:PLAYING]" : "[STATUS:STOPPED]", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(isPlaying ? 1.0f : 0.6f));

    // Update button appearance
    playButton.setColour(juce::TextButton::buttonColourId,
                        isPlaying ? juce::Colours::white.withAlpha(0.3f) : juce::Colours::black);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.5f));
    playButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    // Update step display
    int currentStep = audioProcessor.getSequencer().getCurrentStep();
    debugLabel.setText("Step: " + juce::String(currentStep + 1) + "/16 | Tempo: " +
                        juce::String(audioProcessor.getSequencer().getTempo(), 1) + " BPM",
                        juce::dontSendNotification);

    // Update XY pad from parameters
    auto xValue = audioProcessor.getAPVTS().getRawParameterValue("combFilterX")->load();
    auto yValue = audioProcessor.getAPVTS().getRawParameterValue("combFilterY")->load();
    combFilterPad.setXValue(xValue);
    combFilterPad.setYValue(yValue);
}

void SpreadsheetsSynthEditor::updateStepButtons()
{
    int currentStep = audioProcessor.getSequencer().getCurrentStep();

    for (int i = 0; i < 16; ++i)
    {
        auto step = audioProcessor.getSequencer().getStep(i);
        stepButtons[i]->updateState(step.isActive, step.hasSlide,
                                     step.hasAccent, step.isChained);
        stepButtons[i]->setIsCurrent(i == currentStep &&
                                      audioProcessor.getSequencer().isPlaying());
    }
}

void SpreadsheetsSynthEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &tempoSlider)
    {
        audioProcessor.getSequencer().setTempo(slider->getValue());
    }
    else
    {
        for (int i = 0; i < 16; ++i)
        {
            if (slider == stepCutoffSliders[i].get())
            {
                auto step = audioProcessor.getSequencer().getStep(i);
                step.cutoffValue = static_cast<float>(slider->getValue());
                audioProcessor.getSequencer().setStep(i, step);
                break;
            }
        }
    }
}

void SpreadsheetsSynthEditor::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        audioProcessor.getSequencer().setPlaying(true);
    }
    else if (button == &stopButton)
    {
        audioProcessor.getSequencer().setPlaying(false);
    }
    else if (button == &clearButton)
    {
        for (int i = 0; i < 16; ++i)
        {
            StepSequencer::Step step;
            step.noteNumber = 36 + (i % 12);
            step.isActive = false;
            audioProcessor.getSequencer().setStep(i, step);
            stepCutoffSliders[i]->setValue(1000.0);
        }
    }
    else
    {
        for (int i = 0; i < 16; ++i)
        {
            if (button == stepButtons[i].get())
            {
                auto step = audioProcessor.getSequencer().getStep(i);

                if (!step.isActive)
                {
                    step.isActive = true;
                    step.hasSlide = false;
                    step.hasAccent = false;
                    step.isChained = false;
                }
                else if (!step.hasSlide)
                {
                    step.hasSlide = true;
                    step.hasAccent = false;
                    step.isChained = false;
                }
                else if (!step.hasAccent)
                {
                    step.hasSlide = false;
                    step.hasAccent = true;
                    step.isChained = false;
                }
                else if (!step.isChained)
                {
                    step.hasSlide = false;
                    step.hasAccent = false;
                    step.isChained = true;
                }
                else
                {
                    step.isActive = false;
                    step.hasSlide = false;
                    step.hasAccent = false;
                    step.isChained = false;
                }

                audioProcessor.getSequencer().setStep(i, step);
                break;
            }
        }
    }
}

void SpreadsheetsSynthEditor::randomizePattern()
{
    juce::Random random;

    // Randomize global synth parameters
    auto* cutoffParam = audioProcessor.getAPVTS().getParameter("cutoff");
    auto* resonanceParam = audioProcessor.getAPVTS().getParameter("resonance");
    auto* decayParam = audioProcessor.getAPVTS().getParameter("decay");
    auto* accentParam = audioProcessor.getAPVTS().getParameter("accent");
    auto* overdriveParam = audioProcessor.getAPVTS().getParameter("overdrive");

    // Set synth parameters to good ranges for acid bass
    if (cutoffParam)
        cutoffParam->setValueNotifyingHost(random.nextFloat() * 0.6f + 0.1f); // 10-70% range

    if (resonanceParam)
        resonanceParam->setValueNotifyingHost(random.nextFloat() * 0.6f + 0.3f); // 30-90% range

    if (decayParam)
        decayParam->setValueNotifyingHost(random.nextFloat() * 0.5f + 0.2f); // 20-70% range

    if (accentParam)
        accentParam->setValueNotifyingHost(random.nextFloat() * 0.6f + 0.2f); // 20-80% range

    if (overdriveParam)
        overdriveParam->setValueNotifyingHost(random.nextFloat() * 0.5f + 0.1f); // 10-60% range

    // Randomize effects parameters
    auto* delayTimeParam = audioProcessor.getAPVTS().getParameter("delayTime");
    auto* delayFeedbackParam = audioProcessor.getAPVTS().getParameter("delayFeedback");
    auto* delayMixParam = audioProcessor.getAPVTS().getParameter("delayMix");
    auto* phaserRateParam = audioProcessor.getAPVTS().getParameter("phaserRate");
    auto* phaserMixParam = audioProcessor.getAPVTS().getParameter("phaserMix");

    if (delayTimeParam)
        delayTimeParam->setValueNotifyingHost(random.nextFloat() * 0.4f + 0.1f); // 10-50% range

    if (delayFeedbackParam)
        delayFeedbackParam->setValueNotifyingHost(random.nextFloat() * 0.5f + 0.2f); // 20-70% range

    if (delayMixParam)
        delayMixParam->setValueNotifyingHost(random.nextFloat() * 0.4f); // 0-40% range

    if (phaserRateParam)
        phaserRateParam->setValueNotifyingHost(random.nextFloat() * 0.3f + 0.1f); // 10-40% range

    if (phaserMixParam)
        phaserMixParam->setValueNotifyingHost(random.nextFloat() * 0.3f); // 0-30% range

    // Randomize sequencer pattern
    int baseNote = 36 + random.nextInt(12); // C2 to B2
    float stepProbability = 0.7f; // 70% chance each step is active

    for (int i = 0; i < 16; ++i)
    {
        StepSequencer::Step step;

        // Decide if step is active
        step.isActive = random.nextFloat() < stepProbability;

        if (step.isActive)
        {
            // Random note within 2 octaves
            step.noteNumber = baseNote + random.nextInt(24) - 12;

            // Random velocity
            step.velocity = random.nextFloat() * 0.5f + 0.5f; // 0.5 to 1.0

            // Random modes with controlled probability
            float modeRoll = random.nextFloat();

            if (modeRoll < 0.15f)
            {
                step.hasSlide = true;
            }
            else if (modeRoll < 0.30f)
            {
                step.hasAccent = true;
            }
            else if (modeRoll < 0.35f)
            {
                step.isChained = true;
            }

            // Random per-step cutoff with bias toward mid frequencies
            float cutoffRange = random.nextFloat();
            if (cutoffRange < 0.6f)
            {
                // 60% chance: mid range (500-2000 Hz)
                step.cutoffValue = 500.0f + random.nextFloat() * 1500.0f;
            }
            else if (cutoffRange < 0.85f)
            {
                // 25% chance: low range (200-500 Hz)
                step.cutoffValue = 200.0f + random.nextFloat() * 300.0f;
            }
            else
            {
                // 15% chance: high range (2000-8000 Hz)
                step.cutoffValue = 2000.0f + random.nextFloat() * 6000.0f;
            }
        }
        else
        {
            step.noteNumber = baseNote;
            step.velocity = 0.7f;
            step.cutoffValue = 1000.0f;
        }

        audioProcessor.getSequencer().setStep(i, step);

        // Update the UI slider for this step's cutoff
        stepCutoffSliders[i]->setValue(step.cutoffValue);
    }

    // Update button states
    updateStepButtons();
}