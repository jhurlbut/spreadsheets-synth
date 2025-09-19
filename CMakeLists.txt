cmake_minimum_required(VERSION 3.15)
project(SpreadsheetsSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Add JUCE as a subdirectory (you'll need to download JUCE and place it in a JUCE folder)
add_subdirectory(JUCE)

juce_add_plugin(SpreadsheetsSynth
    PLUGIN_MANUFACTURER_CODE Sprl
    PLUGIN_CODE Sprd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Spreadsheets Synth"
    COMPANY_NAME "SpreadsheetsLabs"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PLUGIN_NAME "SpreadsheetsSynth"
    PLUGIN_DESC "303-style bass synthesizer with sequencer"
    BUNDLE_ID "com.spreadsheetslabs.spreadsheetssynth")

juce_generate_juce_header(SpreadsheetsSynth)

target_sources(SpreadsheetsSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Synth/TB303Voice.cpp
        Source/Synth/TB303Voice.h
        Source/Synth/TB303Synth.cpp
        Source/Synth/TB303Synth.h
        Source/Sequencer/StepSequencer.cpp
        Source/Sequencer/StepSequencer.h
        Source/Effects/EffectsProcessor.cpp
        Source/Effects/EffectsProcessor.h
        Source/GUI/SpreadsheetsDisplay.cpp
        Source/GUI/SpreadsheetsDisplay.h
        Source/GUI/XYPad.cpp
        Source/GUI/XYPad.h
        Source/GUI/AcidTabButton.cpp
        Source/GUI/AcidTabButton.h
        Source/GUI/CRTShaderOverlay.cpp
        Source/GUI/CRTShaderOverlay.h)

target_compile_definitions(SpreadsheetsSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=1)

target_link_libraries(SpreadsheetsSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)