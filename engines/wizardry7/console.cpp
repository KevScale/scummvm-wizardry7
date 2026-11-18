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

#include <stdarg.h>

#include "common/debug.h"
#include "common/file.h"
#include "common/str.h"
#include "common/str-array.h"
#include "common/tokenizer.h"
#include "common/util.h"

#include "wizardry7/console.h"
#include "wizardry7/items.h"
#include "wizardry7/maze.h"
#include "wizardry7/monster.h"
#include "wizardry7/attack.h"
#include "wizardry7/xp.h"
#include "graphics/screen.h"

#include "wizardry7/font.h"
#include "wizardry7/gfx.h"
#include "wizardry7/save.h"
#include "wizardry7/msg.h"
#include "wizardry7/party.h"
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

Wizardry7Console::Wizardry7Console(Wizardry7Engine *vm) : GUI::Debugger(), _vm(vm), _scriptMode(false) {
	registerCmd("msg", WRAP_METHOD(Wizardry7Console, cmdMessage));
	registerCmd("msginfo", WRAP_METHOD(Wizardry7Console, cmdMessageInfo));
	registerCmd("records", WRAP_METHOD(Wizardry7Console, cmdRecords));
	registerCmd("record", WRAP_METHOD(Wizardry7Console, cmdRecord));
	registerCmd("rng", WRAP_METHOD(Wizardry7Console, cmdRng));
	registerCmd("rngseed", WRAP_METHOD(Wizardry7Console, cmdSeed));
	registerCmd("status", WRAP_METHOD(Wizardry7Console, cmdStatus));
	registerCmd("mainmenu", WRAP_METHOD(Wizardry7Console, cmdMainMenu));
	registerCmd("charactermenu", WRAP_METHOD(Wizardry7Console, cmdCharacterMenu));
	registerCmd("addcharacter", WRAP_METHOD(Wizardry7Console, cmdAddCharacter));
	registerCmd("chargen", WRAP_METHOD(Wizardry7Console, cmdCharGen));
	registerCmd("chargenui", WRAP_METHOD(Wizardry7Console, cmdCharGenUI));
	registerCmd("create", WRAP_METHOD(Wizardry7Console, cmdCreate));
	registerCmd("party", WRAP_METHOD(Wizardry7Console, cmdParty));
	registerCmd("screen", WRAP_METHOD(Wizardry7Console, cmdScreen));
	registerCmd("screenshot", WRAP_METHOD(Wizardry7Console, cmdScreenshot));
	registerCmd("bundle", WRAP_METHOD(Wizardry7Console, cmdBundle));
	registerCmd("fontinfo", WRAP_METHOD(Wizardry7Console, cmdFontInfo));
	registerCmd("glyphs", WRAP_METHOD(Wizardry7Console, cmdGlyphs));
	registerCmd("glyph", WRAP_METHOD(Wizardry7Console, cmdGlyph));
	registerCmd("saveinfo", WRAP_METHOD(Wizardry7Console, cmdSaveInfo));
	registerCmd("text", WRAP_METHOD(Wizardry7Console, cmdText));
	registerCmd("roster", WRAP_METHOD(Wizardry7Console, cmdRoster));
	registerCmd("item", WRAP_METHOD(Wizardry7Console, cmdItem));
	registerCmd("sheet", WRAP_METHOD(Wizardry7Console, cmdSheet));
	registerCmd("review", WRAP_METHOD(Wizardry7Console, cmdReview));
	registerCmd("saveparty", WRAP_METHOD(Wizardry7Console, cmdSaveParty));
	registerCmd("xp", WRAP_METHOD(Wizardry7Console, cmdXp));
	registerCmd("maze", WRAP_METHOD(Wizardry7Console, cmdMaze));
	registerCmd("monster", WRAP_METHOD(Wizardry7Console, cmdMonster));
	registerCmd("anim", WRAP_METHOD(Wizardry7Console, cmdAnim));
	registerCmd("encounter", WRAP_METHOD(Wizardry7Console, cmdEncounter));
	registerCmd("placement", WRAP_METHOD(Wizardry7Console, cmdPlacement));
	registerCmd("placeview", WRAP_METHOD(Wizardry7Console, cmdPlaceView));
	registerCmd("viewfixed", WRAP_METHOD(Wizardry7Console, cmdViewFixed));
	registerCmd("viewpaint", WRAP_METHOD(Wizardry7Console, cmdViewPaint));
	registerCmd("viewkind", WRAP_METHOD(Wizardry7Console, cmdViewKind));
	registerCmd("viewtrace", WRAP_METHOD(Wizardry7Console, cmdViewTrace));
	registerCmd("script", WRAP_METHOD(Wizardry7Console, cmdScript));
}

int Wizardry7Console::print(const char *format, ...) {
	va_list args;
	va_start(args, format);
	const Common::String text = Common::String::vformat(format, args);
	va_end(args);

	if (_scriptMode) {
		debugN("%s", text.c_str());
		return text.size();
	}
	return print("%s", text.c_str());
}

bool Wizardry7Console::runScript(const Common::Path &path) {
	Common::FSNode node(path);
	if (!node.exists() || node.isDirectory()) {
		print("Cannot open script '%s'\n", path.toString().c_str());
		return false;
	}
	Common::SeekableReadStream *stream = node.createReadStream();
	if (!stream) {
		print("Cannot read script '%s'\n", path.toString().c_str());
		return false;
	}

	bool ok = true;
	while (true) {
		const Common::String line = stream->readLine();
		if (line.empty() && stream->eos())
			break;
		if (line.empty() || line.hasPrefix("#"))
			continue;
		print("> %s\n", line.c_str());

		Common::StringTokenizer tokenizer(line, " \t");
		Common::StringArray words;
		while (!tokenizer.empty())
			words.push_back(tokenizer.nextToken());

		if (words.empty())
			continue;
		const char *argv[16];
		const uint count = MIN(words.size(), (uint)ARRAYSIZE(argv));
		for (uint i = 0; i < count; ++i)
			argv[i] = words[i].c_str();

		const Common::String command = words[0];
		if (command == "msg")
			ok &= cmdMessage(count, argv);
		else if (command == "msginfo")
			ok &= cmdMessageInfo(count, argv);
		else if (command == "records")
			ok &= cmdRecords(count, argv);
		else if (command == "record")
			ok &= cmdRecord(count, argv);
		else if (command == "rng")
			ok &= cmdRng(count, argv);
		else if (command == "rngseed")
			ok &= cmdSeed(count, argv);
		else if (command == "status")
			ok &= cmdStatus(count, argv);
		else if (command == "mainmenu")
			ok &= cmdMainMenu(count, argv);
		else if (command == "charactermenu")
			ok &= cmdCharacterMenu(count, argv);
		else if (command == "addcharacter")
			ok &= cmdAddCharacter(count, argv);
		else if (command == "chargen")
			ok &= cmdCharGen(count, argv);
		else if (command == "chargenui")
			ok &= cmdCharGenUI(count, argv);
		else if (command == "create")
			ok &= cmdCreate(count, argv);
		else if (command == "party")
			ok &= cmdParty(count, argv);
		else if (command == "screen")
			ok &= cmdScreen(count, argv);
		else if (command == "screenshot")
			ok &= cmdScreenshot(count, argv);
		else if (command == "bundle")
			ok &= cmdBundle(count, argv);
		else if (command == "fontinfo")
			ok &= cmdFontInfo(count, argv);
		else if (command == "glyphs")
			ok &= cmdGlyphs(count, argv);
		else if (command == "glyph")
			ok &= cmdGlyph(count, argv);
		else if (command == "saveinfo")
			ok &= cmdSaveInfo(count, argv);
		else if (command == "text")
			ok &= cmdText(count, argv);
		else if (command == "roster")
			ok &= cmdRoster(count, argv);
		else if (command == "item")
			ok &= cmdItem(count, argv);
		else if (command == "sheet")
			ok &= cmdSheet(count, argv);
		else if (command == "review")
			ok &= cmdReview(count, argv);
		else if (command == "saveparty")
			ok &= cmdSaveParty(count, argv);
		else if (command == "xp")
			ok &= cmdXp(count, argv);
		else if (command == "maze")
			ok &= cmdMaze(count, argv);
		else if (command == "monster")
			ok &= cmdMonster(count, argv);
		else if (command == "anim")
			ok &= cmdAnim(count, argv);
		else if (command == "encounter")
			ok &= cmdEncounter(count, argv);
		else if (command == "placement")
			ok &= cmdPlacement(count, argv);
		else if (command == "placeview")
			ok &= cmdPlaceView(count, argv);
		else if (command == "viewfixed")
			ok &= cmdViewFixed(count, argv);
		else if (command == "viewpaint")
			ok &= cmdViewPaint(count, argv);
		else if (command == "viewkind")
			ok &= cmdViewKind(count, argv);
		else if (command == "viewtrace")
			ok &= cmdViewTrace(count, argv);
		else if (command == "script")
			ok &= cmdScript(count, argv);
		else {
			print("Unknown script command '%s'\n", command.c_str());
			ok = false;
		}
	}
	delete stream;
	return ok;
}

Wizardry7Console::~Wizardry7Console() {
}

bool Wizardry7Console::cmdMessage(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: msg <id>\n");
		return true;
	}

	int id;
	if (!parseInteger(argv[1], id) || id < 0 || id > 0xFFFF) {
		print("Invalid message id '%s'\n", argv[1]);
		return true;
	}

	MessageDatabase *messages = _vm->getMessages();
	if (!messages->isLoaded()) {
		print("Message database is not loaded\n");
		return true;
	}
	if (!messages->hasMessage(id)) {
		print("Message %d is not in the index\n", id);
		return true;
	}

	print("msg %d: %s\n", id, escapeMessage(messages->getMessage(id)).c_str());
	return true;
}

bool Wizardry7Console::cmdMessageInfo(int argc, const char **argv) {
	MessageDatabase *messages = _vm->getMessages();
	if (!messages->isLoaded()) {
		print("Message database is not loaded\n");
		return true;
	}

	print("Entries:   %u\n", messages->getEntryCount());
	print("Id range:  %u..%u\n", (uint)messages->getFirstId(), (uint)messages->getLastId());
	print("Encoding:  %s\n", messages->isHuffmanCompressed() ? "Huffman (DOS)" : "plain (Gold)");
	return true;
}

bool Wizardry7Console::cmdRecords(int argc, const char **argv) {
	ScenarioDatabase *scenario = _vm->getScenario();
	if (!scenario->isLoaded()) {
		print("Scenario database is not loaded\n");
		return true;
	}

	print("type  name             size  count     base\n");
	for (int type = 0; type < kRecordTypeCount; ++type) {
		print("%4d  %-15s %5u %6u  0x%06x\n", type,
			ScenarioDatabase::getRecordTypeName(type),
			(uint)scenario->getRecordSize(type),
			scenario->getRecordCount(type),
			scenario->getBaseOffset(type));
	}
	return true;
}

bool Wizardry7Console::cmdRecord(int argc, const char **argv) {
	if (argc < 3 || argc > 5) {
		print("Usage: record <type> <index> [offset] [length]\n");
		return true;
	}

	int type, index;
	if (!parseInteger(argv[1], type) || !parseInteger(argv[2], index)) {
		print("Invalid record type or index\n");
		return true;
	}

	ScenarioDatabase *scenario = _vm->getScenario();
	if (!scenario->isLoaded()) {
		print("Scenario database is not loaded\n");
		return true;
	}

	int offset = 0;
	int length = scenario->getRecordSize(type);
	if (argc >= 4 && (!parseInteger(argv[3], offset) || offset < 0)) {
		print("Invalid offset '%s'\n", argv[3]);
		return true;
	}
	if (argc == 5 && (!parseInteger(argv[4], length) || length < 1)) {
		print("Invalid length '%s'\n", argv[4]);
		return true;
	}

	Common::Array<byte> record;
	if (!scenario->readRecord(type, index, record)) {
		print("No record %d of type %d\n", index, type);
		return true;
	}

	if ((uint)offset >= record.size())
		offset = 0;
	const uint end = MIN(record.size(), (uint)(offset + length));

	print("type %d (%s), record %d, remapped %d, size %u, bytes 0x%x..0x%x\n",
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
		print("%s\n", line.c_str());
	}
	return true;
}

bool Wizardry7Console::cmdRng(int argc, const char **argv) {
	int count = 10;
	int max = 0;
	if (argc >= 2 && !parseInteger(argv[1], count)) {
		print("Invalid count '%s'\n", argv[1]);
		return true;
	}
	if (argc >= 3 && !parseInteger(argv[2], max)) {
		print("Invalid maximum '%s'\n", argv[2]);
		return true;
	}
	if (count < 0 || count > 10000 || max < 0) {
		print("Count and maximum must be non-negative\n");
		return true;
	}

	WizardryRNG *rng = _vm->getRNG();
	print("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
	for (int i = 0; i < count; ++i) {
		if (max > 0)
			print("%u%s", rng->getRandom(max), (i + 1 == count) ? "\n" : " ");
		else
			print("%u%s", rng->rand16(), (i + 1 == count) ? "\n" : " ");
	}
	return true;
}

bool Wizardry7Console::cmdSeed(int argc, const char **argv) {
	WizardryRNG *rng = _vm->getRNG();
	if (argc == 1) {
		print("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
		return true;
	}
	if (argc != 4) {
		print("Usage: rngseed [s1 s2 s3]\n");
		return true;
	}

	int seeds[3];
	for (int i = 0; i < 3; ++i) {
		if (!parseInteger(argv[i + 1], seeds[i]) || seeds[i] < 0 || seeds[i] > 0xFFFF) {
			print("Invalid seed '%s'\n", argv[i + 1]);
			return true;
		}
	}
	rng->reset(seeds[0], seeds[1], seeds[2]);
	print("seed: %u %u %u\n", (uint)rng->getSeed(0), (uint)rng->getSeed(1), (uint)rng->getSeed(2));
	return true;
}

bool Wizardry7Console::cmdStatus(int argc, const char **argv) {
	_vm->showStatusScreen();
	return true;
}

bool Wizardry7Console::cmdMainMenu(int argc, const char **argv) {
	const Common::String font = argc >= 2 ? argv[1] : "VBFONT0.VGA";
	int selected = -1;
	if (argc >= 3 && !parseInteger(argv[2], selected))
		return true;
	if (!_vm->showMainMenu(font, selected)) {
		print("Cannot draw the main menu with '%s'\n", font.c_str());
		return true;
	}
	print("Showing main menu with '%s' (selected %d)\n", font.c_str(), selected);
	return true;
}

bool Wizardry7Console::cmdCharacterMenu(int argc, const char **argv) {
	const Common::String font = argc >= 2 ? argv[1] : "VBFONT0.VGA";
	if (!_vm->showCharacterMenu(font)) {
		print("Cannot draw the character menu with '%s'\n", font.c_str());
		return true;
	}
	print("Showing character menu with '%s'\n", font.c_str());
	return true;
}

bool Wizardry7Console::cmdAddCharacter(int argc, const char **argv) {
	const Common::Path roster = argc >= 2 ? Common::Path(argv[1]) : Common::Path();
	if (!_vm->showAddCharacter("VBFONT0.VGA", roster)) {
		print("Cannot draw the add-character roster\n");
		return true;
	}
	print("Showing add-character roster%s%s\n", argc >= 2 ? " from " : "",
		argc >= 2 ? argv[1] : "");
	return true;
}

bool Wizardry7Console::cmdCharGen(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: chargen <sex|race|profession|name|bonus>\n");
		return true;
	}
	int selection = 0;
	if (argc >= 3 && !parseInteger(argv[2], selection)) {
		print("Bad selection '%s'\n", argv[2]);
		return true;
	}
	if (!_vm->showCharacterCreation(argv[1], "VBFONT0.VGA", selection)) {
		print("Cannot show character-creation step '%s'\n", argv[1]);
		return true;
	}
	print("Showing character-creation step '%s'\n", argv[1]);
	return true;
}

bool Wizardry7Console::cmdCharGenUI(int argc, const char **argv) {
	_vm->runCharacterCreationUI("VBFONT0.VGA", argc >= 2 ? Common::Path(argv[1]) : Common::Path());
	return true;
}

bool Wizardry7Console::cmdViewTrace(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: viewtrace <trace.txt>\n");
		return true;
	}
	if (!_vm->showViewTrace(Common::Path(argv[1]))) {
		print("Cannot replay trace '%s'\n", argv[1]);
		return true;
	}
	print("Replayed view trace '%s'\n", argv[1]);
	return true;
}

bool Wizardry7Console::cmdViewKind(int argc, const char **argv) {
	if (argc != 5) {
		print("Usage: viewkind <maze> <cell> <column> <row>\n");
		return true;
	}
	int mazeId = 0, cell = 0, column = 0, row = 0;
	if (!parseInteger(argv[1], mazeId) || !parseInteger(argv[2], cell) ||
		!parseInteger(argv[3], column) || !parseInteger(argv[4], row) ||
		mazeId < 0 || mazeId >= 32 || cell < 0 || cell >= 16 ||
		column < 0 || column >= 8 || row < 0 || row >= 8) {
		print("Bad maze/cell/column/row\n");
		return true;
	}
	Wizardry7::MazeRecord maze;
	if (!maze.load(*_vm->getScenario(), (uint)mazeId)) {
		print("Cannot load maze %d\n", mazeId);
		return true;
	}
	const uint face = Wizardry7::MazeRecord::cellFace((uint)cell, (uint)column, (uint)row);
	print("maze %d cell %d col %d row %d: face %u A %u B %u C %u -> kind %u\n",
		mazeId, cell, column, row, face, maze.getHazard(0, face), maze.getHazard(1, face),
		maze.getHazard(2, face), maze.getViewKind((uint)cell, (uint)column, (uint)row));
	return true;
}

bool Wizardry7Console::cmdViewPaint(int argc, const char **argv) {
	if (argc < 2 || argc > 3 || strlen(argv[1]) != 25) {
		print("Usage: viewpaint <25 kind digits, column-major> [mirror]\n");
		return true;
	}
	byte kinds[5][5];
	for (uint i = 0; i < 25; ++i) {
		if (argv[1][i] < '0' || argv[1][i] > '3') {
			print("Kinds must be digits 0..3\n");
			return true;
		}
		kinds[i / 5][i % 5] = argv[1][i] - '0';
	}
	int mirror = 0;
	if (argc == 3 && !parseInteger(argv[2], mirror)) {
		print("Bad mirror\n");
		return true;
	}
	if (!_vm->showViewPainter(kinds, mirror != 0)) {
		print("Cannot render painter pieces\n");
		return true;
	}
	print("Rendered painter pieces (mirror %d)\n", mirror != 0);
	return true;
}

bool Wizardry7Console::cmdViewFixed(int argc, const char **argv) {
	if (argc < 2 || argc > 3) {
		print("Usage: viewfixed <mode> [mirror]\n");
		return true;
	}
	int mode = 0, mirror = 0;
	if (!parseInteger(argv[1], mode) || (argc == 3 && !parseInteger(argv[2], mirror))) {
		print("Bad mode/mirror\n");
		return true;
	}
	if (!_vm->showViewFixed((uint)mode, mirror != 0)) {
		print("Cannot render fixed view mode %d\n", mode);
		return true;
	}
	print("Rendered fixed pieces for view mode %d (mirror %d)\n", mode, mirror != 0);
	return true;
}

bool Wizardry7Console::cmdPlaceView(int argc, const char **argv) {
	if (argc < 3 || argc > 5) {
		print("Usage: placeview <file.VGA> <record> [dx dy]\n");
		return true;
	}
	int record = 0, dx = 0, dy = 0;
	if (!parseInteger(argv[2], record) || record < 0 ||
		(argc >= 4 && !parseInteger(argv[3], dx)) ||
		(argc >= 5 && !parseInteger(argv[4], dy))) {
		print("Bad record/offset\n");
		return true;
	}
	Common::String file(argv[1]);
	if (!file.hasSuffixIgnoreCase(".VGA"))
		file += ".VGA";
	if (!_vm->showPlacement(Common::Path(file), (uint)record, dx, dy)) {
		print("Cannot render placement %d from '%s'\n", record, file.c_str());
		return true;
	}
	print("Rendered %s placement %d at offset %d,%d\n", file.c_str(), record, dx, dy);
	return true;
}

bool Wizardry7Console::cmdPlacement(int argc, const char **argv) {
	if (argc != 3) {
		print("Usage: placement <file.VGA> <record>\n");
		return true;
	}
	int record = 0;
	if (!parseInteger(argv[2], record) || record < 0) {
		print("Bad record '%s'\n", argv[2]);
		return true;
	}
	Common::String file(argv[1]);
	if (!file.hasSuffixIgnoreCase(".VGA"))
		file += ".VGA";
	VgaBundle bundle;
	if (!bundle.load(Common::Path(file))) {
		print("Cannot load '%s'\n", file.c_str());
		return true;
	}
	VgaPlacement p;
	if (!bundle.getPlacement((uint)record, p)) {
		print("No placement record %d (%u records)\n", record, bundle.getRecordCount());
		return true;
	}
	print("%s record %d: bitmap %u x %d y %u x_extra %u rows %u (screen_x %d)\n",
		file.c_str(), record, p.bitmap, p.x, p.y, p.xExtra, p.rows,
		(p.xExtra + p.x) * 8);
	return true;
}

bool Wizardry7Console::cmdEncounter(int argc, const char **argv) {
	if (argc != 3) {
		print("Usage: encounter <maze> <slot>\n");
		return true;
	}
	int mazeId = 0, slot = 0;
	if (!parseInteger(argv[1], mazeId) || !parseInteger(argv[2], slot) ||
		mazeId < 0 || mazeId >= 32 || slot < 0 || slot >= 16) {
		print("Bad maze/slot\n");
		return true;
	}
	Wizardry7::MazeRecord maze;
	if (!maze.load(*_vm->getScenario(), (uint)mazeId)) {
		print("Cannot load maze %d\n", mazeId);
		return true;
	}
	const int monsterId = maze.selectEncounterMonster((uint)slot, *_vm->getRNG());
	if (monsterId < 0) {
		print("maze %d slot %d: no encounter\n", mazeId, slot);
		return true;
	}
	Wizardry7::MonsterInfo info;
	const bool named = Wizardry7::loadMonster(*_vm->getScenario(), (uint16)monsterId, info);
	print("maze %d slot %d: monster %d%s%s\n", mazeId, slot, monsterId,
		named ? " " : "", named ? info.names[0].c_str() : "");
	return true;
}

bool Wizardry7Console::cmdAnim(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: anim <picture-id>\n");
		return true;
	}
	int id = 0;
	if (!parseInteger(argv[1], id) || id < 0 || id >= 96) {
		print("Bad picture id '%s'\n", argv[1]);
		return true;
	}
	Wizardry7::AttackAnimation anim;
	if (!anim.load(*_vm->getScenario(), (uint)id)) {
		print("No animation record %d\n", id);
		return true;
	}
	print("picture %d: attacks start %u count %u pair %04x/%04x overrides %02x/%02x\n",
		id, anim.getAttackStart(), anim.getAttackCount(), anim.getPairA(), anim.getPairB(),
		anim.getOverrideA(), anim.getOverrideB());
	for (uint i = 0; i < anim.getAttackCount(); ++i)
		print("  attack %u: base %u rand %u extra %u\n", i, anim.getBaseDamage(i),
			anim.getRandomDamage(i), anim.getExtraRandomDamage(i));
	return true;
}

bool Wizardry7Console::cmdMonster(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: monster <id>\n");
		return true;
	}
	int id = 0;
	if (!parseInteger(argv[1], id) || id < 0 || id > 0xFFFF) {
		print("Bad monster id '%s'\n", argv[1]);
		return true;
	}
	Wizardry7::MonsterInfo info;
	if (!Wizardry7::loadMonster(*_vm->getScenario(), (uint16)id, info)) {
		print("No monster %d\n", id);
		return true;
	}
	print("monster %d: %s / %s (%s / %s)\n", id, info.names[0].c_str(),
		info.names[1].c_str(), info.names[2].c_str(), info.names[3].c_str());
	print("xp %u spawn %u actors %u pic %u body %u flags %u\n", info.xp,
		info.spawnCount, info.actorCount, info.pictureId, info.bodyType, info.flags);
	print("level dice %08x hp %08x st %08x variants %u@%u%% %u@%u%%\n",
		info.levelDice, info.hpDice, info.stDice, info.variantId[0],
		info.variantChance[0], info.variantId[1], info.variantChance[1]);
	return true;
}

bool Wizardry7Console::cmdMaze(int argc, const char **argv) {
	if (argc != 2 && argc != 5) {
		print("Usage: maze <id> [x y facing]\n");
		return true;
	}
	int mazeId = 0;
	if (!parseInteger(argv[1], mazeId) || mazeId < 0 || mazeId >= 32) {
		print("Bad maze id '%s'\n", argv[1]);
		return true;
	}
	Wizardry7::MazeRecord maze;
	if (!maze.load(*_vm->getScenario(), (uint)mazeId)) {
		print("Cannot load maze %d\n", mazeId);
		return true;
	}
	if (argc == 2) {
		print("maze %d loaded; encounter slots:\n", mazeId);
		for (int i = 0; i < Wizardry7::MazeRecord::kEncounterSlots; ++i)
			print("  %2d chance %3u common %2u uncommon %2u\n", i,
				maze.getEncounterChance(i), maze.getEncounterGroup(i, false),
				maze.getEncounterGroup(i, true));
		return true;
	}
	int x = 0, y = 0, facing = 0;
	if (!parseInteger(argv[2], x) || !parseInteger(argv[3], y) ||
		!parseInteger(argv[4], facing) || x < 0 || y < 0 || facing < 0 ||
		x > 7 || y > 7 || facing > 7) {
		print("Bad face coordinates\n");
		return true;
	}
	const uint face = Wizardry7::MazeRecord::faceIndex(x, y, facing);
	print("maze %d face %u (x%d y%d f%d): tileA %u tileB %u special %u flags %u hazards %u%u%u\n",
		mazeId, face, x, y, facing, maze.getTileA(face), maze.getTileB(face),
		maze.getSpecial(face), maze.getFlags(face), maze.getHazard(0, face),
		maze.getHazard(1, face), maze.getHazard(2, face));
	return true;
}

bool Wizardry7Console::cmdXp(int argc, const char **argv) {
	if (argc != 3) {
		print("Usage: xp <class> <xp>\n");
		return true;
	}
	int classIndex = 0, xp = 0;
	if (!parseInteger(argv[1], classIndex) || classIndex < 0 || classIndex >= 14 ||
		!parseInteger(argv[2], xp) || xp < 0) {
		print("Bad class/xp\n");
		return true;
	}
	Wizardry7::ExperienceTable table;
	if (!table.load(*_vm->getScenario())) {
		print("Cannot load XP table\n");
		return true;
	}
	const uint8 level = table.getLevel((uint8)classIndex, (uint32)xp);
	print("class %d xp %d -> level %u rank %u; next %u\n", classIndex, xp,
		level, Wizardry7::ExperienceTable::getRank(level),
		table.getThreshold((uint8)classIndex, level + 1));
	return true;
}

bool Wizardry7Console::cmdSaveParty(int argc, const char **argv) {
	if (argc != 5) {
		print("Usage: saveparty <SAVEGAME> <ROSTER> <slot,slot,...> <OUT>\n");
		return true;
	}
	SaveGame save;
	if (!save.load(Common::Path(argv[1]))) {
		print("Cannot read save '%s'\n", argv[1]);
		return true;
	}
	CharacterRoster roster;
	if (!roster.load(Common::Path(argv[2]))) {
		print("Cannot read roster '%s'\n", argv[2]);
		return true;
	}
	Party party;
	Common::StringTokenizer tokenizer(argv[3], ",");
	while (!tokenizer.empty()) {
		const Common::String token = tokenizer.nextToken();
		int slot = 0;
		if (!parseInteger(token.c_str(), slot) || slot < 0 || slot >= (int)roster.getSlotCount() ||
			!roster.isSlotUsed(slot)) {
			print("Bad roster slot '%s'\n", token.c_str());
			return true;
		}
		if (!party.addMember((uint8)slot)) {
			print("Party is full or duplicate slot %d\n", slot);
			return true;
		}
	}
	if (party.size() == 0) {
		print("Empty party\n");
		return true;
	}
	if (!save.setPartyFromRoster(roster, party) || !save.save(Common::Path(argv[4]))) {
		print("Cannot write party save '%s'\n", argv[4]);
		return true;
	}
	print("Wrote %u party members to '%s'\n", party.size(), argv[4]);
	return true;
}

bool Wizardry7Console::cmdReview(int argc, const char **argv) {
	if (argc != 3) {
		print("Usage: review <PCFILE> <slot>\n");
		return true;
	}
	int slot = 0;
	if (!parseInteger(argv[2], slot) || slot < 0) {
		print("Bad slot '%s'\n", argv[2]);
		return true;
	}
	if (!_vm->showCharacterReview(Common::Path(argv[1]), (uint)slot)) {
		print("Cannot review roster slot %d\n", slot);
		return true;
	}
	print("Showing character review for slot %d\n", slot);
	return true;
}

bool Wizardry7Console::cmdSheet(int argc, const char **argv) {
	if (argc != 3) {
		print("Usage: sheet <PCFILE> <slot>\n");
		return true;
	}
	int slot = 0;
	if (!parseInteger(argv[2], slot) || slot < 0) {
		print("Bad slot '%s'\n", argv[2]);
		return true;
	}
	CharacterRoster roster;
	if (!roster.load(Common::Path(argv[1])) || (uint)slot >= roster.getSlotCount() ||
		!roster.isSlotUsed((uint)slot)) {
		print("No used roster slot %d\n", slot);
		return true;
	}
	const SaveCharacter &c = roster.getCharacter((uint)slot);
	static const char *const races[] = {
		"HUMAN", "HOBBIT", "FELPURR", "ELF", "FAERIE", "RAWULF",
		"DWARF", "LIZARDMAN", "MOOK", "GNOME", "DRACON"
	};
	const char *raceName = c.race < 11 ? races[c.race] : "?";
	print("%s  %c-%s %s  align %u\n", c.name.c_str(), c.gender ? 'F' : 'M',
		raceName, CharacterCreation::getClassName(c.klass), c.alignment);
	ExperienceTable xpTable;
	uint8 level = 1;
	if (xpTable.load(*_vm->getScenario()))
		level = xpTable.getLevel(c.klass, c.xp);
	const uint8 rank = ExperienceTable::getRank(level);
	Common::String rankName;
	const uint16 rankMsg = 800 + c.klass * 10 + rank;
	if (_vm->getMessages()->hasMessage(rankMsg))
		rankName = _vm->getMessages()->getMessage(rankMsg);
	print("level %u rank %u%s%s  age %u  hp %u/%u  st %u/%u  ac %u (slots %u,%u,%u,%u,%u)  gold %u  portrait %u\n",
		level, rank, rankName.empty() ? "" : " ", rankName.c_str(), c.age, c.hp, c.hpMax,
		c.stamina, c.staminaMax, c.ac, c.acSlots[0], c.acSlots[1], c.acSlots[2],
		c.acSlots[3], c.acSlots[4], c.gold, c.portrait);
	static const char *const names[7] = { "STR", "INT", "PIE", "VIT", "DEX", "SPD", "PER" };
	for (uint i = 0; i < 7; ++i)
		print("%s %u%s", names[i], c.stats[i], i == 6 ? "\n" : "  ");
	for (uint i = 0; i < 5; ++i) {
		if (!c.inventory[i])
			continue;
		ItemInfo info;
		if (loadItem(*_vm->getScenario(), c.inventory[i], info))
			print("  item %u: %s x%u\n", c.inventory[i], info.displayName.c_str(),
				c.inventoryCount[i] ? c.inventoryCount[i] : 1);
		else
			print("  item %u: (unknown)\n", c.inventory[i]);
	}
	return true;
}

bool Wizardry7Console::cmdItem(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: item <id>\n");
		return true;
	}
	int id = 0;
	if (!parseInteger(argv[1], id) || id < 0 || id > 0xFFFF) {
		print("Bad item id '%s'\n", argv[1]);
		return true;
	}
	Wizardry7::ItemInfo info;
	if (!Wizardry7::loadItem(*_vm->getScenario(), (uint16)id, info)) {
		print("No item %d\n", id);
		return true;
	}
	print("item %d: %s\n", id, info.displayName.c_str());
	print("price %u weight %u kind %u subtype %u rank %u ac %u flags %u\n",
		info.price, info.weight, info.kind, info.subtype, info.rank, info.acBonus, info.flags);
	print("class %04x race %04x gender %u spell %u/%u charges %u\n",
		info.classMask, info.raceMask, info.genderMask, info.getSpellId(),
		info.getSpellPower(), info.getDefaultCharges());
	return true;
}

bool Wizardry7Console::cmdCreate(int argc, const char **argv) {
	if (argc < 2) {
		print("Usage: create start [gold]|sex N|race N|class N|name NAME|bonus STAT|finish|status|record FILE|roster ROSTER OUT\n");
		return true;
	}
	const Common::String action = argv[1];
	if (action == "start") {
		const bool gold = argc >= 3 && Common::String(argv[2]).equalsIgnoreCase("gold");
		_vm->startCreation(gold);
		print("Creation started at %s step\n", gold ? "name" : "sex");
		return true;
	}
	if (!_vm->isCreationStarted()) {
		print("Creation not started; run 'create start'\n");
		return true;
	}
	if (action == "sex" || action == "race" || action == "class" || action == "bonus") {
		if (argc != 3) {
			print("Usage: create %s N\n", action.c_str());
			return true;
		}
		int value = 0;
		if (!parseInteger(argv[2], value)) {
			print("Bad number '%s'\n", argv[2]);
			return true;
		}
		bool ok = false;
		if (action == "sex")
			ok = _vm->creationChooseSex(value);
		else if (action == "race")
			ok = _vm->creationChooseRace(value);
		else if (action == "class")
			ok = _vm->creationChooseProfession(value);
		else {
			if (_vm->getCreation().getStep() == CharacterCreationFlow::kStepName &&
				!_vm->getCreation().finishName()) {
				print("Set a valid name before bonus points\n");
				return true;
			}
			ok = _vm->creationAddBonus(value);
		}
		print("%s %d: %s\n", action.c_str(), value, ok ? "ok" : "rejected");
		return true;
	}
	if (action == "name") {
		if (argc != 3) {
			print("Usage: create name NAME\n");
			return true;
		}
		const bool ok = _vm->creationSetName(argv[2]);
		if (ok)
			_vm->getCreation().finishName();
		print("name: %s\n", ok ? "ok" : "rejected");
		return true;
	}
	if (action == "record") {
		if (argc != 3) {
			print("Usage: create record FILE\n");
			return true;
		}
		if (!_vm->saveCreationRecord(Common::Path(argv[2]))) {
			print("No completed creation result (run finish first)\n");
			return true;
		}
		print("Wrote partial record to '%s'\n", argv[2]);
		return true;
	}
	if (action == "roster") {
		if (argc != 4) {
			print("Usage: create roster ROSTER OUT\n");
			return true;
		}
		if (!_vm->saveCreationToRoster(Common::Path(argv[2]), Common::Path(argv[3]))) {
			print("Could not write creation result into roster\n");
			return true;
		}
		print("Wrote character into roster '%s' -> '%s'\n", argv[2], argv[3]);
		return true;
	}
	if (action == "finish") {
		Wizardry7::CreatedCharacter result;
		if (!_vm->creationFinish(result)) {
			print("finish rejected (name/bonus step incomplete?)\n");
			return true;
		}
		print("CREATED %s race %u sex %u class %u\n", result.name.c_str(),
			result.race, result.sex, result.classIndex);
		print("stats");
		for (uint i = 0; i < Wizardry7::CharacterCreation::kStatCount; ++i)
			print(" %u", result.stats[i]);
		print("\nage %u hp %u st %u cc %u ac %u\n", result.rolls.ageDays,
			result.rolls.hpMax, result.rolls.staminaMax, result.rolls.carryingCapacity,
			result.rolls.ac);
		print("items");
		for (uint i = 0; i < result.itemCount; ++i)
			print(" %u:%u", result.items[i].itemId, result.items[i].count);
		print("\n");
		return true;
	}
	if (action == "status") {
		const CharacterCreationFlow &flow = _vm->getCreation();
		print("step %d bonus %d sex %u race %u class %d\n", (int)flow.getStep(),
			flow.getCharacter().getBonusPoints(), flow.getCharacter().getSex(),
			flow.getRace(), flow.getCharacter().getClass());
		return true;
	}
	print("Unknown create action '%s'\n", action.c_str());
	return true;
}

bool Wizardry7Console::cmdParty(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: party <PCFILE.GLD | PCFILE.DBS>\n");
		return true;
	}
	CharacterRoster roster;
	if (!roster.load(Common::Path(argv[1]))) {
		print("Cannot read roster '%s'\n", argv[1]);
		return true;
	}
	Party party;
	party.loadFromRoster(roster);
	print("Party (%u member(s))\n", party.size());
	for (uint i = 0; i < party.size(); ++i) {
		const uint8 slot = party.getMember(i);
		const SaveCharacter &c = roster.getCharacter(slot);
		print("  %u: slot %u %-8s race %u sex %u class %u\n", i + 1, slot,
			c.name.c_str(), c.race, c.gender, c.klass);
	}
	return true;
}

bool Wizardry7Console::cmdScreen(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: screen <name>   (e.g. TIT1SCRN, MAINSCRN)\n");
		return true;
	}
	if (!_vm->showScreen(argv[1])) {
		print("Cannot show screen '%s'\n", argv[1]);
		return true;
	}
	print("Showing %s\n", argv[1]);
	return true;
}

bool Wizardry7Console::cmdScreenshot(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: screenshot <file.bmp>\n");
		return true;
	}
	if (!_vm->saveScreenshot(Common::Path(argv[1]))) {
		print("Cannot write screenshot '%s'\n", argv[1]);
		return true;
	}
	print("Wrote screenshot '%s'\n", argv[1]);
	return true;
}

bool Wizardry7Console::cmdBundle(int argc, const char **argv) {
	if (argc < 2 || argc > 5) {
		print("Usage: bundle <file.VGA> [chunk [x y]]\n");
		return true;
	}

	VgaBundle bundle;
	if (!bundle.load(Common::Path(argv[1]))) {
		print("Cannot load VGA bundle '%s'\n", argv[1]);
		return true;
	}

	if (argc == 2) {
		print("'%s': %u chunks, %u records\n", argv[1], bundle.getChunkCount(), bundle.getRecordCount());
		for (uint i = 0; i < bundle.getChunkCount(); ++i) {
			uint width, height;
			uint32 offset, size;
			bundle.getChunkInfo(i, width, height, offset, size);
			print("  %3u: %3ux%-3u @ 0x%06x (%u bytes)\n", i, width, height, offset, size);
		}
		return true;
	}

	int index, x = 0, y = 0;
	if (!parseInteger(argv[2], index) || index < 0 || (uint)index >= bundle.getChunkCount()) {
		print("Invalid chunk '%s'\n", argv[2]);
		return true;
	}
	if (argc >= 4 && !parseInteger(argv[3], x)) {
		print("Invalid x coordinate '%s'\n", argv[3]);
		return true;
	}
	if (argc == 5 && !parseInteger(argv[4], y)) {
		print("Invalid y coordinate '%s'\n", argv[4]);
		return true;
	}

	Graphics::Screen *screen = _vm->getScreen();
	if (!screen) {
		print("No framebuffer\n");
		return true;
	}
	screen->fillRect(Common::Rect(screen->w, screen->h), 0);
	bundle.drawChunk(index, screen, x, y);
	if (_vm->getPalette()->isLoaded())
		_vm->getPalette()->apply(screen);
	screen->update();
	print("Showing chunk %d of '%s' at %d,%d\n", index, argv[1], x, y);
	return true;
}

bool Wizardry7Console::cmdFontInfo(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: fontinfo <font.VGA>\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	print("'%s': %ux%u, %u glyphs, %s, stride %u, base advance %d, bytes/glyph %u, flags 0x%02x\n",
		argv[1], font.getWidth(), font.getHeight(), font.getGlyphCount(),
		font.is8bpp() ? "8bpp" : "1bpp", font.getStride(), font.getBaseAdvance(),
		font.getBytesPerGlyph(), font.getFlags());
	return true;
}

bool Wizardry7Console::cmdGlyphs(int argc, const char **argv) {
	if (argc < 2 || argc > 4) {
		print("Usage: glyphs <font.VGA> [firstGlyph] [lastGlyph]\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	int first = 0, last = font.getGlyphCount() - 1;
	if (argc >= 3 && !parseInteger(argv[2], first))
		return true;
	if (argc == 4 && !parseInteger(argv[3], last))
		return true;
	if (first < 0 || last < 0 || first > last || (uint)last >= font.getGlyphCount()) {
		print("Glyph range must be within 0..%u\n", font.getGlyphCount() - 1);
		return true;
	}

	Graphics::Screen *screen = _vm->getScreen();
	if (!screen)
		return true;
	screen->fillRect(Common::Rect(screen->w, screen->h), 0);
	if (_vm->getPalette()->isLoaded())
		_vm->getPalette()->apply(screen);

	int x = 2, y = 2;
	for (int glyph = first; glyph <= last; ++glyph) {
		const uint advance = font.getGlyphAdvance(glyph);
		if (x + (int)advance >= screen->w) {
			x = 2;
			y += font.getHeight() + 2;
		}
		if (y + (int)font.getHeight() >= screen->h)
			break;
		font.drawGlyph(glyph, screen, x, y, 15, true, 0);
		x += advance + 1;
	}
	screen->update();
	print("Drew glyphs %d..%d from '%s'\n", first, last, argv[1]);
	return true;
}

bool Wizardry7Console::cmdGlyph(int argc, const char **argv) {
	if (argc < 3 || argc > 5) {
		print("Usage: glyph <font.VGA> <glyphIndex> [x] [y]\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	int glyph = 0, x = 0, y = 0;
	if (!parseInteger(argv[2], glyph) || glyph < 0 || (uint)glyph >= font.getGlyphCount())
		return true;
	if (argc >= 4 && !parseInteger(argv[3], x))
		return true;
	if (argc == 5 && !parseInteger(argv[4], y))
		return true;
	Graphics::Screen *screen = _vm->getScreen();
	if (!screen)
		return true;
	if (_vm->getPalette()->isLoaded())
		_vm->getPalette()->apply(screen);
	const uint advance = font.drawGlyph(glyph, screen, x, y, 15, true, 0);
	screen->update();
	print("Glyph %d advance %u\n", glyph, advance);
	return true;
}

bool Wizardry7Console::cmdSaveInfo(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: saveinfo <SAVEGAME.DBS | file.sgm>\n");
		return true;
	}
	SaveGame save;
	if (!save.load(Common::Path(argv[1]))) {
		print("Cannot read save '%s'\n", argv[1]);
		return true;
	}

	print("'%s': %s, %u bytes, %u character(s)\n", argv[1],
		SaveGame::getKindName(save.getKind()), save.getSize(), save.getCharacterCount());
	for (uint i = 0; i < save.getCharacterCount(); ++i) {
		const SaveCharacter &c = save.getCharacter(i);
		print("  %2u %-8s age %u xp %u hp %u/%u st %u/%u ac %u gold %u wt %u race %u class %u align %u knack %u\n",
			i, c.name.c_str(), c.age, c.xp, c.hp, c.hpMax, c.stamina, c.staminaMax,
			c.ac, c.gold, c.weight, c.race, c.klass, c.alignment, c.knack);
	}
	return true;
}

bool Wizardry7Console::cmdText(int argc, const char **argv) {
	if (argc < 3 || argc > 6) {
		print("Usage: text <font.VGA> <msgId> [x] [y] [color]\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	int id, x = 0, y = 0, color = 15;
	if (!parseInteger(argv[2], id) || id < 0 || id > 0xFFFF)
		return true;
	if (argc >= 4 && !parseInteger(argv[3], x))
		return true;
	if (argc >= 5 && !parseInteger(argv[4], y))
		return true;
	if (argc == 6 && !parseInteger(argv[5], color))
		return true;

	MessageDatabase *messages = _vm->getMessages();
	if (!messages->isLoaded() || !messages->hasMessage(id)) {
		print("No message %d\n", id);
		return true;
	}
	const Common::String text = messages->getMessage(id);
	Graphics::Screen *screen = _vm->getScreen();
	if (!screen)
		return true;
	if (_vm->getPalette()->isLoaded())
		_vm->getPalette()->apply(screen);

	uint drawn = 0;
	for (uint i = 0; i < text.size(); ++i) {
		const byte c = text[i];
		if (c == '\n' || c == '\r') {
			x = 0;
			y += font.getHeight() + 1;
			continue;
		}
		if (c < 32) // TODO: control codes need their own handling
			continue;
		const uint advance = font.getAdvance(c);
		if (x + (int)advance >= screen->w) {
			x = 0;
			y += font.getHeight() + 1;
		}
		if (y + (int)font.getHeight() >= screen->h)
			break;
		font.drawChar(c, screen, x, y, (byte)color, true, 0);
		x += advance;
		++drawn;
	}
	screen->update();
	print("Drew %u character(s) of message %d with '%s'\n", drawn, id, argv[1]);
	return true;
}

bool Wizardry7Console::cmdRoster(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: roster <PCFILE.GLD | PCFILE.DBS>\n");
		return true;
	}
	CharacterRoster roster;
	if (!roster.load(Common::Path(argv[1]))) {
		print("Cannot read roster '%s'\n", argv[1]);
		return true;
	}

	print("'%s': %u slots\n", argv[1], roster.getSlotCount());
	for (uint i = 0; i < roster.getSlotCount(); ++i) {
		if (!roster.isSlotUsed(i)) {
			print("  %2u  (empty)\n", i);
			continue;
		}
		const SaveCharacter &c = roster.getCharacter(i);
		print("  %2u  %-8s age %u hp %u/%u st %u/%u ac %u gold %u race %u gender %u class %u align %u\n",
			i, c.name.c_str(), c.age, c.hp, c.hpMax, c.stamina, c.staminaMax,
			c.ac, c.gold, c.race, c.gender, c.klass, c.alignment);
	}
	return true;
}

bool Wizardry7Console::cmdScript(int argc, const char **argv) {
	if (argc != 2) {
		print("Usage: script <file>\n");
		return true;
	}
	return runScript(Common::Path(argv[1]));
}

} // End of namespace Wizardry7
