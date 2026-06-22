# Factory kit — drop your 12 WAVs here

Put one short one-shot sample per LM-1 voice in this folder (WAV or AIFF). On the
next CMake **configure + build**, they're embedded into the plugin as the factory
default kit. Until files are present, the plugin uses built-in procedural sounds.

## Filenames

Matching is forgiving — case, spaces, hyphens and underscores are ignored, and a
few common aliases are accepted. Any of `Bass.wav` / `bass.wav` / `KICK.wav` map
to the Bass voice, etc. Recommended names (one file each):

| Voice | Recommended file | Also accepts |
|-------|------------------|--------------|
| Bass (kick) | `Bass.wav`       | kick, bassdrum, bd |
| Snare       | `Snare.wav`      | sd |
| Hi-Hat      | `Hi-Hat.wav`     | hat, hh |
| Cabasa      | `Cabasa.wav`     | — |
| Tambourine  | `Tambourine.wav` | tamb |
| Tom Lo      | `Tom Lo.wav`     | tom1, lowtom |
| Tom Hi      | `Tom Hi.wav`     | tom2, hightom |
| Conga Lo    | `Conga Lo.wav`   | conga1, lowconga |
| Conga Hi    | `Conga Hi.wav`   | conga2, highconga |
| Cowbell     | `Cowbell.wav`    | cow |
| Clave       | `Clave.wav`      | claves |
| Clap        | `Clap.wav`       | handclap |

Any voice without a matching file falls back to its procedural placeholder. You
can also load samples per-voice at runtime from the plugin's **Load** button —
this folder only sets the *default* kit.

Samples keep their native sample rate; the engine resamples for tuning and host
rate at playback, so 44.1 kHz mono one-shots are ideal.
