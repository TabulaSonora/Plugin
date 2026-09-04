# Tabula Sonora, the plugin

The Roland Sound Canvas VA voice as a VST3, Audio Unit, CLAP and LV2 instrument, and a standalone
app, on the [NativeTS](https://github.com/TabulaSonora/NativeTS) engine. It plays General MIDI,
GS and XG over MIDI, System Exclusive included, in any host that takes one of those formats.

## You need your own `SCCore.dll`

The engine is inert without one, from a licensed SOUND Canvas VA installation. Nothing
Roland-derived is in this repository. The plugin looks for the file where the other Tabula Sonora
front ends leave it -- the Apple player's container, the Linux player's data directory,
`$TS_SCCORE_DLL` -- and otherwise asks once and remembers. The whole file is hashed the first
time and identified quickly afterwards; the engine refuses a build it does not know.

macOS may refuse a plugin access to the Apple player's copy, since that lives in another
application's container. When it does, the plugin says so and the file picker opens on the file:
choosing it is the permission.

## Building

Needs CMake 3.24+, a C++20 compiler, and on macOS Xcode. Dependencies are submodules.

```sh
git clone --recurse-submodules https://github.com/losnoco/TSPlug.git
cd TSPlug
cmake --preset debug
cmake --build --preset debug
ctest --preset debug          # set TS_SCCORE_DLL to run the loaded-ROM half
```

A top-level build installs into `~/Library/Audio/Plug-Ins`. `cmake --preset release` for a
release build; `universal` for arm64 and x86_64 together.

## What it is and is not

- One instance is one sixteen-part module. Plugin formats carry one MIDI stream and no port
  number, so the module's second port cannot be addressed; use two instances.
- Match the module by default, exceed it on request. Both departures from the hardware -- the
  extended interpolator and the plugin's own output resampler -- are off until you turn them on.
- Mute and solo per part are plugin state, saved with the session, not automatable parameters.
- Reported latency is 133 frames at 32 kHz, the module's own note-on staging plus the resampler.

## Licence

BSD 3-Clause, see `LICENSE`. `NOTICE.md` is the engine's and travels with every bundle: the
reverb and chorus coefficients the engine carries are Roland-derived, and a binary that links it
inherits the terms set out there.
