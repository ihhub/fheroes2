# [fheroes2](../README.md) Chinese (CJK) Support

This document describes how Simplified and Traditional Chinese rendering works in fheroes2 and what the
user has to provide to use it. Unlike the other localizations, Chinese is not rendered through an 8-bit
code page baked into the bitmap font. Instead it is kept as UTF-8 and drawn at runtime through SDL_ttf
using a separate CJK font.

## Build requirements

CJK rendering depends on **SDL_ttf**:

- CMake: the `ENABLE_CJK_TEXT` option is `ON` by default. SDL_ttf is detected automatically; if it is not
  found, the game still builds and simply runs without CJK rendering.
- Makefile: pass `FHEROES2_WITH_CJK_TEXT=1` (auto-detected through `pkg-config` when available).

Decoding of legacy map text (see below) additionally depends on **iconv** on non-Windows platforms
(`FHEROES2_WITH_ICONV=1` / detected automatically by CMake). On Windows the system code page conversion
API is used instead, so iconv is not required there.

If the game is built without SDL_ttf, Simplified/Traditional Chinese are hidden from the language
selection dialog and the game falls back to English.

## Providing a CJK font

**No CJK font is bundled with the project.** Fonts are large and have their own licensing terms, so each
user supplies one. The font is located in this order:

1. The `cjk_font_path` value in `fheroes2.cfg`, pointing to a `.ttf`, `.ttc`, or `.otf` file. This may be
   a redistributable font shipped alongside the game or a font already installed on the system.
2. Any `.ttf` / `.ttc` / `.otf` file placed under `files/fonts/` next to the game data.

When packaging a build that bundles a font, make sure the font's license permits redistribution
(for example the SIL Open Font License). Do **not** commit font files into the repository.

## Legacy map text decoding (GBK / Big5)

Classic Heroes of Might and Magic II maps (`.mp2`) store text in legacy Chinese encodings (GBK for
Simplified Chinese, Big5 for Traditional Chinese), not UTF-8. fheroes2 decodes such text to UTF-8 on the
fly so it can be displayed with the CJK font.

This decoding is **only attempted when the active text language is Simplified or Traditional Chinese**:

- For original `.mp2` maps there is no reliable per-map language marker, so the **current UI language** is
  used. If the UI is set to English while loading a Chinese `.mp2` map, the legacy text is left untouched
  and will appear as raw bytes. Switch the UI to the matching Chinese variant to see it decoded.
- For fheroes2 (`.fh2m`) maps the language selected for the map guides the decoding.

This is an intentional trade-off: blindly reinterpreting every high-bit byte sequence as GBK/Big5 would
corrupt text in other languages. Text that already is valid UTF-8 is never re-decoded.
