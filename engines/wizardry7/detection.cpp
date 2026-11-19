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

#include "base/plugins.h"

#include "wizardry7/detection.h"
#include "wizardry7/detection_tables.h"

class Wizardry7MetaEngineDetection : public AdvancedMetaEngineDetection<Wizardry7::Wizardry7GameDescription> {
public:
	Wizardry7MetaEngineDetection() : AdvancedMetaEngineDetection(Wizardry7::gameDescriptions, Wizardry7::wizardry7Games) {
		// The Gold data files live in Basedata/. The detection entries name
		// them without the prefix, so also look one directory deeper than the
		// selected path: this accepts either the install root or Basedata/
		// itself as the game path.
		static const char *const directoryGlobs[] = { "Basedata", nullptr };
		_directoryGlobs = directoryGlobs;
		_maxScanDepth = 2;
		_flags = kADFlagMatchFullPaths;
	}

	const char *getName() const override {
		return "wizardry7";
	}

	const char *getEngineName() const override {
		return "Wizardry VII";
	}

	const char *getOriginalCopyright() const override {
		return "Wizardry VII (C) 1992-1996 Sir-Tech Software";
	}
};

REGISTER_PLUGIN_STATIC(WIZARDRY7_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, Wizardry7MetaEngineDetection);
