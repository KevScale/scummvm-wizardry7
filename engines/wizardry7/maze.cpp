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

#include "common/endian.h"

#include "wizardry7/maze.h"
#include "wizardry7/rng.h"
#include "wizardry7/scenario.h"

namespace Wizardry7 {

MazeRecord::MazeRecord() : _loaded(false) {
}

bool MazeRecord::load(const ScenarioDatabase &scenario, uint mazeIndex) {
	Common::Array<byte> record;
	if (!scenario.isLoaded() || mazeIndex >= scenario.getRecordCount(kRecordMazeMain) ||
		!scenario.readRecord(kRecordMazeMain, mazeIndex, record))
		return false;
	return loadFromData(record.data(), record.size());
}

bool MazeRecord::loadFromData(const byte *data, uint32 size) {
	_loaded = false;
	if (!data || size < kRecordSize)
		return false;
	_raw.resize(kRecordSize);
	memcpy(_raw.data(), data, kRecordSize);
	_loaded = true;
	return true;
}

uint MazeRecord::readBits(uint offset, uint bitOffset, uint width) const {
	const uint bit = bitOffset * width;
	const uint byte = offset + bit / 8;
	const uint shift = bit & 7;
	uint value = _raw[byte] | (_raw[byte + 1] << 8);
	value >>= shift;
	return value & ((1u << width) - 1);
}

byte MazeRecord::getTileA(uint face) const {
	return (face < kFaces) ? ((_raw[0x090 + face / 2] >> ((face & 1) * 4)) & 0x0F) : 0;
}

byte MazeRecord::getTileB(uint face) const {
	return (face < kFaces) ? ((_raw[0x290 + face / 2] >> ((face & 1) * 4)) & 0x0F) : 0;
}

byte MazeRecord::getSpecial(uint face) const {
	return (face < kFaces) ? (byte)readBits(0x4B0, face, 6) : 0;
}

byte MazeRecord::getFlags(uint face) const {
	return (face < kFaces) ? (byte)readBits(0x7B0, face, 2) : 0;
}

bool MazeRecord::getHazard(uint layer, uint face) const {
	if (layer > 2 || face >= kFaces)
		return false;
	const uint offset = 0x8B0 + layer * 0x80;
	return (_raw[offset + face / 8] >> (face & 7)) & 1;
}

bool MazeRecord::stepCell(uint &cell, uint &localZ, uint &localX, int dz, int dx) const {
	uint oz = 0, ox = 0;
	getCellOrigin(cell, oz, ox);
	const int z = (int)oz + (int)localZ + dz;
	const int x = (int)ox + (int)localX + dx;
	return findCell(z, x, cell, localZ, localX);
}

bool MazeRecord::getKindRay(uint cell, uint localZ, uint localX, int dz, int dx,
		uint8 kinds[5]) const {
	for (uint i = 0; i < 5; ++i) {
		kinds[i] = getViewKind(cell, localZ, localX);
		if (!stepCell(cell, localZ, localX, dz, dx))
			return false;
	}
	return true;
}

void MazePosition::stepForward() {
	switch (facing) {
	case 0: ++b; break;
	case 1: ++a; break;
	case 2: --b; break;
	default: --a; break;
	}
}

void MazePosition::stepBackward() {
	switch (facing) {
	case 0: --b; break;
	case 1: --a; break;
	case 2: ++b; break;
	default: ++a; break;
	}
}

bool MazePosition::resolve(const MazeRecord &mazeRecord, uint &cell, uint &localA, uint &localB) const {
	return mazeRecord.findCell(a, b, cell, localA, localB);
}

bool MazeRecord::deriveKindTable(int partyA, int partyB, uint facing, uint8 kinds[5][5]) const {
	memset(kinds, 0, sizeof(uint8) * 25);
	static const int kDepth5[5] = { -2, -1, 0, 1, 2 };
	static const int kDepth3[3] = { 0, -1, 1 };
	for (int column = 0; column < 5; ++column) {
		const int *depths = column <= 1 ? kDepth3 : kDepth5;
		const int count = column <= 1 ? 3 : 5;
		for (int i = 0; i < count; ++i) {
			const int depth = depths[i];
			int a = partyA - column;
			int b = partyB;
			switch (facing) {
			case 0: a += depth; break;
			case 1: b -= depth; break;
			case 2: a -= depth; break;
			default: b += depth; break;
			}
			uint cell = 0, localA = 0, localB = 0;
			if (!findCell(a, b, cell, localA, localB))
				continue;
			// key = cell*64 + bLocal*8 + aLocal (the validated orientation)
			kinds[column][depth + 2] = getViewKind(cell, localB, localA);
		}
	}
	return true;
}

uint8 MazeRecord::getViewKind(uint cell, uint z, uint x) const {
	const uint face = cellFace(cell, z, x);
	if (getHazard(1, face)) // +0x930: plane C
		return 3;
	if (getHazard(2, face)) // +0x9B0: plane A
		return 2;
	return 0;
}

byte MazeRecord::getEncounterGroup(uint slot, bool uncommon) const {
	const uint index = slot * 2 + (uncommon ? 1 : 0);
	const uint byteIndex = 0xAB0 + index / 2;
	return (index < 32) ? ((_raw[byteIndex] >> ((index & 1) * 4)) & 0x0F) : 0;
}

byte MazeRecord::getEncounterChance(uint slot) const {
	return slot < kEncounterSlots ? _raw[0xAC0 + slot] : 0;
}

int MazeRecord::selectEncounterMonster(uint slot, WizardryRNG &rng) const {
	if (slot >= kEncounterSlots)
		return -1;
	const uint32 chance = getEncounterChance(slot);
	const uint32 roll = rng.getRandom(100);
	if (chance <= roll)
		return -1;
	const byte group = getEncounterGroup(slot, false);
	const uint32 columnGroup = rng.getRandom(100) & 3;
	const uint32 bandRoll = rng.getRandom(100);
	const uint band = bandRoll < 0x4A ? 0 : bandRoll < 0x5C ? 1 : 2;
	return getMonster(group, columnGroup, band);
}

void MazeRecord::getCellOrigin(uint cell, uint &originZ, uint &originX) const {
	if (cell >= 16) {
		originZ = originX = 0;
		return;
	}
	originZ = _raw[0x490 + cell];
	originX = _raw[0x4A0 + cell];
}

bool MazeRecord::findCell(int z, int x, uint &cell, uint &localZ, uint &localX) const {
	for (uint i = 0; i < 16; ++i) {
		const int oz = _raw[0x490 + i];
		const int ox = _raw[0x4A0 + i];
		if (z >= oz && z <= oz + 7 && x >= ox && x <= ox + 7) {
			cell = i;
			localZ = (uint)(z - oz);
			localX = (uint)(x - ox);
			return true;
		}
	}
	return false;
}

byte MazeRecord::getMonster(uint row, uint columnGroup, uint band) const {
	if (row >= 12 || columnGroup >= 4 || band >= 3)
		return 0;
	return _raw[row * 12 + columnGroup * 3 + band];
}

} // End of namespace Wizardry7
