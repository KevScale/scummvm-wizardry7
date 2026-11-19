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

#ifndef WIZARDRY7_RNG_H
#define WIZARDRY7_RNG_H

#include "common/scummsys.h"

namespace Wizardry7 {

class TraceLog;

/**
 * The combined Wichmann-Hill generator used by Wizardry VII for all rolls.
 *
 * Three concurrent 16-bit LCGs are stepped with Schrage's method, then the
 * three scaled outputs are folded together. The game starts every session
 * with the fixed seed (3000, 1, 29999).
 */
class WizardryRNG {
public:
	WizardryRNG();

	/** Attach the gameplay trace; draws are then logged while it records. */
	void setTrace(TraceLog *trace) { _trace = trace; }

	/** Restore the initial seed used at process start. */
	void reset();

	/** Restore an explicit seed, e.g. when loading a game state. */
	void reset(uint16 seed1, uint16 seed2, uint16 seed3);

	/** Return the next raw value in the range 0..32767. */
	uint16 rand16();

	/** Return a value in the range 0..max-1, or 0 if max is 0. */
	uint32 getRandom(uint32 max);

	uint16 getSeed(int index) const;
	void setSeed(int index, uint16 value);

private:
	static uint16 step(uint16 state, int schrageModulus, int multiplier, int increment, int modulus);

	/** Step all three generators and fold the outputs; no logging. */
	uint16 advance();

	uint16 _seed[3];
	TraceLog *_trace;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_RNG_H
