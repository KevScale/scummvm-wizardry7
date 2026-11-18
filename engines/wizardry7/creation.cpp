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

#include "wizardry7/rng.h"

#include "common/util.h"

namespace Wizardry7 {

// Race minimums (see docs/engine/chargen-rules.md).
static const byte kRaceMinimums[11][CharacterCreation::kStatCount] = {
	{ 9, 8, 8, 9, 9, 8, 8, 0 }, // Human
	{ 7, 10, 10, 7, 9, 9, 8, 0 }, // Elf
	{ 11, 6, 10, 12, 7, 7, 7, 0 }, // Dwarf
	{ 10, 7, 13, 10, 8, 6, 6, 0 }, // Gnome
	{ 8, 7, 6, 9, 10, 7, 13, 0 }, // Hobbit
	{ 5, 11, 6, 6, 10, 14, 12, 0 }, // Faerie
	{ 12, 5, 5, 14, 8, 10, 3, 0 }, // Lizardman
	{ 10, 7, 6, 12, 10, 8, 6, 0 }, // Dracon
	{ 7, 10, 7, 7, 10, 12, 10, 0 }, // Felpurr
	{ 8, 6, 12, 10, 8, 8, 10, 0 }, // Rawulf
	{ 10, 10, 6, 10, 7, 7, 9, 0 } // Mook
};

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

int CharacterCreation::rollBonusPoints(WizardryRNG &rng) {
	int bonus = rng.getRandom(6) + 5;
	if (rng.getRandom(20) == 0)
		bonus += 8;
	if (rng.getRandom(20) == 0)
		bonus += 8;
	if (bonus < 9)
		bonus = rng.getRandom(4) + 8;
	return bonus;
}

CharacterCreation::CharacterCreation() : _bonusPoints(0), _sex(0), _classIndex(-1) {
	memset(_stats, 0, sizeof(_stats));
}

void CharacterCreation::setStats(const byte *stats) {
	memcpy(_stats, stats, kStatCount);
}

void CharacterCreation::setStat(uint index, byte value) {
	if (index < kStatCount)
		_stats[index] = value;
}

void CharacterCreation::applyRaceMinimums(uint8 race) {
	if (race >= 11)
		return;
	for (uint i = 0; i < kStatCount; ++i) {
		if (_stats[i] < kRaceMinimums[race][i])
			_stats[i] = kRaceMinimums[race][i];
	}
}

void CharacterCreation::applySexModifiers(byte sex) {
	if (sex != 1)
		return;
	if (_stats[0] >= 2)
		_stats[0] -= 2;
	else
		_stats[0] = 0;
	++_stats[6];
	++_stats[7];
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
	_classIndex = classIndex;
	return true;
}

CreationRolls CharacterCreation::rollDerived(uint8 raceIndex, WizardryRNG &rng) const {
	CreationRolls rolls;
	rolls.ageDays = rng.getRandom(1000) + 6570;

	int classRoll = 0;
	switch (_classIndex) {
	case 0:  classRoll = rng.getRandom(5) + 6; break; // Fighter
	case 1:  classRoll = rng.getRandom(3) + 2; break; // Mage
	case 2:  classRoll = rng.getRandom(4) + 4; break; // Priest
	case 3:  classRoll = rng.getRandom(4) + 3; break; // Thief
	case 4:  classRoll = rng.getRandom(4) + rng.getRandom(4) + 6; break; // Ranger
	case 5:  classRoll = rng.getRandom(4) + 3; break; // Alchemist
	case 6:  classRoll = rng.getRandom(4) + 2; break; // Bard
	case 7:  classRoll = rng.getRandom(3) + 3; break; // Psionic
	case 8:  classRoll = rng.getRandom(5) + 5; break; // Valkyrie
	case 9:  classRoll = rng.getRandom(4) + 3; break; // Bishop
	case 10: classRoll = rng.getRandom(7) + 8; break; // Lord
	case 11: classRoll = rng.getRandom(4) + rng.getRandom(4) + 6; break; // Samurai
	case 12: classRoll = rng.getRandom(4) + 4; break; // Monk
	case 13: classRoll = rng.getRandom(5) + 4; break; // Ninja
	default: break;
	}

	const byte str = _stats[0];
	const byte vit = _stats[3];
	rolls.hpMax = classRoll + (vit < 8 ? 3 : 4);
	if (vit > 15)
		++rolls.hpMax;
	if (vit > 17)
		++rolls.hpMax;

	rolls.staminaMax = (str + vit * 2) * 3;
	if (vit > 15)
		rolls.staminaMax += vit;
	if (vit > 17)
		rolls.staminaMax += vit;

	int goldBase = (vit + str * 2) * 3;
	if (str > 15)
		goldBase += str;
	if (str > 17)
		goldBase += str;
	rolls.carryingCapacity = goldBase * 15;
	if (raceIndex == 5) // FAERIE
		rolls.carryingCapacity = rolls.carryingCapacity * 2 / 3;

	return rolls;
}

Common::Array<uint> CharacterCreation::getEligibleClasses() const {
	Common::Array<uint> classes;
	for (uint i = 0; i < kClassCount; ++i) {
		if (isClassEligible(i))
			classes.push_back(i);
	}
	return classes;
}

CharacterCreationFlow::CharacterCreationFlow() : _step(kStepSex), _race(0) {
	memset(_bonusSpent, 0, sizeof(_bonusSpent));
}

void CharacterCreationFlow::begin(const byte *stats, int bonusPoints) {
	_character.setStats(stats);
	_character.setBonusPoints(bonusPoints);
	_name.clear();
	_race = 0;
	memset(_bonusSpent, 0, sizeof(_bonusSpent));
	_step = kStepSex;
}

bool CharacterCreationFlow::chooseSex(byte sex) {
	if (_step != kStepSex || sex > 1)
		return false;
	_character.setSex(sex);
	_step = kStepRace;
	return true;
}

bool CharacterCreationFlow::chooseRace(uint8 race) {
	if (_step != kStepRace || race > 10)
		return false;
	_race = race;
	_step = kStepProfession;
	return true;
}

bool CharacterCreationFlow::chooseProfession(uint classIndex) {
	if (_step != kStepProfession || !_character.selectClass(classIndex))
		return false;
	_step = kStepName;
	return true;
}

bool CharacterCreationFlow::setName(const Common::String &name) {
	if (_step != kStepName || name.empty() || name.size() > 8)
		return false;
	for (uint i = 0; i < name.size(); ++i) {
		const byte c = name[i];
		if (c < 32 || c >= 127)
			return false;
	}
	_name = name;
	return true;
}

bool CharacterCreationFlow::finishName() {
	if (_step != kStepName || _name.empty())
		return false;
	_step = kStepBonus;
	return true;
}

bool CharacterCreationFlow::addBonusPoint(uint statIndex) {
	if (_step != kStepBonus || statIndex >= CharacterCreation::kStatCount)
		return false;
	if (_character.getBonusPoints() <= 0 || _character.getStat(statIndex) >= 18)
		return false;
	_character.setStat(statIndex, _character.getStat(statIndex) + 1);
	_character.setBonusPoints(_character.getBonusPoints() - 1);
	++_bonusSpent[statIndex];
	return true;
}

bool CharacterCreationFlow::removeBonusPoint(uint statIndex) {
	if (_step != kStepBonus || statIndex >= CharacterCreation::kStatCount || _bonusSpent[statIndex] == 0)
		return false;
	_character.setStat(statIndex, _character.getStat(statIndex) - 1);
	_character.setBonusPoints(_character.getBonusPoints() + 1);
	--_bonusSpent[statIndex];
	return true;
}

bool CharacterCreationFlow::finish() {
	if (_step != kStepBonus || _character.getBonusPoints() != 0)
		return false;
	_step = kStepDone;
	return true;
}

const char *CharacterCreation::getClassName(uint classIndex) {
	static const char *const names[kClassCount] = {
		"FIGHTER", "MAGE", "PRIEST", "THIEF", "RANGER", "ALCHEMIST", "BARD",
		"PSIONIC", "VALKYRIE", "BISHOP", "LORD", "SAMURAI", "MONK", "NINJA"
	};
	return classIndex < kClassCount ? names[classIndex] : "UNKNOWN";
}

} // End of namespace Wizardry7
