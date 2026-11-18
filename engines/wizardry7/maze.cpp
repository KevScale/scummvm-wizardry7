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

uint8 MazeRecord::getViewKind(uint cell, uint column, uint row) const {
	const uint face = cellFace(cell, column, row);
	if (getHazard(2, face))
		return 3;
	if (getHazard(0, face))
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

byte MazeRecord::getMonster(uint row, uint columnGroup, uint band) const {
	if (row >= 12 || columnGroup >= 4 || band >= 3)
		return 0;
	return _raw[row * 12 + columnGroup * 3 + band];
}

} // End of namespace Wizardry7
