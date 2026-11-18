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

#include "common/events.h"
#include "common/file.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/util.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/screen.h"

#include "wizardry7/console.h"
#include "wizardry7/wizardry7.h"

namespace Wizardry7 {

static const int kScreenWidth = 640;
static const int kScreenHeight = 480;
static const uint32 kTextColor = 200;

static void drawLine(Graphics::Screen *screen, const Graphics::Font *font, int &y, const Common::String &text) {
	font->drawString(screen, text, 20, y, screen->w - 40, kTextColor);
	y += font->getFontHeight() + 4;
}

Wizardry7Engine::Wizardry7Engine(OSystem *syst, const Wizardry7GameDescription *gameDesc) :
	Engine(syst), _gameDescription(gameDesc), _screen(nullptr),
	_messagesLoaded(false), _scenarioLoaded(false) {

	setDebugger(new Wizardry7Console(this));
}

Wizardry7Engine::~Wizardry7Engine() {
	delete _screen;
}

bool Wizardry7Engine::loadGameData() {
	const bool gold = isGold();
	_messagesLoaded = _messages.load(Common::Path("MSG.HDR"),
		Common::Path(gold ? "MSG.GLD" : "MSG.DBS"),
		gold ? Common::Path() : Common::Path("MISC.HDR"));
	_scenarioLoaded = _scenario.load(Common::Path("MASTER.HDR"),
		Common::Path("DISK.HDR"),
		Common::Path(gold ? "SCENARIO.GLD" : "SCENARIO.DBS"));

	return _messagesLoaded && _scenarioLoaded;
}

Common::Error Wizardry7Engine::run() {
	loadGameData();

	initGraphics(kScreenWidth, kScreenHeight);
	_screen = new Graphics::Screen(kScreenWidth, kScreenHeight);

	byte palette[256 * 3];
	for (uint i = 0; i < 256; ++i) {
		palette[i * 3 + 0] = i;
		palette[i * 3 + 1] = i;
		palette[i * 3 + 2] = i;
	}
	_screen->setPalette(palette, 0, 256);

	drawStatusScreen();

	Common::Event event;
	while (!shouldQuit()) {
		while (_system->getEventManager()->pollEvent(event)) {
			if (event.type == Common::EVENT_QUIT)
				break;
		}
		_system->delayMillis(10);
	}

	return Common::kNoError;
}

void Wizardry7Engine::drawStatusScreen() {
	_screen->fillRect(Common::Rect(_screen->w, _screen->h), 0);
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kConsoleFont);
	int y = 16;

	drawLine(_screen, font, y, "Wizardry VII: Crusaders of the Dark Savant");
	drawLine(_screen, font, y, "ScummVM engine skeleton - data layer milestone");
	y += 8;
	drawLine(_screen, font, y, Common::String::format("Build: %s", isGold() ? "Wizardry Gold (Windows)" : "DOS original"));

	if (_messagesLoaded) {
		drawLine(_screen, font, y, Common::String::format("Messages: %u entries, ids %u..%u, %s",
			_messages.getEntryCount(), (uint)_messages.getFirstId(), (uint)_messages.getLastId(),
			_messages.isHuffmanCompressed() ? "Huffman (DOS)" : "plain (Gold)"));
	} else {
		drawLine(_screen, font, y, "Messages: NOT FOUND (see console for warnings)");
	}

	if (_scenarioLoaded) {
		drawLine(_screen, font, y, Common::String::format("Scenario: %u bytes, 10 record types", _scenario.getDataSize()));
		for (int type = 0; type < kRecordTypeCount; ++type) {
			drawLine(_screen, font, y, Common::String::format("  %d %-15s %5u x %-6u @ 0x%06x",
				type, ScenarioDatabase::getRecordTypeName(type),
				(uint)_scenario.getRecordSize(type), _scenario.getRecordCount(type),
				_scenario.getBaseOffset(type)));
		}
	} else {
		drawLine(_screen, font, y, "Scenario: NOT FOUND (see console for warnings)");
	}

	y += 8;
	WizardryRNG preview;
	Common::String rolls;
	for (int i = 0; i < 5; ++i)
		rolls += Common::String::format("%s%u", i ? " " : "", preview.getRandom(100));
	drawLine(_screen, font, y, Common::String::format("RNG from default seed: rand(100) = %s", rolls.c_str()));

	y += 8;
	drawLine(_screen, font, y, "Debug console (Ctrl+Shift+D): msg 100, records, record 1 1, rng 10, rngseed");
	drawLine(_screen, font, y, "No rendering or game logic yet; this build validates the decoded data layer.");

	_screen->update();
}

} // End of namespace Wizardry7
