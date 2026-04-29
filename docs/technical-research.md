# OBS Easy Config Technical Research

## Goal

Build an OBS Studio plugin that adds a dock panel for fast Scene/Profile/Scene Collection switching and automatic recording/replay-buffer directory routing.

The most important workflow is:

1. User chooses a base directory and types a relative rule such as `{date}/{app}`.
2. Plugin resolves the rule before recording starts.
3. Plugin creates the target directory if needed.
4. Plugin updates the active OBS profile recording directory.
5. Recording starts into the intended subdirectory.
6. Replay buffer uses the same managed directory before it starts.

## Options Considered

### Native C++/Qt OBS Plugin

Recommended.

Pros:

- Runs inside OBS and can create a real dock panel.
- Uses OBS frontend APIs directly for scenes, scene collections, profiles, and recording events.
- No need to ask users to enable/configure obs-websocket.
- Best path to packaging as a normal OBS plugin.

Cons:

- C++/CMake/Qt build setup is heavier than scripting.
- Cross-platform packaging needs more care.

### Native C++ Plugin With Non-Qt UI

Possible, but not recommended for a docked OBS panel.

Pros:

- Core plugin logic can still use libobs/frontend APIs.
- A non-Qt settings surface can be built with OBS properties for simple controls.

Cons:

- OBS frontend dock APIs are Qt-oriented, so a true dock panel still needs a Qt widget boundary.
- Platform-native UI stacks would need separate Windows/macOS/Linux implementations.
- More complexity for a worse integration experience.

Best use:

- Keep core logic UI-agnostic, but use Qt only as a thin shell around the dock.

### Lua/Python OBS Script

Not recommended for the main product.

Pros:

- Fastest prototype path.
- Useful for proving path-template logic.

Cons:

- Poor fit for a polished dock panel.
- Harder to package as a product-like plugin.
- Platform-specific foreground-app detection becomes awkward.

Best use:

- Quick prototype of path-template behavior or OBS config writes before building the native plugin.

### External App Using obs-websocket

Useful as a companion or test harness, not recommended as the core plugin.

Pros:

- Simple to build with TypeScript/Rust/Python.
- obs-websocket exposes scene/profile/record-directory operations.

Cons:

- Not naturally embedded as an OBS dock.
- Requires websocket server configuration and authentication.
- More moving parts for a workflow that should feel native.

Best use:

- Companion desktop app, automation tool, or integration test harness.

### Browser Dock + Local Service

Not recommended for MVP.

Pros:

- UI can be built with web frontend tools.

Cons:

- Requires a local service to access filesystem/platform APIs safely.
- Still needs obs-websocket or native bridge for OBS control.
- More operational complexity than a native dock.

Best use:

- If web UI iteration speed matters more than native packaging and the user accepts a local service.

### Hybrid Native Backend + Web UI

Possible but heavy for MVP.

Pros:

- UI can be built with React/Vue/Svelte.
- Core OBS operations can remain in a native plugin.

Cons:

- Needs an embedded web view or local frontend surface.
- Needs a bridge between native plugin code and the web UI.
- Packaging, security, and debugging are more complex than Qt Widgets.

Best use:

- Later rewrite if the UI becomes complex enough to justify a web frontend.

### No Dock, OBS Tools/Menu/Properties Only

Possible but too limited for the requested workflow.

Pros:

- Simplest native plugin UI.
- Avoids most custom dock layout work.

Cons:

- Not a convenient always-visible control panel.
- Poor fit for fast switching and path preview.

Best use:

- Minimal fallback UI or early internal debug controls.

## Recommended Stack

- Language: C++17 or C++20.
- UI: Qt Widgets for the dock shell, with core logic kept UI-independent.
- Build: CMake, preferably based on `obsproject/obs-plugintemplate`.
- First OBS target: OBS Studio 30+.
- Tests: CTest plus a small C++ unit-test framework for rule/path logic.
- Config storage: plugin-owned JSON under `obs_module_config_path(...)`; write OBS recording path only when applying rules.

## Important OBS APIs

Frontend API:

- `obs_frontend_add_dock_by_id`
- `obs_frontend_remove_dock`
- `obs_frontend_add_event_callback`
- `obs_frontend_get_scene_names`
- `obs_frontend_get_scenes`
- `obs_frontend_set_current_scene`
- `obs_frontend_get_scene_collections`
- `obs_frontend_set_current_scene_collection`
- `obs_frontend_get_profiles`
- `obs_frontend_set_current_profile`
- `obs_frontend_get_profile_config`

Config API:

- `config_get_string`
- `config_set_string`
- `config_save_safe`

Module config path:

- `obs_module_config_path`

## Recording Directory Behavior

OBS stores recording directories in the active profile config.

Relevant keys:

- Simple output mode: `SimpleOutput/FilePath`
- Advanced standard recording: `AdvOut/RecFilePath`
- Advanced FFmpeg file output: `AdvOut/FFFilePath`

The plugin should inspect `Output/Mode` and, for advanced mode, `AdvOut/RecType` to choose the key to update.

Replay buffer output uses the same recording directory setting. The plugin
should apply the path before recording starts, ideally on
`OBS_FRONTEND_EVENT_RECORDING_STARTING`, and before replay buffer starts via
`OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING`.

## Rule Engine

Initial variables:

- `{date}` as `YYYY-MM-DD`
- `{year}`
- `{month}`
- `{day}`
- `{profile}`
- `{scene_collection}`
- `{scene}`
- `{app}`
- `{tag}`

Rules:

- The editable template is relative to the selected base directory; the plugin
  prepends the base path.

- Unknown variables should be visible in validation errors.
- Empty variables should either resolve to a fallback like `Unknown` or be skipped by a setting.
- Path segments must be sanitized for Windows/macOS/Linux.
- Directory creation should happen before OBS starts recording or starts the
  replay buffer.
- UI should always show a preview path.

## Foreground App Detection

Make this optional for MVP.

Windows:

- `GetForegroundWindow`
- `GetWindowThreadProcessId`
- process name/path lookup

macOS:

- `NSWorkspace.sharedWorkspace.frontmostApplication`

Linux:

- X11 may support active-window lookup.
- Wayland is limited by compositor/security rules, so `{app}` may need a manual tag or mapping fallback.

## MVP Recommendation

Build in this order:

1. Scaffold native plugin from the official OBS plugin template.
2. Add dock panel with current profile/scene/scene collection display.
3. Implement list refresh and one-click switching.
4. Implement path-template parser and unit tests.
5. Add base directory, relative template editor, manual tag, and preview.
6. Implement recording path apply logic.
7. Apply automatically on recording-starting event.
8. Add foreground app detection after core path switching is stable.

## Open Decisions

- Minimum OBS version: recommend 30+.
- First supported platform: recommend Windows first if game/software detection is central.
- Test framework: Catch2, doctest, or a minimal no-dependency CTest binary.
- Config format: recommend plugin-owned JSON plus active OBS profile updates when rules apply.
- Product naming in UI: "Easy Config", "Recording Router", or a Chinese name.

## Reference Links

- OBS frontend API: https://docs.obsproject.com/reference-frontend-api
- OBS config-file API: https://docs.obsproject.com/reference-libobs-util-config-file
- OBS module API: https://docs.obsproject.com/reference-modules
- OBS plugin template: https://github.com/obsproject/obs-plugintemplate
- OBS source code: https://github.com/obsproject/obs-studio
- obs-websocket protocol: https://raw.githubusercontent.com/obsproject/obs-websocket/master/docs/generated/protocol.json
