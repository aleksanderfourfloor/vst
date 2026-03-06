# Mini Serum

Mini Serum is a compact JUCE-based software synthesizer plugin intended to load in desktop DAWs such as Ableton Live, FL Studio, Logic Pro, and Reaper through `AU` and `VST3`.

Current voice architecture:

- 12-voice polyphony with MIDI note input
- Two main oscillators with selectable `Sine`, `Saw`, `Square`, and `Triangle` waves
- Oscillator blend and oscillator B detune
- Sine sub oscillator
- Noise layer
- Resonant low-pass filter
- ADSR amplitude envelope
- Drive and master output control

## Local build

This project vendors JUCE in [`extern/JUCE`](/Users/aleksander/Documents/Development/vst/extern/JUCE).

1. Install `cmake` if it is missing:

```bash
brew install cmake
```

2. Configure the Xcode project:

```bash
cmake -S . -B build -G Xcode
```

3. Build the plugin targets:

```bash
cmake --build build --config Release --target MiniSerum_VST3 MiniSerum_AU
```

With `COPY_PLUGIN_AFTER_BUILD` enabled in [`CMakeLists.txt`](/Users/aleksander/Documents/Development/vst/CMakeLists.txt), successful builds copy the plugin bundles into the standard macOS plugin locations so Ableton Live can scan them.
