# Product Spec: First Usable Version

## Goal

Provide an OBS dock named `Easy Config` for fast Profile/Scene Collection
switching, recording/replay-buffer directory automation, video setting
shortcuts, and replay-buffer tuning.

## MVP Behavior

- Show compact controls for Profile and Scene Collection switching.
- Show separate Resolution and FPS shortcut groups with 2-4 editable presets.
- Resolution shortcuts update OBS output resolution only, not canvas size.
- FPS shortcuts update OBS common FPS only.
- Disable Resolution and FPS shortcuts while recording, streaming, or replay
  buffer is active.
- Show replay-buffer duration and memory fields directly in the dock.
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
- `resolutionPresets`
- `fpsPresets`
- `lastReplayBufferSeconds`
- `lastReplayBufferMegabytes`

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
- `resolutionPresets`: `720p`, `1080p`, `1440p`, `4K`
- `fpsPresets`: `30`, `60`, `120`
- `lastReplayBufferSeconds`: `20`
- `lastReplayBufferMegabytes`: `512`
