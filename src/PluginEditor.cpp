#include "PluginEditor.h"

namespace
{
const auto backgroundTop = juce::Colour (0xff09111f);
const auto backgroundBottom = juce::Colour (0xff03060b);
const auto panelFill = juce::Colour (0xff111c2d);
const auto panelOutline = juce::Colour (0x30f5f7ff);
const auto accent = juce::Colour (0xff55e3ff);
const auto accentWarm = juce::Colour (0xffff9654);
const auto textStrong = juce::Colour (0xfff4f7fb);
const auto textMuted = juce::Colour (0xff8fa4be);
} // namespace

class MiniSerumAudioProcessorEditor::RotaryControl final : public juce::Component
{
public:
    RotaryControl (juce::AudioProcessorValueTreeState& state,
                   const juce::String& parameterID,
                   const juce::String& caption)
        : attachment (state, parameterID, slider)
    {
        addAndMakeVisible (label);
        label.setText (caption, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, textMuted);
        label.setFont (juce::FontOptions (12.5f, juce::Font::bold));

        addAndMakeVisible (slider);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 18);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x40ffffff));
        slider.setColour (juce::Slider::thumbColourId, accentWarm);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxTextColourId, textStrong);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x18000000));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromTop (18));
        slider.setBounds (area);
    }

private:
    juce::Label label;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

class MiniSerumAudioProcessorEditor::WaveSelector final : public juce::Component
{
public:
    WaveSelector (juce::AudioProcessorValueTreeState& state,
                  const juce::String& parameterID,
                  const juce::String& caption)
        : attachment (state, parameterID, comboBox)
    {
        addAndMakeVisible (label);
        label.setText (caption, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setColour (juce::Label::textColourId, textMuted);
        label.setFont (juce::FontOptions (12.5f, juce::Font::bold));

        addAndMakeVisible (comboBox);
        comboBox.addItemList (juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1);
        comboBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x22000000));
        comboBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x35ffffff));
        comboBox.setColour (juce::ComboBox::textColourId, textStrong);
        comboBox.setColour (juce::ComboBox::arrowColourId, accentWarm);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromTop (18));
        comboBox.setBounds (area.removeFromTop (30));
    }

private:
    juce::Label label;
    juce::ComboBox comboBox;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;
};

MiniSerumAudioProcessorEditor::MiniSerumAudioProcessorEditor (MiniSerumAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor),
      processorRef (audioProcessor)
{
    auto& state = processorRef.getValueTreeState();

    auto addWaveSelector = [this, &state] (WaveSelector*& destination, const juce::String& paramID, const juce::String& caption)
    {
        auto control = std::make_unique<WaveSelector> (state, paramID, caption);
        destination = control.get();
        addAndMakeVisible (*control);
        controls.push_back (std::move (control));
    };

    auto addRotaryControl = [this, &state] (RotaryControl*& destination, const juce::String& paramID, const juce::String& caption)
    {
        auto control = std::make_unique<RotaryControl> (state, paramID, caption);
        destination = control.get();
        addAndMakeVisible (*control);
        controls.push_back (std::move (control));
    };

    addWaveSelector (osc1WaveSelectorPtr, "osc1Wave", "Oscillator A");
    addWaveSelector (osc2WaveSelectorPtr, "osc2Wave", "Oscillator B");

    addRotaryControl (oscMixControlPtr, "oscMix", "Blend");
    addRotaryControl (osc2DetuneControlPtr, "osc2Detune", "Detune");
    addRotaryControl (subLevelControlPtr, "subLevel", "Sub");
    addRotaryControl (noiseLevelControlPtr, "noiseLevel", "Noise");
    addRotaryControl (driveControlPtr, "drive", "Drive");
    addRotaryControl (filterCutoffControlPtr, "filterCutoff", "Cutoff");
    addRotaryControl (filterResonanceControlPtr, "filterResonance", "Reso");
    addRotaryControl (attackControlPtr, "attack", "Attack");
    addRotaryControl (decayControlPtr, "decay", "Decay");
    addRotaryControl (sustainControlPtr, "sustain", "Sustain");
    addRotaryControl (releaseControlPtr, "release", "Release");
    addRotaryControl (masterControlPtr, "masterGain", "Master");

    setOpaque (true);
    setSize (860, 470);
}

MiniSerumAudioProcessorEditor::~MiniSerumAudioProcessorEditor() = default;

void MiniSerumAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient backgroundGradient (backgroundTop, 0.0f, 0.0f, backgroundBottom, 0.0f, static_cast<float> (getHeight()), false);
    backgroundGradient.addColour (0.55, juce::Colour (0xff0e1830));
    g.setGradientFill (backgroundGradient);
    g.fillAll();

    auto drawPanel = [&g] (juce::Rectangle<float> area, const juce::String& title, const juce::String& subtitle)
    {
        g.setColour (panelFill);
        g.fillRoundedRectangle (area, 18.0f);

        g.setColour (panelOutline);
        g.drawRoundedRectangle (area, 18.0f, 1.0f);

        auto header = area.reduced (18.0f, 16.0f);
        g.setColour (textStrong);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (title, header.removeFromTop (24.0f), juce::Justification::centredLeft);

        g.setColour (textMuted);
        g.setFont (juce::FontOptions (12.0f, juce::Font::plain));
        g.drawText (subtitle, header.removeFromTop (18.0f), juce::Justification::centredLeft);
    };

    g.setColour (accent.withAlpha (0.18f));
    g.fillRoundedRectangle (juce::Rectangle<float> (24.0f, 22.0f, 220.0f, 42.0f), 18.0f);
    g.setColour (textStrong);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("Mini Serum", 34, 24, 240, 32, juce::Justification::centredLeft);

    g.setColour (textMuted);
    g.setFont (juce::FontOptions (13.0f, juce::Font::plain));
    g.drawText ("Two oscillators, sub, noise, filter and amp envelope", 36, 58, 400, 18, juce::Justification::centredLeft);

    drawPanel ({ 22.0f, 96.0f, 268.0f, 352.0f }, "Oscillators", "Choose shapes, blend both engines and detune oscillator B.");
    drawPanel ({ 304.0f, 96.0f, 272.0f, 352.0f }, "Tone", "Dial in body, bite and filter contour.");
    drawPanel ({ 590.0f, 96.0f, 248.0f, 352.0f }, "Amp", "Shape the note and keep output under control.");
}

void MiniSerumAudioProcessorEditor::resized()
{
    const auto oscillatorArea = juce::Rectangle<int> (34, 148, 244, 282);
    const auto toneArea = juce::Rectangle<int> (316, 148, 248, 282);
    const auto ampArea = juce::Rectangle<int> (602, 148, 224, 282);

    auto layoutTwoColumn = [] (juce::Rectangle<int> area, std::initializer_list<juce::Component*> components)
    {
        const auto columnWidth = (area.getWidth() - 14) / 2;
        const auto rowHeight = 108;
        auto index = 0;

        for (auto* component : components)
        {
            const auto row = index / 2;
            const auto column = index % 2;
            component->setBounds (area.getX() + column * (columnWidth + 14),
                                  area.getY() + row * (rowHeight + 10),
                                  columnWidth,
                                  rowHeight);
            ++index;
        }
    };

    layoutTwoColumn (oscillatorArea,
                     { osc1WaveSelectorPtr, osc2WaveSelectorPtr, oscMixControlPtr, osc2DetuneControlPtr });

    layoutTwoColumn (toneArea,
                     { subLevelControlPtr, noiseLevelControlPtr, driveControlPtr, filterCutoffControlPtr, filterResonanceControlPtr });

    layoutTwoColumn (ampArea,
                     { attackControlPtr, decayControlPtr, sustainControlPtr, releaseControlPtr, masterControlPtr });
}
