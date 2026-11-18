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

#ifndef WIZARDRY7_H
#define WIZARDRY7_H

#include "common/error.h"

#include "engines/engine.h"

#include "wizardry7/detection.h"
#include "wizardry7/msg.h"
#include "wizardry7/rng.h"
#include "wizardry7/scenario.h"

namespace Graphics {
class Screen;
}

namespace Wizardry7 {

class Wizardry7Console;

class Wizardry7Engine : public Engine {
public:
	Wizardry7Engine(OSystem *syst, const Wizardry7GameDescription *gameDesc);
	~Wizardry7Engine() override;

	Common::Error run() override;

	bool isGold() const { return _gameDescription->gameType == kGameGold; }

	WizardryRNG *getRNG() { return &_rng; }
	MessageDatabase *getMessages() { return &_messages; }
	ScenarioDatabase *getScenario() { return &_scenario; }

private:
	bool loadGameData();
	void drawStatusScreen();

	const Wizardry7GameDescription *_gameDescription;
	WizardryRNG _rng;
	MessageDatabase _messages;
	ScenarioDatabase _scenario;
	Graphics::Screen *_screen;
	bool _messagesLoaded;
	bool _scenarioLoaded;
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_H
