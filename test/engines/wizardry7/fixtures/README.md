# Wizardry VII test fixtures

Synthetic input files for the engine integration tests
(`test/engines/wizardry7/fixtures.h`). They exercise the file-loading entry
points (`load(const Common::Path &)`) that the engine uses against real game
data, which the inline buffers in `parsers.h` cannot reach.

**Provenance:** every byte is written by `tools/make_fixtures.py` from the
format descriptions in the RE repository's `docs/formats/`. No byte comes from
a shipped game file, so these files are safe to keep in the GPLv3+ tree.
Never replace one with a slice of real data — if a test needs real data it must
read `WIZ7_GOLD_DATA` / `WIZ7_DOS_DATA` and skip when unset.

| file | contents |
|------|----------|
| `vga_bundle.vga` | `VgaBundle`: one 8x1 chunk plus one placement record |
| `font.vga` | `GameFont`: two 2x2 1bpp glyphs (codes 0 and 1) |
| `save_dos.sav` | DOS save container: party header + one 584-byte record |
| `roster.pcf` | PCFILE roster: 24-byte header, two slots (`AAA`, `BBB`) |
| `msg_gold.hdr`, `msg_gold.gld` | Gold message index + plain string pool |
| `msg_dos.hdr`, `msg_dos.dbs` | DOS message index + Huffman string pool |
| `misc.hdr` | The 512-entry Huffman tree the DOS pool is coded with |
| `master.hdr`, `disk.hdr`, `scenario.gld` | SCENARIO database: XP, item, maze and monster records |

Regenerate after a format change:

```sh
tools/make_fixtures.py           # rewrite the files
tools/make_fixtures.py --check    # verify them (non-zero exit if stale)
```

The build passes this directory to the test suite as `WIZ7_FIXTURE_DIR`
(`test/module.mk`), and the tests add it to `SearchMan`, so the engine resolves
a fixture name exactly the way it resolves `MSG.DBS`, `SCENARIO.GLD` and the
rest. Sizes are synthetic except where a parser requires a specific record
size (item 72, maze 2768, monster 232, XP 64, character 584).
