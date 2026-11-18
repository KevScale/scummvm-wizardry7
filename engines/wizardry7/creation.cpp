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
 */

#include "wizardry7/creation.h"

#include "common/util.h"

namespace Wizardry7 {

// Minimum stats for each class (see docs/engine/chargen-rules.md).
static const byte kClassMinimums[CharacterCreation::kClassCount][CharacterCreation::kStatCount] = {
	{ 12, 0, 0, 0, 0, 0, 0, 0 }, // Fighter
	{ 0, 12, 0, 0, 0, 0, 0, 0 }, // Mage
	{ 0, 0, 12, 0, 0, 0, 8, 0 }, // Priest
	{ 0, 0, 0, 12, 0, 8, 0, 0 }, // Thief
	{ 10, 8, 8, 11, 10, 8, 8, 0 }, // Ranger
	{ 0, 13, 0, 0, 13, 0, 0, 0 }, // Alchemist
	{ 0, 0, 10, 0, 0, 8, 12, 0 }, // Bard
	{ 10, 14, 0, 14, 0, 0, 10, 0 }, // Psionic
	{ 10, 0, 11, 11, 10, 11, 8, 0 }, // Valkyrie
	{ 15, 15, 15, 0, 0, 0, 8, 0 }, // Bishop
	{ 12, 9, 12, 12, 9, 9, 14, 0 }, // Lord
	{ 12, 11, 0, 9, 12, 14, 8, 0 }, // Samurai
	{ 13, 8, 13, 0, 10, 13, 8, 0 }, // Monk
	{ 12, 10, 10, 12, 12, 12, 0, 0 } // Ninja
};

CharacterCreation::CharacterCreation() : _bonusPoints(0), _sex(0) {
	memset(_stats, 0, sizeof(_stats));
}

void CharacterCreation::setStats(const byte *stats) {
	memcpy(_stats, stats, kStatCount);
}

void CharacterCreation::setStat(uint index, byte value) {
	if (index < kStatCount)
		_stats[index] = value;
}

int CharacterCreation::getClassCost(uint classIndex) const {
	if (classIndex >= kClassCount)
		return -1;
	if (classIndex == 8 && _sex != 1) // Valkyrie
		return -1;

	int cost = 0;
	for (uint i = 0; i < kStatCount; ++i) {
		const int required = kClassMinimums[classIndex][i];
		if (_stats[i] < required)
			cost += required - _stats[i];
	}
	return cost;
}

bool CharacterCreation::isClassEligible(uint classIndex) const {
	const int cost = getClassCost(classIndex);
	return cost >= 0 && cost <= _bonusPoints;
}

bool CharacterCreation::selectClass(uint classIndex) {
	const int cost = getClassCost(classIndex);
	if (cost < 0 || cost > _bonusPoints)
		return false;
	for (uint i = 0; i < kStatCount; ++i) {
		if (_stats[i] < kClassMinimums[classIndex][i])
			_stats[i] = kClassMinimums[classIndex][i];
	}
	_bonusPoints -= cost;
	return true;
}

Common::Array<uint> CharacterCreation::getEligibleClasses() const {
	Common::Array<uint> classes;
	for (uint i = 0; i < kClassCount; ++i) {
		if (isClassEligible(i))
			classes.push_back(i);
	}
	return classes;
}

const char *CharacterCreation::getClassName(uint classIndex) {
	static const char *const names[kClassCount] = {
		"FIGHTER", "MAGE", "PRIEST", "THIEF", "RANGER", "ALCHEMIST", "BARD",
		"PSIONIC", "VALKYRIE", "BISHOP", "LORD", "SAMURAI", "MONK", "NINJA"
	};
	return classIndex < kClassCount ? names[classIndex] : "UNKNOWN";
}

} // End of namespace Wizardry7
