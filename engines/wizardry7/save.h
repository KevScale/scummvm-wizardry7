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

enum SaveKind {
	kSaveUnknown,
	kSaveDos,
	kSaveGold
};

struct SaveCharacter {
	SaveCharacter() : age(0), xp(0), hp(0), hpMax(0), stamina(0), staminaMax(0),
		weight(0), gold(0), ac(0), race(0), alignment(0), klass(0), knack(0) {}

	Common::String name;
	uint32 age;
	uint32 xp;
	uint16 hp;
	uint16 hpMax;
	uint16 stamina;
	uint16 staminaMax;
	uint16 weight;
	uint16 gold;
	uint16 ac;
	uint8 race;
	uint8 alignment;
	uint8 klass;
	uint8 knack;
};

/**
 * Read-only savegame probe. DOS SAVEGAME.DBS is fully framed; Gold .SGM is
 * Huffman-compressed and its runtime-built table is not in the shipped files,
 * so Gold saves are recognised but not decoded yet.
 */
class SaveGame {
public:
	SaveGame();

	bool load(const Common::Path &path);

	SaveKind getKind() const { return _kind; }
	uint32 getSize() const { return _size; }
	uint getCharacterCount() const { return _characters.size(); }
	const SaveCharacter &getCharacter(uint index) const { return _characters[index]; }

	static const char *getKindName(SaveKind kind);

private:
	bool parseDos(const byte *data, uint32 size);
	bool readFile(const Common::Path &path, Common::Array<byte> &data) const;

	SaveKind _kind;
	uint32 _size;
	Common::Array<SaveCharacter> _characters;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_SAVE_H
