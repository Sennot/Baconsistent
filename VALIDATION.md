# Baconsistent v0.6.0 validation

This source tree is designed as a repository-root archive: `mod.json`, `CMakeLists.txt`, `.github`, `src`, `resources`, and `tests` live directly at the root.

## Sandbox checks

- Standalone C++23 training/core tests with GCC.
- v0.6.0 source check verifies the pause progress title uses separate compile-time `fmt::format` literals for Original/Turkmenistan/Udmurtia/Tatarstan rather than a runtime-selected format string.
- `TrainingPopup::init()` is explicitly marked `override`.
- Manual progress core tests cover increment, decrement, complete, clamping, and reopening a completed stage.
- `mod.json` includes native Geode keybind settings for +1 and Complete Stage.
- `-Wall -Wextra -Werror -pedantic` enabled for the standalone test build.
- Standalone C++23 tests with Clang + AddressSanitizer + UndefinedBehaviorSanitizer.
- `mod.json` JSON parse, v0.6.0 metadata check, short-description limit check, theme enum/color-setting checks.
- GitHub Actions YAML parse / Windows-only workflow check.
- PNG signature / decode validation for all bundled resources.
- Source grep verifies `Load Stage`, `onLoadStage`, and StartPos-activation UI code are removed.
- Source grep verifies v0.5 StartPos scanning does not call profile bind/register automatically from `loadLevel`; only an explicit saved v2 binding can activate training.
- Profile-create source check verifies the 2.1/2.2 chooser, optional bind toggle, and no implicit binding.
- Final ZIP is extracted into an empty directory and standalone tests are rerun from the extracted source.

## Important limitation

The sandbox does not contain a local Geode SDK installation, so a full native Geometry Dash / Geode Win64 compile-link cannot be truthfully validated here. The included Windows GitHub Actions job is the final SDK/ABI build check.


## v0.5.2 profile checks
- Dedicated ProfileManagerPopup source present.
- Profile list rows are selection-only; Bind/Unbind/Delete are separate callbacks and hit targets.
- `flushCrashSafe()` writes progress, selected stage, stats and round before `Mod::saveData()`.

## v0.5.3 legacy recovery checks
- Pure C++ tests cover parsing `levels.online:*` / `levels.local:*` legacy saved keys and reject modern `levels.profile-*` payloads.
- Source check verifies legacy import copies counts/targets/stats/selected/round, regenerates plan signature, keeps old keys untouched, and calls `Mod::saveData()`.
- The Profile Manager exposes the themed Legacy Recovery popup and does not auto-bind an imported profile.

- Udmurtia preset / ud-* asset routing / pause icon / profile manager branding static checks: PASS
- Udmurtia asset decode and expected dimensions: PASS

- Stage Layout Editor core draft regression: fixed 0/100, ordered paired boundaries, drag clamp and reset.
- Source check: editor is hosted inside PauseLayer, uses real PlayLayer geometry, and profile saves remain crash-safe.
