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

#ifndef WIZARDRY7_TRACE_H
#define WIZARDRY7_TRACE_H

#include "common/array.h"
#include "common/path.h"
#include "common/str.h"

namespace Common {
class WriteStream;
}

namespace Wizardry7 {

/**
 * RNG-indexed gameplay event log.
 *
 * Every line is prefixed with the number of RNG draws performed so far, so a
 * trace of the original game and one of this engine can be compared line by
 * line even when presentation timing differs. A dump looks like this:
 *
 *   wizardry7-trace 1
 *   seed 3000 1 29999
 *   0 RNG n=100 s=3000,1,29999 -> 71
 *   1 create race=0 sex=0 class=0 age=7000 hp=12 portrait=3
 *   2 encounter slot=2 monster=42
 *
 * `RNG` lines carry the modulus the game asked for, the pre-call Wichmann-Hill
 * state and the value handed back, matching the field harness' rngtrace log.
 * Payloads are ids and values only; decoded game text is never stored.
 */
class TraceLog {
public:
	TraceLog();

	/** Drop the buffer and start recording; the seed is the state at this point. */
	void start(uint16 seed1, uint16 seed2, uint16 seed3);

	/** Stop recording but keep the buffer for a later dump. */
	void stop();

	void reset();

	bool isEnabled() const { return _enabled; }
	uint getDrawIndex() const { return _draws; }
	uint getLineCount() const { return _lines.size(); }
	const Common::String &getLine(uint index) const { return _lines[index]; }
	uint16 getSeed(int index) const { return _seed[index]; }

	/**
	 * Record one RNG call. @p consumed is false for calls the game answers
	 * without advancing its state (a zero modulus), which still appear in the
	 * log but do not move the index.
	 */
	void addRngDraw(uint32 modulus, uint16 seed1, uint16 seed2, uint16 seed3,
		uint16 value, bool consumed = true);

	/** Record an event such as "msg 100" or "damage target=1 amount=17". */
	void addEvent(const Common::String &event);

	bool write(Common::WriteStream &out) const;
	bool dump(const Common::Path &path, Common::String &error) const;

	/** Retained line limit; further lines are counted but not stored. */
	static const uint kMaxLines = 100000;

private:
	void append(const Common::String &line);

	bool _enabled;
	uint _draws;
	uint _dropped;
	uint16 _seed[3];
	Common::Array<Common::String> _lines;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_TRACE_H
