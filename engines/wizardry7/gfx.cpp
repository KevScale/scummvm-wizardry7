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

#include "wizardry7/gfx.h"

namespace Wizardry7 {

GamePalette::GamePalette() : _loaded(false) {
	memset(_rgb, 0, sizeof(_rgb));
}

bool GamePalette::load(const Common::Path &path) {
	Common::File file;
	if (!file.open(path)) {
		warning("Wizardry7: cannot open palette '%s'", path.toString().c_str());
		return false;
	}
	if (file.size() < 768) {
		warning("Wizardry7: palette '%s' is too short", path.toString().c_str());
		return false;
	}

	for (uint i = 0; i < 768; ++i) {
		const byte value = file.readByte();
		_rgb[i] = (value << 2) | (value >> 4);
	}
	_loaded = true;
	return true;
}

void GamePalette::apply(Graphics::Screen *screen) const {
	if (screen)
		screen->setPalette(_rgb, 0, 256);
}

VgaBundle::VgaBundle() : _recordCount(0), _recordTableOffset(0) {
}

bool VgaBundle::load(const Common::Path &path) {
	Common::File file;
	if (!file.open(path)) {
		warning("Wizardry7: cannot open VGA bundle '%s'", path.toString().c_str());
		return false;
	}
	const uint32 size = file.size();
	Common::Array<byte> data;
	data.resize(size);
	if (file.read(data.data(), size) != size)
		return false;
	return loadFromData(data.data(), size);
}

bool VgaBundle::loadFromData(const byte *source, uint32 size) {
	_data.clear();
	_entries.clear();
	_recordCount = 0;

	if (size < 4) {
		warning("Wizardry7: VGA bundle data is too short");
		return false;
	}
	_data.resize(size);
	memcpy(_data.data(), source, size);

	const uint16 chunkCount = READ_LE_UINT16(_data.data());
	_recordCount = READ_LE_UINT16(_data.data() + 2);
	if (chunkCount == 0) {
		warning("Wizardry7: VGA bundle has no chunks");
		return false;
	}

	const uint32 tableEnd = 4 + chunkCount * 6 + _recordCount * 5;
	if (tableEnd > size) {
		warning("Wizardry7: VGA bundle has a truncated table");
		return false;
	}

	_entries.resize(chunkCount);
	for (uint i = 0; i < chunkCount; ++i) {
		const byte *entry = _data.data() + 4 + i * 6;
		const uint16 segment = READ_LE_UINT16(entry);
		const uint16 offset = READ_LE_UINT16(entry + 2);
		const uint16 dims = READ_LE_UINT16(entry + 4);

		Entry &chunk = _entries[i];
		chunk.offset = segment * 16 + offset;
		chunk.stride = (dims & 0xFF) * 8;
		chunk.height = dims >> 8;
	}

	if (_entries[0].offset != tableEnd) {
		warning("Wizardry7: VGA bundle first chunk is at 0x%x, expected 0x%x",
			_entries[0].offset, tableEnd);
		return false;
	}

	for (uint i = 0; i < chunkCount; ++i) {
		const uint32 next = (i + 1 < chunkCount) ? _entries[i + 1].offset : size;
		const Entry &chunk = _entries[i];
		if (chunk.offset > next || next > size || chunk.stride == 0 || chunk.height == 0 ||
			(uint32)chunk.stride * chunk.height != next - chunk.offset) {
			warning("Wizardry7: invalid chunk %u in VGA bundle", i);
			_entries.clear();
			return false;
		}
	}

	_recordTableOffset = 4 + chunkCount * 6;
	return true;
}

bool VgaBundle::getChunkInfo(uint index, uint &width, uint &height, uint32 &offset, uint32 &size) const {
	if (index >= _entries.size())
		return false;
	const Entry &chunk = _entries[index];
	width = chunk.stride;
	height = chunk.height;
	offset = chunk.offset;
	size = (uint32)chunk.stride * chunk.height;
	return true;
}

bool VgaBundle::drawChunk(uint index, Graphics::Screen *screen, int x, int y) const {
	if (!screen || index >= _entries.size())
		return false;

	const Entry &chunk = _entries[index];
	int srcX = 0;
	int srcY = 0;
	int width = chunk.stride;
	int height = chunk.height;

	if (x < 0) {
		srcX = -x;
		width -= srcX;
		x = 0;
	}
	if (y < 0) {
		srcY = -y;
		height -= srcY;
		y = 0;
	}
	if (x + width > screen->w)
		width = screen->w - x;
	if (y + height > screen->h)
		height = screen->h - y;
	if (width <= 0 || height <= 0)
		return false;

	const byte *pixels = _data.data() + chunk.offset + srcY * chunk.stride + srcX;
	screen->copyRectToSurface(pixels, chunk.stride, x, y, width, height);
	return true;
}

uint16 VgaBundle::getRecordWord(uint record, uint field) const {
	if (record >= _recordCount || field >= 3)
		return 0;
	return READ_LE_UINT16(_data.data() + _recordTableOffset + record * 5 + field * 2);
}

byte VgaBundle::getRecordByte(uint record, uint offset) const {
	if (record >= _recordCount || offset >= 5)
		return 0;
	return _data[_recordTableOffset + record * 5 + offset];
}

bool loadScreen(const Common::Path &path, Common::Array<byte> &pixels, uint &width, uint &height) {
	Common::File file;
	if (!file.open(path)) {
		warning("Wizardry7: cannot open screen '%s'", path.toString().c_str());
		return false;
	}

	const uint32 size = file.size();
	if (size == 320 * 200) {
		width = 320;
		height = 200;
	} else if (size == 640 * 400) {
		width = 640;
		height = 400;
	} else {
		warning("Wizardry7: unexpected screen size %u in '%s'", size, path.toString().c_str());
		return false;
	}

	pixels.resize(size);
	return file.read(pixels.data(), size) == size;
}

} // End of namespace Wizardry7
