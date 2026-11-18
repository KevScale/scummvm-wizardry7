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
