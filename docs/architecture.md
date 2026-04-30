# Architecture

## Shape

The first usable version separates the plugin into three layers:

- Core logic: path-template resolution, sanitization, and plugin config serialization.
- OBS integration: thin wrapper around OBS frontend/profile config APIs.
- Qt dock: compact form UI that calls the integration layer.

## Core Logic

Core logic must not include Qt or OBS headers. This keeps path behavior unit-testable without OBS installed.

Important modules:

- `path-template.*`
- `plugin-config.*`

## OBS Integration

`ObsController` owns direct OBS API calls:

- Listing and switching Scene Collection and Profile.
- Reading the current Scene for path-template variables without replacing OBS'
  built-in Scene switcher.
- Reading the current OBS recording directory to initialize the plugin base
  directory when no plugin value has been saved yet.
- Loading/saving plugin-owned config path.
- Resolving the current OBS state into path-template variables.
- Writing the resolved path to the active OBS profile recording-path setting
  only when path management is enabled and recording or replay buffer is
  starting.
- Reading and writing OBS output resolution, FPS, and replay-buffer
  duration/memory settings.
- Blocking video setting changes while OBS outputs are active.

## UI

`EasyConfigDock` is a thin Qt Widgets form:

- It exposes Profile and Scene Collection switching plus recording/replay-path
  fields.
- It exposes Resolution and FPS shortcut buttons with edit dialogs.
- It exposes replay-buffer duration and memory inputs.
- It persists form changes through `ObsController`.
- It previews paths and triggers apply operations.

No path parsing rules should live in the UI layer.


## Testing

Unit tests cover core path behavior only. OBS and Qt integration is verified manually by loading the plugin in OBS.
