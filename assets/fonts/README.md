# assets/fonts

Barlow and Barlow Condensed (SIL OFL 1.1) converted for LVGL at the sizes and glyph sets in
`docs/specification/02-ui-spec.md` §2.2, plus the licence file and the script that regenerates them
(Slice 3). Budget for all fonts: 300 kB.

Files:

- `src/`: the four TTFs and `OFL.txt`, downloaded from google/fonts at the commit in `SOURCES.md`.
- `gen_fonts.py`: downloads missing sources and runs `npx lv_font_conv@1.5.3` with LVGL 9.6's own
  options; the 14 generated fonts go to `lib/ui/src/fonts/` (compiled with `lib/ui`), declared in
  `lib/ui/src/fonts/fonts.h`. Regenerating gives identical files. Needs Node.js.
