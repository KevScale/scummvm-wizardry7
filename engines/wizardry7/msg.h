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

#ifndef WIZARDRY7_MSG_H
#define WIZARDRY7_MSG_H

#include "common/array.h"
#include "common/path.h"
#include "common/str.h"

namespace Wizardry7 {

/**
 * Message string database.
 *
 * Both shipped data builds store an index of {startId, page, offset, span}
 * entries and a pool of length-prefixed strings. Gold's pool is plain text;
 * the DOS pool is Huffman-compressed with the tree shipped in MISC.HDR.
 */
class MessageDatabase {
public:
	MessageDatabase();

	/**
	 * Load an index and its string pool.
	 *
	 * @param headerFile  MSG.HDR
	 * @param poolFile    MSG.GLD (plain) or MSG.DBS (Huffman)
	 * @param huffmanFile MISC.HDR for the DOS pool, empty for Gold
	 */
	bool load(const Common::Path &headerFile, const Common::Path &poolFile, const Common::Path &huffmanFile = Common::Path());

	bool isLoaded() const { return !_entries.empty(); }
	bool isHuffmanCompressed() const { return !_huffmanTable.empty(); }

	uint getEntryCount() const { return _entries.size(); }
	uint16 getFirstId() const;
	uint16 getLastId() const;

	bool hasMessage(uint16 id) const;
	Common::String getMessage(uint16 id) const;

private:
	struct Entry {
		uint16 startId;
		uint16 offset;
		uint8 span;
		uint32 page;
	};

	bool findEntry(uint16 id, uint &index, uint &skip) const;
	Common::String decodeString(uint32 offset) const;

	bool loadPool(const Common::Path &path);
	bool loadHuffmanTable(const Common::Path &path);

	Common::Array<Entry> _entries;
	Common::Array<byte> _pool;
	Common::Array<int16> _huffmanTable;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_MSG_H
