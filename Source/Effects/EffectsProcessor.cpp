#include "EffectsProcessor.h"

EffectsProcessor::EffectsProcessor()
{
}

EffectsProcessor::~EffectsProcessor()
{
}

void EffectsProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    effectsChain.prepare(spec);

    auto& delay = effectsChain.get<delayIndex>();
    delay.setMaximumDelayInSamples(static_cast<int>(sampleRate * 2.0));

    delayBufferSize = static_cast<int>(sampleRate * 2.0);
    delayBuffer.resize(delayBufferSize * 2);
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    delayWritePosition = 0;

    // Initialize comb filters with different delay times (in ms)
    const float combDelayTimes[numCombFilters] = { 29.7f, 37.1f, 41.1f, 43.7f };

    for (int i = 0; i < numCombFilters; ++i)
    {
        combBufferSizes[i] = static_cast<int>((combDelayTimes[i] / 1000.0f) * sampleRate);
        combBuffers[i].resize(combBufferSizes[i] * 2);
        std::fill(combBuffers[i].begin(), combBuffers[i].end(), 0.0f);
        combWritePositions[i] = 0;
        combFeedback[i] = 0.5f;
    }

    dryBuffer.setSize(2, samplesPerBlock);
}

void EffectsProcessor::releaseResources()
{
    effectsChain.reset();
}

void EffectsProcessor::processBlock(juce::AudioBuffer<float>& buffer)
{
    dryBuffer.makeCopyOf(buffer);

    processDelay(buffer);

    dsp::AudioBlock<float> block(buffer);
    dsp::ProcessContextReplacing<float> context(block);

    auto& phaser = effectsChain.get<phaserIndex>();
    phaser.process(context);

    processCombFilter(buffer);
}

void EffectsProcessor::processDelay(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    int delaySamples = static_cast<int>(delayTime * sampleRate);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        const float* dryData = dryBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            int delayReadPosition = (delayWritePosition - delaySamples + delayBufferSize) % delayBufferSize;
            int bufferIndex = channel * delayBufferSize + delayReadPosition;

            float delaySample = delayBuffer[bufferIndex];

            float inputWithFeedback = dryData[sample] + (delaySample * delayFeedback);

            int writeIndex = channel * delayBufferSize + delayWritePosition;
            delayBuffer[writeIndex] = inputWithFeedback;

            channelData[sample] = dryData[sample] * (1.0f - delayMix) + delaySample * delayMix;
        }
    }

    delayWritePosition = (delayWritePosition + numSamples) % delayBufferSize;
}

void EffectsProcessor::processCombFilter(juce::AudioBuffer<float>& buffer)
{
    if (combFilterMix < 0.01f)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // X controls delay time scaling (0.5 to 2.0)
    float delayScale = 0.5f + combFilterX * 1.5f;

    // Y controls feedback amount (0.3 to 0.95)
    float feedbackAmount = 0.3f + combFilterY * 0.65f;

    juce::AudioBuffer<float> combOutput(numChannels, numSamples);
    combOutput.clear();

    for (int filter = 0; filter < numCombFilters; ++filter)
    {
        int scaledBufferSize = static_cast<int>(combBufferSizes[filter] * delayScale);
        scaledBufferSize = juce::jlimit(1, combBufferSizes[filter] * 2, scaledBufferSize);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float* inputData = buffer.getReadPointer(channel);
            float* outputData = combOutput.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                int readPos = (combWritePositions[filter] - scaledBufferSize + combBufferSizes[filter] * 2) % (combBufferSizes[filter] * 2);
                int bufferIndex = channel * combBufferSizes[filter] * 2 + readPos;

                float delayedSample = combBuffers[filter][bufferIndex % combBuffers[filter].size()];

                float input = inputData[sample];
                float combSample = input + (delayedSample * feedbackAmount);

                int writeIndex = channel * combBufferSizes[filter] * 2 + combWritePositions[filter];
                combBuffers[filter][writeIndex % combBuffers[filter].size()] = combSample;

                outputData[sample] += combSample / numCombFilters;
            }
        }

        combWritePositions[filter] = (combWritePositions[filter] + numSamples) % (combBufferSizes[filter] * 2);
    }

    // Mix comb output with original signal
    for (int channel = 0; channel < numChannels; ++channel)
    {
        buffer.applyGain(channel, 0, numSamples, 1.0f - combFilterMix);
        buffer.addFrom(channel, 0, combOutput, channel, 0, numSamples, combFilterMix);
    }
}

void EffectsProcessor::updateParameters(juce::AudioProcessorValueTreeState& apvts)
{
    delayTime = apvts.getRawParameterValue("delayTime")->load();
    delayFeedback = apvts.getRawParameterValue("delayFeedback")->load();
    delayMix = apvts.getRawParameterValue("delayMix")->load();

    float phaserRate = apvts.getRawParameterValue("phaserRate")->load();
    float phaserDepth = apvts.getRawParameterValue("phaserDepth")->load();
    float phaserFeedback = apvts.getRawParameterValue("phaserFeedback")->load();
    float phaserMix = apvts.getRawParameterValue("phaserMix")->load();

    auto& phaser = effectsChain.get<phaserIndex>();
    phaser.setRate(phaserRate);
    phaser.setDepth(phaserDepth);
    phaser.setFeedback(phaserFeedback);
    phaser.setMix(phaserMix);

    combFilterX = apvts.getRawParameterValue("combFilterX")->load();
    combFilterY = apvts.getRawParameterValue("combFilterY")->load();
    combFilterMix = apvts.getRawParameterValue("combFilterMix")->load();
}