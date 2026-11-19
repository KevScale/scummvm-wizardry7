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

#ifndef WIZARDRY7_DATADIR_H
#define WIZARDRY7_DATADIR_H

#include "common/fs.h"

namespace Wizardry7 {

/**
 * Find the directory holding the game data below the launcher's game path.
 *
 * Wizardry Gold keeps its data (MSG.GLD, SCENARIO.GLD, ...) in a Basedata/
 * subdirectory, so the launcher can be pointed either at the install root or
 * straight at Basedata/. The DOS build is a flat directory.
 *
 * @return The directory holding the data files: @p gamePath itself, the
 *         Basedata/ child of an install root, or an invalid node when neither
 *         holds them.
 */
Common::FSNode findDataDirectory(const Common::FSNode &gamePath);

/**
 * Make the game data reachable through SearchMan.
 *
 * Registers the Basedata/ child of an install root. The game path itself is
 * registered by Engine::initializePath(), and a directory that is already in
 * the search path is not added a second time.
 *
 * @return true when this call added a directory to SearchMan.
 */
bool registerDataDirectory(const Common::FSNode &gamePath);

} // End of namespace Wizardry7

#endif // WIZARDRY7_DATADIR_H
