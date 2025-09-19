#pragma once

#include <JuceHeader.h>
#include "TB303Voice.h"

class TB303Sound : public juce::SynthesiserSound
{
public:
    TB303Sound() {}

    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class TB303Synth
{
public:
    TB303Synth();
    ~TB303Synth();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    void updateParameters(juce::AudioProcessorValueTreeState& apvts);

private:
    static constexpr int maxVoices = 1;

    juce::Synthesiser synth;
    std::vector<TB303Voice*> voices;
};