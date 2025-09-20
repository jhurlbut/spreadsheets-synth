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

    // Comb filter removed - harmonics processing now in TB303Voice

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
}