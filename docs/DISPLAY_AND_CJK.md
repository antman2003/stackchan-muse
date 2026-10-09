# Display and CJK

> Moved from the workspace lab notebook on 2026-10-09.
> Device identifiers (SSID, LAN address, MAC, BLE name, and a personal name in a sample reply) are replaced with placeholders.
> Cross-links that pointed at old README anchors are rewritten below where they would break.

## Status

Readable mixed Chinese and English conversation display is **not finished**.

What exists:

- Muse already returns Chinese text. Short replies are readable at the stock font size.
- A generated 20 px CJK font exists at `stackchan-muse/esp32/components/muse/muse_font_cjk_20.c` and again in `lab/fonts/`. Both files are **4,463,636 bytes**. That number is the generated C source size. Neither file is referenced by the firmware build.
- **693 KB is a different quantity, and it is not stale.** The research below uses it for the uncompressed binary flash estimate (`--format bin --no-compress`, 2 bpp, GB2312 subset). The same notes already say the font is about 4.3 MB of C source and about 693 KB of flash. Do not plan flash from the `.c` file size.
- The C file that would be linked stores compressed bitmaps (`bitmap_format = 1`). Its `glyph_bitmap` array is **537,954 bytes**. Glyph descriptors and character maps add more. The linked flash total has not been re-measured with a build. The uncompressed `.bin` probe is not in the tree, so 693 KB was not recomputed from that file.
- Captions can appear, including while the xiaole prototype is speaking. That is not the finished conversation layout.

What is still required: CJK fallback for the full reply, correct line breaking, a readable size, and paging or scrolling for long replies, with the avatar still on screen.

## Caption and Chinese font research

### 2. Readable captions + Chinese [2][3] — one change, mostly done

**Status: not finished.** The heading is the old lab label. Readable Chinese conversation display is still open. The font file is not in the build.

These are the same fix: a CJK font ships Latin glyphs too, so one font solves
both. **Root cause of the tiny text** is that `muse_ui.c` classifies this board
as a small screen —

```c
bool short_landscape = s_w > s_h && s_h < 320;   /* 320>240 && 240<320 => true */
s_small = s_h < 200 || s_w < 200 || short_landscape;
```

That rule was written for the ESP32-S3-BOX-3, which has the identical 320x240
panel. Being `s_small`, `font_pick()` returns the compact option everywhere, so
captions render in **`lv_font_unscii_8` — 8 pixels tall on a 240-pixel screen**.

**Root cause of blank Chinese** is purely missing glyphs. The text pipeline
already passes CJK through untouched: in `muse_text_ascii()` a codepoint like
U+4E2D is not in the Latin range and matches no emoji/symbol range, so
`stand_in()` returns `NULL` and the function returns `-1` meaning "keep it".
The bytes reach LVGL intact; `sdkconfig.muse` just enables ASCII-only fonts
(`unscii_8/16`, `montserrat_14/16/20/28`).

#### Read PR #32 before touching this — it is ahead of us

[PR #32](https://github.com/facebookincubator/muse-gadget-sdk/pull/32) already
solves Chinese captions, and does it better than my plan in two ways:

- **It uses LVGL's `fallback` chain instead of replacing the font.** The
  caption font stays unscii-16 and merely points its `fallback` at a CJK font,
  so ASCII keeps the crisp original glyphs and, crucially, **the cell size is
  unchanged, so caption columns and paging still work**. My plan swapped in a
  20 px Noto wholesale, which would have silently broken the column maths that
  `CAPTION_W`, `MINI_CELL_PX` and the reply pager depend on.
- **It fixes line breaking, which I had missed entirely.** CJK has no spaces,
  so `next_line()` treats a whole Chinese sentence as one unbreakable word and
  breaks at the last space, leaving ragged, half-empty pages. The PR allows
  breaks between CJK characters, forbids a line starting with closing
  punctuation, and keeps ASCII runs like `30%` whole. Without this, Chinese
  would render and still look broken.

Their font is GNU Unifont 16.0.04 at 16x16 covering U+4E00–U+9FFF plus kana,
CJK punctuation and fullwidth forms: ~850 KB of flash, no RAM, from a 6.8 MB
generated C file, behind `CONFIG_MUSE_CJK_FONT` (off by default). Licensing is
SIL OFL 1.1 plus GPLv2 with the font-embedding exception. It ships host tests
(`test_muse_caption_wrap.py`) and was verified on real hardware.

**Our plan:** take their `fallback` structure and their line-break fix
wholesale, then layer our own change on top — because they kept 16 px and you
asked for *bigger*. Our Noto font is nicer and smaller (693 KB vs 850 KB), so
substitute it if it can be made to sit on a matching cell. Going larger means
redoing the sizing maths deliberately rather than by accident, which is exactly
the work their approach makes visible.

Done already: `lab\fonts\gen_font.py` generates
`muse_font_cjk_20.c` (693 KB of flash) and it is copied into
`components/muse/`. Still to do:

1. Add `"muse_font_cjk_20.c"` to `srcs` in `components/muse/CMakeLists.txt`.
2. In `muse_ui.c`, add `LV_FONT_DECLARE(muse_font_cjk_20);` and use it for
   `s_caption_lbl`, sizing the band from `lv_font_get_line_height()` rather
   than the hardcoded `2 * 8 + 2 + 4`.
3. Also switch `s_reply_lbl` (its font is pinned to `lv_font_unscii_16` at
   ~line 668), or Chinese reply *pages* stay blank even once captions work.
   Its layout assumes a monospace grid (`cw = glyph_width('M')`); hanzi advance
   exactly 20 px while Latin is ~half, so derive `cw` from a hanzi to stay
   conservative and let LVGL wrap.
4. Consider `CONFIG_LV_FONT_FMT_TXT_LARGE=y`. At 693 KB we are under the 1 MB
   (2^20) limit of the default 20-bit `bitmap_index`, but not by much.
5. Build, flash app only, verify with `zhtest.py`.

Font sizing is a real budget decision. Measured at 20 px over the **whole** CJK
block: 1 bpp = 1,032 KB, 2 bpp = 2,117 KB, 4 bpp = 4,042 KB — and only about
2,044 KB of the 4 MB app partition is free, so full coverage forces jagged
1 bpp. Subsetting to GB2312's 6,763 hanzi (effectively all everyday simplified
Chinese) is a third of the glyphs and buys anti-aliasing: **693 KB at 2 bpp**.

Note Noto CJK declares `line_height = 38` at this size because of a few outlier
glyphs, while 99% of hanzi fit in 18 px above the baseline and 3 px below.
LVGL uses `line_height` as line pitch, so leaving it at 38 would spend a third
of the screen on two caption lines. `gen_font.py` rewrites it to 24/4.


### Fonts on embedded LVGL

- Measure in **binary**, not in the generated `.c`. Our font is 4.3 MB of C
  source but only 693 KB of flash. Generate with `--format bin --no-compress`
  first purely to size it.
- Generate the glyph list from `codecs`, not a downloaded list: iterating
  valid GB2312 byte pairs in Python yields exactly 6,763 hanzi, offline and
  deterministic.
- CJK font vertical metrics are often far looser than the ink needs. Measure
  the real extents from the glyph descriptors and override `line_height` /
  `base_line`, and do it **in the generator** so it survives regeneration.
- Full CJK coverage versus anti-aliasing is a genuine trade: at a fixed size you
  can have all 21k glyphs at 1 bpp, or a 6.7k subset with smoothing, but not
  both.
- CJK fonts break monospace layout assumptions: hanzi advance a full em while
  Latin is about half. Code computing columns from `glyph_width('M')` will be
  wrong.


## Upstream CJK prior art

The open upstream pull request for a real CJK font (PR #32, about 1.3 MB, Traditional Chinese) is the main reference for how to add a font without forking the whole UI. See also the upstream table in `DEVELOPMENT_HISTORY.md`.

Do not treat PR #32 as something this tree has merged. **needs re-verification** before copying its approach: confirm the PR is still open and that its font still fits the 4 MB `ota_0` slot beside the current avatar.
