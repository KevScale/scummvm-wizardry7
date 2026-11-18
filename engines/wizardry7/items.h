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

#ifndef WIZARDRY7_ITEMS_H
#define WIZARDRY7_ITEMS_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Wizardry7 {

class ScenarioDatabase;

/** One SCENARIO type-1 item record (72 bytes). */
struct ItemInfo {
	ItemInfo();

	Common::String name;        // raw 22-byte field, NUL-stripped
	Common::String displayName; // '=' -> "OF ", '/' dropped, '_' -> space
	uint16 price;               // +0x16
	uint16 weight;              // +0x24, displayed weight x10
	uint16 specialMsg;          // +0x28
	uint16 classMask;           // +0x32, 14 class bits
	uint16 raceMask;            // +0x34, 11 race bits
	uint16 spellEffect;         // +0x22, low = spell id, high = power
	uint8 paramA;               // +0x1A
	uint8 paramB;               // +0x1C
	uint8 paramC;               // +0x1D
	uint8 damageDiceCount;      // +0x20
	uint8 damageDieSize;        // +0x21
	uint8 genderMask;           // +0x36
	uint8 flags;                // +0x37
	uint8 kind;                 // +0x38 (equip slot)
	uint8 subtype;              // +0x39 (proficiency/effect group)
	uint8 rank;                 // +0x3B
	uint8 acBonus;              // +0x40

	uint8 getSpellId() const { return (uint8)(spellEffect & 0xFF); }
	uint8 getSpellPower() const { return (uint8)(spellEffect >> 8); }
	uint16 getDefaultCharges() const { return (uint16)paramB * paramC + paramA; }
};

/** Decode the raw 22-byte name field into the display string. */
Common::String decodeItemName(const byte *field, uint maxLen);

/** Parse one 72-byte SCENARIO type-1 record. */
bool parseItemRecord(const byte *record, uint32 size, ItemInfo &info);

/** Read an item by id from the loaded scenario database. */
bool loadItem(const ScenarioDatabase &scenario, uint16 itemId, ItemInfo &info);

/** Cached view of all type-1 records. */
class ItemDatabase {
public:
	ItemDatabase();

	bool load(const ScenarioDatabase &scenario);
	bool isLoaded() const { return !_items.empty(); }
	uint getCount() const { return _items.size(); }
	const ItemInfo &getItem(uint index) const { return _items[index]; }
	bool getItem(uint16 itemId, ItemInfo &info) const;

private:
	Common::Array<ItemInfo> _items;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_ITEMS_H
