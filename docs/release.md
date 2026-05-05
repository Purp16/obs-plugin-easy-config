# Release Build and Distribution

This project builds release artifacts with GitHub Actions.

Current default CI baseline:

- OBS Studio: `32.1.2`
- OBS deps / Qt deps: `2025-07-11`
- Windows: x64, Visual Studio 2022, NSIS installer plus zip
- macOS: universal `arm64;x86_64`, unsigned pkg installer plus tarball
- Linux: Ubuntu 24.04 x86_64 tarball

This baseline tracks the current OBS release line for packaging automation. If
the public compatibility promise is OBS 30.2+, change `OBS_VERSION` and
`OBS_DEPS_VERSION` in `.github/workflows/release.yml` to the matching OBS 30.2.x
dependency set and build against that older runtime, then smoke-test the same
artifacts on newer OBS releases.

## Artifacts

CI produces these files:

- `obs-plugin-easy-config-<version>-windows-x64-installer.exe`
- `obs-plugin-easy-config-<version>-windows-x64.zip`
- `obs-plugin-easy-config-<version>-macos.pkg`
- `obs-plugin-easy-config-<version>-macos.tar.gz`
- `obs-plugin-easy-config-<version>-linux-x86_64.tar.gz`

Windows and macOS installers install into the program-level OBS plugin
directory. The zip/tarball artifacts are useful for manual testing and fallback
installs.

The macOS build uses downloaded OBS frameworks for linking, but encodes
`/Applications/OBS.app/Contents/Frameworks` as the installed runtime path by
default. If you distribute for a nonstandard OBS.app location, rebuild with a
different `OBS_INSTALL_RPATH_FRAMEWORK_DIR`.

## GitHub Actions

Pull requests and pushes to `master`/`main` run the build matrix and upload
artifacts to the workflow run.

To create a release candidate without a tag:

1. Open GitHub Actions.
2. Run the `Build and release` workflow manually.
3. Optionally set `version`, for example `0.1.0-rc1`.
4. Download artifacts from the workflow run and smoke-test them manually.

To create a draft GitHub release:

```sh
git tag v0.1.0
git push origin v0.1.0
```

The workflow builds all platforms and creates a draft release with the artifacts
attached. Review the draft release notes, upload any manually signed/notarized
replacement artifacts if needed, then publish.

## Windows Installer

The Windows package step uses NSIS and the template at
`packaging/windows/obs-plugin-easy-config.nsi`.

The installer defaults to:

```text
C:\Program Files\obs-studio
```

It verifies that the selected directory contains:

```text
bin\64bit\obs64.exe
```

Installed files:

```text
obs-plugins\64bit\obs-plugin-easy-config.dll
data\obs-plugins\obs-plugin-easy-config\locale\*.ini
```

The zip artifact uses the same program-level OBS layout and can be extracted
over an OBS install for manual testing.

## macOS Installer

The macOS package step uses `pkgbuild`. It installs:

```text
/Library/Application Support/obs-studio/plugins/obs-plugin-easy-config.plugin
```

The package produced by CI is unsigned. On current macOS versions, unsigned
packages can trigger Gatekeeper warnings. For public releases, prefer signing
and notarizing the `.pkg` before publishing.

Suggested signing follow-up:

1. Add Apple Developer ID Installer certificate secrets to GitHub Actions.
2. Sign the package with `productsign`.
3. Submit the signed package with `xcrun notarytool`.
4. Staple the notarization ticket with `xcrun stapler`.

The `.tar.gz` artifact contains only `obs-plugin-easy-config.plugin` for manual
copy testing.

## Linux Tarball

The Linux artifact contains the OBS user-plugin layout:

```text
obs-plugin-easy-config/
  bin/64bit/obs-plugin-easy-config.so
  data/locale/*.ini
```

Manual install:

```sh
mkdir -p ~/.config/obs-studio/plugins
tar -C ~/.config/obs-studio/plugins -xzf obs-plugin-easy-config-<version>-linux-x86_64.tar.gz
```

For distro packages, keep the same file layout but install under the OBS
program plugin directories used by the target distro.

## Local Packaging

After configuring and building a plugin build directory, package locally with:

Windows PowerShell:

```powershell
scripts/package-windows.ps1 -BuildDir build-plugin -Version 0.1.0
```

macOS:

```sh
scripts/package-macos.sh build-plugin 0.1.0
```

Linux:

```sh
scripts/package-linux.sh build-plugin 0.1.0
```

Outputs are written to `dist/`.

## Release Checklist

Before publishing:

- Confirm `CMakeLists.txt` project version matches the tag.
- Run the GitHub Actions build on the target tag.
- Install and launch OBS on Windows.
- Verify the `Easy Config` dock appears on Windows.
- Install and launch OBS on macOS.
- Verify the `Easy Config` dock appears on macOS.
- Install and launch OBS on Linux.
- Verify the `Easy Config` dock appears on Linux.
- Verify scene/profile/scene-collection switching.
- Verify a real test recording lands in the expected directory.
- Check OBS logs for plugin load/unload errors.
- Publish the draft release after installer smoke tests pass.
