#include "StepSequencer.h"

StepSequencer::StepSequencer()
{
    // Initialize with a more obvious pattern - every step active
    for (int i = 0; i < maxSteps; ++i)
    {
        steps[i].noteNumber = 36 + (i % 12);
        steps[i].isActive = true;  // All steps active for testing
        steps[i].velocity = 0.7f;
        steps[i].cutoffValue = 1000.0f;
    }

    // Initialize timing
    calculateStepLength();
}

StepSequencer::~StepSequencer()
{
}

void StepSequencer::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    calculateStepLength();
}

void StepSequencer::calculateStepLength()
{
    double beatsPerSecond = currentTempo / 60.0;
    double secondsPerBeat = 1.0 / beatsPerSecond;
    double secondsPerSixteenth = secondsPerBeat / 4.0;
    samplesPerStep = static_cast<int>(secondsPerSixteenth * sampleRate);
}

void StepSequencer::processBlock(juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midiMessages,
                                  juce::AudioPlayHead* playHead)
{
    // Check if we should use host transport
    bool useHostTransport = false;

    if (playHead != nullptr && !manualMode)
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        playHead->getCurrentPosition(info);

        if (info.isPlaying)
        {
            useHostTransport = true;
            playing = true;

            // Update tempo from host
            if (info.bpm > 0)
            {
                currentTempo = info.bpm;
                calculateStepLength();
            }

            double ppqPosition = info.ppqPosition;
            double ppqPerStep = 0.25;
            int newStepIndex = static_cast<int>(ppqPosition / ppqPerStep) % patternLength;

            if (newStepIndex != currentStepIndex)
            {
                currentStepIndex = newStepIndex;
                currentSamplePosition = 0;
                sendNoteEvents(midiMessages, 0);
            }
        }
    }

    // Use internal clock if playing manually or host isn't playing
    if (playing && !useHostTransport)
    {
        int numSamples = buffer.getNumSamples();
        int samplesProcessed = 0;

        while (samplesProcessed < numSamples)
        {
            int samplesThisTime = std::min(numSamples - samplesProcessed,
                                            samplesPerStep - currentSamplePosition);

            if (currentSamplePosition == 0)
            {
                sendNoteEvents(midiMessages, samplesProcessed);
            }

            currentSamplePosition += samplesThisTime;
            samplesProcessed += samplesThisTime;

            if (currentSamplePosition >= samplesPerStep)
            {
                moveToNextStep();
            }
        }
    }

    // Handle stopping
    if (!playing && noteIsPlaying && lastNoteNumber >= 0)
    {
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNoteNumber), 0);
        noteIsPlaying = false;
        lastNoteNumber = -1;
    }
}

void StepSequencer::moveToNextStep()
{
    currentStepIndex = (currentStepIndex + 1) % patternLength;
    currentSamplePosition = 0;
}

void StepSequencer::sendNoteEvents(juce::MidiBuffer& midiMessages, int samplePosition)
{
    const Step& currentStep = steps[currentStepIndex];

    if (!currentStep.isActive)
    {
        if (noteIsPlaying && lastNoteNumber >= 0 && !currentStep.isChained)
        {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNoteNumber), samplePosition);
            noteIsPlaying = false;
            lastNoteNumber = -1;
        }
        return;
    }

    if (!currentStep.isChained && noteIsPlaying && lastNoteNumber >= 0)
    {
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNoteNumber), samplePosition);
        noteIsPlaying = false;
    }

    float velocity = currentStep.hasAccent ? 1.0f : currentStep.velocity;
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    int velocityInt = static_cast<int>(velocity * 127.0f);

    if (currentStep.hasSlide && currentStepIndex < patternLength - 1)
    {
        const Step& nextStep = steps[(currentStepIndex + 1) % patternLength];
        if (nextStep.isActive)
        {
            midiMessages.addEvent(
                juce::MidiMessage::controllerEvent(1, 65, 127), samplePosition);
        }
    }

    midiMessages.addEvent(
        juce::MidiMessage::noteOn(1, currentStep.noteNumber, (juce::uint8)velocityInt),
        samplePosition);

    noteIsPlaying = true;
    lastNoteNumber = currentStep.noteNumber;

    if (onStepCutoffChange)
    {
        onStepCutoffChange(currentStepIndex, currentStep.cutoffValue);
    }
}

void StepSequencer::setStep(int stepIndex, const Step& step)
{
    if (stepIndex >= 0 && stepIndex < maxSteps)
    {
        steps[stepIndex] = step;
    }
}

StepSequencer::Step StepSequencer::getStep(int stepIndex) const
{
    if (stepIndex >= 0 && stepIndex < maxSteps)
    {
        return steps[stepIndex];
    }
    return Step();
}

void StepSequencer::setPatternLength(int length)
{
    patternLength = juce::jlimit(1, maxSteps, length);
}

void StepSequencer::setTempo(double bpm)
{
    currentTempo = juce::jlimit(60.0, 200.0, bpm);
    calculateStepLength();
}

void StepSequencer::setPlaying(bool shouldPlay)
{
    playing = shouldPlay;
    manualMode = shouldPlay;  // Enter manual mode when user controls playback

    if (!playing && noteIsPlaying && lastNoteNumber >= 0)
    {
        noteIsPlaying = false;
        lastNoteNumber = -1;
    }

    if (playing)
    {
        currentStepIndex = 0;
        currentSamplePosition = 0;
        calculateStepLength();  // Ensure timing is set
    }
}