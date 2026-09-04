# TSPlug

A JUCE 9 instrument plugin (VST3, AU, CLAP, LV2, Standalone) that embeds
[NativeTS](https://github.com/TabulaSonora/NativeTS), the C++20 reimplementation of the Roland
Sound Canvas VA voice. The engine renders on the host's audio thread; this repository is the
plugin around it. Company LoSnoCo, manufacturer code `LSCo`, plugin code `Tsva`.

## Building

```sh
git submodule update --init --recursive
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```

Presets: `debug`, `release`, `debug-vcpkg`/`release-vcpkg` (nlohmann_json from a vcpkg toolchain
instead of FetchContent), `xcode`, `universal`. `COPY_PLUGIN_AFTER_BUILD` is on for a top-level
build, so a build installs into `~/Library/Audio/Plug-Ins`. After changing the plugin's identity
or plist, `killall -9 AudioComponentRegistrar` before `auval -v aumu Tsva LSCo`.

`external/JUCE`, `external/NativeTS` and `external/clap-juce-extensions` are submodules; an empty
one falls back to a sibling `../JUCE` or `../NativeTS` checkout. The engine pin follows the Apple
player's. Bump it deliberately, and copy `NOTICE.md` over ours when it changes -- the configure
compares the two and stops on a mismatch.

## Rules that are not preferences

- **Nothing Roland-derived enters this repository.** The engine reads a user-supplied `SCCore.dll`
  as data. `.gitignore` blocks `*.dll` and `*.wav`; keep it that way, and never commit a render.
- **Never add `-ffast-math`, and never let `-ffp-contract` back on.** `ts::numeric_semantics`
  carries `-fwrapv -ffp-contract=off` to every translation unit through `ts::tabulasonora`. The
  inline DSP in the engine's public headers is wrong without it.
- **Never run the engine unoptimised.** `TSPLUG_FAST_DEBUG` compiles the engine at `-O2` in Debug.
- **`Source/engine` has no JUCE in it.** `tests/engine_glue_test.cpp` links it without JUCE and is
  the proof; keep it that way so the engine layer stays portable to the next host.
- **Parameters are append-only.** Identifiers and their `ParameterID` version hints are part of
  the saved format. Add at the end with the next version; never rename or reorder.
- **Identity is permanent.** `PLUGIN_CODE`, `BUNDLE_ID`, `CLAP_ID` and `LV2URI` are what a host
  writes into a session. `Tsva` is deliberately not the Apple AUv3's `tbsn`, so both load in Logic.

## Threads

| Thread | May | Must not |
|---|---|---|
| Audio (`processBlock`) | `try_lock` the instrument, `send*At`, `render`, atomics, write the snapshot | allocate, block, take the lock unconditionally, touch the parameter tree's strings, call `VoicePool::active()` |
| Message | parameter listeners, `Instrument::setSettings` (brief lock, rebuilds the generator), read snapshots, resolve names through `Instrument::withNotes` | hold the lock across anything slow |
| ROM loader (`RomLoader`, one `juce::Thread`) | build a whole `Session` with no lock held, hash the file, write the settings file | touch UI; results cross back through an `AsyncUpdater` |

`ts::ToneGenerator` has one owner at a time: whoever holds `Instrument::lock_`. A `try_lock` miss
on the audio thread costs one block of silence, and only a rebuild or a ROM swap can cause one.

The first time a wave sounds, the engine decodes it and allocates on the audio thread. That is
accepted for now, as the Apple AUv3 accepts it; a loader-thread warm-up over
`NoteRenderer::sampler()` is the place to fix it if it ever matters.

## Ports and parts

Fixed at one port, sixteen parts. JUCE hands a plugin one sixteen-channel MIDI stream and no
cable nibble in any format, so a second port could never be addressed from a host. One instance
is one sixteen-part module; use two instances for thirty-two.

## Engine facts the code depends on

- The engine renders only at 32 kHz, planar float, in 32-sample blocks. `Source/engine/Resampler.hpp`
  converts to the host rate: Catmull-Rom with a four-frame carry (the plugin's own path), or the
  module's `ts::OutputFilter` driven a frame at a time (the authentic one, and the default).
- MIDI sample offsets are read against `ToneGeneratorOptions::host_sample_rate`, which is set
  from `prepareToPlay`; resolution is one millisecond.
- Every setting but gain rebuilds the generator over the same `NoteRenderer`; part state is
  carried across by replaying bank, program, CC 7/10/11/91/93.
- Latency is reported as 133 engine frames: the module's own 128-frame event staging plus five for
  the resampler, constant whichever resampler runs, because a latency that moves with a parameter
  is one a host cannot use.
- Mute and solo live in `Instrument::channels_` (a `ts::ChannelMask` of atomics) and are plugin
  state, not parameters.

## UI

The editor follows the LoSnoCo design system (`~/.claude/skills/losnoco-design`): one typeface
(Inter LoSnoCo, embedded, weights from the variable axis), uppercase tracked labels, flat fills and
hairline borders, teal focus rings, one orange primary action per view, no logo. Tokens are in
`Source/ui/Tokens.hpp`; never write a brand hex anywhere else. Reusable components take the `LS`
prefix. Dark mode follows the OS.

## Comment voice

Comments explain *why*, in prose. A non-obvious decision names the alternative that was rejected
and the concrete failure it caused. A change that removes a constraint removes the comment that
guarded it.
