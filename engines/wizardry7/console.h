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

#ifndef WIZARDRY7_CONSOLE_H
#define WIZARDRY7_CONSOLE_H

#include "common/path.h"

#include "gui/debugger.h"

namespace Wizardry7 {

class Wizardry7Engine;

class Wizardry7Console : public GUI::Debugger {
public:
	explicit Wizardry7Console(Wizardry7Engine *vm);
	~Wizardry7Console() override;

	int print(const char *format, ...) GCC_PRINTF(2, 3);
	void setScriptMode(bool enabled) { _scriptMode = enabled; }
	bool runScript(const Common::Path &path);

private:
	bool cmdMessage(int argc, const char **argv);
	bool cmdMessageInfo(int argc, const char **argv);
	bool cmdRecords(int argc, const char **argv);
	bool cmdRecord(int argc, const char **argv);
	bool cmdRng(int argc, const char **argv);
	bool cmdSeed(int argc, const char **argv);
	bool cmdStatus(int argc, const char **argv);
	bool cmdMainMenu(int argc, const char **argv);
	bool cmdCharacterMenu(int argc, const char **argv);
	bool cmdAddCharacter(int argc, const char **argv);
	bool cmdCharGen(int argc, const char **argv);
	bool cmdCharGenUI(int argc, const char **argv);
	bool cmdItem(int argc, const char **argv);
	bool cmdSheet(int argc, const char **argv);
	bool cmdReview(int argc, const char **argv);
	bool cmdSaveParty(int argc, const char **argv);
	bool cmdXp(int argc, const char **argv);
	bool cmdMaze(int argc, const char **argv);
	bool cmdMonster(int argc, const char **argv);
	bool cmdAnim(int argc, const char **argv);
	bool cmdCreate(int argc, const char **argv);
	bool cmdParty(int argc, const char **argv);
	bool cmdScreen(int argc, const char **argv);
	bool cmdScreenshot(int argc, const char **argv);
	bool cmdBundle(int argc, const char **argv);
	bool cmdFontInfo(int argc, const char **argv);
	bool cmdGlyphs(int argc, const char **argv);
	bool cmdGlyph(int argc, const char **argv);
	bool cmdSaveInfo(int argc, const char **argv);
	bool cmdText(int argc, const char **argv);
	bool cmdRoster(int argc, const char **argv);
	bool cmdScript(int argc, const char **argv);

	Wizardry7Engine *_vm;
	bool _scriptMode;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_CONSOLE_H
