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

#include "wizardry7/items.h"
#include "wizardry7/scenario.h"

namespace Wizardry7 {

ItemInfo::ItemInfo() : price(0), weight(0), specialMsg(0), classMask(0),
	raceMask(0), spellEffect(0), paramA(0), paramB(0), paramC(0),
	damageDiceCount(0), damageDieSize(0), genderMask(0), flags(0), kind(0),
	subtype(0), rank(0), acBonus(0) {
}

Common::String decodeItemName(const byte *field, uint maxLen) {
	Common::String out;
	for (uint i = 0; i < maxLen; ++i) {
		const byte c = field[i];
		if (c == 0)
			break;
		if (c == '=') {
			out += "OF ";
		} else if (c == '/') {
			// two-line name separator; the display text drops it
		} else if (c == '_') {
			out += ' ';
		} else {
			out += (char)c;
		}
	}
	while (!out.empty() && out.lastChar() == ' ')
		out.deleteLastChar();
	return out;
}

bool parseItemRecord(const byte *record, uint32 size, ItemInfo &info) {
	if (!record || size < 0x48)
		return false;
	info = ItemInfo();
	info.name = decodeItemName(record, 22);
	// Keep the raw name too (decodeItemName expands placeholders).
	info.name.clear();
	for (uint i = 0; i < 22 && record[i]; ++i)
		info.name += (char)record[i];
	info.displayName = decodeItemName(record, 22);
	info.price = READ_LE_UINT16(record + 0x16);
	info.paramA = record[0x1A];
	info.paramB = record[0x1C];
	info.paramC = record[0x1D];
	info.damageDiceCount = record[0x20];
	info.damageDieSize = record[0x21];
	info.spellEffect = READ_LE_UINT16(record + 0x22);
	info.weight = READ_LE_UINT16(record + 0x24);
	info.specialMsg = READ_LE_UINT16(record + 0x28);
	info.classMask = READ_LE_UINT16(record + 0x32);
	info.raceMask = READ_LE_UINT16(record + 0x34);
	info.genderMask = record[0x36];
	info.flags = record[0x37];
	info.kind = record[0x38];
	info.subtype = record[0x39];
	info.rank = record[0x3B];
	info.acBonus = record[0x40];
	return true;
}

bool loadItem(const ScenarioDatabase &scenario, uint16 itemId, ItemInfo &info) {
	Common::Array<byte> record;
	if (itemId >= scenario.getRecordCount(kRecordItem) ||
		!scenario.readRecord(kRecordItem, itemId, record))
		return false;
	return parseItemRecord(record.data(), record.size(), info);
}

ItemDatabase::ItemDatabase() {
}

bool ItemDatabase::load(const ScenarioDatabase &scenario) {
	_items.clear();
	if (!scenario.isLoaded())
		return false;
	const uint count = scenario.getRecordCount(kRecordItem);
	_items.resize(count);
	for (uint i = 0; i < count; ++i) {
		Common::Array<byte> record;
		if (!scenario.readRecord(kRecordItem, i, record) ||
			!parseItemRecord(record.data(), record.size(), _items[i])) {
			_items.clear();
			return false;
		}
	}
	return true;
}

bool ItemDatabase::getItem(uint16 itemId, ItemInfo &info) const {
	if (itemId >= _items.size())
		return false;
	info = _items[itemId];
	return true;
}

} // End of namespace Wizardry7
