#pragma once

#include <JuceHeader.h>

class EffectsProcessor
{
public:
    EffectsProcessor();
    ~EffectsProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float>& buffer);

    void updateParameters(juce::AudioProcessorValueTreeState& apvts);

private:
    dsp::ProcessorChain<dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::Linear>,
                        dsp::Phaser<float>> effectsChain;

    static constexpr size_t delayIndex = 0;
    static constexpr size_t phaserIndex = 1;

    double sampleRate { 44100.0 };

    float delayTime { 0.375f };
    float delayFeedback { 0.5f };
    float delayMix { 0.3f };

    std::vector<float> delayBuffer;
    int delayBufferSize { 0 };
    int delayWritePosition { 0 };

    // Comb filter arrays for 4 parallel comb filters
    static constexpr int numCombFilters = 4;
    std::array<std::vector<float>, numCombFilters> combBuffers;
    std::array<int, numCombFilters> combBufferSizes;
    std::array<int, numCombFilters> combWritePositions;
    std::array<float, numCombFilters> combFeedback;

    float combFilterX { 0.5f };
    float combFilterY { 0.5f };
    float combFilterMix { 0.3f };

    juce::AudioBuffer<float> dryBuffer;

    void processDelay(juce::AudioBuffer<float>& buffer);
    void processCombFilter(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsProcessor)
};