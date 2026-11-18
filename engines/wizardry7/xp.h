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

#ifndef WIZARDRY7_XP_H
#define WIZARDRY7_XP_H

#include "common/scummsys.h"

namespace Wizardry7 {

class ScenarioDatabase;

/**
 * SCENARIO type-0 records: 14 classes x 16 cumulative XP thresholds.
 *
 * threshold(class, level) is the XP required to reach `level` (level 1 = 0).
 * Levels above 16 continue with the last observed delta. Rank thresholds are
 * level >= {2,4,7,10,14,18,24} -> rank 1..7.
 */
class ExperienceTable {
public:
	enum { kClasses = 14, kLevels = 16 };

	ExperienceTable();

	bool load(const ScenarioDatabase &scenario);
	bool parseRecord(const byte *record, uint32 size, uint classIndex);
	bool isLoaded() const { return _loaded; }

	uint32 getThreshold(uint8 classIndex, uint level) const;
	uint8 getLevel(uint8 classIndex, uint32 xp) const;
	static uint8 getRank(uint8 level);

private:
	uint32 _thresholds[kClasses][kLevels];
	bool _loaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_XP_H
