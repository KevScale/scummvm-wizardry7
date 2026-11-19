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

#include "common/file.h"
#include "common/stream.h"
#include "common/str.h"

#include "wizardry7/trace.h"

namespace Wizardry7 {

TraceLog::TraceLog() : _enabled(false), _draws(0), _dropped(0) {
	_seed[0] = _seed[1] = _seed[2] = 0;
}

void TraceLog::reset() {
	_enabled = false;
	_draws = 0;
	_dropped = 0;
	_seed[0] = _seed[1] = _seed[2] = 0;
	_lines.clear();
}

void TraceLog::start(uint16 seed1, uint16 seed2, uint16 seed3) {
	reset();
	_seed[0] = seed1;
	_seed[1] = seed2;
	_seed[2] = seed3;
	_enabled = true;
}

void TraceLog::stop() {
	_enabled = false;
}

void TraceLog::append(const Common::String &line) {
	if (_lines.size() >= kMaxLines) {
		++_dropped;
		return;
	}
	_lines.push_back(line);
}

void TraceLog::addRngDraw(uint32 modulus, uint16 seed1, uint16 seed2, uint16 seed3,
		uint16 value, bool consumed) {
	if (!_enabled)
		return;

	append(Common::String::format("%u RNG n=%u s=%u,%u,%u -> %u", _draws, modulus,
		seed1, seed2, seed3, value));
	if (consumed)
		++_draws;
}

void TraceLog::addEvent(const Common::String &event) {
	if (!_enabled)
		return;

	append(Common::String::format("%u %s", _draws, event.c_str()));
}

bool TraceLog::write(Common::WriteStream &out) const {
	out.writeString(Common::String::format("wizardry7-trace 1\nseed %u %u %u\n",
		_seed[0], _seed[1], _seed[2]));
	for (uint i = 0; i < _lines.size(); ++i) {
		out.writeString(_lines[i]);
		out.writeByte('\n');
	}
	if (_dropped)
		out.writeString(Common::String::format("truncated dropped=%u\n", _dropped));
	return !out.err();
}

bool TraceLog::dump(const Common::Path &path, Common::String &error) const {
	Common::DumpFile out;
	if (!out.open(path)) {
		error = "cannot create file";
		return false;
	}

	const bool ok = write(out);
	out.close();
	if (!ok)
		error = "write failed";
	return ok;
}

} // End of namespace Wizardry7
