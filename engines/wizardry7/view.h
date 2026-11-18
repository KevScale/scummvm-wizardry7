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

#ifndef WIZARDRY7_VIEW_H
#define WIZARDRY7_VIEW_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Graphics { class Screen; }

namespace Wizardry7 {

class VgaBundle;

/** One scene-compositor draw request. */
struct ViewPiece {
	uint8 slot;     // scene bundle slot (0 MAZE,1 TREE,2 TOWN,4 OTWN,5 WATR,6 TILE)
	uint16 piece;   // descriptor record
	uint16 sel;     // 0xFFFF => use piece
	uint8 writemode;
};

/**
 * First stage of the maze view compositor: fixed pieces emitted by the view-mode
 * switch and the painter's direct draws. The object-list spawner is not modelled
 * yet; see docs/engine/view-compositor.md.
 */
class ViewCompositor {
public:
	ViewCompositor();

	static Common::Array<ViewPiece> fixedPieces(uint mode, bool mirror);

	bool loadBundles();
	bool drawPiece(Graphics::Screen *screen, const ViewPiece &piece) const;

private:
	VgaBundle *_bundles[7];
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_VIEW_H
