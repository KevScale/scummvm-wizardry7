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
#include "wizardry7/scenario.h"

#include "common/endian.h"
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
	++_stats[7]; // trailing byte: DOS calls it KAR, Gold uses it as portrait
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

	rolls.ac = 10;
	const byte spd = _stats[5];
	if (spd > 15)
		--rolls.ac;
	if (spd > 17)
		--rolls.ac;
	if (raceIndex == 5) // FAERIE
		rolls.ac -= 2;
	return rolls;
}

void CharacterCreation::getProfessionGridPosition(uint index, uint count, uint &row, uint &col) {
	const uint rows = (count + 2) / 3;
	col = rows ? index / rows : 0;
	row = rows ? index % rows : 0;
}

Common::Array<uint> CharacterCreation::getEligibleClasses() const {
	Common::Array<uint> classes;
	for (uint i = 0; i < kClassCount; ++i) {
		if (isClassEligible(i))
			classes.push_back(i);
	}
	return classes;
}

bool writeInventorySlot(byte *slot, uint16 itemId, const byte *itemRecord, uint8 count) {
	if (!slot || !itemRecord)
		return false;
	memset(slot, 0, 12);
	WRITE_LE_UINT16(slot, itemId);
	WRITE_LE_UINT16(slot + 2, READ_LE_UINT16(itemRecord + 0x24)); // weight
	slot[4] = itemRecord[0x38]; // kind/equip slot
	slot[5] = itemRecord[0x39]; // subtype
	slot[6] = count > 1 ? count : 0; // charges/count (non-stack items store 0)
	slot[7] = 0x80; // identified
	if (itemRecord[0x37] & 8)  slot[7] |= 4;
	if (itemRecord[0x37] & 4)  slot[7] |= 8;
	if (itemRecord[0x37] & 0x10) slot[7] |= 0x10;
	if (itemRecord[0x37] & 0x20) slot[7] |= 0x20;
	if (itemRecord[0x37] & 2)  slot[7] |= 0x40;
	slot[8] = itemRecord[0x28];
	slot[9] = itemRecord[0x29];
	slot[10] = itemRecord[0x3C];
	slot[11] = itemRecord[0x40]; // AC bonus
	return true;
}

bool writeCharacterInventory(const CreatedCharacter &character, const ScenarioDatabase &scenario,
		byte *record, uint size) {
	if (!record || size < kCharacterRecordSize || !scenario.isLoaded())
		return false;
	Common::Array<byte> itemRecord;
	for (uint i = 0; i < character.itemCount && i < 5; ++i) {
		const uint16 itemId = character.items[i].itemId;
		if (itemId >= scenario.getRecordCount(kRecordItem) ||
			!scenario.readRecord(kRecordItem, itemId, itemRecord) ||
			itemRecord.size() < 0x41)
			return false;
		if (!writeInventorySlot(record + 0x40 + i * 12, itemId, itemRecord.data(),
				character.items[i].count))
			return false;
	}
	return true;
}

bool writeCharacterRecord(const CreatedCharacter &character, byte *record, uint size) {
	if (!record || size < kCharacterRecordSize || character.name.size() > 8)
		return false;
	memset(record, 0, kCharacterRecordSize);
	memcpy(record, character.name.c_str(), character.name.size());
	WRITE_LE_UINT32(record + 0x08, character.rolls.ageDays);
	WRITE_LE_UINT16(record + 0x18, character.rolls.hpMax);
	WRITE_LE_UINT16(record + 0x1A, character.rolls.hpMax);
	WRITE_LE_UINT16(record + 0x1C, character.rolls.staminaMax);
	WRITE_LE_UINT16(record + 0x1E, character.rolls.staminaMax);
	WRITE_LE_UINT16(record + 0x1B6, character.rolls.ac);
	for (uint i = 0x1B9; i <= 0x1BD; ++i)
		record[i] = character.rolls.ac;
	for (uint i = 0; i < 7; ++i)
		record[0x170 + i] = character.stats[i];
	record[0x177] = character.stats[7]; // DOS KAR / Gold portrait; rolled later
	record[0x240] = 1;
	record[0x241] = 10;
	record[0x243] = 1;
	record[0x244] = 1;
	for (uint i = 0; i < character.itemCount && i < 5; ++i) {
		const uint32 slot = 0x40 + i * 12;
		WRITE_LE_UINT16(record + slot, character.items[i].itemId);
		record[slot + 7] = 0x80; // known/identified starting item
	}
	record[0x237] = character.race;
	record[0x238] = character.sex;
	record[0x239] = character.classIndex;
	// 0x236 knack, 0x23A alignment, inventory and skills remain unwritten.
	return true;
}

CharacterCreationFlow::CharacterCreationFlow() : _step(kStepSex), _goldOrder(false), _race(0) {
	memset(_bonusSpent, 0, sizeof(_bonusSpent));
}

void CharacterCreationFlow::begin(const byte *stats, int bonusPoints, bool goldOrder) {
	_character.setStats(stats);
	_character.setBonusPoints(bonusPoints);
	_name.clear();
	_race = 0;
	_goldOrder = goldOrder;
	memset(_bonusSpent, 0, sizeof(_bonusSpent));
	_step = kStepName; // both builds ask for the name first

}

bool CharacterCreationFlow::chooseSex(byte sex) {
	if (_step != kStepSex || sex > 1)
		return false;
	_character.setSex(sex);
	_step = kStepProfession;
	return true;
}

bool CharacterCreationFlow::chooseRace(uint8 race) {
	if (_step != kStepRace || race > 10)
		return false;
	_race = race;
	_step = kStepSex;
	return true;
}

bool CharacterCreationFlow::chooseProfession(uint classIndex) {
	if (_step != kStepProfession || !_character.selectClass(classIndex))
		return false;
	_step = kStepBonus;
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
	_step = kStepRace;
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

bool CharacterCreationFlow::buildCharacter(WizardryRNG &rng, CreatedCharacter &out) {
	if (_step != kStepBonus || _character.getBonusPoints() != 0 || _character.getClass() < 0)
		return false;
	out.name = _name;
	out.race = _race;
	out.sex = _character.getSex();
	out.classIndex = (uint8)_character.getClass();
	for (uint i = 0; i < CharacterCreation::kStatCount; ++i)
		out.stats[i] = _character.getStat(i);
	out.rolls = _character.rollDerived(_race, rng);
	out.itemCount = StartingEquipment::getItems(out.classIndex, _race, out.items);
	_step = kStepDone;
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
