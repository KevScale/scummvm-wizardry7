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

#ifndef WIZARDRY7_MAZE_H
#define WIZARDRY7_MAZE_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Wizardry7 {

class ScenarioDatabase;

/**
 * One SCENARIO type-2 maze record (2768 bytes).
 *
 * Face index is F = x*0x40 + y*8 + facing (1024 packed faces). The header
 * 0x000..0x08F is a 12x12 encounter-group -> monster table; the remaining
 * tables are documented in docs/formats/maze-and-monsters.md.
 */
class MazeRecord {
public:
	enum {
		kRecordSize = 2768,
		kFaces = 1024,
		kEncounterSlots = 16
	};

	MazeRecord();

	bool load(const ScenarioDatabase &scenario, uint mazeIndex);
	bool loadFromData(const byte *data, uint32 size);
	bool isLoaded() const { return _loaded; }
	const byte *getRaw() const { return _raw.data(); }

	static uint faceIndex(uint x, uint y, uint facing) {
		return x * 0x40 + y * 8 + facing;
	}
	byte getTileA(uint face) const;
	byte getTileB(uint face) const;
	byte getSpecial(uint face) const;
	byte getFlags(uint face) const;
	bool getHazard(uint layer, uint face) const; // 0..2 => +0x8B0/+0x930/+0x9B0
	byte getEncounterGroup(uint slot, bool uncommon) const;
	byte getEncounterChance(uint slot) const;
	byte getMonster(uint row, uint columnGroup, uint band) const;

private:
	uint readBits(uint offset, uint bitOffset, uint width) const;

	Common::Array<byte> _raw;
	bool _loaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_MAZE_H
