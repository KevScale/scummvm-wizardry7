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

#include "wizardry7/attack.h"
#include "wizardry7/rng.h"
#include "wizardry7/scenario.h"

namespace Wizardry7 {

AttackAnimation::AttackAnimation() : _loaded(false) {
}

bool AttackAnimation::load(const ScenarioDatabase &scenario, uint pictureId) {
	Common::Array<byte> record;
	if (pictureId >= scenario.getRecordCount(kRecordMonsterStats) ||
		!scenario.readRecord(kRecordMonsterStats, pictureId, record))
		return false;
	return loadFromData(record.data(), record.size());
}

bool AttackAnimation::loadFromData(const byte *data, uint32 size) {
	_loaded = false;
	if (!data || size < kRecordSize)
		return false;
	_raw.resize(kRecordSize);
	memcpy(_raw.data(), data, kRecordSize);
	_loaded = true;
	return true;
}

uint8 AttackAnimation::getBaseDamage(uint attack) const {
	const uint offset = 0x08 + attack;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getRandomDamage(uint attack) const {
	const uint offset = 0x12 + attack;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getExtraRandomDamage(uint attack) const {
	const uint offset = 0x30 + attack;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getSpecialIndex(uint damageIndex) const {
	const uint offset = 0x26 + damageIndex;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getSpecialSprite(uint special) const {
	const uint offset = 0xA5 + special * 4;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getSpecialEffectByte(uint special) const {
	const uint offset = 0xA5 + special * 4 + 1;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getSeverityBase(uint special) const {
	const uint offset = 0xA5 + special * 4 + 2;
	return offset < _raw.size() ? _raw[offset] : 0;
}

uint8 AttackAnimation::getSeverityRandom(uint special) const {
	const uint offset = 0xA5 + special * 4 + 3;
	return offset < _raw.size() ? _raw[offset] : 0;
}

AttackRoll AttackAnimation::rollAttack(uint attack, WizardryRNG &rng) const {
	AttackRoll roll;
	roll.damageIndex = getBaseDamage(attack) + rng.getRandom(getRandomDamage(attack));
	roll.specialIndex = getSpecialIndex(roll.damageIndex);
	roll.severity = getSeverityBase(roll.specialIndex) + 1 +
		rng.getRandom(getSeverityRandom(roll.specialIndex));
	return roll;
}

uint16 AttackAnimation::getPairA() const {
	return READ_LE_UINT16(_raw.data() + 0x148);
}

uint16 AttackAnimation::getPairB() const {
	return READ_LE_UINT16(_raw.data() + 0x14A);
}

} // End of namespace Wizardry7
