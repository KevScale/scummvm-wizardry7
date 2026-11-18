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

#ifndef WIZARDRY7_SCENARIO_H
#define WIZARDRY7_SCENARIO_H

#include "common/array.h"
#include "common/file.h"
#include "common/path.h"

namespace Wizardry7 {

enum RecordType {
	kRecordClassXP = 0,
	kRecordItem = 1,
	kRecordMazeMain = 2,
	kRecordMazeOverlay = 3,
	kRecordMonsterName = 4,
	kRecordNPC = 5,
	kRecordScript = 6,
	kRecordLoot = 7,
	kRecordMonsterStats = 8,
	kRecordSong = 9,
	kRecordTypeCount = 10
};

/**
 * The SCENARIO record database.
 *
 * MASTER.HDR holds a u16 record size per type; DISK.HDR holds a u32 base
 * offset per type followed by byte remap tables used by types 2-5, 7 and 8.
 * All record types are concatenated in SCENARIO.GLD (Gold) / SCENARIO.DBS
 * (DOS); the latter two files are byte-identical.
 */
class ScenarioDatabase {
public:
	ScenarioDatabase();

	bool load(const Common::Path &masterHeader, const Common::Path &diskHeader, const Common::Path &dataFile);

	bool isLoaded() const { return _loaded; }
	uint32 getDataSize() const { return _dataSize; }

	uint16 getRecordSize(int type) const;
	uint32 getRecordCount(int type) const;
	uint32 getBaseOffset(int type) const;
	int getRemappedIndex(int type, int index) const;

	/** Read one record, applying the DISK.HDR remap table for the type. */
	bool readRecord(int type, int index, Common::Array<byte> &record) const;

	static const char *getRecordTypeName(int type);

private:
	static bool getRemapInfo(int type, uint16 &offset, uint16 &size);

	uint16 _sizes[kRecordTypeCount];
	uint32 _bases[kRecordTypeCount + 1];
	uint32 _counts[kRecordTypeCount];
	Common::Array<byte> _diskHeader;
	mutable Common::File _dataFile;
	uint32 _dataSize;
	bool _loaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_SCENARIO_H
