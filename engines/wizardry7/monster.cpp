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

#include "wizardry7/monster.h"
#include "wizardry7/scenario.h"

namespace Wizardry7 {

static Common::String readFixedString(const byte *data, uint length) {
	Common::String out;
	for (uint i = 0; i < length && data[i]; ++i)
		out += (char)data[i];
	return out;
}

MonsterInfo::MonsterInfo() : xp(0), spawnCount(0), levelDice(0), actorCount(0),
	hpDice(0), stDice(0), bodyType(0), pictureId(0), flags(0) {
	memset(variantId, 0, sizeof(variantId));
	memset(variantChance, 0, sizeof(variantChance));
}

bool parseMonsterRecord(const byte *record, uint32 size, MonsterInfo &info) {
	if (!record || size < 232)
		return false;
	info = MonsterInfo();
	for (uint i = 0; i < 4; ++i)
		info.names[i] = readFixedString(record + i * 16, 16);
	info.xp = READ_LE_UINT32(record + 0x40);
	info.spawnCount = READ_LE_UINT16(record + 0x7A);
	info.levelDice = READ_LE_UINT32(record + 0x7E);
	info.actorCount = record[0x80];
	info.hpDice = READ_LE_UINT32(record + 0x82);
	info.stDice = READ_LE_UINT32(record + 0x86);
	info.variantId[0] = record[0xAA];
	info.variantId[1] = record[0xAB];
	info.variantChance[0] = record[0xAC];
	info.variantChance[1] = record[0xAD];
	info.bodyType = record[0xD6];
	info.pictureId = record[0xD7];
	info.flags = record[0xE2];
	return true;
}

bool loadMonster(const ScenarioDatabase &scenario, uint16 monsterId, MonsterInfo &info) {
	Common::Array<byte> record;
	if (monsterId >= scenario.getRecordCount(kRecordMonsterName) ||
		!scenario.readRecord(kRecordMonsterName, monsterId, record))
		return false;
	return parseMonsterRecord(record.data(), record.size(), info);
}

MonsterDatabase::MonsterDatabase() {
}

bool MonsterDatabase::load(const ScenarioDatabase &scenario) {
	_monsters.clear();
	if (!scenario.isLoaded())
		return false;
	const uint count = scenario.getRecordCount(kRecordMonsterName);
	_monsters.resize(count);
	for (uint i = 0; i < count; ++i) {
		Common::Array<byte> record;
		if (!scenario.readRecord(kRecordMonsterName, i, record) ||
			!parseMonsterRecord(record.data(), record.size(), _monsters[i])) {
			_monsters.clear();
			return false;
		}
	}
	return true;
}

bool MonsterDatabase::getMonster(uint16 monsterId, MonsterInfo &info) const {
	if (monsterId >= _monsters.size())
		return false;
	info = _monsters[monsterId];
	return true;
}

} // End of namespace Wizardry7
