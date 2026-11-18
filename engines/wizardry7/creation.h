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

#ifndef WIZARDRY7_CREATION_H
#define WIZARDRY7_CREATION_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "wizardry7/equipment.h"

namespace Wizardry7 {

class WizardryRNG;
class ScenarioDatabase;

/** Fields rolled when a class is finalized (before starting equipment). */
struct CreationRolls {
	uint32 ageDays;
	uint16 hpMax;
	uint16 staminaMax;
	uint16 carryingCapacity; // stored value for 0x697D72 (display divides by 10)
	uint8 ac;
};

/**
 * Character-creation class eligibility.
 *
 * Each profession has eight minimum stats encoded as letters ('A' = 0).
 * A class is offered when the total deficit between the character's stats and
 * those minimums can be paid from the remaining bonus-point pool; choosing the
 * class raises the stats to the minimums and deducts the deficit. Valkyrie is
 * additionally female-only.
 */
class CharacterCreationFlow;

class CharacterCreation {
public:
	enum { kStatCount = 8, kClassCount = 14 };

	CharacterCreation();

	void setStats(const byte *stats);
	void setStat(uint index, byte value);
	byte getStat(uint index) const { return index < kStatCount ? _stats[index] : 0; }
	void setBonusPoints(int points) { _bonusPoints = points; }
	int getBonusPoints() const { return _bonusPoints; }
	void setSex(byte sex) { _sex = sex; }
	byte getSex() const { return _sex; }

	/** Initial bonus pool: rand(6)+5, two 1-in-20 +8 rolls, minimum 9. */
	static int rollBonusPoints(WizardryRNG &rng);

	/** Raise the stats to the race's minimums (race 0..10). */
	void applyRaceMinimums(uint8 race);
	/** Female adjustment: STR -2, PER +1, KAR +1. */
	void applySexModifiers(byte sex);
	int getClass() const { return _classIndex; }

	/** Bonus-point cost of raising the stats to this class's minimums, or -1. */
	int getClassCost(uint classIndex) const;
	bool isClassEligible(uint classIndex) const;

	/** Spend bonus points and raise the stats to the class minimums. */
	bool selectClass(uint classIndex);

	/** Roll age, HP, ST and starting gold for the selected class. */
	CreationRolls rollDerived(uint8 raceIndex, WizardryRNG &rng) const;

	/** Eligible classes in original class-id order. */
	Common::Array<uint> getEligibleClasses() const;

	static const char *getClassName(uint classIndex);

private:
	byte _stats[kStatCount];
	int _bonusPoints;
	byte _sex;
	int _classIndex;
};

/** Result of a completed creation dialog. */
struct CreatedCharacter {
	Common::String name;
	uint8 race;
	uint8 sex;
	uint8 classIndex;
	byte stats[CharacterCreation::kStatCount];
	CreationRolls rolls;
	StartingItem items[StartingEquipment::kMaxItems];
	uint itemCount;
};

enum { kCharacterRecordSize = 584 };

/**
 * Write the fields the engine currently understands into a PCFILE/.SGM
 * character record. Inventory, skills, AC and portrait are not written yet;
 * callers must treat this as a partial record.
 */
bool writeCharacterRecord(const CreatedCharacter &character, byte *record, uint size);

/** Fill one 12-byte inventory slot from an item record (FUN_00458322 layout). */
bool writeInventorySlot(byte *slot, uint16 itemId, const byte *itemRecord, uint8 count);

/** Fill the five starting slots from the scenario item database. */
bool writeCharacterInventory(const CreatedCharacter &character, const ScenarioDatabase &scenario,
	byte *record, uint size);

/** Step/selection state for the DOS character-creation dialogs. */
class CharacterCreationFlow {
public:
	enum Step {
		kStepSex,
		kStepRace,
		kStepProfession,
		kStepName,
		kStepBonus,
		kStepDone
	};

	CharacterCreationFlow();

	/** Start with the rolled/base stats and the initial bonus pool. */
	void begin(const byte *stats, int bonusPoints);
	Step getStep() const { return _step; }
	CharacterCreation &getCharacter() { return _character; }
	const CharacterCreation &getCharacter() const { return _character; }
	const Common::String &getName() const { return _name; }
	uint8 getRace() const { return _race; }

	bool chooseSex(byte sex);
	bool chooseRace(uint8 race);
	bool chooseProfession(uint classIndex);
	bool setName(const Common::String &name);
	bool finishName();
	bool addBonusPoint(uint statIndex);
	bool removeBonusPoint(uint statIndex);
	/** Finish and roll the final age/HP/ST/capacity + starting kit. */
	bool buildCharacter(WizardryRNG &rng, CreatedCharacter &out);
	bool finish();

private:
	CharacterCreation _character;
	Step _step;
	Common::String _name;
	uint8 _race;
	uint8 _bonusSpent[CharacterCreation::kStatCount];
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_CREATION_H
