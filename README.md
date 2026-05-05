# OBS Plugin Easy Config

An OBS Studio dock plugin for fast Profile/Scene Collection switching,
recording/replay-buffer path automation, video setting shortcuts, and
replay-buffer tuning.

## Current Status

This is the first usable implementation scaffold:

- C++17 core logic for path-template resolution and config serialization.
- Qt Widgets dock UI.
- OBS frontend integration wrapper.
- Doctest-style unit tests for path rules.
- Editable Resolution and FPS shortcut presets.
- Replay-buffer duration and memory controls.
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

The plugin uses OBS frontend APIs and is intended to stay compatible with OBS
Studio 30+. The main compatibility risk is binary
compatibility, especially Qt and OBS SDK versions. For release builds, compile
against the oldest OBS version you want to support, then test the same package
on newer OBS versions.

Recommended release baseline:

- Minimum OBS target: OBS Studio 30.2+
- OBS headers/SDK: match the minimum supported OBS release
- Qt: match the Qt version bundled with that OBS release
- Local smoke test: also test on the newest OBS release you support

Avoid building release packages with a newer Qt than the target OBS runtime.
For example, OBS 32.1.2 on macOS uses Qt 6.8.3; a plugin built with Homebrew Qt
6.11 may load today, but can fail later if it references newer Qt symbols.

### Common Inputs

All platform builds need:

- CMake
- Ninja, or the native generator for the platform
- C++17 compiler
- Qt 6 Core/Widgets
- OBS Studio runtime libraries
- OBS Studio source headers for the target OBS version

Useful CMake variables:

- `CMAKE_PREFIX_PATH`: Qt install prefix
- `OBS_SOURCE_DIR`: OBS source checkout for headers
- `OBS_LIBRARY_DIR`: directory containing OBS runtime/import libraries
- `OBS_APP_FRAMEWORK_DIR`: macOS framework directory used while building
- `OBS_INSTALL_RPATH_FRAMEWORK_DIR`: macOS framework directory encoded in
  installed plugin bundles; defaults to `/Applications/OBS.app/Contents/Frameworks`
- `EASY_CONFIG_BUILD_PLUGIN=OFF`: build only the pure unit tests

### macOS

Example for local macOS testing with OBS installed at `/Applications/OBS.app`:

```sh
cmake -S . -B build-plugin -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/macos \
  -DOBS_SOURCE_DIR=/path/to/obs-studio \
  -DOBS_LIBRARY_DIR=/Applications/OBS.app/Contents/Frameworks
cmake --build build-plugin
ctest --test-dir build-plugin --output-on-failure
```

Install into the current macOS OBS user plugin directory:

```sh
cmake --build build-plugin --target install-user-plugin
```

The installed bundle is:

```text
~/Library/Application Support/obs-studio/plugins/obs-plugin-easy-config.plugin
```

The bundle includes `Contents/Resources/locale/*.ini`; without those files OBS
will fall back to the default English strings for plugin UI text.

### Windows

Use the same OBS and Qt versions as the target OBS release. A typical local
layout is:

```text
C:\deps\obs-studio
C:\deps\obs-deps
C:\Qt\6.x.x\msvc2022_64
```

Configure and build with Visual Studio 2022:

```bat
cmake -S . -B build-plugin -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64 ^
  -DOBS_SOURCE_DIR=C:\deps\obs-studio ^
  -DOBS_LIBRARY_DIR=C:\deps\obs-deps\lib
cmake --build build-plugin --config RelWithDebInfo
ctest --test-dir build-plugin -C RelWithDebInfo --output-on-failure
```

For local testing, copy the built plugin binary and `data/locale` files into
the OBS user plugin layout for Windows:

```text
%APPDATA%\obs-studio\plugins\obs-plugin-easy-config\
  bin\64bit\obs-plugin-easy-config.dll
  data\locale\en-US.ini
  data\locale\zh-CN.ini
```

### Linux

Use the OBS and Qt packages for the target distro/OBS release, or build inside
a pinned container image. Example with Ninja:

```sh
cmake -S . -B build-plugin -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/qt6 \
  -DOBS_SOURCE_DIR=/path/to/obs-studio \
  -DOBS_LIBRARY_DIR=/path/to/obs-libs
cmake --build build-plugin
ctest --test-dir build-plugin --output-on-failure
```

For local testing, copy the shared object and locale files into:

```text
~/.config/obs-studio/plugins/obs-plugin-easy-config/
  bin/64bit/obs-plugin-easy-config.so
  data/locale/en-US.ini
  data/locale/zh-CN.ini
```

### Fast Toolchain Switching

For quick local switching, keep one build directory per target:

```sh
cmake -S . -B build-obs30-macos -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/obs30/qt \
  -DOBS_SOURCE_DIR=/path/to/obs-studio-30 \
  -DOBS_LIBRARY_DIR=/path/to/obs30/Frameworks

cmake -S . -B build-obs32-macos -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/obs32/qt \
  -DOBS_SOURCE_DIR=/path/to/obs-studio-32 \
  -DOBS_LIBRARY_DIR=/Applications/OBS.app/Contents/Frameworks
```

Then switch by choosing the build directory:

```sh
cmake --build build-obs30-macos
cmake --build build-obs32-macos
```

Long term, prefer adding `CMakePresets.json` and a pinned dependency file
similar to `obs-plugintemplate` so CI can build macOS, Windows, and Linux
packages with known OBS/Qt versions.

## Release Builds

Release builds are automated with GitHub Actions. Push a version tag such as
`v0.1.0` to build Windows, macOS, and Linux artifacts and create a draft GitHub
release.

Current release artifacts include:

- Windows x64 NSIS installer and zip
- macOS universal pkg installer and tarball
- Linux x86_64 tarball

See `docs/release.md` for the full release workflow, local packaging commands,
and smoke-test checklist.

### Compatibility Smoke Test

On macOS, you can test a built plugin against a temporary OBS app without
touching your normal OBS profile by using a temporary `HOME`:

```sh
export TEST_HOME="$(mktemp -d)"
mkdir -p "$TEST_HOME/Library/Application Support/obs-studio/plugins"
cp -R build-plugin/obs-plugin-easy-config.plugin \
  "$TEST_HOME/Library/Application Support/obs-studio/plugins/"
HOME="$TEST_HOME" /path/to/OBS.app/Contents/MacOS/OBS
```

After OBS opens, verify:

- OBS log contains `[obs-plugin-easy-config] loaded`
- The `Easy Config` dock is available
- OBS closes without an easy-config crash or unload error

Example with OBS Studio 30.0.2 Apple Silicon:

```sh
mkdir -p /tmp/obs-easy-config-compat
cd /tmp/obs-easy-config-compat
curl -L --fail -o OBS-Studio-30.0.2-macOS-Apple.dmg \
  https://github.com/obsproject/obs-studio/releases/download/30.0.2/OBS-Studio-30.0.2-macOS-Apple.dmg

hdiutil attach OBS-Studio-30.0.2-macOS-Apple.dmg
export TEST_HOME="$(mktemp -d)"
mkdir -p "$TEST_HOME/Library/Application Support/obs-studio/plugins"
cp -R /path/to/repo/build-plugin/obs-plugin-easy-config.plugin \
  "$TEST_HOME/Library/Application Support/obs-studio/plugins/"

HOME="$TEST_HOME" "/Volumes/OBS/OBS.app/Contents/MacOS/OBS"
```

After testing, detach the image:

```sh
hdiutil detach /Volumes/OBS
```

## Current Behavior

- Dock title: `Easy Config`
- Default rule: `baseDirectory` plus `{date}/{tag}`
- Path templates support date/time variables including `{date}`, `{datetime}`,
  `{time}`, `{hour}`, `{minute}`, and `{second}`.
- Empty tag fallback: `untagged`
- First version intentionally does not implement `{app}` foreground application detection.
- Resolution shortcuts change both OBS base canvas resolution and output
  resolution.
- FPS shortcuts update OBS FPS mode and values, including integer presets such
  as 120 FPS.
- Video setting shortcut buttons are disabled while recording, streaming, or
  replay buffer is active.
- The dock uses a compact responsive layout. Paired sections switch from two
  columns to one column when each item would be narrower than 220 px. Extra
  vertical space remains below the dock content instead of stretching row gaps.
- The bottom-right `Options` button opens display toggles, the combined
  Resolution/FPS preset editor, and About.
