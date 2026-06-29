# Nixie — Roadmap & status

**What this is:** a **modern drum-machine instrument** (AU/VST3/Standalone) that
sounds and looks like the Linn LM-1 but is built for a modern DAW workflow — a
**step sequencer + sampler** with 12 authentic LM-1 voices, real time signatures
and triplet/shuffle grids, a genre groove library, a per-voice mixer with multi-out,
and MIDI drag-out into the DAW.

The originally planned scope is **shipped** (currently **v0.4.1**). This file now
records what's done and what could still come; the per-release detail lives in
[CHANGELOG.md](CHANGELOG.md). The deeper architecture rationale is in
[Nixie_Design_and_Plan.md](Nixie_Design_and_Plan.md).

---

## Shipped

- ✅ **AU + VST3 + Standalone** from one CMake/JUCE 8 project; universal binary;
  passes `auval`.
- ✅ **12 authentic LM-1 voices** on a reference-counted `DrumKit` (lock-free swap);
  open hat choked by closed hat; **no crash cymbal** (authentic).
- ✅ **Per-voice sample loading** (WAV/AIFF) + restore-to-factory; optional bundled
  factory kit via binary data; procedural sounds as a silent-fail fallback.
- ✅ **Per-voice mixer:** level / pan / tune / mute / solo, 12-strip editor.
- ✅ **Step sequencer:** host-synced + internal clock, sample-accurate.
- ✅ **Time grid (v0.4.1):** per-pattern meter (2/4, 3/4, 4/4, 5/8, 6/8, 7/8, 9/8,
  12/8) + step rate (1/4, 1/8, 1/16, 1/8T, 1/16T); step count derived from
  meter × rate; meter-aware beat grouping. Triplet rates = real shuffle/compound.
- ✅ **Step-grid UI:** click/drag, mouse-wheel velocity, sweeping playhead.
- ✅ **Real-time record:** arm REC, play MIDI/pads onto the grid (nearest-step quantize).
- ✅ **Shuffle:** global + per-track musical swing (Straight/Light/Medium/Triplet/
  Hard, per-track Follow).
- ✅ **MIDI export:** "Export MIDI…" file + drag-pattern-to-DAW handle.
- ✅ **Groove library:** 12 banks × 10 slots — banks 1–10 are **100 factory genre
  grooves**; banks 11–12 are user-saveable and persisted to disk. Bank LED +
  "GENRE - pattern name" readout.
- ✅ **Multi-out (v0.4.0):** stereo Main + 12 direct outs; per-channel output
  selector so a voice or group can be processed on its own track in LUNA.
- ✅ **Full-state presets:** `.nixiepreset` save/load of the whole setup via the gear menu.
- ✅ **Character & styling:** wood cheeks, faceplate with `#fc5824` frames/labels,
  custom vintage knobs/faders/buttons, red LED readouts, global Master/Lo-Fi/Tune.

---

## The instrument we're emulating (spec reference)

The Linn LM-1 (Roger Linn, 1980) — the first drum machine to use digital samples.

- **Mixer:** 12 instrument channels (we omit the metronome _click_), each with a
  volume fader and a 3-position pan switch. The 12 instruments: **Bass (kick),
  Snare, Hi-Hat, Cabasa, Tambourine, Tom Lo, Tom Hi, Conga Lo, Conga Hi, Cowbell,
  Clave, Clap** — famously **no crash cymbal**.
- **Sequencer:** Z80 CPU, 48 PPQ, 12-track step sequencer; Play/Stop, Record, Tempo,
  Timing Correct (quantize), Shuffle (swing).
- **Sound:** 8-bit PCM, ~28 kHz, AM6070 companding DAC → the signature lo-fi grit.
- **Tuning:** per-voice tuning pots (rear panel) via playback-rate change.

**Voice → GM-note map** (collision-free, so MIDI export & a pad controller both work):

| #   | Voice       | Note | #   | Voice    | Note |
| --- | ----------- | ---- | --- | -------- | ---- |
| 0   | Bass (Kick) | 36   | 6   | Tom Hi   | 48   |
| 1   | Snare       | 38   | 7   | Conga Lo | 64   |
| 2   | Hi-Hat      | 42   | 8   | Conga Hi | 63   |
| 3   | Cabasa      | 69   | 9   | Cowbell  | 56   |
| 4   | Tambourine  | 54   | 10  | Clave    | 75   |
| 5   | Tom Lo      | 45   | 11  | Clap     | 39   |

(The open hat is voice 12, note 46; it renders through the Hi-Hat channel/fader.)

---

## Possible future work

- **`pluginval --strictness-level 10`** clean + a deeper real-time-safety audit of
  `processBlock`.
- **Song / pattern chaining** — arrange banks into a song (currently you drive
  arrangement from the DAW via MIDI drag-out).
- **Per-slot full state** — banks load pattern + tempo + meter; capturing mixer/kit
  per slot is still deferred to `.nixiepreset`.
- **Code-sign (Developer ID) + notarize + staple** so it installs on other Macs
  without Gatekeeper warnings.
- **Audio polish** — higher-quality interpolation, selectable companding curves,
  per-voice decay/reverse.

## Adapted / omitted for a digital VST

- **Timing Correct (quantize):** the grid is inherently quantized; real-time record
  uses nearest-step quantize.
- **Numeric keypad + 2-digit LED data entry:** replaced by direct mouse/grid editing;
  the LED readouts return as cosmetic charm.
- **Rear-panel tuning pots:** per-voice Tune knobs in the mixer.
- **Metronome click channel:** omitted — the DAW provides a click.
- **Non-velocity-sensitive pads:** we _add_ velocity (MIDI + per-step accent).

---

## Working tips

- **Iterate in the Standalone build** (`build/Nixie_artefacts/Release/Standalone/`) —
  far faster than reopening LUNA. Fully quit + relaunch it to pick up a rebuild.
- **Keep `auval` green** after changes (`auval -v aumu Nix1 Coff`).
- **Never touch the audio thread carelessly:** no `new`/`delete`, locks, file, or GUI
  calls in `processBlock`; pass data in via atomics / the reference-counted kit swap.
- **Reconfigure** (`cmake -B build -G Xcode`) after editing `CMakeLists.txt` or adding
  a source file, then build.
