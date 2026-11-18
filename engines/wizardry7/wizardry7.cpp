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

#include "common/config-manager.h"
#include "common/events.h"
#include "common/events.h"
#include "common/file.h"
#include "common/file.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/util.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/screen.h"
#include "image/bmp.h"

#include "wizardry7/console.h"
#include "wizardry7/font.h"
#include "wizardry7/save.h"
#include "wizardry7/wizardry7.h"

namespace Wizardry7 {

static const int kScreenWidth = 320;
static const int kScreenHeight = 200;
static const uint32 kTextColor = 200;

static void drawLine(Graphics::Screen *screen, const Graphics::Font *font, int &y, const Common::String &text) {
	font->drawString(screen, text, 20, y, screen->w - 40, kTextColor);
	y += font->getFontHeight() + 4;
}

Wizardry7Engine::Wizardry7Engine(OSystem *syst, const Wizardry7GameDescription *gameDesc) :
	Engine(syst), _gameDescription(gameDesc), _screen(nullptr),
	_creationStarted(false), _hasCreationResult(false), _messagesLoaded(false),
	_scenarioLoaded(false) {

	setDebugger(new Wizardry7Console(this));
}

Wizardry7Engine::~Wizardry7Engine() {
	delete _screen;
}

bool Wizardry7Engine::loadGameData() {
	const bool gold = isGold();
	_palette.load(Common::Path("PALETTE.DEF"));
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

	const Common::String scriptPath = ConfMan.get("wizardry7_script");
	if (!scriptPath.empty()) {
		Wizardry7Console *console = (Wizardry7Console *)getDebugger();
		console->setScriptMode(true);
		const bool ok = console->runScript(Common::Path(scriptPath));
		return ok ? Common::kNoError : Common::kUnknownError;
	}

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

bool Wizardry7Engine::showScreen(const Common::String &name) {
	if (!_screen)
		return false;

	Common::String fileName(name);
	if (!fileName.hasSuffixIgnoreCase(".VGA"))
		fileName += ".VGA";
	const Common::Path path(fileName);

	Common::Array<byte> pixels;
	uint width = 0, height = 0;
	if (!loadScreen(path, pixels, width, height))
		return false;
	if (width > (uint)_screen->w || height > (uint)_screen->h) {
		warning("Wizardry7: screen '%s' does not fit the framebuffer", name.c_str());
		return false;
	}

	_screen->fillRect(Common::Rect(_screen->w, _screen->h), 0);
	_screen->copyRectToSurface(pixels.data(), width, 0, 0, width, height);
	if (_palette.isLoaded())
		_palette.apply(_screen);
	_screen->update();
	return true;
}

static void drawButton(Graphics::Screen *screen, GameFont &font, const char *text,
		int x, int y, int width, int height, byte textColor = 1, bool selected = false) {
	if (selected) {
		screen->hLine(x - 2, y - 2, x + width + 1, 22);
		screen->hLine(x - 2, y + height + 1, x + width + 1, 22);
		screen->vLine(x - 2, y - 2, y + height + 1, 22);
		screen->vLine(x + width + 1, y - 2, y + height + 1, 22);
	}
	screen->fillRect(Common::Rect(x, y, x + width, y + height), 192);
	screen->hLine(x, y - 1, x + width - 1, 176);
	screen->vLine(x - 1, y, y + height - 1, 176);
	screen->hLine(x, y + height, x + width - 1, 197);
	screen->vLine(x + width, y, y + height - 1, 197);
	screen->setPixel(x - 1, y - 1, 81);
	screen->setPixel(x + width, y - 1, 182);
	screen->setPixel(x - 1, y + height, 182);
	screen->setPixel(x + width, y + height, 198);

	int textWidth = 0;
	for (const char *p = text; *p; ++p)
		textWidth += font.getAdvance((byte)*p);
	int tx = x + (width - textWidth) / 2 + 1;
	const int ty = y + 1;
	for (const char *p = text; *p; ++p)
		tx += font.drawChar((byte)*p, screen, tx, ty, textColor, false, 0);
}

bool Wizardry7Engine::showMainMenu(const Common::String &fontName, int selected) {
	if (!showScreen("MAINSCRN"))
		return false;

	GameFont font;
	if (!font.load(Common::Path(fontName)))
		return false;

	static const char *const items[] = {
		"CHARACTER MENU", "ADD CHARACTER", "LOAD SAVEGAME",
		"CONFIGURATION", "IMPORT (BANE)", "QUIT GAME"
	};
	const int barX = 106;
	const int barY = 44;
	const int barWidth = 107;
	const int barHeight = 7;
	const int barStep = 13;

	for (uint i = 0; i < ARRAYSIZE(items); ++i)
		drawButton(_screen, font, items[i], barX, barY + i * barStep, barWidth, barHeight,
			i == (uint)selected ? 26 : 1);
	_screen->update();
	return true;
}

bool Wizardry7Engine::showCharacterMenu(const Common::String &fontName) {
	if (!showScreen("PCVWSCRN"))
		return false;

	GameFont font;
	if (!font.load(Common::Path(fontName)))
		return false;

	static const char *const items[] = {
		"CREATE", "REVIEW", "DELETE", "RENAME", "PICTURE", "EXIT"
	};
	_screen->fillRect(Common::Rect(3, 187, 314, 198), 0);
	for (uint i = 0; i < ARRAYSIZE(items); ++i)
		drawButton(_screen, font, items[i], 5 + i * 52, 189, 47, 7, 1, i == 0);
	_screen->update();
	return true;
}

static const char *getClassAbbreviation(uint8 klass) {
	static const char *const abbrevs[] = {
		"FIG", "MAG", "PRI", "THI", "RAN", "ALC", "BAR",
		"PSN", "VAL", "LOR", "BIS", "MON", "SAM", "NIN"
	};
	return klass < ARRAYSIZE(abbrevs) ? abbrevs[klass] : "???";
}

/** A raised button with a black outline and a light bottom/right edge. */
static void drawOutlinedButton(Graphics::Screen *screen, GameFont &font, const char *text,
		int x, int y, int width, int height, byte textColor = 85) {
	// Drop shadow, black outline, inset bevel and a light bottom/right edge.
	screen->hLine(x, y - 1, x + width, 197);
	screen->vLine(x - 1, y, y + height, 197);
	screen->setPixel(x - 1, y - 1, 197);
	screen->setPixel(x + width, y, 180);
	screen->fillRect(Common::Rect(x, y, x + width, y + height), 0);
	screen->fillRect(Common::Rect(x + 1, y + 1, x + width - 1, y + height - 1), 192);
	screen->hLine(x + 2, y + 1, x + width - 3, 176);
	screen->vLine(x + 1, y + 2, y + height - 3, 176);
	screen->hLine(x + 1, y + height - 2, x + width - 2, 197);
	screen->vLine(x + width - 2, y + 1, y + height - 2, 197);
	screen->setPixel(x + 1, y + 1, 81);
	screen->setPixel(x + width - 2, y + 1, 182);
	screen->setPixel(x + 1, y + height - 2, 182);
	screen->setPixel(x + width - 2, y + height - 2, 198);
	screen->hLine(x, y + height, x + width, 180);
	screen->vLine(x + width, y + 1, y + height, 180);

	int textWidth = 0;
	for (const char *p = text; *p; ++p)
		textWidth += font.getAdvance((byte)*p);
	int tx = x + (width - textWidth) / 2 + 1;
	for (const char *p = text; *p; ++p)
		tx += font.drawChar((byte)*p, screen, tx, y + 3, textColor, false, 0);
}

static void drawVerticalArrow(Graphics::Screen *screen, int x, int y, bool up, byte color) {
	static const int upWidths[5] = { 1, 3, 5, 3, 3 };
	static const int downWidths[5] = { 3, 3, 5, 3, 1 };
	for (int row = 0; row < 5; ++row) {
		const int len = up ? upWidths[row] : downWidths[row];
		const int start = x + (5 - len) / 2;
		screen->hLine(start, y + row, start + len - 1, color);
	}
}

bool Wizardry7Engine::showAddCharacter(const Common::String &fontName, const Common::Path &rosterPath) {
	if (!showMainMenu(fontName, 1))
		return false;

	GameFont font;
	if (!font.load(Common::Path(fontName)))
		return false;

	CharacterRoster roster;
	const Common::Path path = rosterPath.empty()
		? Common::Path(isGold() ? "PCFILE.GLD" : "PCFILE.DBS") : rosterPath;
	if (!roster.load(path))
		return false;

	// Gold panel (same raised style as the other castle menus).
	_screen->fillRect(Common::Rect(54, 142, 266, 199), 0);
	_screen->fillRect(Common::Rect(56, 144, 264, 197), 194);
	_screen->hLine(55, 143, 264, 180);
	_screen->vLine(55, 143, 197, 180);
	_screen->hLine(56, 197, 263, 197);
	_screen->vLine(264, 144, 197, 197);

	font.drawString("ADD CHARACTER", _screen, 61, 152, 1);
	drawOutlinedButton(_screen, font, "OK", 62, 165, 75, 11);
	drawOutlinedButton(_screen, font, "CANCEL", 62, 181, 75, 11);

	// Roster list well.
	_screen->hLine(146, 147, 242, 197);
	_screen->vLine(146, 147, 193, 197);
	_screen->fillRect(Common::Rect(147, 148, 242, 193), 0);
	_screen->fillRect(Common::Rect(148, 149, 241, 192), 195);
	_screen->hLine(147, 193, 242, 180);
	_screen->vLine(242, 148, 193, 180);

	const int rows = MIN<int>(6, roster.getSlotCount());
	int drawn = 0;
	for (uint i = 0; i < roster.getSlotCount() && drawn < rows; ++i) {
		if (!roster.isSlotUsed(i))
			continue;
		const SaveCharacter &c = roster.getCharacter(i);
		const Common::String klass = Common::String::format("%c-%s", c.gender ? 'F' : 'M',
			getClassAbbreviation(c.klass));
		font.drawString(c.name.c_str(), _screen, 150, 150 + drawn * 6, 180);
		font.drawString(klass.c_str(), _screen, 210, 150 + drawn * 6, 180);
		++drawn;
	}

	// Scroll bar: up/down buttons, a top thumb, and the dark track.
	_screen->hLine(246, 147, 258, 197);
	_screen->vLine(246, 147, 193, 197);
	_screen->fillRect(Common::Rect(247, 148, 258, 157), 0);
	_screen->fillRect(Common::Rect(248, 150, 257, 156), 192);
	_screen->hLine(249, 149, 255, 176);
	_screen->vLine(248, 150, 155, 176);
	_screen->hLine(249, 155, 256, 197);
	_screen->vLine(256, 150, 155, 197);
	_screen->setPixel(248, 149, 81);
	_screen->setPixel(256, 149, 182);
	_screen->setPixel(248, 155, 182);
	_screen->setPixel(256, 155, 198);
	_screen->fillRect(Common::Rect(247, 157, 258, 162), 0);
	_screen->fillRect(Common::Rect(248, 158, 257, 162), 192);
	_screen->hLine(249, 157, 255, 176);
	_screen->vLine(248, 158, 160, 176);
	_screen->hLine(249, 161, 256, 197);
	_screen->vLine(256, 158, 161, 197);
	_screen->setPixel(248, 157, 81);
	_screen->setPixel(256, 157, 182);
	_screen->setPixel(248, 161, 182);
	_screen->setPixel(256, 161, 198);
	_screen->fillRect(Common::Rect(247, 162, 258, 184), 0);
	_screen->fillRect(Common::Rect(248, 163, 257, 184), 195);
	_screen->fillRect(Common::Rect(247, 184, 258, 193), 0);
	_screen->fillRect(Common::Rect(248, 186, 257, 192), 192);
	_screen->hLine(249, 185, 255, 176);
	_screen->vLine(248, 186, 190, 176);
	_screen->hLine(249, 191, 256, 197);
	_screen->vLine(256, 186, 191, 197);
	_screen->setPixel(248, 185, 81);
	_screen->setPixel(256, 185, 182);
	_screen->setPixel(248, 191, 182);
	_screen->setPixel(256, 191, 198);
	_screen->hLine(247, 193, 258, 180);
	_screen->vLine(258, 148, 193, 180);
	drawVerticalArrow(_screen, 250, 150, true, 0);
	drawVerticalArrow(_screen, 250, 186, false, 0);

	_screen->update();
	return true;
}

static const char *chargenRaceName(int race) {
	static const char *const names[] = {
		"HUMAN", "HOBBIT", "FELPURR", "ELF", "FAERIE", "RAWULF",
		"DWARF", "LIZARDMAN", "MOOK", "GNOME", "DRACON"
	};
	return race >= 0 && race < (int)ARRAYSIZE(names) ? names[race] : "?";
}

static void drawCreationStats(GameFont &font, Graphics::Screen *screen,
		const CharacterCreationFlow &flow) {
	if (flow.getStep() == CharacterCreationFlow::kStepSex)
		return;

	const CharacterCreation &character = flow.getCharacter();
	font.drawString(Common::String::format("%c-%s", character.getSex() ? 'F' : 'M',
		chargenRaceName(flow.getRace())).c_str(), screen, 105, 16, 1);
	if (character.getClass() >= 0)
		font.drawString(CharacterCreation::getClassName(character.getClass()), screen, 105, 24, 1);
	static const char *const statNames[CharacterCreation::kStatCount] = {
		"STR", "INT", "PIE", "VIT", "DEX", "SPD", "PER", "KAR"
	};
	for (int i = 0; i < CharacterCreation::kStatCount; ++i) {
		const int y = 38 + i * 6;
		font.drawString(statNames[i], screen, 112, y, 1);
		font.drawString(Common::String::format("%u", character.getStat(i)).c_str(),
			screen, 138, y, 1);
	}
	font.drawString("BONUS", screen, 98, 88, 26);
	font.drawString(Common::String::format("%d", character.getBonusPoints()).c_str(),
		screen, 136, 88, 1);
}

static void drawModalPanel(Graphics::Screen *screen, int x, int y, int width, int height) {
	screen->fillRect(Common::Rect(x, y, x + width, y + height), 0);
	screen->hLine(x + 1, y + 1, x + width - 2, 180);
	screen->vLine(x + 1, y + 2, y + height - 3, 180);
	screen->fillRect(Common::Rect(x + 2, y + 2, x + width - 1, y + height - 2), 194);
	screen->hLine(x + 1, y + height - 2, x + width - 2, 197);
	screen->vLine(x + width - 2, y + 1, y + height - 2, 197);
	screen->setPixel(x + width - 2, y + 1, 180);
	screen->setPixel(x + 1, y + height - 2, 180);
}

static void drawModalButton(Graphics::Screen *screen, GameFont &font, const char *text,
		int x, int y, int width, int height, byte textColor, bool selected = false) {
	const byte border = selected ? 22 : 0;
	screen->fillRect(Common::Rect(x, y, x + width, y + height), border);
	screen->fillRect(Common::Rect(x + 1, y + 1, x + width - 1, y + height - 1), 192);
	screen->hLine(x + 2, y + 1, x + width - 3, 176);
	screen->vLine(x + 1, y + 2, y + height - 3, 176);
	screen->hLine(x + 1, y + height - 2, x + width - 2, 197);
	screen->vLine(x + width - 2, y + 1, y + height - 2, 197);
	screen->setPixel(x + 1, y + 1, 81);
	screen->setPixel(x + width - 2, y + 1, 182);
	screen->setPixel(x + 1, y + height - 2, 182);
	screen->setPixel(x + width - 2, y + height - 2, 198);

	int textWidth = 0;
	for (const char *p = text; *p; ++p)
		textWidth += font.getAdvance((byte)*p);
	int tx = x + (width - textWidth) / 2 + 1;
	for (const char *p = text; *p; ++p)
		tx += font.drawChar((byte)*p, screen, tx, y + 3, textColor, false, 0);
}

static void drawChargenGrid(GameFont &font, Graphics::Screen *screen,
		const char *const *labels, int count, int rows, int selected) {
	static const int columns[3] = { 56, 126, 196 };
	for (int i = 0; i < count; ++i) {
		const int row = i / 3;
		const int col = i % 3;
		if (row >= rows)
			break;
		drawModalButton(screen, font, labels[i], columns[col], 133 + row * 12,
			69, 11, 1, i == selected);
	}
}

bool Wizardry7Engine::showCharacterCreation(const Common::String &step, const Common::String &fontName, int selection,
		const Common::String &name) {
	if (!showScreen("PCVWSCRN"))
		return false;

	GameFont font;
	if (!font.load(Common::Path(fontName)))
		return false;

	if (step.equalsIgnoreCase("sex")) {
		drawModalPanel(_screen, 68, 113, 185, 37);
		font.drawString("SELECT CHARACTER SEX", _screen, 100, 121, 1);
		drawModalButton(_screen, font, "MALE", 73, 133, 69, 11, 1, selection == 0);
		drawModalButton(_screen, font, "FEMALE", 179, 133, 69, 11, 1, selection == 1);
	} else if (step.equalsIgnoreCase("race")) {
		static const char *const races[] = {
			"HUMAN", "HOBBIT", "FELPURR",
			"ELF", "FAERIE", "RAWULF",
			"DWARF", "LIZARDMAN", "MOOK",
			"GNOME", "DRACON"
		};
		drawModalPanel(_screen, 51, 113, 219, 73);
		font.drawString("SELECT CHARACTER RACE", _screen, 97, 121, 1);
		drawChargenGrid(font, _screen, races, ARRAYSIZE(races), 4, selection);
	} else if (step.equalsIgnoreCase("profession")) {
		drawModalPanel(_screen, 51, 113, 219, 61);
		font.drawString("SELECT CHARACTER PROFESSION", _screen, 79, 121, 1);
		// The original list leaves gaps rather than filling row-major.
		static const char *const names[7] = {
			"FIGHTER", "THIEF", "BARD", "MAGE", "RANGER", "PRIEST", "ALCHEMIST"
		};
		static const uint classIds[7] = { 0, 3, 6, 1, 4, 2, 5 };
		static const int xs[7] = { 56, 126, 196, 56, 126, 56, 126 };
		static const int ys[7] = { 133, 133, 133, 145, 145, 157, 157 };
		for (uint i = 0; i < 7; ++i)
			drawModalButton(_screen, font, names[i], xs[i], ys[i], 69, 11, 1,
				(int)classIds[i] == selection);
	} else if (step.equalsIgnoreCase("name")) {
		drawModalPanel(_screen, 54, 142, 212, 57);
		font.drawString("CREATE NEW CHARACTER", _screen, 100, 152, 1);
		drawOutlinedButton(_screen, font, "OK", 62, 165, 75, 11, 85);
		drawOutlinedButton(_screen, font, "CANCEL", 62, 181, 75, 11, 85);
		font.drawString("CHARACTER NAME", _screen, 158, 165, 155);
		// White name field with the initial text cursor.
		_screen->hLine(147, 172, 254, 197);
		_screen->vLine(146, 172, 182, 197);
		_screen->fillRect(Common::Rect(147, 173, 254, 182), 0);
		_screen->fillRect(Common::Rect(148, 174, 253, 181), 1);
		int cursorX = 150;
		if (!name.empty()) {
			font.drawString(name.c_str(), _screen, 150, 175, 0);
			for (uint i = 0; i < name.size(); ++i)
				cursorX += font.getAdvance((byte)name[i]);
		}
		_screen->fillRect(Common::Rect(cursorX, 175, cursorX + 5, 180), 0);
		_screen->hLine(147, 182, 254, 180);
		_screen->vLine(254, 173, 182, 180);
	} else if (step.equalsIgnoreCase("bonus")) {
		drawModalPanel(_screen, 63, 130, 195, 37);
		font.drawString("ABILITY BONUS POINTS", _screen, 101, 138, 1);
		drawModalButton(_screen, font, "CLICK HERE WHEN DONE", 87, 150, 147, 11, 26);
	} else {
		warning("Wizardry7: unknown character-creation step '%s'", step.c_str());
		return false;
	}

	if (_creationStarted)
		drawCreationStats(font, _screen, _creation);
	_screen->makeAllDirty();
	_screen->update();
	return true;
}

bool Wizardry7Engine::showProfessionChoices(const Common::Array<uint> &classes,
		const Common::String &fontName, int selection) {
	if (!showScreen("PCVWSCRN"))
		return false;
	GameFont font;
	if (!font.load(Common::Path(fontName)))
		return false;

	const int count = MIN<int>(classes.size(), CharacterCreation::kClassCount);
	const int rows = (count + 2) / 3;
	const int height = 24 + rows * 12 + 1;
	drawModalPanel(_screen, 51, 113, 219, height);
	font.drawString("SELECT CHARACTER PROFESSION", _screen, 79, 121, 1);
	static const int columns[3] = { 56, 126, 196 };
	for (int i = 0; i < count; ++i) {
		const int row = i / 3;
		const int col = i % 3;
		drawModalButton(_screen, font, CharacterCreation::getClassName(classes[i]),
			columns[col], 133 + row * 12, 69, 11, 1, (int)classes[i] == selection);
	}
	_screen->makeAllDirty();
	_screen->update();
	return true;
}

bool Wizardry7Engine::runCharacterCreationUI(const Common::String &fontName) {
	if (!startCreation())
		return false;

	int selection = 0;
	Common::String name;
	bool done = false;
	while (!done && !shouldQuit()) {
		const CharacterCreationFlow::Step step = _creation.getStep();
		if (step == CharacterCreationFlow::kStepProfession) {
			Common::Array<uint> classes = _creation.getCharacter().getEligibleClasses();
			selection = CLIP<int>(selection, 0, MAX<int>(0, (int)classes.size() - 1));
			if (!showProfessionChoices(classes, fontName,
					classes.empty() ? -1 : classes[selection]))
				return false;
		} else {
			switch (step) {
			case CharacterCreationFlow::kStepSex:
			case CharacterCreationFlow::kStepRace:
				selection = CLIP<int>(selection, 0, step == CharacterCreationFlow::kStepSex ? 1 : 10);
				if (!showCharacterCreation(step == CharacterCreationFlow::kStepSex ? "sex" : "race",
						fontName, selection))
					return false;
				break;
			case CharacterCreationFlow::kStepName:
				if (!showCharacterCreation("name", fontName, 0, name))
					return false;
				break;
			case CharacterCreationFlow::kStepBonus:
				if (!showCharacterCreation("bonus", fontName, 0))
					return false;
				break;
			default:
				done = true;
				continue;
			}
		}

		Common::Event event;
		bool gotEvent = false;
		while (!gotEvent && !shouldQuit()) {
			while (g_system->getEventManager()->pollEvent(event)) {
				if (event.type == Common::EVENT_QUIT) {
					shouldQuit();
					return true;
				}
				if (event.type == Common::EVENT_KEYDOWN || event.type == Common::EVENT_LBUTTONDOWN) {
					gotEvent = true;
					break;
				}
			}
			g_system->delayMillis(10);
		}
		if (!gotEvent)
			break;

		if (event.type == Common::EVENT_KEYDOWN) {
			const Common::KeyCode key = event.kbd.keycode;
			if (key == Common::KEYCODE_ESCAPE)
				break;
			switch (step) {
			case CharacterCreationFlow::kStepSex:
				if (key == Common::KEYCODE_LEFT || key == Common::KEYCODE_RIGHT)
					selection ^= 1;
				else if (key == Common::KEYCODE_RETURN || key == Common::KEYCODE_SPACE)
					creationChooseSex(selection);
				break;
			case CharacterCreationFlow::kStepRace:
				if (key == Common::KEYCODE_UP) selection = MAX(0, selection - 3);
				else if (key == Common::KEYCODE_DOWN) selection = MIN(10, selection + 3);
				else if (key == Common::KEYCODE_LEFT) selection = MAX(0, selection - 1);
				else if (key == Common::KEYCODE_RIGHT) selection = MIN(10, selection + 1);
				else if (key == Common::KEYCODE_RETURN || key == Common::KEYCODE_SPACE)
					creationChooseRace(selection);
				break;
			case CharacterCreationFlow::kStepProfession: {
				Common::Array<uint> classes = _creation.getCharacter().getEligibleClasses();
				if (key == Common::KEYCODE_UP) selection = MAX(0, selection - 3);
				else if (key == Common::KEYCODE_DOWN) selection = MIN<int>((int)classes.size() - 1, selection + 3);
				else if (key == Common::KEYCODE_LEFT) selection = MAX(0, selection - 1);
				else if (key == Common::KEYCODE_RIGHT) selection = MIN<int>((int)classes.size() - 1, selection + 1);
				else if ((key == Common::KEYCODE_RETURN || key == Common::KEYCODE_SPACE) && !classes.empty())
					creationChooseProfession(classes[selection]);
				break;
			}
			case CharacterCreationFlow::kStepName:
				if (key == Common::KEYCODE_RETURN) {
					creationSetName(name);
					if (_creation.getStep() == CharacterCreationFlow::kStepName)
						_creation.finishName();
				} else if (key == Common::KEYCODE_BACKSPACE && !name.empty()) {
					name.deleteLastChar();
				} else if (event.kbd.ascii >= 32 && event.kbd.ascii < 127 && name.size() < 8) {
					name += (char)event.kbd.ascii;
				}
				break;
			case CharacterCreationFlow::kStepBonus:
				if (key >= Common::KEYCODE_1 && key <= Common::KEYCODE_8)
					creationAddBonus((uint)(key - Common::KEYCODE_1));
				else if (key == Common::KEYCODE_RETURN || key == Common::KEYCODE_SPACE) {
					if (creationFinish(_creationResult))
						done = true;
				}
				break;
			default:
				done = true;
				break;
			}
		} else {
			const int mx = event.mouse.x;
			const int my = event.mouse.y;
			switch (step) {
			case CharacterCreationFlow::kStepSex:
				if (my >= 133 && my < 145) {
					if (mx >= 73 && mx < 142) creationChooseSex(0);
					else if (mx >= 179 && mx < 248) creationChooseSex(1);
				}
				break;
			case CharacterCreationFlow::kStepRace:
			case CharacterCreationFlow::kStepProfession: {
				if (my >= 133) {
					const int row = (my - 133) / 12;
					const int col = (mx >= 196) ? 2 : (mx >= 126) ? 1 : (mx >= 56) ? 0 : -1;
					if (col >= 0 && my - 133 - row * 12 < 11) {
						const int index = row * 3 + col;
						if (step == CharacterCreationFlow::kStepRace && index < 11)
							creationChooseRace(index);
						else if (step == CharacterCreationFlow::kStepProfession) {
							Common::Array<uint> classes = _creation.getCharacter().getEligibleClasses();
							if (index < (int)classes.size())
								creationChooseProfession(classes[index]);
						}
					}
				}
				break;
			}
			case CharacterCreationFlow::kStepName:
				if (mx >= 62 && mx < 137) {
					if (my >= 165 && my < 176) {
						creationSetName(name);
						if (_creation.getStep() == CharacterCreationFlow::kStepName)
							_creation.finishName();
					} else if (my >= 181 && my < 192) {
						done = true;
					}
				}
				break;
			case CharacterCreationFlow::kStepBonus:
				if (mx >= 87 && mx < 234 && my >= 150 && my < 162) {
					if (creationFinish(_creationResult)) {
						debug("Wizardry7: created %s (%u/%u/%u)", _creationResult.name.c_str(),
							_creationResult.race, _creationResult.sex, _creationResult.classIndex);
						done = true;
					}
				}
				break;
			default:
				break;
			}
		}
	}
	return true;
}

bool Wizardry7Engine::startCreation() {
	byte zero[CharacterCreation::kStatCount] = { 0 };
	_creation.begin(zero, 0);
	_creationStarted = true;
	_hasCreationResult = false;
	return true;
}

bool Wizardry7Engine::creationChooseSex(uint sex) {
	if (!_creationStarted || sex > 1)
		return false;
	return _creation.chooseSex((byte)sex);
}

bool Wizardry7Engine::creationChooseRace(uint race) {
	if (!_creationStarted || !_creation.chooseRace((uint8)race))
		return false;
	CharacterCreation &character = _creation.getCharacter();
	character.applyRaceMinimums((uint8)race);
	character.applySexModifiers(character.getSex());
	character.setBonusPoints(CharacterCreation::rollBonusPoints(_rng));
	return true;
}

bool Wizardry7Engine::creationChooseProfession(uint classIndex) {
	return _creationStarted && _creation.chooseProfession(classIndex);
}

bool Wizardry7Engine::creationSetName(const Common::String &name) {
	return _creationStarted && _creation.setName(name);
}

bool Wizardry7Engine::creationAddBonus(uint statIndex) {
	return _creationStarted && _creation.addBonusPoint(statIndex);
}

bool Wizardry7Engine::creationFinish(CreatedCharacter &out) {
	if (!_creationStarted)
		return false;
	if (_creation.getStep() == CharacterCreationFlow::kStepName && !_creation.finishName())
		return false;
	if (!_creation.buildCharacter(_rng, out))
		return false;
	_creationResult = out;
	_hasCreationResult = true;
	return true;
}

bool Wizardry7Engine::saveCreationRecord(const Common::Path &path) {
	if (!_hasCreationResult)
		return false;
	byte record[kCharacterRecordSize];
	if (!writeCharacterRecord(_creationResult, record, sizeof(record)))
		return false;
	if (!writeCharacterInventory(_creationResult, _scenario, record, sizeof(record)))
		return false;
	Common::DumpFile out;
	if (!out.open(path))
		return false;
	const bool ok = out.write(record, sizeof(record)) == sizeof(record);
	out.close();
	return ok;
}

bool Wizardry7Engine::saveScreenshot(const Common::Path &path) {
	if (!_screen)
		return false;

	Common::DumpFile out;
	if (!out.open(path)) {
		warning("Wizardry7: cannot write screenshot '%s'", path.toString().c_str());
		return false;
	}

	byte palette[768];
	_screen->getPalette(palette);
	const bool ok = Image::writeBMP(out, *_screen, palette);
	out.close();
	return ok;
}

void Wizardry7Engine::drawStatusScreen() {
	byte palette[256 * 3];
	for (uint i = 0; i < 256; ++i) {
		palette[i * 3 + 0] = i;
		palette[i * 3 + 1] = i;
		palette[i * 3 + 2] = i;
	}
	_screen->setPalette(palette, 0, 256);

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
