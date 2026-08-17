# Third-party notices

## Blitzkrieg

Baconsistent uses Blitzkrieg as an open-source technical reference for StartPos discovery, legacy/modern percentage compatibility, pause-training UX patterns, and the separation of reusable training profiles from physical level bindings. Baconsistent adapts those ideas to a different progression model: fixed stages, configurable repetition targets, statistics and automatic rounds. No Blitzkrieg art assets are bundled.

Upstream repository: `https://github.com/ZhulinskiiDanil/blitzkrieg`

The upstream project is distributed under the MIT License. Its notice is preserved below.

MIT License

Copyright (c) 2026 Zhulynskyi Danylo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


## Death Tracker

Baconsistent uses Death Tracker (`abb2k/death-tracker`) as a technical reference for detecting a noclip-suppressed death through the `PlayLayer::destroyPlayer` hook chain. Baconsistent intentionally adapts the behavior to its own training rules: noclip being enabled is not enough to invalidate a run; only an actually suppressed lethal collision before the fixed-stage endpoint blocks that attempt. No Death Tracker art assets are bundled.

Upstream repository: `https://github.com/abb2k/death-tracker`

## Turkmenistan state symbols (UI theme)

The optional Turkmenistan joke theme includes rasterized copies/derivatives of the national flag and state emblem sourced from Wikimedia Commons:

- `Flag of Turkmenistan.svg` — Wikimedia Commons; state-symbol/public-domain/CC0 source metadata.
- `Emblem of Turkmenistan.svg` — Wikimedia Commons; public-domain state-symbol source metadata.

These assets are used only as decorative UI elements in the optional theme. Baconsistent is not affiliated with or endorsed by the Government of Turkmenistan.


## Udmurt Republic state symbols (UI theme)

The optional Udmurtia joke theme uses original Baconsistent raster artwork derived from the public design of the Udmurt Republic flag and coat-of-arms color/symbol language: equal vertical black, white and red flag bands and the red eight-point solar sign. The bundled `ud-flag.png`, `ud-emblem.png`, and `ud-*` UI assets were generated specifically for Baconsistent rather than copied from a third-party art pack.

Design references:
- Federation Council regional-symbol page for the Udmurt Republic.
- Udmurt Republic flag law / public descriptions of the official flag.
- Udmurt regional branding resource describing the eight-point solar sign used on the flag and coat of arms.

These assets are decorative UI elements in an optional joke theme. Baconsistent is not affiliated with or endorsed by the authorities of the Udmurt Republic.
