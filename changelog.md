# Changelog

## v0.6.0 Experimental

- Added an in-level Stage Layout Editor.
- Open it from the Stages page while paused; the real level remains visible behind the editor.
- Inner profile boundaries are drawn as draggable vertical lines.
- Dragging a boundary updates both paired 2.1 and 2.2 percentages.
- 0% and 100% remain fixed and boundaries cannot cross each other.
- Added a full-profile ruler for selecting/focusing boundaries that are off-camera.
- Drag empty level space to pan the paused gameplay view.
- Added Prev / Next / Focus plus Save / Cancel / Reset controls.
- Save updates the bound profile crash-safely while preserving repetitions, targets, rounds and stage stats by index.
- Opening/saving the editor does not count as a gameplay attempt.
- Editor UI uses Original, Night, Turkmenistan, Udmurtia, Tatarstan and Custom Colors.
- Existing v0.5.6 manual progress controls and keybinds are retained.

## v0.5.3

### Legacy session recovery
- Added a themed **LEGACY (N)** recovery entry to the Profile Manager.
- Detects pre-profile persistence keys under `levels.online:*` and `levels.local:*` without treating modern `levels.profile-*` payloads as legacy.
- Imports old StartPos 2.1/2.2 boundary pairs, repetition counts, per-stage targets, selected stage, statistics and round data when those fields exist.
- Supports older Baconsistent saves that predate targets/statistics/rounds by leaving missing fields at safe modern defaults.
- Regenerates the plan signature from normalized physical 2.1 boundaries so recovered counts attach to the correct stages.
- Lets the user choose 2.1 or 2.2 for the recovered profile.
- Legacy keys are never deleted or overwritten; re-import remains possible if a recovered profile is later deleted.
- Deleting a carried-over v0.4 `online:*` / `local:*` registry entry now removes it from the modern profile index without erasing its legacy payload or metadata backup.
- Imported profiles and their payload are committed immediately with `Mod::saveData()` for crash safety.
- Original, Night, Turkmenistan and custom colors apply to the recovery UI.


## v0.5.2

### Profile Manager redesign
- Replaced the old inline profile rows with a dedicated themed Profile Manager inspired by Blitzkrieg's separate create/edit profile UX.
- Profile rows now only select a profile; binding and deletion use separate non-overlapping controls in a details panel.
- Fixed the DEL hitbox bug where clicking delete could trigger the full-row Bind action.
- Added explicit selected-profile details, BIND / UNBIND, DELETE and CREATE PROFILE controls.
- Original, Night, Turkmenistan and custom-color palettes now apply to the entire profile manager.

### Crash-safe profile progress
- `flushCrashSafe()` is now write-through: every flush serializes counts, targets, selected stage, statistics and round before `Mod::saveData()`.
- Successful A-to-B repetitions remain immediately committed to disk.
- Profile selection and auto-matched stage changes are also flushed so the active profile state stays internally consistent after a crash.

## v0.5.1

- Fixed Windows/Clang build failure in the pause progress bar caused by passing a runtime-selected format string to fmt 12.
- Marked `TrainingPopup::init()` as `override` to remove the Geode/Clang override warning.
- No gameplay/profile/theme behavior changes from v0.5.0.


## v0.5.0

### Profiles overhaul
- Removed **Load Stage** completely.
- StartPos scanning no longer auto-creates or auto-binds profiles.
- Added a dedicated **Profiles** tab.
- Added manual **Create Profile** from the currently loaded StartPos copy.
- Added profile naming.
- Added per-profile **2.1 / 2.2 percentage mode** selection at creation time.
- Added optional **Bind current level after create** toggle.
- Added explicit **Bind**, **Unbind**, and **Delete profile** controls.
- Saved profiles remain usable even if the physical StartPos copy is later deleted.
- The same profile can be bound to unrelated physical level copies / originals by the user.
- Old v0.4 profile entries remain discoverable, but old automatic bindings are intentionally ignored so v0.5 starts with explicit bindings only.

### Appearance
- Added **Original**, **Night**, and **Turkmenistan** themes.
- Added custom accent, surface, text, muted-text, and success colors.
- Added Night-specific panel / row / tab / progress / header assets.
- Added Turkmenistan-themed panels, progress art, flag, emblem, pause icon behavior and intentionally humorous Ashgabat UI labels.
- Themes remain pause/UI-only and do not add a permanent gameplay HUD.

### Existing systems retained
- Fixed-stage xN repetition training.
- Rounds and statistics.
- Crash-safe saves.
- Practice Mode and noclip validation.
- Pause-only stage progress bar and success notifications.

## v0.4.4
- Fixed physical StartPos / stage index mapping and the zero-stage reset edge case.

## v0.4.3
- Added the first bindable profile registry and removed equal-percentage fallback training.

## v0.4.2
- Fixed false-positive noclip hits during StartPos/reset bookkeeping.

## v0.4.1
- Added crash-safe saves, Practice Mode protection and noclip-suppressed-death validation.

## v0.4.0
- Added pause-only StartPos repetition bar and branded successful-run notifications.

## v0.3.x
- UI/UX overhaul, stage browser, stats, rounds and per-stage targets.
