#include "TB303Voice.h"

// HarmonicProcessor Implementation
HarmonicProcessor::HarmonicProcessor()
{
    oversampler = std::make_unique<dsp::Oversampling<float>>(1, 2,
        dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
}

void HarmonicProcessor::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    oversampler->initProcessing(samplesPerBlock);
    reset();
}

void HarmonicProcessor::reset()
{
    subPhase = 0.0f;
    subPhase2 = 0.0f;
    lastSample = 0.0f;
    zeroCrossingCounter = 0;
    oversampler->reset();
}

void HarmonicProcessor::updateParameters(float harmAmount, float subDepth)
{
    harmonicAmount = harmAmount;
    subharmonicDepth = subDepth;
}

float HarmonicProcessor::asymmetricTanh(float x, float drive, float asymmetry)
{
    float dc = asymmetry * std::abs(x) * 0.5f;
    float shaped = std::tanh((x + dc) * drive);
    return shaped - dc * 0.5f;
}

float HarmonicProcessor::chebyshevMix(float x, float amount)
{
    x = juce::jlimit(-1.0f, 1.0f, x);
    float T2 = 2.0f * x * x - 1.0f;  // 2nd harmonic
    float T3 = 4.0f * x * x * x - 3.0f * x;  // 3rd harmonic
    float T4 = 8.0f * x * x * x * x - 8.0f * x * x + 1.0f;  // 4th harmonic
    return x + (T2 * amount * 0.2f) + (T3 * amount * 0.15f) + (T4 * amount * 0.1f);  // Much stronger harmonics
}

float HarmonicProcessor::process(float input, float frequency)
{
    float output = input;

    // A. Generate subharmonics (Y-axis control)
    if (subharmonicDepth > 0.01f)
    {
        // Sub-octave (f/2)
        subPhase += (frequency * 0.5f * 2.0f * juce::MathConstants<float>::pi) / sampleRate;
        if (subPhase > juce::MathConstants<float>::twoPi)
            subPhase -= juce::MathConstants<float>::twoPi;
        float sub1 = std::sin(subPhase) * subharmonicDepth * 0.7f;  // Increased from 0.3f

        // Sub-sub-octave (f/4)
        subPhase2 += (frequency * 0.25f * 2.0f * juce::MathConstants<float>::pi) / sampleRate;
        if (subPhase2 > juce::MathConstants<float>::twoPi)
            subPhase2 -= juce::MathConstants<float>::twoPi;
        float sub2 = std::sin(subPhase2) * subharmonicDepth * 0.4f;  // Increased from 0.15f

        output += sub1 + sub2;
    }

    // B. Apply waveshaping for overtones (X-axis control)
    if (harmonicAmount > 0.01f)
    {
        // Create a single-sample audio block for oversampling
        float* data = &output;
        dsp::AudioBlock<float> block(&data, 1, 1);
        auto oversampledBlock = oversampler->processSamplesUp(block);

        float* oversampledData = oversampledBlock.getChannelPointer(0);
        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
        {
            float sample = oversampledData[i];

            // Asymmetric tanh for even/odd harmonics
            float drive = 1.0f + harmonicAmount * 8.0f;  // Increased from 3.0f
            float asymmetry = harmonicAmount * 0.5f;  // Increased from 0.2f
            float shaped = asymmetricTanh(sample, drive, asymmetry);

            // Add Chebyshev harmonics
            shaped = chebyshevMix(shaped, harmonicAmount);

            oversampledData[i] = shaped;
        }

        oversampler->processSamplesDown(block);
        output = *data;

        // Mix dry/wet (more aggressive effect)
        output = input * (1.0f - harmonicAmount * 0.7f) + output * (0.3f + harmonicAmount * 0.7f);
    }

    lastSample = output;
    return output;
}

// TB303Voice Implementation
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
    harmonicProcessor.prepare(sampleRate, samplesPerBlock);

    envelope.setSampleRate(sampleRate);
    filterEnvelope.setSampleRate(sampleRate);

    filter.setMode(dsp::LadderFilterMode::LPF24);

    polyBLEPPhase = 0.0f;
    lastPhase = 0.0f;
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
    // Square wave now uses PolyBLEP, so we don't need to reinitialize here
}

void TB303Voice::updateHarmonicParameters(float harmonicAmount, float subharmonicDepth)
{
    harmonicProcessor.updateParameters(harmonicAmount, subharmonicDepth);
}

float TB303Voice::polyBLEP(float phase, float phaseInc)
{
    float dt = phaseInc / juce::MathConstants<float>::twoPi;

    // Handle discontinuity at phase = 0
    if (phase < phaseInc)
    {
        float t = phase / phaseInc;
        return 2.0f * t - t * t - 1.0f;
    }
    // Handle discontinuity at phase = π
    else if (phase > juce::MathConstants<float>::pi &&
             lastPhase <= juce::MathConstants<float>::pi)
    {
        float t = (phase - juce::MathConstants<float>::pi) / phaseInc;
        return -(2.0f * t - t * t - 1.0f);
    }

    return 0.0f;
}

float TB303Voice::generatePolyBLEPSquare(float frequency)
{
    float phaseInc = (frequency * juce::MathConstants<float>::twoPi) / sampleRate;

    polyBLEPPhase += phaseInc;
    if (polyBLEPPhase >= juce::MathConstants<float>::twoPi)
        polyBLEPPhase -= juce::MathConstants<float>::twoPi;

    // Generate basic square wave
    float value = polyBLEPPhase < juce::MathConstants<float>::pi ? 1.0f : -1.0f;

    // Add PolyBLEP correction at discontinuities
    value += polyBLEP(polyBLEPPhase, phaseInc);

    lastPhase = polyBLEPPhase;

    // Apply amplitude boost for square wave
    return value * squareWaveBoost;
}


void TB303Voice::startNote(int midiNoteNumber, float velocity,
                            juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    targetFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    if (!isSliding)
    {
        currentFrequency = targetFrequency;
        oscillator.setFrequency(currentFrequency);
        // Reset PolyBLEP phase for square wave
        if (currentWaveform == Waveform::Square)
        {
            polyBLEPPhase = 0.0f;
            lastPhase = 0.0f;
        }
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

        // Generate oscillator sample (with PolyBLEP for square)
        float oscSample;
        if (currentWaveform == Waveform::Square)
        {
            oscSample = generatePolyBLEPSquare(currentFrequency);
        }
        else
        {
            oscSample = oscillator.processSample(0.0f);
        }

        // Apply harmonic processing to add overtones/undertones
        oscSample = harmonicProcessor.process(oscSample, currentFrequency);

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