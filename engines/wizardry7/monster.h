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

#ifndef WIZARDRY7_MONSTER_H
#define WIZARDRY7_MONSTER_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Wizardry7 {

class ScenarioDatabase;

/** One SCENARIO type-4 monster record (232 bytes). */
struct MonsterInfo {
	MonsterInfo();

	Common::String names[4]; // singular, plural, short, short plural
	uint32 xp;               // +0x40
	uint16 spawnCount;       // +0x7A
	uint32 levelDice;        // +0x7E (raw packed dice)
	uint8 actorCount;        // +0x80
	uint32 hpDice;           // +0x82 (raw packed dice)
	uint32 stDice;           // +0x86 (raw packed dice)
	uint8 variantId[2];      // +0xAA/+0xAB
	uint8 variantChance[2];  // +0xAC/+0xAD
	uint8 bodyType;          // +0xD6
	uint8 pictureId;         // +0xD7
	uint8 flags;             // +0xE2, bit0 = ignore difficulty scaling
};

bool parseMonsterRecord(const byte *record, uint32 size, MonsterInfo &info);
bool loadMonster(const ScenarioDatabase &scenario, uint16 monsterId, MonsterInfo &info);

class MonsterDatabase {
public:
	MonsterDatabase();

	bool load(const ScenarioDatabase &scenario);
	bool isLoaded() const { return !_monsters.empty(); }
	uint getCount() const { return _monsters.size(); }
	const MonsterInfo &getMonster(uint index) const { return _monsters[index]; }
	bool getMonster(uint16 monsterId, MonsterInfo &info) const;

private:
	Common::Array<MonsterInfo> _monsters;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_MONSTER_H
