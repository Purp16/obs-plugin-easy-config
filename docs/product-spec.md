# Product Spec: First Usable Version

## Goal

Provide an OBS dock named `Easy Config` for fast OBS state switching and recording-directory automation.

## MVP Behavior

- Show compact controls for Scene, Scene Collection, and Profile.
- Allow switching those OBS states from the dock.
- Save plugin settings globally, not per OBS Profile.
- Use `baseDirectory` plus `{date}/{tag}` as the default recording path rule.
- Resolve and preview the target path in the dock.
- Apply the path manually with `Apply Now`.
- Apply the path automatically when recording is starting if enabled.
- Create missing directories before writing the OBS recording path.
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
