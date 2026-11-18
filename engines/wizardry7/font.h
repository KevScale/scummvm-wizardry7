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

#ifndef WIZARDRY7_FONT_H
#define WIZARDRY7_FONT_H

#include "common/array.h"
#include "common/path.h"
#include "common/scummsys.h"

namespace Graphics {
class Screen;
}

namespace Wizardry7 {

/**
 * A VBFONT / VCFONT / VCPORT bitmap font.
 *
 * 0x110-byte header, then one or two glyph bitmap blocks. A 128-entry table at
 * header+0x10 maps character codes to glyph indices (even bytes) and extra
 * advance widths (odd bytes).
 */
class GameFont {
public:
	GameFont();

	bool load(const Common::Path &path);
	bool loadFromData(const byte *data, uint32 size);
	bool isLoaded() const { return !_data.empty(); }

	uint getWidth() const { return _width; }
	uint getHeight() const { return _height; }
	uint getGlyphCount() const { return _glyphCount; }
	bool is8bpp() const { return _pixelMode == 1; }
	int getBaseAdvance() const { return _baseAdvance; }
	uint getStride() const { return _stride; }
	uint getMaskStride() const { return _maskStride; }
	uint getFlags() const { return _flags; }
	uint getAdvance(byte character) const;
	uint getBytesPerGlyph() const { return _bytesPerGlyph; }

	/** Draw one character; returns the advance in pixels (0 if unmapped). */
	uint drawChar(byte character, Graphics::Screen *screen, int x, int y, byte color, bool opaque, byte background) const;

	/** Draw a glyph by index (for icon/portrait fonts). */
	uint drawGlyph(uint index, Graphics::Screen *screen, int x, int y, byte color, bool opaque, byte background) const;
	uint getGlyphAdvance(uint index) const;

private:
	const byte *glyphData(uint index) const;

	Common::Array<byte> _data;
	uint _width;
	uint _height;
	uint _pixelMode;
	uint _stride;
	uint _maskStride;
	uint _glyphCount;
	uint _flags;
	int _baseAdvance;
	uint _bytesPerGlyph;
	uint _secondOffset;
	uint _secondBytesPerGlyph;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_FONT_H
