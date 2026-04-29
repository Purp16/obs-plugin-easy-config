# Product Spec: First Usable Version

## Goal

Provide an OBS dock named `Easy Config` for fast Profile/Scene Collection switching and recording/replay-buffer directory automation.

## MVP Behavior

- Show compact controls for Profile and Scene Collection switching.
- Do not duplicate OBS' built-in Scene switcher in the dock; read the current
  Scene only for path preview and `{scene}` resolution.
- Save plugin settings globally, not per OBS Profile.
- Initialize `baseDirectory` from the current OBS recording directory when the
  plugin does not have a saved base directory yet.
- Use `baseDirectory` plus `{date}/{tag}` as the default recording/replay path rule.
- Resolve and preview the target path in the dock.
- Treat `autoApplyBeforeRecording` as the user-facing "Enable recording/replay path
  management" switch.
- When enabled, apply the resolved path automatically when recording is
  starting.
- When enabled, also apply the resolved path automatically before the replay
  buffer starts, because OBS saves replay buffer files into the active recording
  directory.
- When disabled, keep previewing and saving plugin settings, but do not write
  OBS' recording directory.
- Create missing directories before writing the OBS recording/replay path.
- Show errors in the dock instead of failing silently.

## MVP Fields

- `baseDirectory`
- `pathTemplate`
- `manualTag`
- `autoApplyBeforeRecording`
- `locale`

## Template Variables

- `{date}`
- `{year}`
- `{month}`
- `{day}`
- `{profile}`
- `{scene_collection}`
- `{scene}`
- `{tag}`

`{app}` is intentionally out of scope for the first usable version.

The path template is always relative to `baseDirectory`. Do not include a
`{base}` variable in the editable template.

## Defaults

- `pathTemplate`: `{date}/{tag}`
- `manualTag`: `untagged`
- `autoApplyBeforeRecording`: `true`
- `locale`: `auto`
