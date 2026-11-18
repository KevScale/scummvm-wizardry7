/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names are too
 * numerous to list here. Please refer to the COPYRIGHT file distributed with
 * this source distribution.
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

#include "graphics/screen.h"

#include "wizardry7/gfx.h"
#include "wizardry7/view.h"

namespace Wizardry7 {

static const char *const kBundleNames[7] = {
	"MAZEDATA", "TREEDATA", "TOWNDATA", "TILEDATA", "OTWNDATA", "WATRDATA", "TILEDATA"
};

ViewCompositor::ViewCompositor() {
	memset(_bundles, 0, sizeof(_bundles));
}

Common::Array<ViewPiece> ViewCompositor::fixedPieces(uint mode, bool mirror) {
	Common::Array<ViewPiece> pieces;
	switch (mode) {
	case 1:
		pieces.push_back({0, 0x1F, (uint16)(mirror ? 0x1F : 0xFFFF), 0});
		pieces.push_back({0, 0x20, (uint16)(mirror ? 0x20 : 0xFFFF), 0});
		break;
	case 2:
		pieces.push_back({1, 4, (uint16)(mirror ? 4 : 0xFFFF), 2});
		pieces.push_back({1, 0x3E, (uint16)(mirror ? 0x3E : 0xFFFF), 0});
		break;
	case 3:
		pieces.push_back({4, 4, (uint16)(mirror ? 4 : 0xFFFF), 0});
		pieces.push_back({2, 0xAD, (uint16)(mirror ? 0xAD : 0xFFFF), 0});
		break;
	case 4:
		pieces.push_back({1, 4, (uint16)(mirror ? 4 : 0xFFFF), 2});
		pieces.push_back({4, 0x2A, (uint16)(mirror ? 0x2A : 0xFFFF), 0});
		break;
	case 5:
		pieces.push_back({4, 0x2A, (uint16)(mirror ? 0x2A : 0xFFFF), 0});
		break;
	case 6:
		pieces.push_back({1, 0x3E, (uint16)(mirror ? 0x3E : 0xFFFF), 0});
		break;
	default:
		break;
	}
	return pieces;
}

// FUN_0044B3BB's 32 arguments, indexed [depth][param-1]; extracted from
// docs/formats/maze-and-monsters.md §View assembly pipeline.
static const uint16 kPainterArgs[5][32] = {
	{
		0, 0, 0, 1, 2, 4, 2, 4, 5, 6, 10, 20, 47, 57, 4, 4, 0, 0, 16, 20, 8, 12,
		10, 20, 10, 20, 10, 20, 182, 182, 206, 211
	},
	{
		0, 1, 0, 1, 2, 4, 2, 4, 5, 6, 5, 15, 42, 52, 4, 4, 0, 0, 16, 20, 8, 12,
		5, 15, 5, 15, 5, 15, 178, 178, 196, 201
	},
	{
		0, 2, 0, 1, 2, 4, 2, 4, 5, 6, 0, 0, 37, 37, 4, 4, 0, 0, 16, 20, 8, 12,
		0, 0, 0, 0, 0, 0, 174, 174, 0xFFFF, 0xFFFF
	},
	{
		0, 3, 0, 1, 2, 4, 2, 4, 5, 6, 15, 5, 52, 42, 4, 4, 0, 0, 16, 20, 8, 12,
		15, 5, 15, 5, 15, 5, 186, 186, 201, 196
	},
	{
		0, 4, 0, 1, 2, 4, 2, 4, 5, 6, 20, 10, 57, 47, 4, 4, 0, 0, 16, 20, 8, 12,
		20, 10, 20, 10, 20, 10, 190, 190, 211, 206
	}
};

static uint16 painterArg(uint depth, uint param) {
	return kPainterArgs[depth][param - 1];
}

bool ViewCompositor::painterDirect(uint kind, uint depth, uint column, bool mirror,
		ViewPiece &piece) {
	if (depth >= 5)
		return false;
	if (kind == 1) {
		if (!mirror) {
			piece = { (uint8)painterArg(depth, 4), (uint16)(column + painterArg(depth, 13)), 0xFFFF, 2 };
		} else {
			piece = { (uint8)painterArg(depth, 4), (uint16)(column + painterArg(depth, 14)),
				(uint16)(column + painterArg(depth, 13)), 2 };
		}
		return true;
	}
	if (kind == 2) {
		if (!mirror) {
			piece = { (uint8)painterArg(depth, 10), (uint16)(column + painterArg(depth, 23)), 0xFFFF, 2 };
		} else {
			piece = { (uint8)painterArg(depth, 10), (uint16)(column + painterArg(depth, 24)),
				(uint16)(column + painterArg(depth, 23)), 2 };
		}
		return true;
	}
	if (kind == 3) {
		if (mirror) {
			piece = { (uint8)painterArg(depth, 9), (uint16)(column + painterArg(depth, 25)), 0xFFFF, 2 };
		} else {
			piece = { (uint8)painterArg(depth, 9), (uint16)(column + painterArg(depth, 26)),
				(uint16)(column + painterArg(depth, 25)), 2 };
		}
		return true;
	}
	return false;
}

Common::Array<ViewPiece> ViewCompositor::painterPieces(const byte kinds[5][5], bool mirror) {
	static const uint kDepthOrder[5] = { 2, 1, 0, 3, 4 };
	Common::Array<ViewPiece> out;
	for (uint column = 0; column < 5; ++column) {
		for (uint i = 0; i < 5; ++i) {
			const uint depth = kDepthOrder[i];
			ViewPiece piece;
			if (kinds[column][depth] && painterDirect(kinds[column][depth], depth, column, mirror, piece))
				out.push_back(piece);
		}
	}
	return out;
}

bool ViewCompositor::loadBundles() {
	for (uint i = 0; i < 7; ++i) {
		if (_bundles[i])
			continue;
		_bundles[i] = new VgaBundle();
		Common::String name = Common::String(kBundleNames[i]) + ".VGA";
		if (!_bundles[i]->load(Common::Path(name))) {
			delete _bundles[i];
			_bundles[i] = nullptr;
		}
	}
	return _bundles[0] != nullptr;
}

bool ViewCompositor::drawPiece(Graphics::Screen *screen, const ViewPiece &piece) const {
	if (!screen || piece.slot >= 7 || !_bundles[piece.slot])
		return false;
	const uint16 record = piece.sel == 0xFFFF ? piece.piece : piece.sel;
	VgaPlacement placement;
	if (!_bundles[piece.slot]->getPlacement(record, placement))
		return false;
	_bundles[piece.slot]->drawChunk(placement.bitmap, screen,
		(placement.xExtra + placement.x) * 8, placement.y);
	return true;
}

} // End of namespace Wizardry7
