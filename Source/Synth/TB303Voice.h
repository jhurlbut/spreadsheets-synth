#pragma once

#include <JuceHeader.h>

// Harmonic processor for adding overtones and undertones
class HarmonicProcessor
{
public:
    HarmonicProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    float process(float input, float frequency);
    void updateParameters(float lfoRate, float lfoDepth);
    void reset();

private:
    double sampleRate { 44100.0 };

    // LFO parameters from XY pad
    float lfoRate { 2.0f };      // X-axis: LFO speed in Hz (0.1 to 20 Hz)
    float lfoDepth { 0.5f };     // Y-axis: LFO modulation depth (0-1)
    
    // LFO state
    float lfoPhase { 0.0f };
    float lfoPhase2 { 0.0f };    // Second LFO with phase offset for subharmonics
    
    // Base harmonic levels (modulated by LFO)
    static constexpr float baseHarmonicAmount { 0.4f };
    static constexpr float baseSubharmonicDepth { 0.3f };

    // Subharmonic generation
    float subPhase { 0.0f };
    float subPhase2 { 0.0f };
    float lastSample { 0.0f };
    int zeroCrossingCounter { 0 };

    // Oversampling for anti-aliasing
    std::unique_ptr<dsp::Oversampling<float>> oversampler;

    // Waveshaping functions
    float asymmetricTanh(float x, float drive, float asymmetry);
    float chebyshevMix(float x, float amount);
};

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

    void updateHarmonicParameters(float lfoRate, float lfoDepth);

private:
    enum class Waveform { Sawtooth, Square };

    dsp::Oscillator<float> oscillator;
    dsp::LadderFilter<float> filter;
    HarmonicProcessor harmonicProcessor;

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

    // PolyBLEP for anti-aliased square wave
    float polyBLEPPhase { 0.0f };
    float lastPhase { 0.0f };
    float generatePolyBLEPSquare(float frequency);
    float polyBLEP(float phase, float phaseInc);

    void updateOscillator();

    // Amplitude compensation for square wave
    static constexpr float squareWaveBoost { 1.4f };
};