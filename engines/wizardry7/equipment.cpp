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

#include "wizardry7/equipment.h"

#include "common/util.h"

namespace Wizardry7 {

namespace {

const uint16 kClassItems[14][StartingEquipment::kMaxItems] = {
	{ 8, 135, 132, 130, 141 }, // Fighter
	{ 18, 122, 123, 130, 335 }, // Mage
	{ 24, 122, 123, 130, 316 }, // Priest
	{ 6, 120, 121, 131, 27 }, // Thief
	{ 31, 33, 126, 127, 131 }, // Ranger
	{ 18, 122, 123, 130, 327 }, // Alchemist
	{ 29, 30, 120, 121, 55 }, // Bard
	{ 1, 122, 123, 130, 241 }, // Psionic
	{ 22, 124, 125, 130, 138 }, // Valkyrie
	{ 24, 122, 123, 130, 163 }, // Bishop
	{ 9, 133, 134, 131, 143 }, // Lord
	{ 10, 4, 122, 123, 130 }, // Samurai
	{ 25, 47, 122, 123, 130 }, // Monk
	{ 47, 160, 158, 159, 161 } // Ninja
};

const uint16 kFaerieItems[StartingEquipment::kMaxItems] = { 397, 398, 399, 316, 384 };

uint8 itemCount(uint16 itemId) {
	switch (itemId) {
	case 27: return 6; // Bite Daggers
	case 30: return 25; // Bullet Stones
	case 33: return 100; // Willow Arrows
	case 47: return 15; // Shurikens
	case 316: return 3; // Lt. Heal potions
	default: return 1;
	}
}

} // End of namespace

uint StartingEquipment::getItems(uint classIndex, uint8 raceIndex, StartingItem *items) {
	if (classIndex >= 14)
		return 0;
	const uint16 *ids = raceIndex == 5 ? kFaerieItems : kClassItems[classIndex];
	for (uint i = 0; i < kMaxItems; ++i) {
		items[i].itemId = ids[i];
		items[i].count = itemCount(ids[i]);
	}
	return kMaxItems;
}

} // End of namespace Wizardry7
