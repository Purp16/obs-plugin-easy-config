# OBS Plugin Easy Config

An OBS Studio dock plugin for fast Scene/Profile switching and recording-path automation.

## Current Status

This is the first usable implementation scaffold:

- C++17 core logic for path-template resolution and config serialization.
- Qt Widgets dock UI.
- OBS frontend integration wrapper.
- Doctest-style unit tests for path rules.
- OBS locale resource files for English and Chinese, with room for additional
  languages through new `data/locale/*.ini` files.

## Build Core Tests

The pure core tests do not require OBS or Qt:

```sh
cmake -S . -B build -G Ninja -DEASY_CONFIG_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build OBS Plugin

The plugin target needs:

- CMake
- Ninja
- Qt 6 Core/Widgets
- OBS Studio runtime libraries
- OBS Studio source headers matching the installed OBS version

On macOS with OBS installed at `/Applications/OBS.app`, provide the OBS source
tree with `OBS_SOURCE_DIR`:

```sh
cmake -S . -B build-plugin -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/macos \
  -DOBS_SOURCE_DIR=/path/to/obs-studio \
  -DOBS_LIBRARY_DIR=/Applications/OBS.app/Contents/Frameworks
cmake --build build-plugin
```

The installed OBS app contains runtime frameworks but does not include all
development headers, so `OBS_SOURCE_DIR` is required for local plugin builds.
Use Qt headers/tools that match the Qt version bundled with the target OBS app
when possible. OBS 32.1.2 for macOS uses Qt 6.8.3; building this plugin with
newer Qt headers can produce symbols that OBS cannot load.

For local macOS testing, install the built plugin bundle into the OBS user
plugin directory and restart OBS:

```sh
cmake --build build-plugin --target install-user-plugin
```

The installed bundle includes `Contents/Resources/locale/*.ini`; without those
files OBS will fall back to the default English strings for plugin UI text.

## MVP Behavior

- Dock title: `Easy Config`
- Default rule: `baseDirectory` plus `{date}/{tag}`
- Empty tag fallback: `untagged`
- First version intentionally does not implement `{app}` foreground application detection.
