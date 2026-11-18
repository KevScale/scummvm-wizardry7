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

#include "common/endian.h"
#include "common/file.h"
#include "common/textconsole.h"

#include "graphics/screen.h"

#include "wizardry7/font.h"

namespace Wizardry7 {

static const uint32 kFontHeaderSize = 0x110;

GameFont::GameFont() :
	_width(0), _height(0), _pixelMode(0), _stride(0), _maskStride(0),
	_glyphCount(0), _flags(0), _baseAdvance(0), _bytesPerGlyph(0),
	_secondOffset(0), _secondBytesPerGlyph(0) {
}

bool GameFont::load(const Common::Path &path) {
	_data.clear();

	Common::File file;
	if (!file.open(path)) {
		warning("Wizardry7: cannot open font '%s'", path.toString().c_str());
		return false;
	}
	const uint32 size = file.size();
	if (size < kFontHeaderSize) {
		warning("Wizardry7: font '%s' is too short", path.toString().c_str());
		return false;
	}
	_data.resize(size);
	if (file.read(_data.data(), size) != size)
		return false;

	_width = _data[0];
	_height = _data[1];
	_pixelMode = _data[2];
	_stride = _data[3];
	_maskStride = _data[4];
	_glyphCount = _data[5];
	_flags = _data[6];
	_baseAdvance = (int8)_data[7];
	_bytesPerGlyph = READ_LE_UINT16(_data.data() + 10);
	_secondOffset = READ_LE_UINT16(_data.data() + 12);
	_secondBytesPerGlyph = READ_LE_UINT16(_data.data() + 14);

	if (_glyphCount == 0 || _bytesPerGlyph == 0 || _width == 0 || _height == 0) {
		warning("Wizardry7: font '%s' has invalid header values", path.toString().c_str());
		_data.clear();
		return false;
	}

	const uint32 firstEnd = kFontHeaderSize + _glyphCount * _bytesPerGlyph;
	if (firstEnd > size) {
		warning("Wizardry7: font '%s' has a truncated glyph block", path.toString().c_str());
		_data.clear();
		return false;
	}
	if ((_flags & 4) && (_secondOffset < firstEnd || _secondBytesPerGlyph == 0 ||
		_secondOffset + _glyphCount * _secondBytesPerGlyph > size)) {
		warning("Wizardry7: font '%s' has an invalid second glyph block", path.toString().c_str());
		_data.clear();
		return false;
	}

	for (uint i = 0; i < 128; ++i) {
		if (_data[0x10 + i * 2] >= _glyphCount) {
			warning("Wizardry7: font '%s' glyph table entry %u is out of range", path.toString().c_str(), i);
			_data.clear();
			return false;
		}
	}

	return true;
}

uint GameFont::getAdvance(byte character) const {
	if (character >= 128)
		return _baseAdvance > 0 ? _baseAdvance : 0;
	const int advance = _baseAdvance + _data[0x11 + character * 2];
	return advance > 0 ? advance : 0;
}

const byte *GameFont::glyphData(byte character) const {
	if (character >= 128)
		return nullptr;
	const uint index = _data[0x10 + character * 2];
	if (index >= _glyphCount)
		return nullptr;
	return _data.data() + kFontHeaderSize + index * _bytesPerGlyph;
}

uint GameFont::drawChar(byte character, Graphics::Screen *screen, int x, int y, byte color, bool opaque, byte background) const {
	const byte *pixels = glyphData(character);
	if (!pixels || !screen)
		return getAdvance(character);

	for (uint row = 0; row < _height; ++row) {
		const int py = y + row;
		if (py < 0 || py >= screen->h)
			continue;
		for (uint col = 0; col < _width; ++col) {
			const int px = x + col;
			if (px < 0 || px >= screen->w)
				continue;

			byte *dst = (byte *)screen->getBasePtr(px, py);
			if (is8bpp()) {
				const byte value = pixels[row * _stride + col];
				if (value != 0xFF)
					*dst = value;
			} else {
				const byte bits = pixels[row * _stride + (col >> 3)];
				if ((bits >> (7 - (col & 7))) & 1)
					*dst = color;
				else if (opaque)
					*dst = background;
			}
		}
	}

	return getAdvance(character);
}

} // End of namespace Wizardry7
