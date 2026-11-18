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

#include "common/debug.h"
#include "common/file.h"
#include "common/textconsole.h"

#include "wizardry7/msg.h"

namespace Wizardry7 {

MessageDatabase::MessageDatabase() {
}

bool MessageDatabase::load(const Common::Path &headerFile, const Common::Path &poolFile, const Common::Path &huffmanFile) {
	_entries.clear();
	_pool.clear();
	_huffmanTable.clear();

	Common::File header;
	if (!header.open(headerFile)) {
		warning("Wizardry7: cannot open message index '%s'", headerFile.toString().c_str());
		return false;
	}

	const uint16 count = header.readUint16LE();
	const bool huffman = !huffmanFile.empty();
	const uint32 entrySize = huffman ? 6 : 7;
	if (count == 0 || header.size() < 2 + (int64)count * entrySize) {
		warning("Wizardry7: truncated message index '%s'", headerFile.toString().c_str());
		return false;
	}

	_entries.resize(count);
	uint16 previousId = 0;
	for (uint i = 0; i < count; ++i) {
		Entry &entry = _entries[i];
		entry.startId = header.readUint16LE();
		entry.offset = header.readUint16LE();
		entry.span = header.readByte();
		entry.page = huffman ? header.readByte() : header.readUint16LE();

		if (i > 0 && entry.startId <= previousId)
			warning("Wizardry7: message index is not sorted at entry %u", i);
		previousId = entry.startId;
	}

	if (huffman && !loadHuffmanTable(huffmanFile))
		return false;

	return loadPool(poolFile);
}

bool MessageDatabase::loadPool(const Common::Path &path) {
	Common::File pool;
	if (!pool.open(path)) {
		warning("Wizardry7: cannot open message pool '%s'", path.toString().c_str());
		return false;
	}

	const uint32 size = pool.size();
	_pool.resize(size);
	return pool.read(_pool.data(), size) == size;
}

bool MessageDatabase::loadHuffmanTable(const Common::Path &path) {
	Common::File file;
	if (!file.open(path)) {
		warning("Wizardry7: cannot open Huffman table '%s'", path.toString().c_str());
		return false;
	}

	if (file.size() < 1024) {
		warning("Wizardry7: Huffman table '%s' is too short", path.toString().c_str());
		return false;
	}

	_huffmanTable.resize(512);
	for (uint i = 0; i < 512; ++i)
		_huffmanTable[i] = file.readSint16LE();

	return true;
}

bool MessageDatabase::findEntry(uint16 id, uint &index, uint &skip) const {
	uint lo = 0;
	uint hi = _entries.size();
	while (lo < hi) {
		const uint mid = (lo + hi) / 2;
		const Entry &entry = _entries[mid];
		if (id < entry.startId) {
			hi = mid;
		} else if (id > entry.startId + entry.span) {
			lo = mid + 1;
		} else {
			index = mid;
			skip = id - entry.startId;
			return true;
		}
	}
	return false;
}

bool MessageDatabase::hasMessage(uint16 id) const {
	uint index, skip;
	return findEntry(id, index, skip);
}

uint16 MessageDatabase::getFirstId() const {
	return _entries.empty() ? 0 : _entries.front().startId;
}

uint16 MessageDatabase::getLastId() const {
	if (_entries.empty())
		return 0;
	const Entry &entry = _entries.back();
	return entry.startId + entry.span;
}

Common::String MessageDatabase::getMessage(uint16 id) const {
	uint index, skip;
	if (!findEntry(id, index, skip)) {
		warning("Wizardry7: message %u not in index", id);
		return Common::String();
	}

	const Entry &entry = _entries[index];
	uint32 offset = entry.page * 1024 + entry.offset;
	for (uint i = 0; i < skip; ++i) {
		if (offset >= _pool.size())
			return Common::String();
		offset += _pool[offset] + 1;
	}
	return decodeString(offset);
}

Common::String MessageDatabase::decodeString(uint32 offset) const {
	// The caller has already advanced past the preceding records of the group.
	if (offset + 1 >= _pool.size())
		return Common::String();

	const uint32 recordSize = _pool[offset];
	if (offset + 1 + recordSize > _pool.size())
		return Common::String();

	if (_huffmanTable.empty()) {
		// Gold: [len][len bytes]
		return Common::String((const char *)_pool.data() + offset + 1, recordSize);
	}

	// DOS: [total-1][decodedLen][Huffman bitstream]
	const uint32 decodedSize = _pool[offset + 1];
	if (decodedSize == 0 || decodedSize >= 256)
		return Common::String();

	Common::Array<char> out;
	out.reserve(decodedSize);
	const uint32 end = offset + 1 + recordSize;
	uint32 pos = offset + 2;
	uint node = 0;
	while (out.size() < decodedSize && pos < end) {
		const byte bits = _pool[pos++];
		for (int bit = 7; bit >= 0; --bit) {
			if (out.size() >= decodedSize)
				break;
			const int16 value = _huffmanTable[node * 2 + ((bits >> bit) & 1)];
			if (value < 0) {
				node = -value;
				if (node >= 512) {
					warning("Wizardry7: corrupt Huffman tree while decoding offset %u", offset);
					return Common::String();
				}
			} else {
				out.push_back((char)value);
				node = 0;
			}
		}
	}

	if (out.size() != decodedSize) {
		warning("Wizardry7: truncated message at offset %u", offset);
		return Common::String();
	}

	return Common::String(out.data(), out.size());
}

} // End of namespace Wizardry7
