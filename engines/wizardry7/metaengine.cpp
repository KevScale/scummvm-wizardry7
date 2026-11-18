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

#include "wizardry7/detection.h"
#include "wizardry7/metaengine.h"
#include "wizardry7/wizardry7.h"

const char *Wizardry7MetaEngine::getName() const {
	return "wizardry7";
}

Common::Error Wizardry7MetaEngine::createInstance(OSystem *syst, Engine **engine, const Wizardry7::Wizardry7GameDescription *desc) const {
	*engine = new Wizardry7::Wizardry7Engine(syst, desc);
	return Common::kNoError;
}

#if PLUGIN_ENABLED_DYNAMIC(WIZARDRY7)
REGISTER_PLUGIN_DYNAMIC(WIZARDRY7, PLUGIN_TYPE_ENGINE, Wizardry7MetaEngine);
#else
REGISTER_PLUGIN_STATIC(WIZARDRY7, PLUGIN_TYPE_ENGINE, Wizardry7MetaEngine);
#endif
