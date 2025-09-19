#pragma once

#include <JuceHeader.h>

class TB303Voice : public juce::SynthesiserVoice
{
public:
    TB303Voice();

    bool canPlaySound (juce::SynthesiserSound*) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newPitchWheelValue) override;

    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepareToPlay (double sampleRate, int samplesPerBlock);

    void updateParameters (float cutoff, float resonance, float decay,
                           float accent, float overdrive, int waveform);

private:
    enum class Waveform { Sawtooth, Square };

    dsp::Oscillator<float> oscillator;
    dsp::LadderFilter<float> filter;

    juce::ADSR envelope;
    juce::ADSR filterEnvelope;

    float currentCutoff { 1000.0f };
    float currentResonance { 0.5f };
    float currentDecay { 0.3f };
    float currentAccent { 0.5f };
    float currentOverdrive { 0.3f };
    Waveform currentWaveform { Waveform::Sawtooth };

    float currentFrequency { 0.0f };
    float targetFrequency { 0.0f };
    float slideRate { 0.01f };
    bool isSliding { false };
    bool isAccented { false };

    double sampleRate { 44100.0 };

    void updateOscillator();
};