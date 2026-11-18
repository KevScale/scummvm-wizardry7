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

#include "common/scummsys.h"

#include "wizardry7/rng.h"

namespace Wizardry7 {

WizardryRNG::WizardryRNG() {
	reset();
}

void WizardryRNG::reset() {
	reset(3000, 1, 29999);
}

void WizardryRNG::reset(uint16 seed1, uint16 seed2, uint16 seed3) {
	_seed[0] = seed1;
	_seed[1] = seed2;
	_seed[2] = seed3;
}

uint16 WizardryRNG::step(uint16 state, int schrageModulus, int multiplier, int increment, int modulus) {
	const int s = state;
	int value = (s % schrageModulus) * multiplier - (s / schrageModulus) * increment;
	if (value < 0)
		value += modulus;
	return (uint16)value;
}

uint16 WizardryRNG::rand16() {
	_seed[0] = step(_seed[0], 177, 171, 2, 30269);
	_seed[1] = step(_seed[1], 176, 172, 35, 30307);
	_seed[2] = step(_seed[2], 178, 170, 63, 30323);

	const int t1 = (_seed[0] % 30269) * 10000 / 30269;
	const int t2 = (_seed[1] % 30307) * 10000 / 30307;
	const int t3 = (_seed[2] % 30323) * 10000 / 30323;

	return (uint16)((t3 + ((t1 + t2) & 0x7FFF)) & 0x7FFF);
}

uint32 WizardryRNG::getRandom(uint32 max) {
	if (max == 0)
		return 0;
	return rand16() % max;
}

uint16 WizardryRNG::getSeed(int index) const {
	assert(index >= 0 && index < 3);
	return _seed[index];
}

void WizardryRNG::setSeed(int index, uint16 value) {
	assert(index >= 0 && index < 3);
	_seed[index] = value;
}

} // End of namespace Wizardry7
