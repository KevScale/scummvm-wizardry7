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

#include "common/file.h"
#include "common/fs.h"
#include "common/tokenizer.h"

#include "graphics/screen.h"

#include "wizardry7/gfx.h"
#include "wizardry7/view.h"

namespace Wizardry7 {

static const char *const kBundleNames[7] = {
	"MAZEDATA", "TREEDATA", "TOWNDATA", "TILEDATA", "OTWNDATA", "WATRDATA", "TILEDATA"
};

static const uint16 kShapeBlock0[0x60] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0006, 0x0000,
    0x0006, 0x0000, 0x0009, 0x0000, 0x0047, 0x0000, 0x0033, 0x0000, 0x0033, 0x0000, 0x0036, 0x0000,
    0x0039, 0x0000, 0x0039, 0x0000, 0x0036, 0x0000, 0x007a, 0x0000, 0x007a, 0x0000, 0x0098, 0x0000,
    0x0098, 0x0000, 0x0009, 0x0000, 0x0009, 0x0000, 0x0031, 0x0000, 0x0031, 0x0000, 0x0035, 0x0000,
    0x0039, 0x0000, 0x0039, 0x0000, 0x0035, 0x0000, 0x008d, 0x0000, 0x0095, 0x0000, 0x0008, 0x0000,
    0x0008, 0x0000, 0x0026, 0x0000, 0x0026, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16 kShapeBlock1[0x60] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0003, 0x0000,
    0x0003, 0x0000, 0x0005, 0x0000, 0x0043, 0x0000, 0x002a, 0x0000, 0x002a, 0x0000, 0x002d, 0x0000,
    0x0030, 0x0000, 0x0030, 0x0000, 0x002d, 0x0000, 0x0074, 0x0000, 0x0074, 0x0000, 0x0092, 0x0000,
    0x0092, 0x0000, 0x0005, 0x0000, 0x0005, 0x0000, 0x0025, 0x0000, 0x0025, 0x0000, 0x0029, 0x0000,
    0x002d, 0x0000, 0x002d, 0x0000, 0x0029, 0x0000, 0x0089, 0x0000, 0x0091, 0x0000, 0x0004, 0x0000,
    0x0004, 0x0000, 0x0022, 0x0000, 0x0022, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16 kShapeBlock2[0x60] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x003f, 0x0000, 0x0021, 0x0000, 0x0021, 0x0000, 0x0024, 0x0000,
    0x0027, 0x0000, 0x0027, 0x0000, 0x0024, 0x0000, 0x006e, 0x0000, 0x006e, 0x0000, 0x008c, 0x0000,
    0x008c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0019, 0x0000, 0x0019, 0x0000, 0x001d, 0x0000,
    0x0021, 0x0000, 0x0021, 0x0000, 0x001d, 0x0000, 0x0085, 0x0000, 0x0085, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x001e, 0x0000, 0x001e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16 kShapeBlock3[0x60] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0009, 0x0000,
    0x0009, 0x0000, 0x000d, 0x0000, 0x004b, 0x0000, 0x003c, 0x0000, 0x003c, 0x0000, 0x003f, 0x0000,
    0x0042, 0x0000, 0x0042, 0x0000, 0x003f, 0x0000, 0x0080, 0x0000, 0x0080, 0x0000, 0x009e, 0x0000,
    0x009e, 0x0000, 0x000d, 0x0000, 0x000d, 0x0000, 0x003d, 0x0000, 0x003d, 0x0000, 0x0041, 0x0000,
    0x0045, 0x0000, 0x0045, 0x0000, 0x0041, 0x0000, 0x0091, 0x0000, 0x0089, 0x0000, 0x000c, 0x0000,
    0x000c, 0x0000, 0x002a, 0x0000, 0x002a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
static const uint16 kShapeBlock4[0x60] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0004, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x0002, 0x0000, 0x0004, 0x0000, 0x000c, 0x0000,
    0x000c, 0x0000, 0x0011, 0x0000, 0x004f, 0x0000, 0x0045, 0x0000, 0x0045, 0x0000, 0x0048, 0x0000,
    0x004b, 0x0000, 0x004b, 0x0000, 0x0048, 0x0000, 0x0086, 0x0000, 0x0086, 0x0000, 0x00a4, 0x0000,
    0x00a4, 0x0000, 0x0011, 0x0000, 0x0011, 0x0000, 0x0049, 0x0000, 0x0049, 0x0000, 0x004d, 0x0000,
    0x0051, 0x0000, 0x0051, 0x0000, 0x004d, 0x0000, 0x0095, 0x0000, 0x008d, 0x0000, 0x0010, 0x0000,
    0x0010, 0x0000, 0x002e, 0x0000, 0x002e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

ViewObjectList::ViewObjectList() {
}

bool ViewObjectList::spawn(const ViewObject &object) {
	if (_objects.size() >= 0x78)
		return false;
	_objects.push_back(object);
	return true;
}

Common::Array<ViewPiece> ViewObjectList::draw(uint columns) const {
	Common::Array<ViewPiece> out;
	for (int column = (int)columns - 1; column >= 0; --column) {
		for (int layer = 0xFF; layer >= 0xFB; --layer) {
			for (int i = (int)_objects.size() - 1; i >= 0; --i) {
				const ViewObject &o = _objects[i];
				if (o.column != column || o.layer != layer || o.slot > 9)
					continue; // slot > 9 is a stale entry from a previous frame
				if (o.pieceA != 0xFFFF)
					out.push_back({o.slot, o.pieceA, o.selA, o.writemodeA});
				if (o.pieceB != 0xFFFF)
					out.push_back({o.slot, o.pieceB, o.selB, o.writemodeB});
			}
			if (layer == 0xFF || layer == 0xFB) {
				const uint8 group = layer == 0xFF ? 0 : 1;
				for (int i = (int)_objects.size() - 1; i >= 0; --i) {
					const ViewObject &o = _objects[i];
					if (o.column != column || o.layer >= 0xFA || o.group != group || o.slot > 9)
						continue;
					if (o.pieceA != 0xFFFF)
						out.push_back({o.slot, o.pieceA, o.selA, o.writemodeA});
					if (o.pieceB != 0xFFFF)
						out.push_back({o.slot, o.pieceB, o.selB, o.writemodeB});
				}
			}
		}
	}
	return out;
}

void ViewCompositor::spawnObjects(uint column, uint kind, uint viewKind, bool mirror,
		uint depth, ViewObjectList &objects) {
	if (depth >= 5)
		return;
	const uint16 *block = depth == 0 ? kShapeBlock0 : depth == 1 ? kShapeBlock1 :
		depth == 2 ? kShapeBlock2 : depth == 3 ? kShapeBlock3 : kShapeBlock4;
	const auto t = [block](uint off) -> uint16 { return off < 0x60 ? block[off] : 0; };
	const uint16 kNone = 0xFFFF;
	ViewObject o = {};

	auto emit = [&](uint8 layer, uint16 pieceA, uint16 pieceB, uint8 p4, uint8 p5,
			uint8 p6, uint8 p7, uint16 selB, uint16 selA, uint8 col, uint8 slot,
			uint8 p12, uint8 group) {
		o.pieceA = pieceA;
		o.pieceB = pieceB;
		o.writemodeA = p4;
		o.writemodeB = p5;
		o.p6 = p6;
		o.p7 = p7;
		o.selB = selB;
		o.selA = selA;
		o.layer = layer;
		o.column = col;
		o.slot = slot;
		o.p12 = p12;
		o.group = group;
		objects.spawn(o);
	};

	uint16 pieceLair = kNone, selLair = kNone;
	if (viewKind > 0 && column == 3) {
		uint16 a, b;
		if (viewKind < 3 || viewKind == 5) {
			a = 0x15; b = 0x16;
		} else {
			a = 0x14; b = 0x15;
		}
		pieceLair = mirror ? b : a;
		selLair = mirror ? a : kNone;
	}

	if (kind >= 1 && kind <= 0xF) {
		if (kind == 1 || kind == 3 || kind == 6 || kind == 7) {
			if (pieceLair != kNone)
				emit(0xFF, pieceLair, kNone, column == 1 ? 2 : 0, 0, 0, 0,
					selLair, kNone, column, t(0x1C), 0, 0);
		} else if (kind == 4) {
			if ((kind & 1) && column < 4)
				emit(0xFC, t(0x26) + column, kNone, 2, 0, 0, 0, kNone, kNone,
					column, t(0x18), 0, 0);
		} else if (kind == 5) {
			if (pieceLair != kNone)
				emit(0xFF, pieceLair, kNone, 2, 0, 0, 0, selLair, kNone,
					column, t(0x20), 0, 0);
		} else if (kind != 8 && kind != 9) {
			if (viewKind == 0) {
				if (column < 3)
					emit(0xFF, mirror ? t(0x24) + column : t(0x22) + column, kNone,
						column == 1 ? 2 : 0, 0, 0, 0,
						mirror ? t(0x22) + column : kNone, kNone, column, t(0x16), 0, 0);
			} else if (viewKind < 3 || viewKind == 5) {
				if (column < 4)
					emit(0xFF, mirror ? t(0x40) + column : t(0x3E) + column, pieceLair,
						2, 2, 0, 0, mirror ? t(0x3E) + column : kNone, selLair,
						column, t(0x1C), 0, 0);
			} else if (viewKind < 5 && column < 4) {
				emit(0xFF, mirror ? t(0x54) + column : t(0x52) + column, pieceLair,
					2, 2, 0, 0, mirror ? t(0x52) + column : kNone, selLair,
					column, t(0x20), 0, 0);
			}
		}
	}

	const uint keep = kind == 0 ? 3 : 4;
	if (column < keep) {
		if (kind == 5) {
			if (column < 4)
				emit(0xFF, mirror ? t(0x58) + column : t(0x56) + column, kNone, 2, 0, 0, 0,
					mirror ? t(0x56) + column : kNone, kNone, column, t(0x1E), 0, 0);
		} else if (kind == 8 || kind == 9) {
			const uint16 base = kind == 8 ? t(0x36) : t(0x3A);
			pieceLair = column + base + 3;
			emit(0xFF, mirror ? t(0x2C) + column : t(0x2A) + column, kNone, 2, 2, 0, 0,
				mirror ? t(0x2A) + column : kNone, kNone, column, t(0x16), 0, 0);
		} else if (kind == 7) {
			emit(0xFF, t(0x4E) + column, kNone, 2, 2, 0, 0,
				mirror ? t(0x4E) + column : kNone, kNone, column, t(0x1E), 0, 0);
		} else if (kind == 1 || kind == 3 || kind == 6) {
			if (viewKind == 0)
				emit(0xFF, mirror ? t(0x2C) + column : t(0x2A) + column, pieceLair, 2, 2, 0, 0,
					mirror ? t(0x2A) + column : kNone, selLair, column, t(0x16), 0, 0);
			else if (viewKind == 2)
				emit(0xFF, mirror ? t(0x44) + column : t(0x42) + column, pieceLair, 2, 2, 0, 0,
					mirror ? t(0x42) + column : kNone, selLair, column, t(0x1A), 0, 0);
		}
	}
}

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

bool ViewCompositor::drawTrace(const Common::Path &path, Graphics::Screen *screen) {
	Common::FSNode node(path);
	if (!node.exists() || node.isDirectory())
		return false;
	Common::SeekableReadStream *stream = node.createReadStream();
	if (!stream)
		return false;
	static const int kModeSlot[8] = { -1, 1, 0, 6, 2, -1, 5, 4 };
	while (!stream->eos()) {
		const Common::String line = stream->readLine();
		if (line.empty())
			continue;
		Common::StringTokenizer tok(line, " \t");
		if (tok.empty())
			continue;
		int values[7] = { 0 };
		uint count = 0;
		while (!tok.empty() && count < 7) {
			const Common::String field = tok.nextToken();
			char *end = nullptr;
			values[count++] = (int)strtol(field.c_str(), &end, 10);
		}
		if (count != 6 && count != 7)
			continue;
		const bool hasTime = count == 7;
		const int mode = values[hasTime ? 1 : 0];
		const uint tile = (uint)values[hasTime ? 2 : 1];
		const int x = values[hasTime ? 4 : 3], y = values[hasTime ? 5 : 4];
		if (mode < 0 || mode >= 8 || kModeSlot[mode] < 0)
			continue;
		const int slot = kModeSlot[mode];
		if (!_bundles[slot])
			continue;
		_bundles[slot]->drawChunk(tile, screen, x, y);
	}
	delete stream;
	return true;
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
