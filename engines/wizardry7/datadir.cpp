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

#include "common/archive.h"
#include "common/str.h"

#include "wizardry7/datadir.h"

namespace Wizardry7 {

/** Gold data directory below an install root. */
static const char *const kDataDirectory = "Basedata";

/** Message database of the Gold (MSG.GLD) and DOS (MSG.DBS) builds. */
static const char *const kDataFiles[] = { "MSG.GLD", "MSG.DBS", nullptr };

static bool holdsGameData(const Common::FSNode &directory) {
	if (!directory.isDirectory())
		return false;

	for (const char *const *name = kDataFiles; *name; ++name) {
		if (directory.getChild(*name).exists())
			return true;
	}

	return false;
}

Common::FSNode findDataDirectory(const Common::FSNode &gamePath) {
	if (holdsGameData(gamePath))
		return gamePath;

	Common::FSList children;
	if (!gamePath.getChildren(children, Common::FSNode::kListDirectoriesOnly))
		return Common::FSNode();

	for (const auto &child : children) {
		if (child.getName().equalsIgnoreCase(kDataDirectory) && holdsGameData(child))
			return child;
	}

	return Common::FSNode();
}

bool registerDataDirectory(const Common::FSNode &gamePath) {
	const Common::FSNode dataDirectory = findDataDirectory(gamePath);
	if (!dataDirectory.exists() || dataDirectory.getPath() == gamePath.getPath())
		return false;

	// Nothing to do when the data sits in the game path itself, and skip a
	// directory that a previous call (or the detector) has already registered.
	const Common::String name = dataDirectory.getName();
	if (SearchMan.hasArchive(name))
		return false;

	SearchMan.addDirectory(name, dataDirectory);
	return true;
}

} // End of namespace Wizardry7
