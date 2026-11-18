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

#ifndef WIZARDRY7_GFX_H
#define WIZARDRY7_GFX_H

#include "common/array.h"
#include "common/path.h"
#include "common/scummsys.h"

namespace Graphics {
class Screen;
}

namespace Wizardry7 {

/** 256-colour palette from PALETTE.DEF (6-bit VGA DAC values). */
class GamePalette {
public:
	GamePalette();

	bool load(const Common::Path &path);
	bool isLoaded() const { return _loaded; }
	const byte *getRGB() const { return _rgb; }
	void apply(Graphics::Screen *screen) const;

private:
	byte _rgb[768];
	bool _loaded;
};

/**
 * A Basedata VGA image bundle: chunk table + optional record table + raw
 * 8bpp chunk data occupied by PALETTE.DEF colours.
 */
class VgaBundle {
public:
	VgaBundle();

	bool load(const Common::Path &path);
	bool isLoaded() const { return !_entries.empty(); }

	uint getChunkCount() const { return _entries.size(); }
	uint getRecordCount() const { return _recordCount; }

	bool getChunkInfo(uint index, uint &width, uint &height, uint32 &offset, uint32 &size) const;
	bool drawChunk(uint index, Graphics::Screen *screen, int x, int y) const;

	uint16 getRecordWord(uint record, uint field) const;
	byte getRecordByte(uint record, uint offset) const;

private:
	struct Entry {
		uint32 offset;
		uint16 stride;
		uint16 height;
	};

	Common::Array<byte> _data;
	Common::Array<Entry> _entries;
	uint16 _recordCount;
	uint32 _recordTableOffset;
};

/** Load a raw 8bpp screen dump (320x200) from Basedata. */
bool loadScreen(const Common::Path &path, Common::Array<byte> &pixels, uint &width, uint &height);

} // End of namespace Wizardry7

#endif // WIZARDRY7_GFX_H
