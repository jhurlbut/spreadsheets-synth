#include "TB303Voice.h"

TB303Voice::TB303Voice()
{
    oscillator.initialise([](float x) { return std::sin(x); });

    envelope.setParameters({ 0.001f, 0.0f, 1.0f, 0.3f });
    filterEnvelope.setParameters({ 0.001f, 0.0f, 0.0f, 0.3f });
}

bool TB303Voice::canPlaySound(juce::SynthesiserSound* sound)
{
    return true;
}

void TB303Voice::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;

    oscillator.prepare(spec);
    filter.prepare(spec);

    envelope.setSampleRate(sampleRate);
    filterEnvelope.setSampleRate(sampleRate);

    filter.setMode(dsp::LadderFilterMode::LPF24);
}

void TB303Voice::updateParameters(float cutoff, float resonance, float decay,
                                   float accent, float overdrive, int waveform)
{
    currentCutoff = cutoff;
    currentResonance = resonance;
    currentDecay = decay;
    currentAccent = accent;
    currentOverdrive = overdrive;
    currentWaveform = static_cast<Waveform>(waveform);

    envelope.setParameters({ 0.001f, 0.0f, 1.0f, decay });
    filterEnvelope.setParameters({ 0.001f, 0.0f, 0.0f, decay * 0.8f });

    updateOscillator();
}

void TB303Voice::updateOscillator()
{
    if (currentWaveform == Waveform::Sawtooth)
    {
        oscillator.initialise([](float x)
        {
            return (2.0f * x / juce::MathConstants<float>::twoPi) - 1.0f;
        });
    }
    else
    {
        oscillator.initialise([](float x)
        {
            return x < juce::MathConstants<float>::pi ? 1.0f : -1.0f;
        });
    }
}


void TB303Voice::startNote(int midiNoteNumber, float velocity,
                            juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    targetFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    if (!isSliding)
    {
        currentFrequency = targetFrequency;
        oscillator.setFrequency(currentFrequency);
    }
    else
    {
        isSliding = true;
    }

    isAccented = velocity > 0.8f;

    envelope.noteOn();
    filterEnvelope.noteOn();
}

void TB303Voice::stopNote(float velocity, bool allowTailOff)
{
    envelope.noteOff();
    filterEnvelope.noteOff();

    if (!allowTailOff || !envelope.isActive())
    {
        clearCurrentNote();
        isSliding = false;
    }
}

void TB303Voice::pitchWheelMoved(int newPitchWheelValue)
{
}

void TB303Voice::controllerMoved(int controllerNumber, int newControllerValue)
{
}

void TB303Voice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                  int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    juce::AudioBuffer<float> synthBuffer(1, numSamples);
    synthBuffer.clear();

    juce::AudioBuffer<float> envBuffer(1, numSamples);
    envBuffer.clear();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (isSliding)
        {
            currentFrequency += (targetFrequency - currentFrequency) * slideRate;
            if (std::abs(currentFrequency - targetFrequency) < 0.1f)
            {
                currentFrequency = targetFrequency;
                isSliding = false;
            }
            oscillator.setFrequency(currentFrequency);
        }

        float oscSample = oscillator.processSample(0.0f);

        float envValue = envelope.getNextSample();
        float filterEnvValue = filterEnvelope.getNextSample();

        float accentMultiplier = isAccented ? (1.0f + currentAccent) : 1.0f;
        float cutoffFreq = currentCutoff * (1.0f + filterEnvValue * 4.0f) * accentMultiplier;
        cutoffFreq = juce::jlimit(20.0f, 20000.0f, cutoffFreq);

        filter.setCutoffFrequencyHz(cutoffFreq);
        filter.setResonance(currentResonance);

        synthBuffer.setSample(0, sample, oscSample);
        envBuffer.setSample(0, sample, envValue * accentMultiplier);
    }

    dsp::AudioBlock<float> block(synthBuffer);
    dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);

    // Apply overdrive/waveshaping
    float drive = 1.0f + currentOverdrive * 9.0f;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float inputSample = synthBuffer.getSample(0, sample);
        float shapedSample = std::tanh(inputSample * drive) / std::tanh(drive);
        synthBuffer.setSample(0, sample, shapedSample);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float envValue = envBuffer.getSample(0, sample);
        float processedSample = synthBuffer.getSample(0, sample);
        synthBuffer.setSample(0, sample, processedSample * envValue * 0.5f);
    }

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom(channel, startSample, synthBuffer, 0, 0, numSamples);
    }

    if (!envelope.isActive())
    {
        clearCurrentNote();
        isSliding = false;
    }
}