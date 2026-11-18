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
#include "common/fs.h"
#include "common/textconsole.h"

#include "wizardry7/save.h"

namespace Wizardry7 {

static const uint32 kDosStateSize = 10720;
static const uint32 kDosRandomSize = 100;
static const uint32 kCharacterSize = 584;
static const uint32 kRosterHeaderSize = 24;

static void parseCharacter(const byte *record, SaveCharacter &character) {
	char name[9];
	memcpy(name, record, 8);
	name[8] = 0;
	character.name = name;
	character.age = READ_LE_UINT32(record + 0x08);
	character.xp = READ_LE_UINT32(record + 0x0C);
	character.hp = READ_LE_UINT16(record + 0x18);
	character.hpMax = READ_LE_UINT16(record + 0x1A);
	character.stamina = READ_LE_UINT16(record + 0x1C);
	character.staminaMax = READ_LE_UINT16(record + 0x1E);
	character.weight = READ_LE_UINT16(record + 0x20);
	character.gold = READ_LE_UINT16(record + 0x22);
	character.ac = READ_LE_UINT16(record + 0x1B6);
	character.knack = record[0x236];
	character.race = record[0x237];
	character.gender = record[0x238]; // 0 = male, 1 = female
	character.klass = record[0x239];
	character.alignment = record[0x23A];
}

static bool readWholeFile(const Common::Path &path, Common::Array<byte> &data) {
	Common::File file;
	if (file.open(path)) {
		data.resize(file.size());
		return file.read(data.data(), data.size()) == data.size();
	}

	Common::FSNode node(path);
	if (!node.exists() || node.isDirectory())
		return false;
	Common::SeekableReadStream *stream = node.createReadStream();
	if (!stream)
		return false;
	data.resize(stream->size());
	const bool ok = stream->read(data.data(), data.size()) == data.size();
	delete stream;
	return ok;
}

CharacterRoster::CharacterRoster() {
}

bool CharacterRoster::load(const Common::Path &path) {
	Common::Array<byte> data;
	if (!readWholeFile(path, data))
		return false;
	return loadFromData(data.data(), data.size());
}

bool CharacterRoster::loadFromData(const byte *source, uint32 size) {
	_characters.clear();
	_used.clear();
	if (size < kRosterHeaderSize)
		return false;
	Common::Array<byte> data;
	data.resize(size);
	memcpy(data.data(), source, size);

	if (data.size() < kRosterHeaderSize)
		return false;
	const uint16 stride = READ_LE_UINT16(data.data());
	const uint16 slots = READ_LE_UINT16(data.data() + 2);
	const uint16 headerSize = READ_LE_UINT16(data.data() + 4);
	if (stride != kCharacterSize || slots == 0 || slots > 16 ||
		headerSize < kRosterHeaderSize ||
		data.size() < headerSize + (uint32)slots * stride)
		return false;

	_characters.resize(slots);
	_used.resize(slots);
	for (uint i = 0; i < slots; ++i) {
		_used[i] = data[8 + i] != 0;
		parseCharacter(data.data() + headerSize + i * stride, _characters[i]);
	}
	return true;
}

SaveGame::SaveGame() : _kind(kSaveUnknown), _size(0) {
}

bool SaveGame::readFile(const Common::Path &path, Common::Array<byte> &data) const {
	return readWholeFile(path, data);
}

bool SaveGame::load(const Common::Path &path) {
	Common::Array<byte> data;
	if (!readFile(path, data)) {
		warning("Wizardry7: cannot read save '%s'", path.toString().c_str());
		return false;
	}
	return loadFromData(data.data(), data.size());
}

bool SaveGame::loadFromData(const byte *data, uint32 size) {
	_kind = kSaveUnknown;
	_size = size;
	_characters.clear();

	if (size < 4)
	if (_size < 4)
		return false;

	const uint16 tag = READ_LE_UINT16(data);
	if (tag == 0x0A && _size >= kDosStateSize && (_size - kDosStateSize - kDosRandomSize) % kCharacterSize == 0)
		return parseDos(data, _size);

	// Gold .SGM: 414-byte header + compressed 16*(1346+1740) payload + trailer.
	if (_size > 414) {
		_kind = kSaveGold;
		return true;
	}
	return false;
}

bool SaveGame::parseDos(const byte *data, uint32 size) {
	const uint32 count = (size - kDosStateSize - kDosRandomSize) / kCharacterSize;
	if (count > 16) {
		warning("Wizardry7: DOS save has %u character records", count);
		return false;
	}

	_kind = kSaveDos;
	_characters.resize(count);
	for (uint i = 0; i < count; ++i) {
		const byte *record = data + kDosStateSize + kDosRandomSize + i * kCharacterSize;
		parseCharacter(record, _characters[i]);
	}
	return true;
}

const char *SaveGame::getKindName(SaveKind kind) {
	switch (kind) {
	case kSaveDos:
		return "DOS SAVEGAME";
	case kSaveGold:
		return "Gold .SGM (compressed, not decoded)";
	default:
		return "unknown";
	}
}

} // End of namespace Wizardry7
