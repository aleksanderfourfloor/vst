#pragma once

#include "PluginProcessor.h"

class MiniSerumAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MiniSerumAudioProcessorEditor (MiniSerumAudioProcessor&);
    ~MiniSerumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class RotaryControl;
    class WaveSelector;

    MiniSerumAudioProcessor& processorRef;

    WaveSelector* osc1WaveSelectorPtr = nullptr;
    WaveSelector* osc2WaveSelectorPtr = nullptr;

    RotaryControl* oscMixControlPtr = nullptr;
    RotaryControl* osc2DetuneControlPtr = nullptr;
    RotaryControl* subLevelControlPtr = nullptr;
    RotaryControl* noiseLevelControlPtr = nullptr;
    RotaryControl* driveControlPtr = nullptr;
    RotaryControl* filterCutoffControlPtr = nullptr;
    RotaryControl* filterResonanceControlPtr = nullptr;
    RotaryControl* attackControlPtr = nullptr;
    RotaryControl* decayControlPtr = nullptr;
    RotaryControl* sustainControlPtr = nullptr;
    RotaryControl* releaseControlPtr = nullptr;
    RotaryControl* masterControlPtr = nullptr;

    std::vector<std::unique_ptr<juce::Component>> controls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniSerumAudioProcessorEditor)
};
