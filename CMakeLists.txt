cmake_minimum_required(VERSION 3.22)

project(MiniSerum VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

add_subdirectory(extern/JUCE)

juce_add_plugin(MiniSerum
    COMPANY_NAME "Aleksander"
    BUNDLE_ID com.aleksander.miniserum
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Aksr
    PLUGIN_CODE Msrm
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Mini Serum")

juce_generate_juce_header(MiniSerum)

target_sources(MiniSerum
    PRIVATE
        src/PluginEditor.cpp
        src/PluginEditor.h
        src/PluginProcessor.cpp
        src/PluginProcessor.h)

target_compile_definitions(MiniSerum
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MiniSerum
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
