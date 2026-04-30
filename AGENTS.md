# AGENTS.md

## Project Map

This repository is for an OBS Studio plugin named `obs-plugin-easy-config`.

The plugin should provide an OBS dock panel for:

- Fast Scene, Scene Collection, and Profile switching.
- Recording-directory automation based on date, current OBS state, manual tags, and eventually foreground game/application.

Source-of-truth docs:

- Technical research and option comparison: `docs/technical-research.md`
- Product/spec decisions: add `docs/product-spec.md` once requirements are finalized.
- Architecture notes: add `docs/architecture.md` once the plugin scaffold exists.

Keep this file short. Put detailed API inventories, design notes, and long rationale in `docs/`.

## Current Project State

The repository is still in planning/scaffolding state.

Recommended baseline:

- Native OBS plugin.
- C++ with CMake.
- Qt Widgets for the OBS dock UI by default, because OBS dock APIs accept Qt widgets.
- Official `obsproject/obs-plugintemplate` as the scaffold source.
- First target: OBS Studio 30.2+ unless requirements change.

Do not introduce an external companion service, browser dock, or obs-websocket-first architecture unless the user explicitly chooses that direction after the tradeoffs are documented.

## Build And Test Commands

After scaffolding, keep these commands current:

```sh
cmake -S . -B build -G Ninja -DEASY_CONFIG_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Building the actual OBS plugin also requires Qt 6 and OBS Studio source headers
matching the installed OBS version. See `README.md` for the current macOS
command.

If the scaffold changes these commands, update this section in the same change.

## Coding Guidelines

- Keep OBS-facing code separate from pure business logic.
- Put path-template parsing, variable resolution, sanitization, and rule selection in testable modules without Qt or OBS dependencies.
- Keep Qt dock classes focused on UI state and user actions.
- Keep platform-specific foreground-app detection behind a narrow interface.
- Prefer small files with clear ownership over large central modules.
- Use project-local helpers and existing patterns once the scaffold exists.
- Prefer ASCII in source/config unless user-facing localized text requires otherwise.

## Recording Path Rules

The rule engine should support these initial variables:

- `{date}` as `YYYY-MM-DD`
- `{datetime}` as `YYYY-MM-DD_HH-mm-ss`
- `{year}`, `{month}`, `{day}`
- `{time}` as `HH-mm-ss`
- `{hour}`, `{minute}`, `{second}`
- `{profile}`
- `{scene_collection}`
- `{scene}`
- `{tag}`

Path handling requirements:

- Treat the editable template as relative to the selected base directory; do
  not require users to include `{base}` in the template.
- Show a preview before applying.
- Sanitize path segments for all supported platforms.
- Normalize separators.
- Create missing directories before recording starts.
- Surface errors in the dock UI.

`{app}` foreground application detection is intentionally out of scope for the
first usable version.

## OBS Integration Boundaries

- Prefer OBS frontend/libobs APIs inside the plugin.
- Store plugin rules in plugin-owned configuration.
- Only write OBS profile recording-path settings when the user applies a rule or recording is about to start.
- Avoid changing unrelated OBS output, encoder, streaming, or scene settings.
- Keep detailed OBS API references in `docs/technical-research.md`, not in this file.

## Validation Expectations

Before marking implementation work complete:

- Build the plugin.
- Run unit tests for path-template and path-sanitization logic.
- Manually load the plugin in OBS when UI or OBS integration changes.
- Verify the dock appears.
- Verify scene/profile/scene-collection switching when touched.
- Verify a real test recording lands in the expected directory when recording-path logic changes.

If a validation step cannot be run locally, say exactly which step was skipped and why.

## Change Discipline

- Do not commit generated build output.
- Do not commit local OBS profile data, user recording paths, or machine-specific settings.
- Keep docs updated when decisions change.
- Keep `AGENTS.md` focused on durable repo instructions; move volatile research details to `docs/`.

## Open Decisions

- First supported platform: Windows-first or cross-platform from the start.
- Final minimum OBS version.
- Plugin/UI display name.
- Packaging order for Windows, macOS, and Linux.
