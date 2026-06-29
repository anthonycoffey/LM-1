# Nixie — Design Document & Project Plan

A plan for building a software drum machine modeled on the **Linn LM-1 Drum Computer (1980)**, packaged as an instrument plugin that runs natively in **Universal Audio LUNA on macOS**.

**Name:** _Nixie_
**Target fidelity:** Hybrid — authentic LM-1 workflow and lo-fi character, with user-loadable samples
**Primary format:** Audio Unit (AU) instrument; VST3 added later from the same codebase

---

> ## ⚑ Status — shipped (currently v0.4.1)
>
> This is the **original architecture & design reference**. The project has since been
> **built and shipped**, so the phased plan further down is essentially complete: 12
> voices with per-voice sampling, a host-synced step sequencer with **real meters +
> triplet/shuffle grids**, a **100-pattern genre library across 12 banks**, a per-voice
> mixer with **AU multi-out**, MIDI drag-out, the lo-fi character, and vintage styling.
>
> Read this doc for the **why** (the rationale still holds). For the current feature set
> read [README.md](README.md); for status and per-release history read
> [ROADMAP.md](ROADMAP.md) and [CHANGELOG.md](CHANGELOG.md).
>
> A few priorities shifted during the build and live in README/ROADMAP rather than being
> rewritten below: the **sequencer + MIDI export** became the core; **shuffle and
> real-time record** were implemented (not omitted as "charm"); pattern slots grew into a
> **12-bank, 100-groove library**; **multi-out** shipped. Where the text below reads as a
> to-do or implies a faithful hardware recreation, treat it as **historical** — the
> current truth is in README/ROADMAP.

---

## 1. Feasibility summary

**How hard is this, honestly?** Moderate, and very achievable for a focused build. The reasons it is _not_ as hard as a typical synth plugin:

- The LM-1 is a **sample-playback** machine, not a synthesizer. The core audio engine is "play a short sample back at a variable rate." There is no oscillator/filter/envelope DSP to model.
- The original hardware was deliberately primitive: 12 voices, 8-bit samples, a step sequencer, per-voice tuning and level. The whole feature set is small and well-documented.

The parts that take real care are not LM-1-specific — they are the standard "shipping a Mac audio plugin" problems: real-time-safe audio code, AU bus/validation requirements, sample-accurate sequencer timing against the host, and macOS code-signing/notarization so it loads cleanly. None are hard individually; together they are where the time goes.

### The one premise to correct up front

LUNA does **not** load "VSTi" plugins — that term is the old VST2 instrument format, which LUNA does not support. On macOS, LUNA loads **Audio Units (AU) natively, enabled by default**, and **VST3** if you manually enable it. So the correct target is **AU first** (zero-friction in LUNA), with VST3 as a cross-platform bonus.

### Why JUCE / C++

You were right that a "powerful language" is needed — plugin audio engines run in a hard real-time thread and are almost universally **C++**. **JUCE** is the standard open-source C++ framework for this: from one codebase it compiles **AU, AUv3, VST3, AAX, LV2, and a standalone app**. It handles the plugin formats, parameter system, GUI, and host sync, so you write the drum machine once and get the AU for LUNA plus a VST3 for everything else.

---

## 2. What the LM-1 actually was (the spec we're emulating)

| Aspect        | LM-1 hardware                                                                                                         | What we replicate                                           |
| ------------- | --------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- |
| Voices        | 12 sampled drums                                                                                                      | 12 lanes (configurable)                                     |
| Sounds        | kick, snare, hi-hat, cabasa, tambourine, 2 toms, 2 congas, cowbell, claves, hand claps (famously **no crash cymbal**) | same default voice set                                      |
| Sample format | 8-bit PCM, ~28 kHz, AM6070 companding DAC                                                                             | optional 8-bit "crush" + companding mode for authentic grit |
| Tuning        | individual tuning pot per voice (changes playback rate/pitch)                                                         | per-voice tune knob via variable-rate playback              |
| Sequencer     | Z80 CPU, **48 PPQ**, 12-track step sequencer                                                                          | step sequencer synced to host, ≥48 PPQ internal resolution  |
| Swing         | "Shuffle" — 6 swing levels from straight to full shuffle                                                              | swing control (continuous, with 6 classic presets)          |
| Mixing        | per-voice level + pan, individual outputs                                                                             | per-voice level/pan + AU multi-out buses                    |

Sourcing note: the original 8-bit ROM samples are copyrighted and their provenance online is murky. The **Hybrid** approach ships our own clean/CC-licensed samples plus user sample loading, and recreates the _character_ (8-bit crush, companding, tuning aliasing) in DSP. That gets the vibe without the legal risk. You can later point it at authentic-style sample packs if you obtain them legitimately.

---

## 3. Architecture

Standard JUCE plugin split: a real-time **processor** and a **UI editor**, plus supporting engine modules.

```
Nixie (JUCE Audio Plug-In)
│
├── PluginProcessor  (audio thread — real-time safe)
│   ├── VoiceEngine          12 sampler voices
│   │   ├── SamplerVoice      variable-rate playback + interpolation
│   │   ├── LoFiStage         optional 8-bit crush + μ-law companding
│   │   └── ChokeGroups       open/closed hi-hat mutual cutoff
│   ├── Sequencer            pattern clock + step playback
│   │   ├── PatternModel      steps × lanes, velocity/accent, swing
│   │   └── HostSync          reads host playhead (tempo, position, play state)
│   ├── MidiHandler          GM drum-map note-in → trigger voices
│   ├── Parameters (APVTS)   all knobs/steps as automatable params
│   └── State                save/restore patterns + settings
│
├── PluginEditor  (message thread — GUI)
│   ├── StepGrid             16-step lanes per voice
│   ├── Transport            play/stop, tempo display, swing
│   ├── VoiceStrip ×12       tune / level / pan / mute / solo
│   ├── PatternBar           pattern slots + chaining (songs)
│   └── SampleBrowser        load user samples per voice
│
└── Resources               bundled default samples (BinaryData)
```

### Audio engine (the core)

Each voice is a one-shot sampler. **Tuning** is implemented by reading the sample at a variable rate (a phase increment), with interpolation (linear to start; cubic/sinc later for quality). The **LoFiStage** is what gives the LM-1 feel: downsample/bit-reduce to 8-bit and apply μ-law-style companding, which is exactly what the AM6070 DAC did. Make it a toggle + amount so you can dial from "clean" to "vintage." **Choke groups** handle the open-hat-cut-by-closed-hat behavior.

Hard rule: the audio callback does **no memory allocation, no locks, no file I/O**. Sample loading and pattern edits happen on other threads and are handed to the audio thread via lock-free messaging (JUCE provides the primitives).

### Sequencer & host sync

The LM-1's identity is its step sequencer. As a plugin, it should:

- Run from the **host transport** — read LUNA's playhead for tempo, song position, and play/stop, so patterns lock to the project and bounce correctly.
- Convert song position → step index at the chosen resolution, applying **swing** (delay every other 16th).
- Optionally also accept **MIDI notes** (GM drum map) so you can play/program voices from LUNA's piano roll or a pad controller, bypassing the internal sequencer.

This dual mode (internal step sequencer _and_ MIDI-playable) is the most useful design and not much extra work.

### Parameters & state

Everything that moves uses JUCE's **AudioProcessorValueTreeState (APVTS)** so it is host-automatable and consistently saved. Pattern data (the grid, velocities, swing, pattern chain) is serialized into the plugin state so projects reopen exactly as left.

### AU specifics (the LUNA-critical details)

- **Multi-out:** the LM-1 had individual outputs; we expose AU output buses (e.g., a stereo main + individual voice outs) configured via `isBusesLayoutSupported`. Keep a simple stereo layout working first — multi-out is a known source of AU validation friction.
- **auval:** AU plugins must pass Apple's `auval` validator before hosts (including LUNA) will load them. Build to pass `auval` early and keep it passing.
- **Signing/notarization:** for it to load on any Mac without Gatekeeper warnings, the plugin must be **code-signed with a Developer ID and notarized** (notarize the outer dmg/pkg, then staple). For your own machine during development this can be deferred; for sharing/distribution it's required.

---

## 4. Feature set (MVP → later)

**MVP (must-have for a usable Nixie in LUNA)**

- 12 voices with bundled default samples
- 16-step grid sequencer, host-tempo synced, with swing
- Per-voice tune, level, pan, mute/solo
- 8-bit "vintage" lo-fi toggle
- Open/closed hi-hat choke group
- MIDI note triggering (GM drum map)
- State save/load; passes `auval`; loads in LUNA

**Phase 2**

- User sample loading per voice
- Multiple pattern slots + pattern chaining (song mode)
- Per-step velocity/accent
- AU multi-output buses (individual voice outs)
- VST3 build

**Nice-to-have / later**

- Per-voice decay/length, reverse, simple distortion
- Pattern import/export, preset kits
- Higher-quality interpolation; selectable companding curves
- AUv3 (for cross-app/iOS), standalone app

---

## 5. Tech stack & tooling

| Concern        | Choice                                                                        |
| -------------- | ----------------------------------------------------------------------------- |
| Language       | C++17/20                                                                      |
| Framework      | JUCE (latest 8.x)                                                             |
| Build          | CMake (recommended over Projucer for reproducible/CI builds)                  |
| IDE/toolchain  | Xcode (required for AU + AUv3 on macOS)                                       |
| Plugin formats | AU first; VST3 next; standalone for fast iteration                            |
| Testing        | `auval` (AU validation), `pluginval` (cross-format stress test), test in LUNA |
| Distribution   | `codesign` (Developer ID) + `notarytool` + `stapler`; package as dmg/pkg      |

You will need an **Apple Developer account** ($99/yr) for the Developer ID certificate used in signing/notarization. Not required to build and run locally during development, but required to share the plugin.

---

## 6. Phased project plan

Estimates assume one developer working part-time and learning JUCE as they go; halve them if you're already fluent in C++/JUCE.

### Phase 0 — Environment & "hello plugin" (≈1 week)

- Install Xcode, CMake, clone JUCE.
- Generate a JUCE Audio Plug-In (instrument) project via CMake; build the **standalone + AU**.
- Confirm it passes `auval` and **loads in LUNA** as an empty instrument.
- _Milestone: a do-nothing AU appears and loads in LUNA._ This de-risks the entire toolchain before any real work.

### Phase 1 — Voice engine (≈1–2 weeks)

- Implement `SamplerVoice` (one-shot playback) + `VoiceEngine` (12 voices).
- Variable-rate playback for tuning; linear interpolation.
- Bundle a default sample set; trigger voices from incoming MIDI notes.
- _Milestone: play all 12 drums from LUNA's piano roll._

### Phase 2 — Sequencer (≈1–2 weeks)

- `PatternModel` (16 steps × 12 lanes) + host-synced clock from the playhead.
- Step playback locked to LUNA tempo/transport; swing control.
- _Milestone: a programmed beat plays in time with the LUNA project and survives a bounce._

### Phase 3 — UI (≈2–3 weeks)

- Step grid, transport, 12 voice strips (tune/level/pan/mute/solo), swing.
- Wire all controls to APVTS; state save/load.
- _Milestone: fully usable from the plugin window; reopens with patterns intact._

### Phase 4 — Character & polish (≈1–2 weeks)

- 8-bit crush + companding lo-fi stage; choke groups; tuning tuned to feel like the hardware.
- `pluginval` clean; CPU/real-time-safety pass.
- _Milestone: it sounds like an LM-1, not a generic sampler._

### Phase 5 — Extras & distribution (≈1–2 weeks+)

- User sample loading, pattern slots/song mode, per-step velocity.
- AU multi-out; VST3 build.
- Code-sign + notarize; package installer.
- _Milestone: shareable, signed, multi-format release._

**Rough total to a polished, LUNA-tested MVP: ~6–10 weeks part-time** (Phases 0–4). Distribution/extras add a couple more.

---

## 7. Key risks & how to manage them

- **Toolchain/AU friction before you've built anything** → do Phase 0 first; getting an empty AU into LUNA proves the hardest plumbing early.
- **Real-time safety bugs** (clicks, crashes) → no allocation/locks in the audio thread from day one; pass `pluginval` continuously.
- **AU multi-out validation** → ship stereo-only first; add multi-out as an isolated, well-tested phase.
- **Sample legality** → use your own/CC samples + user loading; recreate character in DSP rather than redistributing ROM dumps.
- **Notarization surprises** → script signing/notarization early (well-documented JUCE CI recipes exist) rather than fighting it at release time.

---

## 8. Sources

- [LUNA Concepts — supported plugin formats (UA Support)](https://help.uaudio.com/hc/en-us/articles/360041866251-LUNA-Concepts)
- [Using Insert Plug-Ins — AU default / VST3 enable on macOS (UA Support)](https://help.uaudio.com/hc/en-us/articles/34498519722132-Using-Insert-Plug-Ins)
- [LUNA Release Notes (UA Support)](https://help.uaudio.com/hc/en-us/articles/360041532452-LUNA-Release-Notes)
- [JUCE — GitHub (formats: VST3, AU, AUv3, LV2, AAX)](https://github.com/juce-framework/JUCE)
- [JUCE — Configuring bus layouts (multi-out / auval)](https://docs.juce.com/master/tutorial_audio_bus_layouts.html)
- [How to code sign and notarize macOS audio plugins (Melatonin)](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)
- [Linn LM-1 — Wikipedia (voices, sequencer, shuffle)](https://en.wikipedia.org/wiki/Linn_LM-1)
- [Roger Linn LM-1 — Polynominal (8-bit/28 kHz, AM6070, 48 PPQ)](https://www.polynominal.com/Roger-Linn-lm1/)
