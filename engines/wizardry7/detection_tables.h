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

#ifndef WIZARDRY7_DETECTION_TABLES_H
#define WIZARDRY7_DETECTION_TABLES_H

namespace Wizardry7 {

static const PlainGameDescriptor wizardry7Games[] = {
	{ "wizardry7", "Wizardry VII: Crusaders of the Dark Savant" },
	{ nullptr, nullptr }
};

// MD5 values cover the first 5000 bytes (AdvancedDetector default).
static const Wizardry7GameDescription gameDescriptions[] = {
	{
		{
			"wizardry7",
			"Gold",
			AD_ENTRY2s("Basedata/MSG.GLD", "1053a6e809f2a9f8898ee03324c32060", 344064,
				"Basedata/SCENARIO.GLD", "2509289c55646ece02b7756454865479", 368320),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_UNSTABLE,
			GUIO0()
		},
		kGameGold
	},
	{
		{
			"wizardry7",
			"DOS",
			AD_ENTRY2s("MSG.DBS", "ee539a8b24e2992af39fba9c34061bed", 262144,
				"SCENARIO.DBS", "2509289c55646ece02b7756454865479", 368320),
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_UNSTABLE,
			GUIO0()
		},
		kGameDOS
	},
	{
		AD_TABLE_END_MARKER,
		kGameGold
	}
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_DETECTION_TABLES_H
