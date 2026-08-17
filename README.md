# Baconsistent

**Baconsistent** is a Windows-only Geode training mod for Geometry Dash 2.2081 by **Strafe**. It implements a fixed-part consistency method: every StartPos-defined stage has a target number of clean completions (20 by default), and finishing every stage advances the profile to the next round.

## v0.6.1 experimental highlights

- Experimental Stage Layout Editor inside the real paused level.
- Every inner profile boundary is drawn as a draggable vertical line.
- Drag a line to change the two adjacent stage percentages.
- Drag empty level space to pan; Prev/Next + Focus jump between boundaries.
- Save writes both paired 2.1 and 2.2 boundaries to the profile. Cancel discards edits.
- 0% and 100% stay fixed. Repetition counts and targets stay attached by stage index.
- Full Original / Night / Turkmenistan / Udmurtia / Tatarstan theme support.


### Manual progress

The selected stage now has **-1**, **+1**, and **complete** controls. Manual changes only edit repetition progress; they do not add attempts, successes, streaks, or playtime.

Two configurable gameplay keybinds are included:

- **F6** by default: add +1 repetition to the currently active stage.
- **F7** by default: complete the currently active stage.

Every manual change uses the same crash-safe profile save path as normal progress.

### Legacy session recovery

The Profile Manager now detects Baconsistent save data from the pre-profile releases (including the old `levels.online:*` and `levels.local:*` persistence layout). Open **Profiles → LEGACY (N)** to recover an old session into a new explicit profile.

Recovery copies the old StartPos layout, repetition counters, targets when present, selected stage, statistics when present, and round number when present. Older versions that did not yet have targets/stats/rounds still import safely with modern defaults for the fields they never stored.

The recovery tool never deletes or rewrites the original legacy keys. Even carried-over v0.4 `online:*` / `local:*` profile entries are protected from destructive deletion so they remain recoverable. After import, the new profile is saved immediately with `Mod::saveData()`. You may choose 2.1 or 2.2 percentages for the recovered profile and then bind it to any level normally; import itself does not auto-bind.

### Explicit profiles, Blitzkrieg-style

Profiles and physical levels are now completely separate.

1. Open a level copy that actually contains StartPos objects.
2. Pause → **Baconsistent → Profiles → Create Profile**.
3. Choose the profile name and whether this profile uses **2.1 percentages** or modern **2.2 percentages**. 2.1 is preselected to match the familiar Blitzkrieg creation flow, but it can be toggled off.
4. Creating a profile does **not** automatically attach the level unless you explicitly enable **Bind current level after create**.
5. On any other level, open **Profiles** and press **BIND** on a saved profile.
6. Use **Unbind** to detach only the current level, or **DELETE** in the Profile Manager to remove the profile and its saved training data.

A profile keeps its StartPos boundary data, repetition counters, per-stage targets, rounds and statistics independently of the StartPos copy. The original copy may be deleted after the profile is created.

If a level is not explicitly bound to a profile, Baconsistent does not track runs, show its pause progress bar, or send training notifications. Merely having StartPos objects is not enough to activate training.

### 2.1 / 2.2 is per profile

The percentage system is chosen when a profile is created:

- **2.1**: X-position based legacy percentages.
- **2.2**: time-based percentages via the modern level timing model.

Different profiles can use different percentage systems at the same time. There is no global 2.1 toggle anymore.

### Themes and personalization

The pause UI has five presets:

- **Original** — the existing Baconsistent bacon / warm style.
- **Night** — dark navy / violet interface.
- **Turkmenistan** — intentionally excessive joke theme with Turkmenistan flag/emblem imagery, green/red/gold surfaces, and Ashgabat-themed UI text.
- **Udmurtia** — black/white/red joke theme with Udmurt solar-sign imagery and Izhevsk-themed UI text.
- **Tatarstan** — green/white/red joke theme with Kazan-themed UI text.

Enable **Custom colors** in Geode settings to override accent, surface, text, muted text, success, bacon-secondary, and danger colors without changing the selected layout preset.

Themes affect Baconsistent UI only. The mod still has no permanent gameplay HUD.

## Training behavior

- Stages come only from a saved StartPos profile.
- Each stage is fixed; it never grows after one lucky completion.
- Default target is 20 repetitions, with per-stage target overrides.
- A successful A→B run is counted immediately at B and may continue into a larger run.
- After every stage reaches its target, Baconsistent starts a new round while keeping lifetime statistics and round history.
- The pause screen can show the active stage's repetition progress using GD's Practice Mode progress area.
- Short branded notifications can appear after successful runs.

## Run validity

Two independent settings are available:

- **Practice Mode protection** — attempts that use Practice Mode can be completely ignored.
- **Noclip protection** — noclip itself is allowed, but a stage is ignored if GD actually attempts a lethal collision before B and another hook/mod suppresses that death. A clean A→B run with noclip enabled may still count.

## Crash-safe data

Every explicit crash-safe flush now serializes the profile's repetition counts, targets, selected stage, statistics and round, then immediately calls Geode's `Mod::saveData()`. Successful A→B completions therefore reach disk immediately instead of waiting for a normal Geometry Dash shutdown.

## Platform

- Geometry Dash: **2.2081**
- Geode: **5.8.2**
- Platform: **Windows x64 only**

## Building

Set `GEODE_SDK` to a Geode 5.8.2 SDK checkout and configure normally with CMake. The repository also contains a Windows-only GitHub Actions workflow using `geode-sdk/build-geode-mod`.

## License

Baconsistent is MIT licensed. See `THIRD_PARTY_NOTICES.md` for technical references and the theme references used by the regional joke themes.


## Profile Manager

v0.6.1 keeps the dedicated themed profile manager from v0.5.3. Select a profile in the left list, then use separate Bind/Unbind/Delete controls in the right details panel. Profile progress is write-through saved after every successful fixed-stage completion.
