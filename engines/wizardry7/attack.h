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

#ifndef WIZARDRY7_ATTACK_H
#define WIZARDRY7_ATTACK_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Wizardry7 {

class ScenarioDatabase;
class WizardryRNG;

/** One resolved step of the type-8 attack roll. */
struct AttackRoll {
	uint8 damageIndex;
	uint8 specialIndex;
	uint8 severity;
};

/**
 * One SCENARIO type-8 per-picture attack/animation record (350 bytes).
 *
 * The record is indexed by the type-4 picture id, not by monster id. Fields are
 * exposed without inventing combat semantics; see
 * docs/formats/maze-and-monsters.md §Type 8.
 */
class AttackAnimation {
public:
	enum { kRecordSize = 350, kSpecialCount = 24 };

	AttackAnimation();

	bool load(const ScenarioDatabase &scenario, uint pictureId);
	bool loadFromData(const byte *data, uint32 size);
	bool isLoaded() const { return _loaded; }

	uint8 getAttackStart() const { return _raw[0x00]; }
	uint8 getAttackCount() const { return _raw[0x04]; }
	uint8 getBaseDamage(uint attack) const;
	uint8 getRandomDamage(uint attack) const;
	uint8 getExtraRandomDamage(uint attack) const;
	uint8 getSpecialIndex(uint damageIndex) const;
	uint8 getSpecialSprite(uint special) const;
	uint8 getSpecialEffectByte(uint special) const;
	uint8 getSeverityBase(uint special) const;
	uint8 getSeverityRandom(uint special) const;
	uint16 getPairA() const;
	uint16 getPairB() const;
	uint8 getOverrideA() const { return _raw[0x15B]; }
	uint8 getOverrideB() const { return _raw[0x15C]; }

	/**
	 * Roll one attack: damage = base + rand(random); special = table[damage];
	 * severity = base + 1 + rand(random). The +0x30 bonus roll is applied only
	 * by the caller when the +0x15B override is active.
	 */
	AttackRoll rollAttack(uint attack, WizardryRNG &rng) const;

private:
	Common::Array<byte> _raw;
	bool _loaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_ATTACK_H
