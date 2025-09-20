#include "PluginProcessor.h"
#include "PluginEditor.h"

SpreadsheetsSynthProcessor::SpreadsheetsSynthProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

SpreadsheetsSynthProcessor::~SpreadsheetsSynthProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SpreadsheetsSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>("waveform", "Waveform",
        juce::StringArray{"Sawtooth", "Square"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("cutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("resonance", "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Envelope Decay",
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("accent", "Accent Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("overdrive", "Overdrive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time",
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.375f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayMix", "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserRate", "Phaser Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserDepth", "Phaser Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserFeedback", "Phaser Feedback",
        juce::NormalisableRange<float>(-0.95f, 0.95f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserMix", "Phaser Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmonicAmount", "Harmonic Saturation",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("subharmonicDepth", "Subharmonic Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

    return { params.begin(), params.end() };
}

const juce::String SpreadsheetsSynthProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpreadsheetsSynthProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpreadsheetsSynthProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpreadsheetsSynthProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpreadsheetsSynthProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int SpreadsheetsSynthProcessor::getNumPrograms()
{
    return 1;
}

int SpreadsheetsSynthProcessor::getCurrentProgram()
{
    return 0;
}

void SpreadsheetsSynthProcessor::setCurrentProgram (int index)
{
}

const juce::String SpreadsheetsSynthProcessor::getProgramName (int index)
{
    return {};
}

void SpreadsheetsSynthProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void SpreadsheetsSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepareToPlay(sampleRate, samplesPerBlock);
    sequencer.prepareToPlay(sampleRate, samplesPerBlock);
    effectsProcessor.prepareToPlay(sampleRate, samplesPerBlock);
}

void SpreadsheetsSynthProcessor::releaseResources()
{
    synth.releaseResources();
    effectsProcessor.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpreadsheetsSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SpreadsheetsSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateParameters();

    juce::MidiBuffer sequencerMidi;
    sequencer.processBlock(buffer, sequencerMidi, getPlayHead());

    juce::MidiBuffer combinedMidi;
    combinedMidi.addEvents(midiMessages, 0, buffer.getNumSamples(), 0);
    combinedMidi.addEvents(sequencerMidi, 0, buffer.getNumSamples(), 0);

    for (const auto metadata : combinedMidi)
    {
        if (metadata.getMessage().isNoteOn())
        {
            noteTriggered(metadata.getMessage().getNoteNumber());
        }
    }

    synth.processBlock(buffer, combinedMidi);

    effectsProcessor.processBlock(buffer);

    auto masterVolume = apvts.getRawParameterValue("masterVolume")->load();
    buffer.applyGain(masterVolume);
}

void SpreadsheetsSynthProcessor::updateParameters()
{
    synth.updateParameters(apvts);
    effectsProcessor.updateParameters(apvts);
}

void SpreadsheetsSynthProcessor::noteTriggered(int noteNumber)
{
    currentLetterIndex = (currentLetterIndex + 1) % 12;
}

bool SpreadsheetsSynthProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SpreadsheetsSynthProcessor::createEditor()
{
    return new SpreadsheetsSynthEditor (*this);
}

void SpreadsheetsSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SpreadsheetsSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpreadsheetsSynthProcessor();
}