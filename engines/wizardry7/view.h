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
struct ViewPiece;

/** One view-list object (FUN_004493E4 layout). */
struct ViewObject {
	uint16 pieceA;
	uint16 pieceB;
	uint16 selB;
	uint16 selA;
	uint8 writemodeA;
	uint8 writemodeB;
	uint8 p6;
	uint8 p7;
	uint8 layer;
	uint8 column;
	uint8 slot;
	uint8 p12;
	uint8 group;
};

/** The view object list and its draw loop (still supplied by the caller). */
class ViewObjectList {
public:
	ViewObjectList();

	void clear() { _objects.clear(); }
	uint size() const { return _objects.size(); }
	bool spawn(const ViewObject &object);
	Common::Array<ViewPiece> draw(uint columns) const;

private:
	Common::Array<ViewObject> _objects;
};

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

	/** Painter direct draw for one (column, depth) and kind 0..3. */
	static bool painterDirect(uint kind, uint depth, uint column, bool mirror,
		ViewPiece &piece);

	/** All painter draws: columns 0..4, depth slots in order 2,1,0,3,4. */
	static Common::Array<ViewPiece> painterPieces(const byte kinds[5][5], bool mirror);

	bool loadBundles();
	bool drawPiece(Graphics::Screen *screen, const ViewPiece &piece) const;

	/** Replay a captured framecap/composecap trace (t_ms mode tile sel x y clip). */
	bool drawTrace(const Common::Path &path, Graphics::Screen *screen);

private:
	VgaBundle *_bundles[7];
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_VIEW_H
