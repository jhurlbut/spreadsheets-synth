#pragma once

#include <JuceHeader.h>

class StepSequencer
{
public:
    struct Step
    {
        int noteNumber = 60;
        float velocity = 0.8f;
        bool isActive = false;
        bool hasSlide = false;
        bool hasAccent = false;
        bool isChained = false;
        float cutoffValue = 1000.0f;
    };

    StepSequencer();
    ~StepSequencer();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages,
                      juce::AudioPlayHead* playHead);

    void setStep(int stepIndex, const Step& step);
    Step getStep(int stepIndex) const;

    void setPatternLength(int length);
    int getPatternLength() const { return patternLength; }

    void setTempo(double bpm);
    double getTempo() const { return currentTempo; }

    bool isPlaying() const { return playing; }
    void setPlaying(bool shouldPlay);

    int getCurrentStep() const { return currentStepIndex; }

    std::function<void(int, float)> onStepCutoffChange;

private:
    static constexpr int maxSteps = 16;

    std::array<Step, maxSteps> steps;
    int patternLength { 16 };

    double sampleRate { 44100.0 };
    double currentTempo { 120.0 };

    int currentStepIndex { 0 };
    int samplesPerStep { 0 };
    int currentSamplePosition { 0 };

    bool playing { false };
    bool manualMode { false };
    bool noteIsPlaying { false };
    int lastNoteNumber { -1 };

    void calculateStepLength();
    void moveToNextStep();
    void sendNoteEvents(juce::MidiBuffer& midiMessages, int samplePosition);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencer)
};