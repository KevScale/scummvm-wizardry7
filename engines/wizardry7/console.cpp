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
#include "graphics/screen.h"

#include "wizardry7/font.h"
#include "wizardry7/gfx.h"
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

Wizardry7Console::Wizardry7Console(Wizardry7Engine *vm) : GUI::Debugger(), _vm(vm), _scriptMode(false) {
	registerCmd("msg", WRAP_METHOD(Wizardry7Console, cmdMessage));
	registerCmd("msginfo", WRAP_METHOD(Wizardry7Console, cmdMessageInfo));
	registerCmd("records", WRAP_METHOD(Wizardry7Console, cmdRecords));
	registerCmd("record", WRAP_METHOD(Wizardry7Console, cmdRecord));
	registerCmd("rng", WRAP_METHOD(Wizardry7Console, cmdRng));
	registerCmd("rngseed", WRAP_METHOD(Wizardry7Console, cmdSeed));
	registerCmd("status", WRAP_METHOD(Wizardry7Console, cmdStatus));
	registerCmd("screen", WRAP_METHOD(Wizardry7Console, cmdScreen));
	registerCmd("screenshot", WRAP_METHOD(Wizardry7Console, cmdScreenshot));
	registerCmd("bundle", WRAP_METHOD(Wizardry7Console, cmdBundle));
	registerCmd("fontinfo", WRAP_METHOD(Wizardry7Console, cmdFontInfo));
	registerCmd("glyphs", WRAP_METHOD(Wizardry7Console, cmdGlyphs));
	registerCmd("glyph", WRAP_METHOD(Wizardry7Console, cmdGlyph));
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
		print("Usage: glyphs <font.VGA> [first] [last]\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	int first = 32, last = 127;
	if (argc >= 3 && !parseInteger(argv[2], first))
		return true;
	if (argc == 4 && !parseInteger(argv[3], last))
		return true;
	if (first < 0 || last > 127 || first > last) {
		print("Range must be within 0..127\n");
		return true;
	}

	Graphics::Screen *screen = _vm->getScreen();
	if (!screen)
		return true;
	screen->fillRect(Common::Rect(screen->w, screen->h), 0);
	if (_vm->getPalette()->isLoaded())
		_vm->getPalette()->apply(screen);

	int x = 2, y = 2;
	for (int c = first; c <= last; ++c) {
		const uint advance = font.getAdvance(c);
		if (x + (int)advance >= screen->w) {
			x = 2;
			y += font.getHeight() + 2;
		}
		if (y + (int)font.getHeight() >= screen->h)
			break;
		font.drawChar(c, screen, x, y, 15, true, 0);
		x += advance + 1;
	}
	screen->update();
	print("Drew glyphs %d..%d from '%s'\n", first, last, argv[1]);
	return true;
}

bool Wizardry7Console::cmdGlyph(int argc, const char **argv) {
	if (argc < 3 || argc > 5) {
		print("Usage: glyph <font.VGA> <code> [x] [y]\n");
		return true;
	}
	GameFont font;
	if (!font.load(Common::Path(argv[1]))) {
		print("Cannot load font '%s'\n", argv[1]);
		return true;
	}
	int code = 0, x = 0, y = 0;
	if (!parseInteger(argv[2], code) || code < 0 || code > 127)
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
	const uint advance = font.drawChar(code, screen, x, y, 15, true, 0);
	screen->update();
	print("Glyph %d advance %u\n", code, advance);
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
