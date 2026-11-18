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

namespace Wizardry7 {

/**
 * Character-creation class eligibility.
 *
 * Each profession has eight minimum stats encoded as letters ('A' = 0).
 * A class is offered when the total deficit between the character's stats and
 * those minimums can be paid from the remaining bonus-point pool; choosing the
 * class raises the stats to the minimums and deducts the deficit. Valkyrie is
 * additionally female-only.
 */
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

	/** Bonus-point cost of raising the stats to this class's minimums, or -1. */
	int getClassCost(uint classIndex) const;
	bool isClassEligible(uint classIndex) const;

	/** Spend bonus points and raise the stats to the class minimums. */
	bool selectClass(uint classIndex);

	/** Eligible classes in original class-id order. */
	Common::Array<uint> getEligibleClasses() const;

	static const char *getClassName(uint classIndex);

private:
	byte _stats[kStatCount];
	int _bonusPoints;
	byte _sex;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_CREATION_H
