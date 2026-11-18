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
#include "common/textconsole.h"

#include "wizardry7/scenario.h"

namespace Wizardry7 {

ScenarioDatabase::ScenarioDatabase() :
	_dataSize(0), _loaded(false) {
	memset(_sizes, 0, sizeof(_sizes));
	memset(_bases, 0, sizeof(_bases));
	memset(_counts, 0, sizeof(_counts));
}

bool ScenarioDatabase::load(const Common::Path &masterHeader, const Common::Path &diskHeader, const Common::Path &dataFile) {
	_loaded = false;
	_diskHeader.clear();

	Common::File master;
	if (!master.open(masterHeader)) {
		warning("Wizardry7: cannot open scenario sizes '%s'", masterHeader.toString().c_str());
		return false;
	}
	if (master.size() < kRecordTypeCount * 2) {
		warning("Wizardry7: scenario sizes '%s' is too short", masterHeader.toString().c_str());
		return false;
	}
	for (int i = 0; i < kRecordTypeCount; ++i) {
		_sizes[i] = master.readUint16LE();
		if (_sizes[i] == 0) {
			warning("Wizardry7: scenario record type %d has size 0", i);
			return false;
		}
	}

	Common::File disk;
	if (!disk.open(diskHeader)) {
		warning("Wizardry7: cannot open scenario offsets '%s'", diskHeader.toString().c_str());
		return false;
	}
	const uint32 diskSize = disk.size();
	if (diskSize < 4 + kRecordTypeCount * 4) {
		warning("Wizardry7: scenario offsets '%s' is too short", diskHeader.toString().c_str());
		return false;
	}
	_diskHeader.resize(diskSize);
	if (disk.read(_diskHeader.data(), diskSize) != diskSize)
		return false;

	for (int i = 0; i < kRecordTypeCount; ++i)
		_bases[i] = READ_LE_UINT32(_diskHeader.data() + 4 + i * 4);

	if (!_dataFile.open(dataFile)) {
		warning("Wizardry7: cannot open scenario data '%s'", dataFile.toString().c_str());
		return false;
	}
	_dataSize = _dataFile.size();
	_bases[kRecordTypeCount] = _dataSize;

	if (_bases[0] != 0)
		warning("Wizardry7: scenario data does not start at offset 0");

	for (int i = 0; i < kRecordTypeCount; ++i) {
		if (_bases[i] > _bases[i + 1] || _bases[i + 1] > _dataSize) {
			warning("Wizardry7: invalid scenario offsets for record type %d", i);
			return false;
		}
		const uint32 span = _bases[i + 1] - _bases[i];
		_counts[i] = span / _sizes[i];
		if (span % _sizes[i])
			warning("Wizardry7: record type %d does not divide its span (%u bytes)", i, span);
	}

	_loaded = true;
	return true;
}

uint16 ScenarioDatabase::getRecordSize(int type) const {
	if (type < 0 || type >= kRecordTypeCount)
		return 0;
	return _sizes[type];
}

uint32 ScenarioDatabase::getRecordCount(int type) const {
	if (type < 0 || type >= kRecordTypeCount)
		return 0;
	return _counts[type];
}

uint32 ScenarioDatabase::getBaseOffset(int type) const {
	if (type < 0 || type >= kRecordTypeCount)
		return 0;
	return _bases[type];
}

bool ScenarioDatabase::getRemapInfo(int type, uint16 &offset, uint16 &size) {
	switch (type) {
	case kRecordMazeMain:
	case kRecordMazeOverlay:
		offset = 0x02C;
		size = 32;
		return true;
	case kRecordNPC:
		offset = 0x04C;
		size = 96;
		return true;
	case kRecordLoot:
		offset = 0x0AC;
		size = 256;
		return true;
	case kRecordMonsterName:
		offset = 0x1AC;
		size = 256;
		return true;
	case kRecordMonsterStats:
		offset = 0x2AC;
		size = 96;
		return true;
	default:
		return false;
	}
}

int ScenarioDatabase::getRemappedIndex(int type, int index) const {
	if (index < 0 || (uint32)index >= getRecordCount(type))
		return -1;

	uint16 offset, size;
	if (!getRemapInfo(type, offset, size))
		return index;

	if (offset + index >= (int)_diskHeader.size() || index >= size)
		return -1;

	return _diskHeader[offset + index];
}

bool ScenarioDatabase::readRecord(int type, int index, Common::Array<byte> &record) const {
	const int remapped = getRemappedIndex(type, index);
	if (remapped < 0 || (uint32)remapped >= getRecordCount(type))
		return false;

	record.resize(_sizes[type]);
	if (!_dataFile.seek((int64)_bases[type] + (int64)_sizes[type] * remapped))
		return false;
	return _dataFile.read(record.data(), record.size()) == record.size();
}

const char *ScenarioDatabase::getRecordTypeName(int type) {
	static const char *const names[kRecordTypeCount] = {
		"class XP", "item", "maze A", "maze B", "monster name",
		"NPC", "script", "loot/shop", "monster stats", "song"
	};
	if (type < 0 || type >= kRecordTypeCount)
		return "unknown";
	return names[type];
}

} // End of namespace Wizardry7
