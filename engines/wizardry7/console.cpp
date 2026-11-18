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

#include "common/str.h"
#include "common/util.h"

#include "wizardry7/console.h"
#include "wizardry7/msg.h"
#include "wizardry7/rng.h"
#include "wizardry7/scenario.h"
#include "wizardry7/wizardry7.h"

namespace Wizardry7 {

static bool parseInteger(const char *text, int &value) {
	char *end = nullptr;
	value = (int)strtol(text, &end, 0);
	return end != text && *end == 0;
}

static Common::String escapeMessage(const Common::String &text) {
	Common::String out;
	for (uint i = 0; i < text.size(); ++i) {
		const byte c = text[i];
		if (c == '\n')
			out += "\\n";
		else if (c == '\r')
			out += "\\r";
		else if (c == '\t')
			out += "\\t";
		else if (c < 32 || c >= 127)
			out += Common::String::format("\\x%02x", c);
		else
			out += (char)c;
	}
	return out;
}

Wizardry7Console::Wizardry7Console(Wizardry7Engine *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("msg", WRAP_METHOD(Wizardry7Console, cmdMessage));
	registerCmd("msginfo", WRAP_METHOD(Wizardry7Console, cmdMessageInfo));
	registerCmd("records", WRAP_METHOD(Wizardry7Console, cmdRecords));
	registerCmd("record", WRAP_METHOD(Wizardry7Console, cmdRecord));
	registerCmd("rng", WRAP_METHOD(Wizardry7Console, cmdRng));
	registerCmd("rngseed", WRAP_METHOD(Wizardry7Console, cmdSeed));
}

Wizardry7Console::~Wizardry7Console() {
}

bool Wizardry7Console::cmdMessage(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: msg <id>\n");
		return true;
	}

	int id;
	if (!parseInteger(argv[1], id) || id < 0 || id > 0xFFFF) {
		debugPrintf("Invalid message id '%s'\n", argv[1]);
		return true;
	}

	MessageDatabase *messages = _vm->getMessages();
	if (!messages->isLoaded()) {
		debugPrintf("Message database is not loaded\n");
		return true;
	}
	if (!messages->hasMessage(id)) {
		debugPrintf("Message %d is not in the index\n", id);
		return true;
	}

	debugPrintf("msg %d: %s\n", id, escapeMessage(messages->getMessage(id)).c_str());
	return true;
}

bool Wizardry7Console::cmdMessageInfo(int argc, const char **argv) {
	MessageDatabase *messages = _vm->getMessages();
	if (!messages->isLoaded()) {
		debugPrintf("Message database is not loaded\n");
		return true;
	}

	debugPrintf("Entries:   %u\n", messages->getEntryCount());
	debugPrintf("Id range:  %u..%u\n", (uint)messages->getFirstId(), (uint)messages->getLastId());
	debugPrintf("Encoding:  %s\n", messages->isHuffmanCompressed() ? "Huffman (DOS)" : "plain (Gold)");
	return true;
}

bool Wizardry7Console::cmdRecords(int argc, const char **argv) {
	ScenarioDatabase *scenario = _vm->getScenario();
	if (!scenario->isLoaded()) {
		debugPrintf("Scenario database is not loaded\n");
		return true;
	}

	debugPrintf("type  name             size  count     base\n");
	for (int type = 0; type < kRecordTypeCount; ++type) {
		debugPrintf("%4d  %-15s %5u %6u  0x%06x\n", type,
			ScenarioDatabase::getRecordTypeName(type),
			(uint)scenario->getRecordSize(type),
			scenario->getRecordCount(type),
			scenario->getBaseOffset(type));
	}
	return true;
}

bool Wizardry7Console::cmdRecord(int argc, const char **argv) {
	if (argc < 3 || argc > 5) {
		debugPrintf("Usage: record <type> <index> [offset] [length]\n");
		return true;
	}

	int type, index;
	if (!parseInteger(argv[1], type) || !parseInteger(argv[2], index)) {
		debugPrintf("Invalid record type or index\n");
		return true;
	}

	ScenarioDatabase *scenario = _vm->getScenario();
	if (!scenario->isLoaded()) {
		debugPrintf("Scenario database is not loaded\n");
		return true;
	}

	int offset = 0;
	int length = scenario->getRecordSize(type);
	if (argc >= 4 && (!parseInteger(argv[3], offset) || offset < 0)) {
		debugPrintf("Invalid offset '%s'\n", argv[3]);
		return true;
	}
	if (argc == 5 && (!parseInteger(argv[4], length) || length < 1)) {
		debugPrintf("Invalid length '%s'\n", argv[4]);
		return true;
	}

	Common::Array<byte> record;
	if (!scenario->readRecord(type, index, record)) {
		debugPrintf("No record %d of type %d\n", index, type);
		return true;
	}

	if ((uint)offset >= record.size())
		offset = 0;
	const uint end = MIN(record.size(), (uint)(offset + length));

	debugPrintf("type %d (%s), record %d, remapped %d, size %u, bytes 0x%x..0x%x\n",
		type, ScenarioDatabase::getRecordTypeName(type), index,
		scenario->getRemappedIndex(type, index), record.size(), offset, end);

	for (uint pos = offset; pos < end; pos += 16) {
		Common::String line = Common::String::format("%06x: ", pos);
		for (uint i = 0; i < 16; ++i) {
			if (pos + i < end)
				line += Common::String::format("%02x ", record[pos + i]);
			else
				line += "   ";
		}
		line += " |";
		for (uint i = 0; i < 16 && pos + i < end; ++i) {
			const byte c = record[pos + i];
			line += (c >= 32 && c < 127) ? (char)c : '.';
		}
		line += '|';
		debugPrintf("%s\n", line.c_str());
	}
	return true;
}

bool Wizardry7Console::cmdRng(int argc, const char **argv) {
	int count = 10;
	int max = 0;
	if (argc >= 2 && !parseInteger(argv[1], count)) {
		debugPrintf("Invalid count '%s'\n", argv[1]);
		return true;
	}
	if (argc >= 3 && !parseInteger(argv[2], max)) {
		debugPrintf("Invalid maximum '%s'\n", argv[2]);
		return true;
	}
	if (count < 0 || count > 10000 || max < 0) {
		debugPrintf("Count and maximum must be non-negative\n");
		return true;
	}

	WizardryRNG *rng = _vm->getRNG();
	debugPrintf("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
	for (int i = 0; i < count; ++i) {
		if (max > 0)
			debugPrintf("%u%s", rng->getRandom(max), (i + 1 == count) ? "\n" : " ");
		else
			debugPrintf("%u%s", rng->rand16(), (i + 1 == count) ? "\n" : " ");
	}
	return true;
}

bool Wizardry7Console::cmdSeed(int argc, const char **argv) {
	WizardryRNG *rng = _vm->getRNG();
	if (argc == 1) {
		debugPrintf("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
		return true;
	}
	if (argc != 4) {
		debugPrintf("Usage: rngseed [s1 s2 s3]\n");
		return true;
	}

	int seeds[3];
	for (int i = 0; i < 3; ++i) {
		if (!parseInteger(argv[i + 1], seeds[i]) || seeds[i] < 0 || seeds[i] > 0xFFFF) {
			debugPrintf("Invalid seed '%s'\n", argv[i + 1]);
			return true;
		}
	}
	rng->reset(seeds[0], seeds[1], seeds[2]);
	debugPrintf("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
	return true;
}

} // End of namespace Wizardry7
