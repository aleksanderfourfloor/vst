#include "PluginEditor.h"

namespace
{
const auto backgroundTop = juce::Colour (0xff0f141a);
const auto backgroundBottom = juce::Colour (0xff171d25);
const auto chrome = juce::Colour (0xff0c1117);
const auto panelTop = juce::Colour (0xff202933);
const auto panelBottom = juce::Colour (0xff171e26);
const auto panelOutline = juce::Colour (0x26ffffff);
const auto panelInnerLine = juce::Colour (0x12000000);
const auto moduleFill = juce::Colour (0xff212a34);
const auto moduleOutline = juce::Colour (0x20ffffff);
const auto graphFill = juce::Colour (0xff10161d);
const auto textStrong = juce::Colour (0xfff2f7fb);
const auto textMuted = juce::Colour (0xffa6b2bf);
const auto textSubtle = juce::Colour (0xff6d7884);
const auto accent = juce::Colour (0xff17c7ff);
const auto accentSoft = juce::Colour (0xff78e0ff);
const auto accentGreen = juce::Colour (0xff76d95d);
const auto knobShadow = juce::Colour (0xbb000000);
const auto knobOuter = juce::Colour (0xff2b3540);
const auto knobFace = juce::Colour (0xff56616d);
const auto selectorFill = juce::Colour (0xff0e141a);
const auto selectorOutline = juce::Colour (0x26ffffff);
const auto valueBadge = juce::Colour (0xff131921);

enum class WaveShape
{
    sine = 0,
    saw,
    square,
    triangle
};

struct EditorLayout
{
    juce::Rectangle<int> header;
    juce::Rectangle<int> oscillatorPanel;
    juce::Rectangle<int> tonePanel;
    juce::Rectangle<int> envelopePanel;
};

WaveShape waveFromIndex (int value)
{
    switch (juce::jlimit (0, 3, value))
    {
        case 0:  return WaveShape::sine;
        case 1:  return WaveShape::saw;
        case 2:  return WaveShape::square;
        default: return WaveShape::triangle;
    }
}

float renderWaveSample (WaveShape shape, float phase)
{
    const auto wrapped = phase - std::floor (phase);
    const auto angle = juce::MathConstants<float>::twoPi * wrapped;

    switch (shape)
    {
        case WaveShape::sine:     return std::sin (angle);
        case WaveShape::saw:      return (2.0f * wrapped) - 1.0f;
        case WaveShape::square:   return wrapped < 0.5f ? 1.0f : -1.0f;
        case WaveShape::triangle: return 1.0f - (4.0f * std::abs (wrapped - 0.5f));
    }

    return 0.0f;
}

juce::String formatTimeValue (double seconds)
{
    if (seconds < 1.0)
        return juce::String (juce::roundToInt (seconds * 1000.0)) + " ms";

    return juce::String (seconds, 2) + " s";
}

juce::String formatParamValue (const juce::String& parameterID, double value)
{
    if (parameterID == "oscMix" || parameterID == "subLevel" || parameterID == "noiseLevel"
        || parameterID == "drive" || parameterID == "sustain" || parameterID == "masterGain")
        return juce::String (juce::roundToInt (value * 100.0)) + "%";

    if (parameterID == "osc2Detune")
        return juce::String (value, 2) + " st";

    if (parameterID == "filterCutoff")
    {
        if (value >= 1000.0)
            return juce::String (value / 1000.0, 2) + " kHz";

        return juce::String (juce::roundToInt (value)) + " Hz";
    }

    if (parameterID == "filterResonance")
        return juce::String (value, 2);

    if (parameterID == "attack" || parameterID == "decay" || parameterID == "release")
        return formatTimeValue (value);

    return juce::String (value, 2);
}

EditorLayout calculateLayout (juce::Rectangle<int> bounds)
{
    EditorLayout layout;
    auto area = bounds.reduced (18);

    layout.header = area.removeFromTop (78);
    area.removeFromTop (12);

    auto topRow = area.removeFromTop (338);
    layout.oscillatorPanel = topRow.removeFromLeft (static_cast<int> (topRow.getWidth() * 0.62f));
    topRow.removeFromLeft (12);
    layout.tonePanel = topRow;

    area.removeFromTop (12);
    layout.envelopePanel = area;

    return layout;
}

void drawPanel (juce::Graphics& g,
                juce::Rectangle<int> bounds,
                const juce::String& title,
                const juce::String& subtitle,
                juce::Colour headerAccent)
{
    auto area = bounds.toFloat();
    juce::ColourGradient fill (panelTop, area.getTopLeft(), panelBottom, area.getBottomLeft(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (area, 15.0f);

    g.setColour (panelOutline);
    g.drawRoundedRectangle (area, 15.0f, 1.0f);

    g.setColour (panelInnerLine);
    g.drawLine (area.getX(), area.getY() + 42.0f, area.getRight(), area.getY() + 42.0f, 1.0f);

    auto headerArea = bounds.reduced (16, 10);
    auto badge = headerArea.removeFromLeft (94).withHeight (20);
    g.setColour (headerAccent.withAlpha (0.15f));
    g.fillRoundedRectangle (badge.toFloat(), 6.0f);
    g.setColour (headerAccent);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("MODULE", badge, juce::Justification::centred);

    headerArea.removeFromLeft (12);
    g.setColour (textStrong);
    g.setFont (juce::FontOptions (15.5f, juce::Font::bold));
    g.drawText (title, headerArea.removeFromTop (20), juce::Justification::centredLeft);

    g.setColour (textSubtle);
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    g.drawText (subtitle, headerArea.removeFromTop (14), juce::Justification::centredLeft);
}

void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> components, int gap)
{
    if (components.size() == 0)
        return;

    const auto width = (area.getWidth() - gap * static_cast<int> (components.size() - 1)) / static_cast<int> (components.size());
    auto x = area.getX();

    for (auto* component : components)
    {
        component->setBounds (x, area.getY(), width, area.getHeight());
        x += width + gap;
    }
}
} // namespace

class MiniSerumAudioProcessorEditor::MinimalLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MinimalLookAndFeel()
    {
        setColour (juce::Label::textColourId, textMuted);
        setColour (juce::ComboBox::backgroundColourId, selectorFill);
        setColour (juce::ComboBox::outlineColourId, selectorOutline);
        setColour (juce::ComboBox::textColourId, textStrong);
        setColour (juce::PopupMenu::backgroundColourId, chrome);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.16f));
        setColour (juce::PopupMenu::highlightedTextColourId, textStrong);
        setColour (juce::PopupMenu::textColourId, textStrong);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                            static_cast<float> (width), static_cast<float> (height)).reduced (10.0f, 6.0f);
        area.removeFromTop (10.0f);
        area.removeFromBottom (26.0f);

        const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto centre = area.getCentre();
        const auto shadowBounds = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ centre.x, centre.y + 3.0f });
        const auto outerBounds = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
        const auto innerBounds = outerBounds.reduced (8.0f);

        g.setColour (knobShadow);
        g.fillEllipse (shadowBounds);

        g.setColour (knobOuter);
        g.fillEllipse (outerBounds);

        juce::Path outerArc;
        outerArc.addCentredArc (centre.x, centre.y, radius - 4.5f, radius - 4.5f, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0x26ffffff));
        g.strokePath (outerArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, radius - 4.5f, radius - 4.5f, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (4.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::ColourGradient faceFill (knobFace.brighter (0.18f), centre.x, outerBounds.getY(),
                                       knobFace.darker (0.30f), centre.x, outerBounds.getBottom(), false);
        g.setGradientFill (faceFill);
        g.fillEllipse (innerBounds);

        g.setColour (juce::Colour (0x20ffffff));
        g.drawEllipse (innerBounds, 1.0f);

        juce::Path tick;
        tick.addRoundedRectangle (-1.8f, -radius * 0.50f, 3.6f, radius * 0.34f, 1.2f);
        tick.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (textStrong);
        g.fillPath (tick);

        g.setColour (juce::Colour (0x16ffffff));
        g.fillEllipse (juce::Rectangle<float> (radius * 0.34f, radius * 0.34f).withCentre (centre));
    }

    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool,
                       int,
                       int,
                       int,
                       int,
                       juce::ComboBox& box) override
    {
        auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (area, 7.0f);

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (area, 7.0f, 1.0f);

        juce::Path arrow;
        const auto cx = static_cast<float> (width - 17);
        const auto cy = static_cast<float> (height * 0.5f);
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.5f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (accentSoft);
        g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return { juce::FontOptions (13.5f, juce::Font::plain) };
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 0, box.getWidth() - 28, box.getHeight());
        label.setFont (juce::FontOptions (13.5f, juce::Font::plain));
        label.setJustificationType (juce::Justification::centredLeft);
    }
};

class MiniSerumAudioProcessorEditor::RotaryControl final : public juce::Component
{
public:
    RotaryControl (juce::AudioProcessorValueTreeState& state,
                   const juce::String& parameterIDToUse,
                   const juce::String& caption)
        : parameterID (parameterIDToUse),
          attachment (state, parameterIDToUse, slider)
    {
        label.setText (caption.toUpperCase(), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, textMuted);
        label.setFont (juce::FontOptions (10.8f, juce::Font::bold));
        addAndMakeVisible (label);

        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setColour (juce::Label::textColourId, textStrong);
        valueLabel.setFont (juce::FontOptions (11.8f, juce::Font::bold));
        valueLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (valueLabel);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::degreesToRadians (220.0f), juce::degreesToRadians (500.0f), true);
        slider.setScrollWheelEnabled (false);
        addAndMakeVisible (slider);

        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (state.getParameter (parameterID)))
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        slider.onValueChange = [this]
        {
            valueLabel.setText (formatParamValue (parameterID, slider.getValue()), juce::dontSendNotification);
        };

        slider.onValueChange();
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (4.0f, 2.0f);
        juce::ColourGradient fill (moduleFill.brighter (0.05f), area.getTopLeft(), moduleFill, area.getBottomLeft(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (area, 12.0f);

        g.setColour (moduleOutline);
        g.drawRoundedRectangle (area, 12.0f, 1.0f);

        auto valueBounds = valueLabel.getBounds().toFloat().expanded (6.0f, 2.0f);
        g.setColour (valueBadge);
        g.fillRoundedRectangle (valueBounds, 6.0f);
        g.setColour (accent.withAlpha (0.20f));
        g.drawRoundedRectangle (valueBounds, 6.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 8);
        label.setBounds (area.removeFromTop (16));
        area.removeFromTop (4);
        valueLabel.setBounds (area.removeFromBottom (24));
        slider.setBounds (area);
    }

private:
    juce::String parameterID;
    juce::Label label;
    juce::Label valueLabel;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

class MiniSerumAudioProcessorEditor::WaveSelector final : public juce::Component
{
public:
    WaveSelector (juce::AudioProcessorValueTreeState& state,
                  const juce::String& parameterIDToUse,
                  const juce::String& titleToUse)
        : parameterID (parameterIDToUse),
          title (titleToUse),
          attachment (state, parameterIDToUse, comboBox)
    {
        titleLabel.setText (title.toUpperCase(), juce::dontSendNotification);
        titleLabel.setJustificationType (juce::Justification::centredLeft);
        titleLabel.setColour (juce::Label::textColourId, textStrong);
        titleLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (titleLabel);

        comboBox.addItemList (juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1);
        comboBox.onChange = [this] { repaint(); };
        addAndMakeVisible (comboBox);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (4.0f);
        g.setColour (moduleFill);
        g.fillRoundedRectangle (area, 12.0f);
        g.setColour (moduleOutline);
        g.drawRoundedRectangle (area, 12.0f, 1.0f);

        auto previewBounds = getLocalBounds().reduced (14, 14);
        previewBounds.removeFromTop (48);
        g.setColour (graphFill);
        g.fillRoundedRectangle (previewBounds.toFloat(), 10.0f);

        g.setColour (juce::Colour (0x18ffffff));
        g.drawRoundedRectangle (previewBounds.toFloat(), 10.0f, 1.0f);

        const auto midY = previewBounds.getCentreY();
        g.setColour (juce::Colour (0x14ffffff));
        g.drawHorizontalLine (midY, static_cast<float> (previewBounds.getX() + 10), static_cast<float> (previewBounds.getRight() - 10));

        for (int i = 1; i < 5; ++i)
        {
            const auto x = previewBounds.getX() + (previewBounds.getWidth() * i) / 5;
            g.setColour (juce::Colour (0x0effffff));
            g.drawVerticalLine (x, static_cast<float> (previewBounds.getY() + 8), static_cast<float> (previewBounds.getBottom() - 8));
        }

        juce::Path wavePath;
        const auto shape = waveFromIndex (comboBox.getSelectedItemIndex());

        for (int i = 0; i < 180; ++i)
        {
            const auto t = static_cast<float> (i) / 179.0f;
            const auto sample = renderWaveSample (shape, t);
            const auto x = previewBounds.getX() + t * static_cast<float> (previewBounds.getWidth());
            const auto y = static_cast<float> (midY) - sample * (previewBounds.getHeight() * 0.34f);

            if (i == 0)
                wavePath.startNewSubPath (x, y);
            else
                wavePath.lineTo (x, y);
        }

        juce::Colour pathColour = title.contains ("A") ? accentGreen : accent;
        g.setColour (pathColour.withAlpha (0.18f));
        g.strokePath (wavePath, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (pathColour);
        g.strokePath (wavePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (14, 10);
        auto topRow = area.removeFromTop (30);
        titleLabel.setBounds (topRow.removeFromLeft (120));
        comboBox.setBounds (topRow.removeFromRight (area.getWidth() > 220 ? 160 : 140).translated (0, 0));
    }

private:
    juce::String parameterID;
    juce::String title;
    juce::Label titleLabel;
    juce::ComboBox comboBox;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;
};

class MiniSerumAudioProcessorEditor::EnvelopeDisplay final : public juce::Component,
                                                             private juce::Timer
{
public:
    explicit EnvelopeDisplay (juce::AudioProcessorValueTreeState& state)
        : attack (*state.getRawParameterValue ("attack")),
          decay (*state.getRawParameterValue ("decay")),
          sustain (*state.getRawParameterValue ("sustain")),
          release (*state.getRawParameterValue ("release"))
    {
        startTimerHz (24);
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (4.0f);
        g.setColour (moduleFill);
        g.fillRoundedRectangle (area, 12.0f);
        g.setColour (moduleOutline);
        g.drawRoundedRectangle (area, 12.0f, 1.0f);

        auto titleArea = getLocalBounds().reduced (16, 12);
        g.setColour (textStrong);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("AMP ENVELOPE", titleArea.removeFromTop (18), juce::Justification::centredLeft);

        g.setColour (textSubtle);
        g.setFont (juce::FontOptions (10.5f, juce::Font::plain));
        g.drawText ("Attack, decay, sustain and release contour", titleArea.removeFromTop (14), juce::Justification::centredLeft);

        auto graph = getLocalBounds().reduced (16, 16);
        graph.removeFromTop (42);
        graph.removeFromBottom (12);

        g.setColour (graphFill);
        g.fillRoundedRectangle (graph.toFloat(), 10.0f);
        g.setColour (juce::Colour (0x16ffffff));
        g.drawRoundedRectangle (graph.toFloat(), 10.0f, 1.0f);

        for (int i = 1; i < 6; ++i)
        {
            const auto x = graph.getX() + (graph.getWidth() * i) / 6;
            g.setColour (juce::Colour (0x0effffff));
            g.drawVerticalLine (x, static_cast<float> (graph.getY() + 8), static_cast<float> (graph.getBottom() - 8));
        }

        for (int i = 1; i < 4; ++i)
        {
            const auto y = graph.getY() + (graph.getHeight() * i) / 4;
            g.setColour (juce::Colour (0x0effffff));
            g.drawHorizontalLine (y, static_cast<float> (graph.getX() + 8), static_cast<float> (graph.getRight() - 8));
        }

        const auto attackSeconds = juce::jmax (0.01f, attack.load());
        const auto decaySeconds = juce::jmax (0.01f, decay.load());
        const auto sustainLevel = juce::jlimit (0.0f, 1.0f, sustain.load());
        const auto releaseSeconds = juce::jmax (0.01f, release.load());

        const auto total = attackSeconds + decaySeconds + 0.35f + releaseSeconds;
        const auto attackX = static_cast<float> (graph.getX()) + graph.getWidth() * (attackSeconds / total);
        const auto decayX = attackX + graph.getWidth() * (decaySeconds / total);
        const auto sustainX = decayX + graph.getWidth() * (0.35f / total);
        const auto bottom = static_cast<float> (graph.getBottom() - 12);
        const auto top = static_cast<float> (graph.getY() + 12);
        const auto sustainY = juce::jmap (sustainLevel, 0.0f, 1.0f, bottom, top);
        const auto right = static_cast<float> (graph.getRight() - 10);
        const auto left = static_cast<float> (graph.getX() + 10);

        juce::Path env;
        env.startNewSubPath (left, bottom);
        env.lineTo (attackX, top);
        env.lineTo (decayX, sustainY);
        env.lineTo (sustainX, sustainY);
        env.lineTo (right, bottom);

        juce::Path fillPath = env;
        fillPath.lineTo (right, bottom);
        fillPath.lineTo (left, bottom);
        fillPath.closeSubPath();

        g.setColour (accent.withAlpha (0.14f));
        g.fillPath (fillPath);

        g.setColour (accent);
        g.strokePath (env, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto drawNode = [&g] (float x, float y)
        {
            g.setColour (backgroundBottom);
            g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ x, y }));
            g.setColour (accentSoft);
            g.drawEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ x, y }), 1.6f);
        };

        drawNode (attackX, top);
        drawNode (decayX, sustainY);
        drawNode (sustainX, sustainY);

        g.setColour (textSubtle);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("A", juce::roundToInt (attackX) - 8, graph.getBottom() - 18, 16, 12, juce::Justification::centred);
        g.drawText ("D", juce::roundToInt (decayX) - 8, graph.getBottom() - 18, 16, 12, juce::Justification::centred);
        g.drawText ("S", juce::roundToInt (sustainX) - 8, graph.getBottom() - 18, 16, 12, juce::Justification::centred);
        g.drawText ("R", graph.getRight() - 18, graph.getBottom() - 18, 16, 12, juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    std::atomic<float>& attack;
    std::atomic<float>& decay;
    std::atomic<float>& sustain;
    std::atomic<float>& release;
};

MiniSerumAudioProcessorEditor::MiniSerumAudioProcessorEditor (MiniSerumAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor),
      processorRef (audioProcessor)
{
    lookAndFeel = std::make_unique<MinimalLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    auto& state = processorRef.getValueTreeState();

    auto addWaveSelector = [this, &state] (WaveSelector*& destination, const juce::String& parameterID, const juce::String& title)
    {
        auto component = std::make_unique<WaveSelector> (state, parameterID, title);
        destination = component.get();
        addAndMakeVisible (*component);
        controls.push_back (std::move (component));
    };

    auto addRotaryControl = [this, &state] (RotaryControl*& destination, const juce::String& parameterID, const juce::String& title)
    {
        auto component = std::make_unique<RotaryControl> (state, parameterID, title);
        destination = component.get();
        addAndMakeVisible (*component);
        controls.push_back (std::move (component));
    };

    auto envelopeDisplay = std::make_unique<EnvelopeDisplay> (state);
    envelopeDisplayPtr = envelopeDisplay.get();
    addAndMakeVisible (*envelopeDisplay);
    controls.push_back (std::move (envelopeDisplay));

    addWaveSelector (osc1WaveSelectorPtr, "osc1Wave", "Oscillator A");
    addWaveSelector (osc2WaveSelectorPtr, "osc2Wave", "Oscillator B");

    addRotaryControl (oscMixControlPtr, "oscMix", "Blend");
    addRotaryControl (osc2DetuneControlPtr, "osc2Detune", "Detune");
    addRotaryControl (subLevelControlPtr, "subLevel", "Sub");
    addRotaryControl (noiseLevelControlPtr, "noiseLevel", "Noise");
    addRotaryControl (driveControlPtr, "drive", "Drive");
    addRotaryControl (filterCutoffControlPtr, "filterCutoff", "Cutoff");
    addRotaryControl (filterResonanceControlPtr, "filterResonance", "Res");
    addRotaryControl (attackControlPtr, "attack", "Attack");
    addRotaryControl (decayControlPtr, "decay", "Decay");
    addRotaryControl (sustainControlPtr, "sustain", "Sustain");
    addRotaryControl (releaseControlPtr, "release", "Release");
    addRotaryControl (masterControlPtr, "masterGain", "Master");

    setOpaque (true);
    setSize (1240, 760);
}

MiniSerumAudioProcessorEditor::~MiniSerumAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MiniSerumAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient backgroundFill (backgroundTop, 0.0f, 0.0f,
                                         backgroundBottom, 0.0f, static_cast<float> (getHeight()), false);
    backgroundFill.addColour (0.55, juce::Colour (0xff111821));
    g.setGradientFill (backgroundFill);
    g.fillAll();

    const auto layout = calculateLayout (getLocalBounds());

    auto topBar = layout.header;
    g.setColour (chrome);
    g.fillRoundedRectangle (topBar.toFloat(), 14.0f);
    g.setColour (panelOutline);
    g.drawRoundedRectangle (topBar.toFloat(), 14.0f, 1.0f);

    auto logoArea = topBar.reduced (18, 12);
    auto rightArea = logoArea.removeFromRight (320);

    g.setColour (accent.withAlpha (0.18f));
    g.fillRoundedRectangle (juce::Rectangle<float> (static_cast<float> (logoArea.getX()),
                                                    static_cast<float> (logoArea.getY() + 3),
                                                    124.0f,
                                                    24.0f),
                            6.0f);
    g.setColour (textStrong);
    g.setFont (juce::FontOptions (32.0f, juce::Font::bold));
    g.drawText ("Mini Serum", logoArea.getX(), logoArea.getY() + 18, 240, 28, juce::Justification::centredLeft);

    g.setColour (accent);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("SYNTH", logoArea.getX() + 12, logoArea.getY() + 6, 90, 16, juce::Justification::centredLeft);

    auto presetBar = juce::Rectangle<int> (360, topBar.getY() + 15, 500, 48);
    g.setColour (juce::Colour (0xff141c24));
    g.fillRoundedRectangle (presetBar.toFloat(), 9.0f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawRoundedRectangle (presetBar.toFloat(), 9.0f, 1.0f);

    g.setColour (textSubtle);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("PATCH", presetBar.getX() + 16, presetBar.getY() + 8, 48, 12, juce::Justification::centredLeft);
    g.setColour (accentSoft);
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText ("Init Glow", presetBar.getX() + 16, presetBar.getY() + 20, 160, 20, juce::Justification::centredLeft);
    g.setColour (textSubtle);
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    g.drawText ("Dual oscillator sketch synth", presetBar.getX() + 180, presetBar.getY() + 22, 220, 16, juce::Justification::centredLeft);

    auto tabArea = juce::Rectangle<int> (presetBar.getRight() + 12, presetBar.getY(), 176, 48);
    const juce::StringArray tabs { "OSC", "FILTER", "ENV" };
    const auto tabWidth = (tabArea.getWidth() - 8) / 3;

    for (int tabIndex = 0; tabIndex < tabs.size(); ++tabIndex)
    {
        const auto x = tabArea.getX() + tabIndex * (tabWidth + 4);
        auto tab = juce::Rectangle<int> (x, tabArea.getY(), tabWidth, tabArea.getHeight());
        const auto active = tabIndex == 0;

        g.setColour (active ? accent.withAlpha (0.16f) : juce::Colour (0xff141c24));
        g.fillRoundedRectangle (tab.toFloat(), 8.0f);
        g.setColour (active ? accent.withAlpha (0.36f) : juce::Colour (0x18ffffff));
        g.drawRoundedRectangle (tab.toFloat(), 8.0f, 1.0f);

        g.setColour (active ? accentSoft : textSubtle);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (tabs[tabIndex], tab, juce::Justification::centred);
    }

    auto status = rightArea.withTrimmedTop (2).withTrimmedBottom (2);
    g.setColour (juce::Colour (0xff141c24));
    g.fillRoundedRectangle (status.toFloat(), 10.0f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawRoundedRectangle (status.toFloat(), 10.0f, 1.0f);

    g.setColour (textSubtle);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("POLY", status.getX() + 16, status.getY() + 10, 40, 14, juce::Justification::centredLeft);
    g.drawText ("MIDI", status.getX() + 98, status.getY() + 10, 40, 14, juce::Justification::centredLeft);
    g.drawText ("OUT", status.getX() + 178, status.getY() + 10, 40, 14, juce::Justification::centredLeft);

    g.setColour (textStrong);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("12 voices", status.getX() + 16, status.getY() + 25, 70, 18, juce::Justification::centredLeft);
    g.drawText ("Input", status.getX() + 98, status.getY() + 25, 50, 18, juce::Justification::centredLeft);
    g.drawText ("Stereo", status.getX() + 178, status.getY() + 25, 60, 18, juce::Justification::centredLeft);

    drawPanel (g, layout.oscillatorPanel, "Oscillator Core", "Wave selection, blending and detune.", accentGreen);
    drawPanel (g, layout.tonePanel, "Tone", "Shape body, noise and filter edge.", accent);
    drawPanel (g, layout.envelopePanel, "Amplitude", "Contour and output level.", accentSoft);
}

void MiniSerumAudioProcessorEditor::resized()
{
    const auto layout = calculateLayout (getLocalBounds());

    auto oscillatorContent = layout.oscillatorPanel.reduced (16, 16);
    oscillatorContent.removeFromTop (52);

    auto oscTop = oscillatorContent.removeFromTop (176);
    auto leftOsc = oscTop.removeFromLeft ((oscTop.getWidth() - 12) / 2);
    oscTop.removeFromLeft (12);
    osc1WaveSelectorPtr->setBounds (leftOsc);
    osc2WaveSelectorPtr->setBounds (oscTop);

    oscillatorContent.removeFromTop (12);
    auto blendRow = oscillatorContent.removeFromTop (132);
    layoutRow (blendRow.withTrimmedLeft (80).withTrimmedRight (80), { oscMixControlPtr, osc2DetuneControlPtr }, 18);

    auto toneContent = layout.tonePanel.reduced (16, 16);
    toneContent.removeFromTop (52);
    auto toneTop = toneContent.removeFromTop (126);
    layoutRow (toneTop, { subLevelControlPtr, noiseLevelControlPtr, driveControlPtr }, 12);
    toneContent.removeFromTop (12);
    auto toneBottom = toneContent.removeFromTop (126);
    layoutRow (toneBottom.withTrimmedLeft (38).withTrimmedRight (38), { filterCutoffControlPtr, filterResonanceControlPtr }, 12);

    auto envelopeContent = layout.envelopePanel.reduced (16, 16);
    envelopeContent.removeFromTop (52);

    auto displayArea = envelopeContent.removeFromLeft (380);
    if (envelopeDisplayPtr != nullptr)
        envelopeDisplayPtr->setBounds (displayArea);

    envelopeContent.removeFromLeft (14);

    auto envKnobArea = envelopeContent.removeFromTop (132);
    layoutRow (envKnobArea, { attackControlPtr, decayControlPtr, sustainControlPtr, releaseControlPtr }, 12);

    envelopeContent.removeFromTop (14);
    masterControlPtr->setBounds (envelopeContent.removeFromTop (132).withTrimmedLeft (envKnobArea.getWidth() - 152));
}
