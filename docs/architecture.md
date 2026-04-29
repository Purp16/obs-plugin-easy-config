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

- Listing and switching Scene, Scene Collection, and Profile.
- Loading/saving plugin-owned config path.
- Resolving the current OBS state into path-template variables.
- Writing the resolved path to the active OBS profile recording-path setting.

## UI

`EasyConfigDock` is a thin Qt Widgets form:

- It displays current OBS state.
- It persists form changes through `ObsController`.
- It previews paths and triggers apply operations.

No path parsing rules should live in the UI layer.

## Testing

Unit tests cover core path behavior only. OBS and Qt integration is verified manually by loading the plugin in OBS.
