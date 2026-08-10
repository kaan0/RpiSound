# Changelog

All notable changes to RpiSound are documented here.

---

## [Unreleased] — dev branch (since 2025-06-20)

### Architecture — complete redesign

The codebase was fully rewritten around a layered interface architecture to
separate hardware concerns from audio logic:

```
SoundManager
  └── ISoundLoader  (PcmLoader)
  └── IAudioEngine  (AudioEngine — mixer)
        └── IAudioDevice  (AudioDevice)
              └── IAudioDriver  (AlsaDriver / CoreAudioDriver)
                    └── IDeviceEnumerator
```

- All layers communicate through pure virtual interfaces — drivers are
  swappable without touching audio logic.
- `Result<T>` (`std::expected<T, std::string>`) used uniformly for error
  propagation; no exceptions thrown across layer boundaries.
- Old monolithic `tiny_alsa_wrapper`, `WavParser`, `PcmConverter`, and
  `Player` classes removed.

### Audio backend — ALSA (tinyalsa)

- `AlsaFacade` — thin C++ wrapper over the tinyalsa C API.
- `AlsaDriver` — implements `IAudioDriver`; opens PCM devices, writes frames.
- `AlsaDeviceEnumerator` — discovers playback and capture devices by scanning
  `/proc/asound/cards`.
- `AudioDeviceFactory` / `AudioDevice` — RAII handle owning the open PCM
  device; delegates all I/O to the driver.
- Default device format corrected to `S16LE` (`kFormatS16LE`) matching the
  `int16_t` audio type — previous code used `S32LE` which caused garbled
  output.

### Audio backend — Core Audio (macOS)

New backend added to allow running and testing the library on macOS without
hardware:

- `CoreAudioFacade` / `CoreAudioUtils` — C++ wrappers over the CoreAudio and
  AudioUnit C APIs.
- `CoreAudioDriver` — implements `IAudioDriver` using an `AudioUnit` render
  callback.
- `CoreAudioDeviceEnumerator` — enumerates system audio devices via
  `AudioObjectGetPropertyData`.
- `CoreAudioPlayState` — lock-free ring buffer (`kCapacity = 8192` samples)
  shared between the mixing thread and the render callback; replaces the old
  blocking single-sample play state.  The render callback now reads pre-mixed
  stereo chunks and outputs silence when the buffer is empty.
- Backend is selected at CMake configure time (`-DUSE_COREAUDIO=ON`).

### Audio type

- `audio_t` is now `float` on Core Audio and `int16_t` on ALSA (was
  `uint16_t`).
- CMake define renamed from `RPISOUND_AUDIO_T_UINT16` to
  `RPISOUND_AUDIO_T_INT16`.
- Python conversion script now generates `S16LE` PCM files for ALSA and
  `F32LE` for Core Audio.
- `AudioEngine::toFloat` / `fromFloat` branch on `std::is_signed_v` and
  `std::is_floating_point_v` at compile time so both backends share one
  implementation.

### PCM file format

Custom binary format used for pre-converted sample files:

```
name:<name>|samplerate:<hz>|channels:<n>|samplewidth:<bytes>|frames:<n>PCM DATA\n
<raw PCM bytes>
```

- `PcmLoader` — loads all `.pcm` files from a named instrument folder,
  validates header fields with `std::from_chars`, enforces size limits.
- `ISoundLoader::getSample` returns `shared_ptr<const SoundSample>` (was a
  dangling-prone `const SoundSample&`).  `nullptr` is returned for missing
  samples; all callers check before use.
- `PcmLoader` stores samples as `shared_ptr<SoundSample>` so `Voice` objects
  in the engine can share ownership and prevent use-after-free if samples are
  ever reloaded.

### 8-voice software mixer (`AudioEngine`)

- `AudioEngine` runs a dedicated mixing thread that loops continuously:
  1. Mixes all active `Voice` objects into a `float` accumulator buffer
     (always `float`, regardless of backend `audio_t`).
  2. Applies per-voice `gain` (mapped from MIDI velocity `0–127`).
  3. Clamps the sum to `[-1, 1]` to prevent distortion on simultaneous hits.
  4. Converts the float buffer to `audio_t` and writes one chunk to
     `IAudioDevice`.
- `writeSample(shared_ptr<const SoundSample>, float gain)` — non-blocking;
  called from the UI/MIDI thread.
  - **Same sample retriggering**: resets the position of the already-playing
    voice to 0.
  - **Free slot**: assigns an idle voice.
  - **All 8 busy**: steals the voice with the least audio remaining (least
    audible cut).
- `Voice` holds `shared_ptr<const SoundSample>`, released immediately when the
  voice goes idle.
- Chunk size (default 512 samples ≈ 5.8 ms at 44100 Hz stereo) is the latency
  knob: lower = less latency, higher = fewer dropouts.
- ALSA `pcmWrite` naturally paces the thread.  Core Audio back-pressure comes
  from the ring buffer in `CoreAudioPlayState::tryWrite`.
- `IAudioEngine` interface updated: `writeSample` now takes
  `shared_ptr<const SoundSample>` and `float gain`.

### SoundManager

- `triggerSound` now fetches the sample once (was two `getSample` lookups),
  checks for `nullptr`, maps `velocity → gain`, and passes ownership of the
  `shared_ptr` to the engine.
- `initialize` / `load` / `selectAudioDevice` — unchanged in behaviour.

### TUI (`RpiSoundUI` / `SoundManagerBackend`)

- Full interactive terminal UI built with FTXUI.
- Four panels: **Audio Devices**, **Available Samples**, **MIDI Mappings**
  (placeholder), **Controls**.
- Velocity slider (`-10 / -1 / +1 / +10` buttons, clamped to `0–127`).
- `SoundUIBackend` concept constrains the backend type at compile time — no
  `std::function` overhead.
- `SoundManagerBackend` adapts `SoundManager` to the concept.
- `MouseReportingGuard` — RAII guard enabling SGR extended mouse reporting;
  cleans up on destruction even if an exception unwinds.
- `q` / `Q` exits the loop.

### Tooling

- **spdlog** — replaced custom logger; used header-only via CMake
  `FetchContent`.
- **clang-tidy** — `.clang-tidy` config added.
- **FTXUI** — fetched via `FetchContent`.
- **CMakeLists** — `USE_COREAUDIO` option; backend sources, libs, and audio
  type compile definition selected automatically; post-build step runs the
  Python conversion script and copies `.pcm` files to the binary directory.
- Old submodule `third_party/tinyalsa` removed; tinyalsa pulled via
  `FetchContent` at configure time.

### Bug fixes

- **`AudioDeviceManager` constructor** — called `.value()` on a potentially-
  error `Result` after logging a warning, causing `std::bad_expected_access`.
  Fixed by only assigning when the result holds a value.
- **`Result<bool>` implicit bool conversion** — `isOpen()` returns
  `Result<bool>`.  The `operator bool` on `std::expected` returns
  `has_value()`, not the contained bool.  `isDeviceOpen()`,
  `AudioDevice::open()`, and `AudioDevice::close()` were all checking the
  wrong thing.  Fixed with explicit `.has_value() && .value()` checks.
- **`AudioDevice::read()`** — stub returned `false` (silently cast to
  `size_t{0}`) instead of `std::unexpected("Not implemented")`.
- **ALSA format mismatch** — default PCM format was `kFormatS32LE` (32-bit)
  while `audio_t` was `uint16_t` (16-bit unsigned), causing garbled audio.
  Fixed: format is now `kFormatS16LE`, type is `int16_t`, and Python script
  generates `S16LE` data.
- **`AudioDeviceInfo::to_string(SampleFormat)`** — switch had no `default`
  branch; added `default: return "Unknown"`.
- **`ISoundManager` missing `<vector>`** — depended on transitive includes;
  explicit include added.
- **Core Audio render callback in-place de-interleave** — original code used
  the L output buffer as a scratch area and iterated backwards to split
  interleaved stereo into L/R, but the backward iteration corrupted source
  samples.  Fixed with a pre-allocated `CoreAudioPlayState::scratch[2048]`
  array used as the read target before splitting.
- **Velocity (MIDI) parameter** — accepted end-to-end but never applied to
  audio output.  Now mapped to `gain = velocity / 127.0f` and applied
  per-voice during mixing.
