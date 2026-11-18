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

#include "wizardry7/scenario.h"
#include "wizardry7/xp.h"

namespace Wizardry7 {

ExperienceTable::ExperienceTable() : _loaded(false) {
	memset(_thresholds, 0, sizeof(_thresholds));
}

bool ExperienceTable::parseRecord(const byte *record, uint32 size, uint classIndex) {
	if (!record || size < kLevels * 4 || classIndex >= kClasses)
		return false;
	for (uint i = 0; i < kLevels; ++i)
		_thresholds[classIndex][i] = READ_LE_UINT32(record + i * 4);
	return true;
}

bool ExperienceTable::load(const ScenarioDatabase &scenario) {
	_loaded = false;
	if (!scenario.isLoaded() || scenario.getRecordCount(kRecordClassXP) < kClasses)
		return false;
	for (uint i = 0; i < kClasses; ++i) {
		Common::Array<byte> record;
		if (!scenario.readRecord(kRecordClassXP, i, record) ||
			!parseRecord(record.data(), record.size(), i))
			return false;
	}
	_loaded = true;
	return true;
}

uint32 ExperienceTable::getThreshold(uint8 classIndex, uint level) const {
	if (classIndex >= kClasses || level <= 1)
		return 0;
	if (level <= kLevels)
		return _thresholds[classIndex][level - 2];
	const uint32 a = _thresholds[classIndex][kLevels - 1];
	const uint32 b = _thresholds[classIndex][kLevels - 2];
	return a + (a - b) * (level - (kLevels + 1));
}

uint8 ExperienceTable::getLevel(uint8 classIndex, uint32 xp) const {
	if (classIndex >= kClasses)
		return 1;
	uint level = 1;
	for (uint candidate = 2; candidate <= kLevels; ++candidate) {
		if (xp < _thresholds[classIndex][candidate - 2])
			break;
		level = candidate;
	}
	uint32 last = _thresholds[classIndex][kLevels - 1];
	const uint32 prev = _thresholds[classIndex][kLevels - 2];
	const uint32 step = last - prev;
	while (step && xp >= last && level < 255) {
		++level;
		if (last > 0xFFFFFFFFu - step)
			break;
		last += step;
	}
	return (uint8)level;
}

uint8 ExperienceTable::getRank(uint8 level) {
	if (level >= 24) return 7;
	if (level >= 18) return 6;
	if (level >= 14) return 5;
	if (level >= 10) return 4;
	if (level >= 7)  return 3;
	if (level >= 4)  return 2;
	if (level >= 2)  return 1;
	return 0;
}

} // End of namespace Wizardry7
