/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WIZARDRY7_SAVE_H
#define WIZARDRY7_SAVE_H

#include "common/array.h"
#include "common/path.h"
#include "common/str.h"

namespace Wizardry7 {

class Party;

enum SaveKind {
	kSaveUnknown,
	kSaveDos,
	kSaveGold
};

struct SaveCharacter {
	SaveCharacter() : age(0), xp(0), hp(0), hpMax(0), stamina(0), staminaMax(0),
		weight(0), carryingCapacity(0), level(0), ac(0), race(0), gender(0),
		alignment(0), klass(0), knack(0), portrait(0), swimming(0), climbing(0),
		scouting(0), kirijutsu(0), music(0), artifacts(0), mythology(0), mapping(0),
		scribe(0), strTier(0) {
		memset(weaponry, 0, sizeof(weaponry));
		memset(spellPoints, 0, sizeof(spellPoints));
		memset(resist, 0, sizeof(resist));
		memset(stats, 0, sizeof(stats));
		memset(inventoryCount, 0, sizeof(inventoryCount));
		memset(acSlots, 0, sizeof(acSlots));
	}

	Common::String name;
	uint32 age;
	uint32 xp;
	uint16 hp;
	uint16 hpMax;
	uint16 stamina;
	uint16 staminaMax;
	uint16 weight;           // carried weight (+0x20)
	uint16 carryingCapacity; // +0x22 (not GP)
	uint16 level;            // +0x24 (shipped pre-mades: 1)
	uint16 ac;
	uint8 race;
	uint8 gender;
	uint8 alignment;
	uint8 klass;
	uint8 knack;
	uint8 portrait;
	uint8 stats[7]; // STR INT PIE VIT DEX SPD PER at record +0x170
	uint16 inventory[20]; // 20 slots at +0x40 (two pages of ten), 12-byte stride
	uint8 inventoryCount[20]; // slot +6 quantity/charges
	uint8 inventoryKind[20]; // slot +4 cached item kind
	uint8 acSlots[5]; // copies at +0x1B9..+0x1BD
	uint8 weaponry[10]; // +0x178..+0x181
	uint8 swimming, climbing, scouting; // +0x182..+0x184
	uint8 kirijutsu; // +0x189
	uint8 music; // +0x18B
	uint8 artifacts, mythology, mapping, scribe; // +0x190..+0x193
	uint16 spellPoints[6][2]; // +0x28..+0x3F: six realms, current/max
	uint8 strTier; // +0x130: (STR>15)+(STR>17)
	uint8 resist[13]; // +0x19B (second identical copy at +0x1A9)
};

/**
 * Read-only savegame probe. DOS SAVEGAME.DBS is fully framed; Gold .SGM is
 * Huffman-compressed and its runtime-built table is not in the shipped files,
 * so Gold saves are recognised but not decoded yet.
 */
/** PCFILE.GLD/.DBS castle roster: 24-byte header + up to 16 characters. */
class CharacterRoster {
public:
	CharacterRoster();

	bool load(const Common::Path &path);
	bool loadFromData(const byte *data, uint32 size);
	bool isLoaded() const { return !_characters.empty(); }
	uint getSlotCount() const { return _characters.size(); }
	bool isSlotUsed(uint slot) const { return slot < _used.size() && _used[slot]; }
	const SaveCharacter &getCharacter(uint slot) const { return _characters[slot]; }
	bool getRawRecord(uint slot, const byte *&record) const {
		if (slot >= _used.size() || _headerSize + (slot + 1) * _stride > _raw.size())
			return false;
		record = _raw.data() + _headerSize + slot * _stride;
		return true;
	}

	/** First unused slot, or -1 when the roster is full. */
	int findFreeSlot() const;
	/** Replace a roster slot with a raw 584-byte character record. */
	bool setCharacterRecord(uint slot, const byte *record, uint32 size);
	/** Serialise the (possibly modified) file back to memory. */
	bool writeToData(Common::Array<byte> &out) const;

private:
	Common::Array<byte> _raw;
	Common::Array<SaveCharacter> _characters;
	uint32 _partyBase = 0;
	uint32 _recordOffset = 0;
	uint16 _partyMaze = 0;
	uint16 _partyX = 0;
	uint16 _partyY = 0;
	uint16 _prevMaze = 0;
	uint16 _prevX = 0;
	uint16 _prevY = 0;
	uint16 _partyFacing = 0;
	uint16 _partyLocalX = 0;
	uint16 _partyLocalY = 0;
	uint16 _partyWindow = 0;
	uint16 _partyCounter = 0;
	Common::Array<bool> _used;
	uint16 _stride;
	uint32 _headerSize;
};

class SaveGame {
public:
	SaveGame();

	bool load(const Common::Path &path);
	bool loadFromData(const byte *data, uint32 size);
	bool save(const Common::Path &path) const;

	/** Replace one DOS/Gold character record (584 B). */
	bool setCharacterRecord(uint index, const byte *record, uint32 size);
	/** Update the current party tuple in the container image. */
	bool setPartyPosition(uint16 maze, uint16 worldX, uint16 worldY, uint16 localX,
		uint16 localY, uint16 window, uint16 facing);
	bool writeToData(Common::Array<byte> &out) const;

	/** Rewrite the DOS active-party records from roster slots, preserving state. */
	bool setPartyFromRoster(const CharacterRoster &roster, const Party &party);

	SaveKind getKind() const { return _kind; }
	uint16 getPartyMaze() const { return _partyMaze; }
	uint16 getPartyX() const { return _partyX; }
	uint16 getPartyY() const { return _partyY; }
	uint16 getPrevMaze() const { return _prevMaze; }
	uint16 getPrevX() const { return _prevX; }
	uint16 getPrevY() const { return _prevY; }
	uint16 getPartyFacing() const { return _partyFacing; }
	uint16 getPartyLocalX() const { return _partyLocalX; }
	uint16 getPartyLocalY() const { return _partyLocalY; }
	uint16 getPartyWindow() const { return _partyWindow; }
	uint16 getPartyCounter() const { return _partyCounter; }
	uint32 getSize() const { return _size; }
	uint getCharacterCount() const { return _characters.size(); }
	const SaveCharacter &getCharacter(uint index) const { return _characters[index]; }

	static const char *getKindName(SaveKind kind);

private:
	bool parseDos(const byte *data, uint32 size);
	bool readFile(const Common::Path &path, Common::Array<byte> &data) const;

	SaveKind _kind;
	uint32 _size;
	Common::Array<byte> _raw;
	Common::Array<SaveCharacter> _characters;
	uint32 _partyBase = 0;
	uint32 _recordOffset = 0;
	uint16 _partyMaze = 0;
	uint16 _partyX = 0;
	uint16 _partyY = 0;
	uint16 _prevMaze = 0;
	uint16 _prevX = 0;
	uint16 _prevY = 0;
	uint16 _partyFacing = 0;
	uint16 _partyLocalX = 0;
	uint16 _partyLocalY = 0;
	uint16 _partyWindow = 0;
	uint16 _partyCounter = 0;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_SAVE_H
