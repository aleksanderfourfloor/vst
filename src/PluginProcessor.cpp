#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
enum class WaveShape
{
    sine = 0,
    saw,
    square,
    triangle
};

WaveShape waveFromParameter (float value)
{
    switch (juce::jlimit (0, 3, juce::roundToInt (value)))
    {
        case 0:  return WaveShape::sine;
        case 1:  return WaveShape::saw;
        case 2:  return WaveShape::square;
        default: return WaveShape::triangle;
    }
}

float renderWaveSample (WaveShape shape, float phase)
{
    const auto wrappedPhase = phase - std::floor (phase);
    const auto angle = juce::MathConstants<float>::twoPi * wrappedPhase;

    switch (shape)
    {
        case WaveShape::sine:
            return std::sin (angle);
        case WaveShape::saw:
            return (2.0f * wrappedPhase) - 1.0f;
        case WaveShape::square:
            return wrappedPhase < 0.5f ? 1.0f : -1.0f;
        case WaveShape::triangle:
            return 1.0f - (4.0f * std::abs (wrappedPhase - 0.5f));
    }

    return 0.0f;
}

class SynthSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice final : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice (juce::AudioProcessorValueTreeState& stateToUse)
        : state (stateToUse)
    {
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void prepare (double newSampleRate, int samplesPerBlock)
    {
        sampleRate = newSampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
        spec.numChannels = 1;

        filter.reset();
        filter.prepare (spec);
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

        adsr.setSampleRate (sampleRate);
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        level = velocity;
        baseFrequency = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));

        osc1Phase = random.nextFloat();
        osc2Phase = random.nextFloat();
        subPhase = random.nextFloat();

        filter.reset();
        adsr.reset();
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! isVoiceActive() || sampleRate <= 0.0)
            return;

        updateParameters();

        const auto totalOutputChannels = outputBuffer.getNumChannels();

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            const auto envelopeSample = adsr.getNextSample();

            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }

            const auto osc1 = nextOscillatorSample (osc1Wave, baseFrequency, osc1Phase);
            const auto osc2 = nextOscillatorSample (osc2Wave,
                                                    baseFrequency * std::pow (2.0f, osc2DetuneSemitones / 12.0f),
                                                    osc2Phase);
            const auto sub = nextOscillatorSample (WaveShape::sine, baseFrequency * 0.5f, subPhase);
            const auto noise = random.nextFloat() * 2.0f - 1.0f;

            auto sample = juce::jmap (oscMix, osc1, osc2);
            sample += subLevel * sub;
            sample += noiseLevel * noise;
            sample /= 1.0f + subLevel + noiseLevel;

            sample = std::tanh (sample * (1.0f + (drive * 5.0f)));
            sample = filter.processSample (0, sample);
            sample *= envelopeSample * level * masterGain * 0.8f;

            for (int channel = 0; channel < totalOutputChannels; ++channel)
                outputBuffer.addSample (channel, startSample + sampleIndex, sample);
        }
    }

private:
    void updateParameters()
    {
        osc1Wave = waveFromParameter (loadParam ("osc1Wave"));
        osc2Wave = waveFromParameter (loadParam ("osc2Wave"));
        oscMix = loadParam ("oscMix");
        osc2DetuneSemitones = loadParam ("osc2Detune");
        subLevel = loadParam ("subLevel");
        noiseLevel = loadParam ("noiseLevel");
        drive = loadParam ("drive");
        masterGain = loadParam ("masterGain");

        juce::ADSR::Parameters env;
        env.attack = loadParam ("attack");
        env.decay = loadParam ("decay");
        env.sustain = loadParam ("sustain");
        env.release = loadParam ("release");
        adsr.setParameters (env);

        filter.setCutoffFrequency (loadParam ("filterCutoff"));
        filter.setResonance (loadParam ("filterResonance"));
    }

    float loadParam (const juce::String& parameterID) const
    {
        jassert (state.getRawParameterValue (parameterID) != nullptr);
        return state.getRawParameterValue (parameterID)->load();
    }

    float nextOscillatorSample (WaveShape shape, float frequency, float& phase)
    {
        const auto sample = renderWaveSample (shape, phase);
        phase += frequency / static_cast<float> (sampleRate);

        if (phase >= 1.0f)
            phase -= std::floor (phase);

        return sample;
    }

    juce::AudioProcessorValueTreeState& state;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::ADSR adsr;
    juce::Random random;

    double sampleRate = 44100.0;
    float baseFrequency = 220.0f;
    float level = 0.0f;

    float osc1Phase = 0.0f;
    float osc2Phase = 0.0f;
    float subPhase = 0.0f;

    WaveShape osc1Wave = WaveShape::saw;
    WaveShape osc2Wave = WaveShape::square;
    float oscMix = 0.5f;
    float osc2DetuneSemitones = 0.0f;
    float subLevel = 0.15f;
    float noiseLevel = 0.0f;
    float drive = 0.0f;
    float masterGain = 0.7f;
};
} // namespace

MiniSerumAudioProcessor::MiniSerumAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    synth.setNoteStealingEnabled (true);

    for (int voiceIndex = 0; voiceIndex < 12; ++voiceIndex)
        synth.addVoice (new SynthVoice (parameters));

    synth.addSound (new SynthSound());
}

MiniSerumAudioProcessor::~MiniSerumAudioProcessor() = default;

void MiniSerumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int voiceIndex = 0; voiceIndex < synth.getNumVoices(); ++voiceIndex)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (voiceIndex)))
            voice->prepare (sampleRate, samplesPerBlock);
}

void MiniSerumAudioProcessor::releaseResources()
{
}

bool MiniSerumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MiniSerumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* MiniSerumAudioProcessor::createEditor()
{
    return new MiniSerumAudioProcessorEditor (*this);
}

bool MiniSerumAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MiniSerumAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MiniSerumAudioProcessor::acceptsMidi() const
{
    return true;
}

bool MiniSerumAudioProcessor::producesMidi() const
{
    return false;
}

bool MiniSerumAudioProcessor::isMidiEffect() const
{
    return false;
}

double MiniSerumAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MiniSerumAudioProcessor::getNumPrograms()
{
    return 1;
}

int MiniSerumAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MiniSerumAudioProcessor::setCurrentProgram (int)
{
}

const juce::String MiniSerumAudioProcessor::getProgramName (int)
{
    return {};
}

void MiniSerumAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void MiniSerumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto stateXml = parameters.copyState().createXml())
        copyXmlToBinary (*stateXml, destData);
}

void MiniSerumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto stateXml = getXmlFromBinary (data, sizeInBytes))
        parameters.replaceState (juce::ValueTree::fromXml (*stateXml));
}

juce::AudioProcessorValueTreeState& MiniSerumAudioProcessor::getValueTreeState() noexcept
{
    return parameters;
}

juce::AudioProcessorValueTreeState::ParameterLayout MiniSerumAudioProcessor::createParameterLayout()
{
    using Parameter = std::unique_ptr<juce::RangedAudioParameter>;

    std::vector<Parameter> parameters;
    parameters.reserve (13);

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> ("osc1Wave", "Osc 1 Wave", juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> ("osc2Wave", "Osc 2 Wave", juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 2));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("oscMix", "Osc Mix", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Detune", "Osc 2 Detune", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.08f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("subLevel", "Sub Level", juce::NormalisableRange<float> (0.0f, 1.0f), 0.18f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("noiseLevel", "Noise Level", juce::NormalisableRange<float> (0.0f, 1.0f), 0.04f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("drive", "Drive", juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("filterCutoff", "Filter Cutoff", juce::NormalisableRange<float> (40.0f, 18000.0f, 0.0f, 0.28f), 7200.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("filterResonance", "Filter Resonance", juce::NormalisableRange<float> (0.3f, 1.4f, 0.001f), 0.75f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("attack", "Attack", juce::NormalisableRange<float> (0.001f, 3.0f, 0.0f, 0.35f), 0.02f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("decay", "Decay", juce::NormalisableRange<float> (0.01f, 3.0f, 0.0f, 0.35f), 0.18f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("sustain", "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.72f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release", juce::NormalisableRange<float> (0.02f, 5.0f, 0.0f, 0.35f), 0.32f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> ("masterGain", "Master", juce::NormalisableRange<float> (0.0f, 1.0f), 0.72f));

    return { parameters.begin(), parameters.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiniSerumAudioProcessor();
}
