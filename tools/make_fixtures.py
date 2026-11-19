#!/usr/bin/env python3
# ScummVM - Graphic Adventure Engine
#
# ScummVM is the legal property of its developers, whose names
# are too numerous to list here. Please refer to the COPYRIGHT
# file distributed with this source distribution.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Generate the Wizardry VII engine fixture files.

The engine test suites used to build their input buffers inline in C++
(``test/engines/wizardry7/parsers.h``). That exercises the parsers, but not
the file-loading path (``Common::Path`` + ``Common::File``) that the engine
actually uses, and it cannot be reused by scenario replays.

This script writes small, deterministic *synthetic* fixture files to disk so
``test/engines/wizardry7/fixtures.h`` can load them through the real
``load(const Common::Path &)`` entry points:

    vga_bundle.vga                 VgaBundle with one chunk + one placement
    font.vga                       GameFont with two 2x2 glyphs
    save_dos.sav                   minimal DOS save container with one record
    roster.pcf                     two-slot PCFILE roster
    msg_gold.hdr  msg_gold.gld     Gold message index + plain string pool
    msg_dos.hdr   msg_dos.dbs      DOS message index + Huffman pool
    misc.hdr                       DOS Huffman tree (512 signed entries)
    master.hdr  disk.hdr  scenario.gld
                                   SCENARIO database: XP, item, maze, monster

None of the bytes come from the shipped game: every field is written from the
format descriptions in the RE repository (``docs/formats/``) with placeholder
text. Nothing here may ever be replaced by, or derived from, game data.

Usage
-----
    tools/make_fixtures.py                 # (re)write the committed fixtures
    tools/make_fixtures.py --check         # verify the committed files, exit 1
                                           # if any is missing or stale
    tools/make_fixtures.py --out DIR       # write somewhere else
    tools/make_fixtures.py --list          # names only
"""

import argparse
import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_OUT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "test", "engines", "wizardry7", "fixtures"))

# Sizes the engine parsers require (see engines/wizardry7/*.h).
CHARACTER_SIZE = 584          # kCharacterSize
ROSTER_HEADER_SIZE = 24       # kRosterHeaderSize
DOS_PARTY_BASE = 844 + 8192   # scenario block + lair/automap, no patch/drop lists
DOS_RANDOM_SIZE = 100         # kDosRandomSize
DOS_RECORD_OFFSET = DOS_PARTY_BASE + 0x4C + 1608 + DOS_RANDOM_SIZE   # = 10820
MAZE_RECORD_SIZE = 2768       # MazeRecord::kRecordSize
ITEM_RECORD_SIZE = 72
MONSTER_RECORD_SIZE = 232
XP_RECORD_SIZE = 64
FONT_HEADER_SIZE = 0x110


def u16(value):
    return struct.pack("<H", value)


def u32(value):
    return struct.pack("<I", value)


def seeded(size):
    """A bytearray of ``size`` zeros; fixtures fill every significant byte."""
    return bytearray(size)


# ---------------------------------------------------------------------------
# Graphics: VgaBundle + GameFont
# ---------------------------------------------------------------------------

def make_vga_bundle():
    """One 8x1 chunk plus one placement record.

    Header: u16 chunk count, u16 record count. Then per chunk a 6-byte entry
    (segment, offset, width/8 | height<<8) and per placement a 5-byte entry
    (bitmap, x as int8, y, xExtra, rows). The first chunk must start right
    after both tables.
    """
    chunk_data = bytes(range(1, 9))
    table_end = 4 + 6 + 5
    data = bytearray()
    data += u16(1)                       # chunk count
    data += u16(1)                       # placement record count
    data += u16(0)                       # segment
    data += u16(table_end)               # offset within the segment
    data += u16((1 << 8) | 1)            # width 1*8, height 1
    data += bytes([3, 0xFE, 4, 5, 6])    # bitmap 3, x -2, y 4, xExtra 5, rows 6
    data += chunk_data
    return bytes(data)


def make_font():
    """Two 2x2 1bpp glyphs, codes 0 and 1, one byte of advance each."""
    data = seeded(FONT_HEADER_SIZE + 2 * 2)
    data[0] = 2                  # width
    data[1] = 2                  # height
    data[2] = 0                  # pixel mode: 1bpp
    data[3] = 1                  # stride
    data[4] = 1                  # mask stride
    data[5] = 2                  # glyph count
    data[6] = 2                  # flags
    data[7] = 0                  # base advance
    data[10:12] = u16(2)         # bytes per glyph
    data[12:14] = u16(FONT_HEADER_SIZE + 2 * 2)   # second block offset (unused)
    # Character table at 0x10: two bytes per code (glyph index, advance).
    data[0x10:0x14] = bytes([0, 1, 1, 1])
    data[0x110:0x114] = bytes([0x80, 0x40, 0x40, 0x80])
    return bytes(data)


# ---------------------------------------------------------------------------
# Saves: DOS save container + PCFILE roster
# ---------------------------------------------------------------------------

def fill_dos_character(record, name, hp=10, hp_max=20):
    """Fill one 584-byte DOS character record (offsets from docs/formats/saves.md)."""
    raw = bytearray(CHARACTER_SIZE)
    raw[0:len(name)] = name.encode("ascii")[:11]
    raw[0x08] = 0x58                     # age in days
    raw[0x09] = 0x1B
    raw[0x0C] = 123                      # xp
    raw[0x18] = hp
    raw[0x1A] = hp_max & 0xFF
    raw[0x1B] = (hp_max >> 8) & 0xFF
    raw[0x1C] = 30                       # stamina
    raw[0x1E] = 40                       # stamina max
    raw[0x20] = 100                      # weight
    raw[0x22] = 50                       # carrying capacity
    raw[0x24] = 1                        # level
    raw[0x1B6] = 7                       # AC
    raw[0x170] = 12                      # STR tier
    raw[0x177] = 12                      # portrait
    raw[0x1B9] = 10                      # AC slot copies
    raw[0x178] = 10                      # weaponry[0]: SWORD
    raw[0x236] = 4                       # swimming
    raw[0x237] = 3                       # climbing
    raw[0x238] = 1                       # gender: female
    raw[0x239] = 2                       # class: priest
    raw[0x23A] = 3                       # alignment
    raw[0x40 + 6] = 1                    # first inventory slot count
    record[0:CHARACTER_SIZE] = raw
    return record


def make_dos_save():
    """844 B scenario block, 8192 B lair, party header, state, one record.

    patchCount and dropCount stay zero, so the party header sits at 9036 and
    the character record at 10820 (see SaveGame::loadFromData).
    """
    data = seeded(DOS_RECORD_OFFSET + CHARACTER_SIZE)
    data[0] = 0x0A                       # DOS in-progress save tag
    base = DOS_PARTY_BASE
    data[base + 0x00:base + 0x02] = u16(14)     # maze
    data[base + 0x02:base + 0x04] = u16(56)     # world X
    data[base + 0x04:base + 0x06] = u16(57)     # world Y
    data[base + 0x06:base + 0x08] = u16(7)      # local X
    data[base + 0x08:base + 0x0A] = u16(6)      # local Y
    data[base + 0x0A:base + 0x0C] = u16(5)      # window
    data[base + 0x0C:base + 0x0E] = u16(2)      # facing
    data[base + 0x0E:base + 0x10] = u16(14)     # previous maze
    data[base + 0x10:base + 0x12] = u16(55)     # previous X
    data[base + 0x12:base + 0x14] = u16(57)     # previous Y
    data[base + 0x26:base + 0x28] = u16(1004)   # party counter
    data[base + 0x4A] = 1                       # character count
    fill_dos_character(memoryview(data)[DOS_RECORD_OFFSET:DOS_RECORD_OFFSET + CHARACTER_SIZE], "TEST")
    return bytes(data)


def make_roster():
    """Two-slot PCFILE roster: 24-byte header, 584-byte records, slot 0 used."""
    data = seeded(ROSTER_HEADER_SIZE + 2 * CHARACTER_SIZE)
    data[0:2] = u16(CHARACTER_SIZE)      # stride
    data[2:4] = u16(2)                   # slots
    data[4:8] = u32(ROSTER_HEADER_SIZE)  # header size
    data[8] = 1                          # slot 0 used
    data[ROSTER_HEADER_SIZE:ROSTER_HEADER_SIZE + 3] = b"AAA"
    data[ROSTER_HEADER_SIZE + CHARACTER_SIZE:ROSTER_HEADER_SIZE + CHARACTER_SIZE + 3] = b"BBB"
    return bytes(data)


# ---------------------------------------------------------------------------
# Messages: Gold plain pool, DOS Huffman pool
# ---------------------------------------------------------------------------

GOLD_MESSAGES = [
    # (start id, span, [texts]) - one index entry per group.
    (100, 2, ["ALPHA", "BETA", "GAMMA"]),
    (200, 0, ["DELTA"]),
]


def make_gold_messages():
    """Plain pool: [len][len bytes] per message, index entry = 7 bytes."""
    pool = bytearray()
    index = bytearray()
    index += u16(len(GOLD_MESSAGES))
    for start_id, span, texts in GOLD_MESSAGES:
        first_offset = len(pool)
        for text in texts:
            raw = text.encode("ascii")
            pool.append(len(raw))
            pool += raw
        index += u16(start_id)
        index += u16(first_offset)       # offset within the page
        index += bytes([span])
        index += u16(0)                  # page
    return {"msg_gold.hdr": bytes(index), "msg_gold.gld": bytes(pool)}


# A minimal Huffman tree: two inner nodes, four leaves. Codes are two bits.
DOS_TREE_PAIRS = [
    # (bit 0, bit 1) per node; negative = child node index, positive = character.
    (-1, -2),
    (ord("A"), ord("B")),
    (ord("C"), ord("D")),
]
DOS_MESSAGES = [
    (300, 1, ["ABCD", "ADCB"]),
    (310, 0, ["CD"]),
]


def make_misc_header():
    """512 signed 16-bit entries: pairs of (left, right) per node, 0 = unused."""
    table = [0] * 512
    for node, (left, right) in enumerate(DOS_TREE_PAIRS):
        table[node * 2] = left
        table[node * 2 + 1] = right
    return struct.pack("<512h", *table)


def huffman_encode(text):
    """Encode with the fixed two-bit tree above; returns (bytes, bit count)."""
    codes = {"A": "00", "B": "01", "C": "10", "D": "11"}
    bits = "".join(codes[ch] for ch in text)
    bits += "0" * ((8 - len(bits) % 8) % 8)
    out = bytearray()
    for i in range(0, len(bits), 8):
        out.append(int(bits[i:i + 8], 2))
    return bytes(out), len(bits)


def make_dos_messages():
    """Huffman pool: [record size-1][decoded length][bitstream], index = 6 bytes."""
    pool = bytearray()
    index = bytearray()
    index += u16(len(DOS_MESSAGES))
    for start_id, span, texts in DOS_MESSAGES:
        first_offset = len(pool)
        for text in texts:
            encoded, _ = huffman_encode(text)
            assert 0 < len(text) < 256
            payload = bytes([len(text)]) + encoded   # decoded length, then bits
            pool.append(len(payload))    # record size counts the length byte
            pool += payload
        index += u16(start_id)
        index += u16(first_offset)
        index += bytes([span])
        index += bytes([0])              # page
    return {
        "msg_dos.hdr": bytes(index),
        "msg_dos.dbs": bytes(pool),
        "misc.hdr": make_misc_header(),
    }


# ---------------------------------------------------------------------------
# SCENARIO database: MASTER.HDR sizes, DISK.HDR offsets, concatenated records
# ---------------------------------------------------------------------------

SCENARIO_TYPES = 10
SCENARIO_SIZES = [XP_RECORD_SIZE, ITEM_RECORD_SIZE, MAZE_RECORD_SIZE, MAZE_RECORD_SIZE,
                  MONSTER_RECORD_SIZE, 96, 8, 8, 96, 8]
SCENARIO_COUNTS = [1, 1, 1, 0, 1, 0, 0, 0, 0, 0]

# DISK.HDR remap tables: (offset, size) per type, from ScenarioDatabase::getRemapInfo.
REMAP_INFO = {
    2: (0x02C, 32),
    3: (0x02C, 32),
    5: (0x04C, 96),
    4: (0x1AC, 256),
    7: (0x0AC, 256),
    8: (0x2AC, 96),
}
DISK_HEADER_SIZE = 0x30C


def make_xp_record():
    """16 cumulative thresholds for one class (ExperienceTable::parseRecord)."""
    values = [1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000,
              256000, 512000, 768000, 1168000, 1568000, 1968000, 2368000, 2968000]
    data = bytearray()
    for value in values:
        data += u32(value)
    return bytes(data)


def make_item_record():
    """One type-1 item record: 'SWORD OF FIRE' with charges and damage dice."""
    data = seeded(ITEM_RECORD_SIZE)
    data[0:12] = b"SWORD =/FIRE"         # '/', '=' decode to spaces in the name
    data[0x16:0x18] = u16(0x1234)        # price -> 0x012C weight below
    data[0x1A] = 2                       # default charges >> divisor
    data[0x1C] = 3                       # spell id
    data[0x1D] = 4                       # spell power
    data[0x20] = 1                       # damage dice count
    data[0x21] = 6                       # damage die size
    data[0x22] = 3                       # kind
    data[0x23] = 4                       # subtype
    data[0x24:0x26] = u16(0x012C)        # weight
    data[0x28] = 0x15                    # charges field
    data[0x32:0x34] = u16(0x3FFF)
    data[0x34:0x36] = u16(0x07FF)
    data[0x36] = 3
    data[0x37] = 4
    data[0x39] = 2
    data[0x3B] = 10                      # rank
    data[0x40] = 2                       # AC bonus
    return bytes(data)


def make_maze_record():
    """One type-2 maze record: one tile set, one encounter slot, one plane."""
    data = seeded(MAZE_RECORD_SIZE)
    data[0] = 0x12                       # header row0/group0/band0
    data[0x090] = 0x53                   # tile A: face0 = 3, face1 = 5
    data[0x291] = 0x06                   # tile B: face2 = 6
    data[0x4B0] = (0x2A & 3) << 6        # special face1 = 0x2A (bits 6..11)
    data[0x4B1] = 0x2A >> 2
    data[0x7B0] = 2 << 6                 # flags face3 = 2
    data[0x8B0 + 1] = 2                  # hazard layer 0, face 9
    data[0x9B0 + 1] = 2                  # plane A face 9 -> kind 2
    data[0xAB0] = 0x43                   # encounter slot 0: common 3, uncommon 4
    data[0xAC0] = 65                     # encounter chance
    return bytes(data)


def make_monster_record():
    """One type-4 monster record: names, XP, spawn/dice fields, variants."""
    data = seeded(MONSTER_RECORD_SIZE)
    data[0x00:0x0A] = b"DANDIPHOOT"
    data[0x10:0x1B] = b"DANDIPHOOTS"
    data[0x20:0x25] = b"PHOOT"
    data[0x30:0x36] = b"PHOOTS"
    data[0x40:0x44] = u32(390)           # xp
    data[0x7A:0x7C] = u16(5)             # spawn count
    data[0x7E:0x82] = u32(0x0103)        # level dice
    data[0x80] = 2                       # actor count (overlaps level dice)
    data[0x82:0x86] = u32(0x0204)        # hp dice
    data[0x86:0x8A] = u32(0x0104)        # ST dice
    data[0xAA] = 1                       # variant id 0
    data[0xAB] = 0                       # variant id 1
    data[0xAC] = 25                      # variant chance 0
    data[0xAD] = 50                      # variant chance 1
    data[0xD6] = 3                       # body type
    data[0xD7] = 26                      # picture id
    data[0xE2] = 1                       # flags
    return bytes(data)


def scenario_records():
    makers = {
        0: make_xp_record,
        1: make_item_record,
        2: make_maze_record,
        4: make_monster_record,
    }
    records = {}
    for record_type, count in enumerate(SCENARIO_COUNTS):
        if count:
            records[record_type] = makers[record_type]()
    return records


def make_scenario_database():
    records = scenario_records()
    master = bytearray()
    for size in SCENARIO_SIZES:
        master += u16(size)

    bases = []
    data = bytearray()
    for record_type in range(SCENARIO_TYPES):
        bases.append(len(data))
        if record_type in records:
            data += records[record_type]
    bases.append(len(data))              # end of the last type

    disk = seeded(DISK_HEADER_SIZE)
    for record_type, base in enumerate(bases[:SCENARIO_TYPES]):
        disk[4 + record_type * 4:4 + record_type * 4 + 4] = u32(base)
    # Identity remap tables, so record n of a remapped type reads record n.
    for record_type, (offset, size) in REMAP_INFO.items():
        count = SCENARIO_COUNTS[record_type]
        for i in range(count):
            disk[offset + i] = i

    return {
        "master.hdr": bytes(master),
        "disk.hdr": bytes(disk),
        "scenario.gld": bytes(data),
    }


# ---------------------------------------------------------------------------

BUILDERS = [
    ("vga_bundle.vga", make_vga_bundle),
    ("font.vga", make_font),
    ("save_dos.sav", make_dos_save),
    ("roster.pcf", make_roster),
]


def build_fixtures():
    """All fixtures as an ordered dict of name -> bytes."""
    fixtures = {}
    for name, builder in BUILDERS:
        fixtures[name] = builder()
    fixtures.update(make_gold_messages())
    fixtures.update(make_dos_messages())
    fixtures.update(make_scenario_database())
    return fixtures


def write_fixtures(out_dir, fixtures):
    os.makedirs(out_dir, exist_ok=True)
    for name, blob in fixtures.items():
        path = os.path.join(out_dir, name)
        with open(path, "wb") as f:
            f.write(blob)
        print("wrote %-20s %6d bytes" % (name, len(blob)))


def check_fixtures(out_dir, fixtures):
    """Compare the committed files with what this script generates."""
    failures = 0
    for name, blob in fixtures.items():
        path = os.path.join(out_dir, name)
        try:
            with open(path, "rb") as f:
                on_disk = f.read()
        except FileNotFoundError:
            print("missing  %s" % name)
            failures += 1
            continue
        if on_disk != blob:
            print("stale    %s (on disk %d bytes, generated %d bytes)" %
                  (name, len(on_disk), len(blob)))
            failures += 1
        else:
            print("ok       %-20s %6d bytes" % (name, len(blob)))
    return failures


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", default=DEFAULT_OUT,
                        help="fixture directory (default: %(default)s)")
    parser.add_argument("--check", action="store_true",
                        help="verify the files instead of writing them")
    parser.add_argument("--list", action="store_true", help="print the fixture names")
    args = parser.parse_args(argv)

    fixtures = build_fixtures()
    if args.list:
        for name in fixtures:
            print(name)
        return 0
    if args.check:
        failures = check_fixtures(args.out, fixtures)
        if failures:
            print("%d fixture(s) need regenerating in %s" % (failures, args.out))
            return 1
        print("all %d fixtures match" % len(fixtures))
        return 0

    write_fixtures(args.out, fixtures)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
