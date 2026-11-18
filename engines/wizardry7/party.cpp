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

#include "wizardry7/party.h"
#include "wizardry7/save.h"

namespace Wizardry7 {

Party::Party() {
	clear();
}

void Party::clear() {
	for (uint i = 0; i < kMaxMembers; ++i)
		_members[i] = kEmpty;
	_size = 0;
}

bool Party::addMember(uint8 rosterSlot) {
	if (rosterSlot > 15 || _size >= kMaxMembers || contains(rosterSlot))
		return false;
	_members[_size++] = rosterSlot;
	return true;
}

bool Party::removeMember(uint8 rosterSlot) {
	for (uint i = 0; i < _size; ++i) {
		if (_members[i] != rosterSlot)
			continue;
		for (uint j = i; j + 1 < _size; ++j)
			_members[j] = _members[j + 1];
		_members[--_size] = kEmpty;
		return true;
	}
	return false;
}

bool Party::moveMember(uint from, uint to) {
	if (from >= _size || to >= _size || from == to)
		return false;
	const uint8 slot = _members[from];
	if (from < to)
		for (uint i = from; i < to; ++i)
			_members[i] = _members[i + 1];
	else
		for (uint i = from; i > to; --i)
			_members[i] = _members[i - 1];
	_members[to] = slot;
	return true;
}

bool Party::contains(uint8 rosterSlot) const {
	for (uint i = 0; i < _size; ++i)
		if (_members[i] == rosterSlot)
			return true;
	return false;
}

uint8 Party::getMember(uint index) const {
	return index < _size ? _members[index] : (uint8)kEmpty;
}

void Party::loadFromRoster(const CharacterRoster &roster) {
	clear();
	for (uint i = 0; i < roster.getSlotCount() && _size < kMaxMembers; ++i)
		if (roster.isSlotUsed(i))
			addMember((uint8)i);
}

void Party::toBytes(byte *out) const {
	for (uint i = 0; i < kMaxMembers; ++i)
		out[i] = _members[i];
}

bool Party::fromBytes(const byte *data, uint size) {
	if (size < kMaxMembers)
		return false;
	Party loaded;
	for (uint i = 0; i < kMaxMembers; ++i) {
		if (data[i] == kEmpty)
			continue;
		if (data[i] > 15 || loaded.contains(data[i]))
			return false;
		loaded.addMember(data[i]);
	}
	*this = loaded;
	return true;
}

} // End of namespace Wizardry7
