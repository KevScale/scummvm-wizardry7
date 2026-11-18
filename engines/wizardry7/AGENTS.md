# AGENTS.md — Wizardry VII engine (ScummVM)

This directory is the **reimplementation** side of the Wizardry VII project. The
reverse-engineering evidence (decompiled functions, format dumps, runtime traces,
screenshots) lives in the **separate RE repository** this ScummVM checkout is
nested in:

```
<re-repo>/                 docs/, notes/, tools/, originals/, extracts/, DEVLOG.md, TODO.md
<re-repo>/scummvm/         this GPLv3+ tree (git submodule, branch `wizardry7`)
```

Keep that boundary intact. The implementation is written **from the written
specifications** in `<re-repo>/docs/` — not by copying decompiler output. Never
paste `notes/decomp/` chunks, addresses, raw game data, or copyrighted assets
into this tree.

## Project-owner guidelines (verbatim requirements)

1. **Both builds are first-class.** Support **Wizardry Gold** (Windows PE data,
   `Basedata/*.GLD`) *and* the **DOS original** (1992 / 1996 Archives,
   `MSG.DBS` + `MISC.HDR` Huffman pool) assets. Every data-layer feature must
   work on both; do not add a Gold-only shortcut without documenting why.
2. **First engine implementation inside ScummVM.** Precedent: the Might and
   Magic (MADS) engine. ScummVM lives in a subfolder of the RE repo and is kept
   as a **git submodule**; engine work happens on a dedicated branch
   (`wizardry7`), never on upstream `master`.
3. **Clean-room separation.** RE docs must not collide with the
   reimplementation. This directory may cite spec documents by name, but must
   not contain or embed RE notes. Legal safety beats convenience.
4. **No game assets.** Never commit anything from `originals/` or `extracts/`.
   Test fixtures must be user-supplied (except our own generated saves).
5. **Research TODOs belong to the RE repo.** If implementation stalls on an
   unknown format or behaviour, add a concrete item to `<re-repo>/TODO.md`
   (with the evidence needed to resolve it) instead of guessing in code.
6. **Keep the dev log.** Meaningful discoveries, corrections, tool failures and
   design decisions go into `<re-repo>/DEVLOG.md` as they happen.
7. **Commit discipline.** Make small, meaningful commits on the `wizardry7`
   branch, following ScummVM commit conventions (`wizardry7: summary`, imperative
   mood, one logical change). Build (or at least compile the engine) before
   committing. No drive-by edits to unrelated engines.
8. **Visual fidelity.** The RE repo's central capture structure is `screens/`
   (`screens/README.md`; canonical names `01_char_menu` … `12_town_shop`, Gold in
   `screens/gold/`); the older granular DOS set is `data/ui-shots/`. Use them as
   the target for layout/colour work and cite the screen name when matching a
   frame; do not ship screenshots here.
9. **ScummVM rules apply.** Read `../../AI-GUIDELINES.md`. The human submitter is
   the author; do not add AI co-author trailers. This branch is experimental and
   must not be proposed upstream without a human maintainer taking ownership,
   reviewing, and being able to maintain it.

## Owner decisions (Nov 2026)

- Both builds are first-class (Gold + DOS); PSX later.
- Gold reference screenshots are captured by a parallel effort into the RE
  repo's central `screens/` structure (`screens/README.md`, `screens/gold/`),
  never here. The older DOS set lives in `data/ui-shots/`.
- Copy protection is skipped entirely (matches the currently sold GOG
  distribution; not reverse-engineered).
- Audio: both digital and MIDI, but a secondary priority after gameplay.
- Fork-first, upstream later. The official ScummVM inclusion requirements and
  the implications are summarised in the RE repo's `reimpl/PLAN.md` §2.1.

## Current milestone

M1 asset/test bring-up, not a playable game:

- scenario record database (MASTER.HDR + DISK.HDR + SCENARIO.GLD/.DBS)
- message database (Gold plain pool + DOS Huffman pool via MISC.HDR)
- Wichmann-Hill RNG (exact 16-bit behaviour)
- `PALETTE.DEF`, raw 320×200 screen dumps, VGA image bundles, bitmap fonts
- debug console: `msg`, `msginfo`, `records`, `record`, `rng`, `rngseed`,
  `status`, `screen`, `screenshot`, `bundle`, `fontinfo`, `glyph`, `glyphs`,
  `text`, `saveinfo`, `chargen`, `chargenui`, `create`, `party`, `item`,
  `sheet`, `review`, `script`
- headless scenario runner via the target config key `wizardry7_script`
  (plus `enable_unsupported_game_warning=false`); use
  `tools/run_scenario.py <data-dir> <script>` from the RE repo
- status/title screens and pixel-exact BMP screenshots; no game loop yet

## Integration points (only these touch upstream files)

- `engines/wizardry7/configure.engine` — auto-discovered by `configure`
- `engines/wizardry7/module.mk` — module objects + `DETECT_OBJS`
- `test/engines/wizardry7/` + one conditional block in `test/module.mk`
  (only if engine unit tests are enabled)

Everything else stays inside `engines/wizardry7/`.

## Data layout

| Build | Game directory | Messages | Scenario | Notes |
|-------|----------------|----------|----------|-------|
| Gold  | root containing `Basedata/` | `Basedata/MSG.GLD` (plain) | `Basedata/SCENARIO.GLD` | engine adds `Basedata` to SearchMan |
| DOS   | flat directory | `MSG.DBS` + `MISC.HDR` (Huffman) | `SCENARIO.DBS` | identical record layout |

Detection keys on the data files, not on `WIZARD.EXE`/`DS.EXE`, so users do not
need the original executables to play.

## Build / run (validation)

```sh
cd <re-repo>/scummvm
./configure --disable-all-engines --enable-engine=wizardry7
make -j$(nproc)
./scummvm --path=<gold-root> wizardry7
./scummvm --path=<dos-data-dir> wizardry7
```

Open the debug console (Ctrl+Alt+D) and try `msg 100`, `records`,
`record 1 1`, `rng 10`, `screen TIT1SCRN`, `bundle MAZEDATA.VGA`,
`fontinfo VBFONT0.VGA`, `glyphs VBFONT0.VGA 32 95`, `text VBFONT0.VGA 100`,
`saveinfo SAVEGAME.DBS`, `screenshot out.bmp`.
