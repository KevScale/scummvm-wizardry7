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

#ifndef WIZARDRY7_PARTY_H
#define WIZARDRY7_PARTY_H

#include "common/scummsys.h"

namespace Wizardry7 {

class CharacterRoster;

/**
 * Active-party order on top of the 16-slot castle roster.
 *
 * Entries are roster slot ids (0..15); empty entries are 0xFF. This is engine
 * state, not the shipped PCFILE layout. The eventual save serializer will map
 * the six ids to the savegame's party-slot array.
 */
class Party {
public:
	enum { kMaxMembers = 6, kEmpty = 0xFF };

	Party();

	void clear();
	bool addMember(uint8 rosterSlot);
	bool removeMember(uint8 rosterSlot);
	bool moveMember(uint from, uint to);
	bool contains(uint8 rosterSlot) const;
	uint size() const { return _size; }
	uint8 getMember(uint index) const;

	/** Fill in roster order (used slots only), up to six members. */
	void loadFromRoster(const CharacterRoster &roster);

	/** Internal save-state representation: six bytes, 0xFF for empty. */
	void toBytes(byte *out) const;
	bool fromBytes(const byte *data, uint size);

private:
	uint8 _members[kMaxMembers];
	uint _size;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_PARTY_H
