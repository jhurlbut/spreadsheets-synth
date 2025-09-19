#include "TB303Synth.h"

TB303Synth::TB303Synth()
{
    synth.clearSounds();
    synth.addSound(new TB303Sound());

    for (int i = 0; i < maxVoices; ++i)
    {
        auto* voice = new TB303Voice();
        synth.addVoice(voice);
        voices.push_back(voice);
    }
}

TB303Synth::~TB303Synth()
{
}

void TB303Synth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (auto* voice : voices)
    {
        voice->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void TB303Synth::releaseResources()
{
}

void TB303Synth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

void TB303Synth::updateParameters(juce::AudioProcessorValueTreeState& apvts)
{
    float cutoff = apvts.getRawParameterValue("cutoff")->load();
    float resonance = apvts.getRawParameterValue("resonance")->load();
    float decay = apvts.getRawParameterValue("decay")->load();
    float accent = apvts.getRawParameterValue("accent")->load();
    float overdrive = apvts.getRawParameterValue("overdrive")->load();
    int waveform = apvts.getRawParameterValue("waveform")->load();

    for (auto* voice : voices)
    {
        voice->updateParameters(cutoff, resonance, decay, accent, overdrive, waveform);
    }
}