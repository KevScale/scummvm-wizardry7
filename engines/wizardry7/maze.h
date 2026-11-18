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
class WizardryRNG;

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
	/** Packed face key: cell*64 + z*8 + x (Gold axis mapping). */
	static uint cellFace(uint cell, uint z, uint x) {
		return cell * 0x40 + z * 8 + x;
	}
	/** Plane-derived kind candidate: plane C (+0x930) -> 3, plane A (+0x9B0) -> 2.
	 *  The game only writes a non-zero kind when the depth walk succeeds. */
	uint8 getViewKind(uint cell, uint z, uint x) const;
	/** Step one local cell across the 16-cell graph. dz/dx follow the Gold axes. */
	bool stepCell(uint &cell, uint &localZ, uint &localX, int dz, int dx) const;
	/** Five depth kinds along a ray; false if the ray leaves the cell graph.
	 *  This is a diagnostic: the final per-column start/step is still open. */
	bool getKindRay(uint cell, uint localZ, uint localX, int dz, int dx, uint8 kinds[5]) const;
	byte getEncounterGroup(uint slot, bool uncommon) const;
	byte getEncounterChance(uint slot) const;
	byte getMonster(uint row, uint columnGroup, uint band) const;

	/** Cell origin tables at +0x490 (z) / +0x4A0 (x), 16 cells of 8x8. */
	void getCellOrigin(uint cell, uint &originZ, uint &originX) const;
	/** Resolve absolute (z,x) to a cell and local z/x coordinates. */
	bool findCell(int z, int x, uint &cell, uint &localZ, uint &localX) const;

	/**
	 * Encounter roll for one of the 16 slots. Returns -1 when the chance check
	 * fails, otherwise the type-4 monster id from the 12x12 header.
	 */
	int selectEncounterMonster(uint slot, WizardryRNG &rng) const;

private:
	uint readBits(uint offset, uint bitOffset, uint width) const;

	Common::Array<byte> _raw;
	bool _loaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_MAZE_H
