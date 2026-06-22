# LM-1

A software drum machine modeled on the Linn LM-1, built with JUCE and targeting
**Audio Units (AU)** so it runs natively in **Universal Audio LUNA** on macOS
(plus VST3 and a standalone app from the same codebase).

This repository is the **prototype scaffold**: a buildable, MIDI-playable sample
instrument with 8 voices, a lo-fi "crush" stage, and master/tune controls. It
makes sound out of the box using procedurally generated drum hits (no external
sample files needed). The path from here to a full LM-1 clone is in
[`ROADMAP.md`](ROADMAP.md).

## What works right now

- Builds **AU + VST3 + Standalone** from one CMake project.
- 8 drum voices (kick, snare, closed/open hat, clap, low/mid tom, cowbell),
  triggered by MIDI (General-MIDI drum map) or the on-screen pads.
- Open hat is choked by closed hat.
- Global **Tune** (resampling) and **Lo-Fi** (bit-crush + sample-rate reduction).
- Master gain. State (parameters) saved/restored with the host project.

## Prerequisites (macOS)

1. **Xcode** (full install, from the App Store) + command-line tools:
   ```bash
   xcode-select --install
   ```
2. **CMake 3.22+**:
   ```bash
   brew install cmake
   ```
   (Install Homebrew first from https://brew.sh if you don't have it.)

JUCE itself is pulled automatically by CMake (`FetchContent`) — you don't need to
download it separately. First configure will clone JUCE, so it needs internet and
takes a few minutes once.

## Build

From the project folder:

```bash
# Configure (generates an Xcode-backed build in ./build)
cmake -B build -G Xcode

# Build the plugin (Release)
cmake --build build --config Release
```

Because `COPY_PLUGIN_AFTER_BUILD TRUE` is set, a successful build installs the
plugin into your user folders:

- AU: `~/Library/Audio/Plug-Ins/Components/LM-1.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/LM-1.vst3`

## Validate the AU (do this before opening LUNA)

```bash
auval -v aumu Lm01 Ynme
```

`aumu` = music device (instrument); `Lm01` and `Ynme` are the `PLUGIN_CODE` and
`PLUGIN_MANUFACTURER_CODE` from `CMakeLists.txt`. A clean `auval` pass means the
AU is well-formed and hosts will load it.

## Run it

- **Fastest iteration:** run the **Standalone** app (built in
  `build/LM_One_artefacts/Release/Standalone/`). Enable a MIDI input or click the
  on-screen pads.
- **In LUNA:** create an Instrument track and insert **LM-1** (AU instruments
  are enabled by default in LUNA on macOS). If it doesn't appear, rescan plugins
  in LUNA's settings.

## Project layout

```
LM-1/
├── CMakeLists.txt          # build config: pulls JUCE, defines the plugin
├── README.md               # this file
├── ROADMAP.md              # step-by-step path to the full prototype
└── src.
    ├── PluginProcessor.h/.cpp   # engine, MIDI handling, parameters, lo-fi
    ├── PluginEditor.h/.cpp      # pads + knobs UI
    ├── DrumVoice.h              # one variable-rate sample voice
    └── DrumSynth.h             # procedural placeholder drum sounds
```

## Before sharing the plugin

For the plugin to load on other people's Macs without Gatekeeper warnings it must
be **code-signed (Developer ID) and notarized**. That requires an Apple Developer
account ($99/yr). See the "Distribution" section of `ROADMAP.md`. For your own
machine during development you can skip this.
