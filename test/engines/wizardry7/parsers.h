#include "common/array.h"
#include "common/scummsys.h"
#include "graphics/screen.h"

#include "wizardry7/creation.h"
#include "wizardry7/equipment.h"
#include "wizardry7/rng.h"
#include "wizardry7/font.h"
#include "wizardry7/gfx.h"
#include "wizardry7/items.h"
#include "wizardry7/maze.h"
#include "wizardry7/monster.h"
#include "wizardry7/attack.h"
#include "wizardry7/view.h"
#include "wizardry7/party.h"
#include "wizardry7/xp.h"
#include "wizardry7/save.h"

#include <cxxtest/TestSuite.h>

class Wizardry7ParserTestSuite : public CxxTest::TestSuite {
public:
	void testVgaBundle() {
		// One 8x1 chunk at offset 10; header is 4 + 1*6 bytes.
		byte data[18] = {
			1, 0, 0, 0,
			0, 0, 10, 0, 1, 1,
			1, 2, 3, 4, 5, 6, 7, 8
		};
		Wizardry7::VgaBundle bundle;
		TS_ASSERT(bundle.loadFromData(data, sizeof(data)));
		TS_ASSERT_EQUALS(bundle.getChunkCount(), 1u);
		TS_ASSERT_EQUALS(bundle.getRecordCount(), 0u);

		uint width, height;
		uint32 offset, size;
		TS_ASSERT(bundle.getChunkInfo(0, width, height, offset, size));
		TS_ASSERT_EQUALS(width, 8u);
		TS_ASSERT_EQUALS(height, 1u);
		TS_ASSERT_EQUALS(offset, 10u);
		TS_ASSERT_EQUALS(size, 8u);
	}

	void testVgaPlacement() {
		byte data[4 + 6 + 5 + 8];
		memset(data, 0, sizeof(data));
		data[0] = 1; // chunks
		data[2] = 1; // records
		data[4] = 0; // segment
		data[6] = 15; // offset 15 (4+6+5)
		data[8] = 1; // stride 8
		data[9] = 1; // height 1
		data[10] = 3; data[11] = 0xFE; data[12] = 4; data[13] = 5; data[14] = 6;
		Wizardry7::VgaBundle bundle;
		TS_ASSERT(bundle.loadFromData(data, sizeof(data)));
		Wizardry7::VgaPlacement p;
		TS_ASSERT(bundle.getPlacement(0, p));
		TS_ASSERT_EQUALS(p.bitmap, 3);
		TS_ASSERT_EQUALS(p.x, -2);
		TS_ASSERT_EQUALS(p.y, 4);
		TS_ASSERT_EQUALS(p.xExtra, 5);
		TS_ASSERT_EQUALS(p.rows, 6);
		TS_ASSERT(!bundle.getPlacement(1, p));
	}

	void testFontGlyphs() {
		Common::Array<byte> data;
		data.resize(0x110 + 2 * 2);
		memset(data.data(), 0, data.size());

		data[0] = 2; // width
		data[1] = 2; // height
		data[2] = 0; // 1bpp
		data[3] = 1; // stride
		data[4] = 1;
		data[5] = 2; // glyph count
		data[6] = 2; // flags
		data[7] = 0; // base advance
		data[10] = 2; // bytes per glyph
		data[12] = 0x14;
		data[13] = 0x01; // block 1 end
		data[14] = 2;

		// Character table: code 0 -> glyph 0/advance 1, code 1 -> glyph 1/advance 1.
		data[0x10] = 0;
		data[0x11] = 1;
		data[0x12] = 1;
		data[0x13] = 1;

		data[0x110] = 0x80; // glyph 0, row 0: pixel 0
		data[0x111] = 0x40; // glyph 0, row 1: pixel 1
		data[0x112] = 0x40; // glyph 1, row 0: pixel 1
		data[0x113] = 0x80; // glyph 1, row 1: pixel 0

		Wizardry7::GameFont font;
		TS_ASSERT(font.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(font.getWidth(), 2u);
		TS_ASSERT_EQUALS(font.getHeight(), 2u);
		TS_ASSERT_EQUALS(font.getGlyphCount(), 2u);
		TS_ASSERT_EQUALS(font.getAdvance(0), 1u);
		TS_ASSERT_EQUALS(font.getAdvance(1), 1u);
		TS_ASSERT_EQUALS(font.getAdvance(0x20), 0u); // code 32 is unmapped

		Graphics::Screen screen(4, 4);
		screen.fillRect(Common::Rect(4, 4), 0);
		TS_ASSERT_EQUALS(font.drawChar(1, &screen, 0, 0, 5, true, 0), 1u);
		TS_ASSERT_EQUALS(*(const byte *)screen.getBasePtr(0, 0), 0);
		TS_ASSERT_EQUALS(*(const byte *)screen.getBasePtr(1, 0), 5);
		TS_ASSERT_EQUALS(*(const byte *)screen.getBasePtr(0, 1), 5);
		TS_ASSERT_EQUALS(*(const byte *)screen.getBasePtr(1, 1), 0);
	}

	void testDosSave() {
		Common::Array<byte> data;
		data.resize(10820 + 584);
		memset(data.data(), 0, data.size());
		data[0] = 0x0A; // in-progress save tag
		data[0x234C] = 14; data[0x234E] = 56; data[0x2350] = 57;
		data[0x2352] = 7; data[0x2354] = 6; data[0x2356] = 5;
		data[0x2358] = 2; data[0x235A] = 14; data[0x235C] = 55; data[0x235E] = 57;
		data[0x2396] = 1; // party count
		data[0x2372] = 0xEC; data[0x2373] = 0x03;

		byte *record = data.data() + 10820;
		memcpy(record, "TEST", 4);
		record[0x08] = 0x58;
		record[0x09] = 0x1B; // age 7000
		record[0x0C] = 123; // xp
		record[0x18] = 10;  // hp
		record[0x1A] = 20;  // hp max
		record[0x1C] = 30;  // stamina
		record[0x1E] = 40;  // stamina max
		record[0x20] = 100; // weight
		record[0x22] = 50;  // carrying capacity
		record[0x1B6] = 7;  // ac
		record[0x236] = 4;
		record[0x237] = 3;
		record[0x238] = 1; // female
		record[0x239] = 2; // priest
		record[0x23A] = 3; // alignment
		record[0x170] = 12; // STR
		record[0x177] = 12; // portrait
		record[0x1B9] = 10; // AC slot copies
		record[0x24] = 1; // level 1
		record[0x178] = 10; // weaponry SWORD
		record[0x182] = 3; record[0x183] = 1; record[0x184] = 1;
		record[0x189] = 5; record[0x18B] = 4;
		record[0x190] = 1;
		record[0x28] = 7; record[0x2A] = 9; // fire MP cur/max
		record[0x130] = 1;
		record[0x40 + 6] = 1; // first slot count

		Wizardry7::SaveGame save;
		TS_ASSERT(save.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(save.getKind(), Wizardry7::kSaveDos);
		TS_ASSERT_EQUALS(save.getCharacterCount(), 1u);
		const Wizardry7::SaveCharacter &c = save.getCharacter(0);
		TS_ASSERT_EQUALS(c.name, Common::String("TEST"));
		TS_ASSERT_EQUALS(c.age, 7000u);
		TS_ASSERT_EQUALS(c.xp, 123u);
		TS_ASSERT_EQUALS(c.hp, 10u);
		TS_ASSERT_EQUALS(c.hpMax, 20u);
		TS_ASSERT_EQUALS(c.stamina, 30u);
		TS_ASSERT_EQUALS(save.getPartyMaze(), 14);
		TS_ASSERT_EQUALS(save.getPartyX(), 56);
		TS_ASSERT_EQUALS(save.getPartyY(), 57);
		TS_ASSERT_EQUALS(save.getPrevMaze(), 14);
		TS_ASSERT_EQUALS(save.getPrevX(), 55);
		TS_ASSERT_EQUALS(save.getPrevY(), 57);
		TS_ASSERT_EQUALS(save.getPartyWindow(), 5);
		TS_ASSERT_EQUALS(save.getPartyLocalY(), 6);
		TS_ASSERT_EQUALS(save.getPartyLocalX(), 7);
		TS_ASSERT_EQUALS(save.getPartyFacing(), 2);
		TS_ASSERT_EQUALS(save.getPartyCounter(), 1004);
		TS_ASSERT_EQUALS(c.ac, 7u);
		TS_ASSERT_EQUALS(c.carryingCapacity, 50u);
		TS_ASSERT_EQUALS(c.level, 1u);
		TS_ASSERT_EQUALS(c.swimming, 3);
		TS_ASSERT_EQUALS(c.climbing, 1);
		TS_ASSERT_EQUALS(c.mapping, 0);
		TS_ASSERT_EQUALS(c.artifacts, 1);
		TS_ASSERT_EQUALS(c.weaponry[0], 10);
		TS_ASSERT_EQUALS(c.kirijutsu, 5);
		TS_ASSERT_EQUALS(c.music, 4);
		TS_ASSERT_EQUALS(c.spellPoints[0][0], 7);
		TS_ASSERT_EQUALS(c.spellPoints[0][1], 9);
		TS_ASSERT_EQUALS(c.strTier, 1);
		TS_ASSERT_EQUALS(c.race, 3);
		TS_ASSERT_EQUALS(c.gender, 1);
		TS_ASSERT_EQUALS(c.klass, 2);
		TS_ASSERT_EQUALS(c.alignment, 3);
		TS_ASSERT_EQUALS(c.stats[0], 12);
		TS_ASSERT_EQUALS(c.portrait, 12);
		TS_ASSERT_EQUALS(c.inventoryCount[0], 1);
		TS_ASSERT_EQUALS(c.acSlots[0], 10);

		byte replacement[584];
		memset(replacement, 0, sizeof(replacement));
		memcpy(replacement, "NEW", 3);
		replacement[0x18] = 5;
		TS_ASSERT(save.setCharacterRecord(0, replacement, sizeof(replacement)));
		TS_ASSERT_EQUALS(save.getCharacter(0).name, Common::String("NEW"));
		TS_ASSERT_EQUALS(save.getCharacter(0).hp, 5u);
		Common::Array<byte> out;
		TS_ASSERT(save.writeToData(out));
		TS_ASSERT_EQUALS(out.size(), data.size());
		TS_ASSERT_EQUALS(memcmp(out.data() + 10820, replacement, sizeof(replacement)), 0);
		TS_ASSERT(save.setPartyPosition(14, 125, 130, 1, 6, 5, 3));
		TS_ASSERT_EQUALS(save.getPartyMaze(), 14);
		TS_ASSERT_EQUALS(save.getPartyX(), 125);
		TS_ASSERT_EQUALS(save.getPartyLocalX(), 1);
		TS_ASSERT_EQUALS(save.getPartyWindow(), 5);
		TS_ASSERT_EQUALS(save.getPartyFacing(), 3);

		Wizardry7::SaveGame roundTrip;
		TS_ASSERT(roundTrip.loadFromData(out.data(), out.size()));
		TS_ASSERT_EQUALS(roundTrip.getCharacter(0).name, Common::String("NEW"));

		Common::Array<byte> rosterData;
		rosterData.resize(24 + 2 * 584);
		memset(rosterData.data(), 0, rosterData.size());
		rosterData[0] = 0x48;
		rosterData[1] = 0x02;
		rosterData[2] = 2;
		rosterData[4] = 24;
		rosterData[8] = 1;
		rosterData[9] = 1;
		memcpy(rosterData.data() + 24, "AAA", 3);
		memcpy(rosterData.data() + 24 + 584, "BBB", 3);
		Wizardry7::CharacterRoster roster;
		TS_ASSERT(roster.loadFromData(rosterData.data(), rosterData.size()));
		Wizardry7::Party party;
		TS_ASSERT(party.addMember(1));
		TS_ASSERT(party.addMember(0));
		Common::Array<byte> beforeParty;
		TS_ASSERT(save.writeToData(beforeParty));
		TS_ASSERT(save.setPartyFromRoster(roster, party));
		TS_ASSERT_EQUALS(save.getCharacterCount(), 2u);
		TS_ASSERT_EQUALS(save.getCharacter(0).name, Common::String("BBB"));
		TS_ASSERT_EQUALS(save.getCharacter(1).name, Common::String("AAA"));
		Common::Array<byte> partyData;
		TS_ASSERT(save.writeToData(partyData));
		TS_ASSERT_EQUALS(partyData.size(), 10820u + 2 * 584u);
		TS_ASSERT_EQUALS(memcmp(partyData.data(), beforeParty.data(), 10820), 0);
	}

	void testRoster() {
		Common::Array<byte> data;
		data.resize(24 + 2 * 584);
		memset(data.data(), 0, data.size());
		data[0] = 0x48; // stride 584
		data[1] = 0x02;
		data[2] = 2; // slots
		data[4] = 24; // header size
		data[8] = 1; // slot 0 used

		memcpy(data.data() + 24, "AAA", 3);
		memcpy(data.data() + 24 + 584, "BBB", 3);

		Wizardry7::CharacterRoster roster;
		TS_ASSERT(roster.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(roster.getSlotCount(), 2u);
		TS_ASSERT(roster.isSlotUsed(0));
		TS_ASSERT(!roster.isSlotUsed(1));
		TS_ASSERT_EQUALS(roster.getCharacter(0).name, Common::String("AAA"));
		TS_ASSERT_EQUALS(roster.getCharacter(1).name, Common::String("BBB"));

		TS_ASSERT_EQUALS(roster.findFreeSlot(), 1);
		byte record[584];
		memset(record, 0, sizeof(record));
		memcpy(record, "NEWCHAR12345", 12);
		record[0x238] = 1; // female
		TS_ASSERT(roster.setCharacterRecord(1, record, sizeof(record)));
		TS_ASSERT(roster.isSlotUsed(1));
		TS_ASSERT_EQUALS(roster.getCharacter(1).name, Common::String("NEWCHAR12345"));
		TS_ASSERT_EQUALS(roster.getCharacter(1).gender, 1);
		TS_ASSERT_EQUALS(roster.findFreeSlot(), -1);

		Common::Array<byte> out;
		TS_ASSERT(roster.writeToData(out));
		TS_ASSERT_EQUALS(out.size(), data.size());
		TS_ASSERT_EQUALS(out[8 + 1], 1);
		TS_ASSERT_EQUALS(memcmp(out.data() + 24 + 584, record, sizeof(record)), 0);

		Wizardry7::CharacterRoster roundTrip;
		TS_ASSERT(roundTrip.loadFromData(out.data(), out.size()));
		TS_ASSERT(roundTrip.isSlotUsed(1));
		TS_ASSERT_EQUALS(roundTrip.getCharacter(1).name, Common::String("NEWCHAR12345"));
	}

	void testCreationEligibility() {
		Wizardry7::CharacterCreation creation;
		const byte humanStats[8] = { 7, 10, 8, 10, 9, 9, 9, 11 };
		creation.setStats(humanStats);
		creation.setBonusPoints(11);
		creation.setSex(0); // male

		// The reference profession list for this F-HUMAN state: Fighter, Mage,
		// Priest, Thief, Ranger, Alchemist, Bard. Valkyrie is female-only.
		TS_ASSERT_EQUALS(creation.getClassCost(0), 5);
		TS_ASSERT_EQUALS(creation.getClassCost(3), 2);
		TS_ASSERT_EQUALS(creation.getClassCost(8), -1);
		Common::Array<uint> eligible = creation.getEligibleClasses();
		TS_ASSERT_EQUALS(eligible.size(), 7u);
		const uint expected[7] = { 0, 1, 2, 3, 4, 5, 6 };
		for (uint i = 0; i < 7; ++i)
			TS_ASSERT_EQUALS(eligible[i], expected[i]);

		TS_ASSERT(creation.selectClass(0)); // Fighter: STR 7 -> 12
		TS_ASSERT_EQUALS(creation.getStat(0), 12);
		TS_ASSERT_EQUALS(creation.getBonusPoints(), 6);
		TS_ASSERT_EQUALS(creation.getClassCost(0), 0);
		TS_ASSERT(!creation.selectClass(7)); // needs more than the remaining pool

		creation.setStats(humanStats);
		creation.setBonusPoints(0);
		creation.setSex(1); // female
		TS_ASSERT_EQUALS(creation.getClassCost(8), 10);
		creation.setStat(0, 10);
		creation.setStat(2, 11);
		creation.setStat(3, 11);
		creation.setStat(4, 10);
		creation.setStat(5, 11);
		creation.setStat(6, 8);
		TS_ASSERT_EQUALS(creation.getClassCost(8), 0);
		TS_ASSERT(creation.isClassEligible(8));
	}

	void testCreationFlow() {
		Wizardry7::CharacterCreationFlow flow;
		const byte humanStats[8] = { 7, 10, 8, 10, 9, 9, 9, 11 };
		flow.begin(humanStats, 11);
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepName);

		TS_ASSERT(!flow.setName("")); // empty names rejected
		TS_ASSERT(flow.setName("TESTY"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepRace);
		TS_ASSERT(!flow.chooseSex(0)); // wrong step
		TS_ASSERT(flow.chooseRace(0)); // human
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepSex);
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepProfession);

		TS_ASSERT(!flow.chooseProfession(7)); // not affordable
		TS_ASSERT(flow.chooseProfession(0)); // fighter
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepBonus);
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 6);
		TS_ASSERT(!flow.finish()); // six points still unspent
		TS_ASSERT(flow.addBonusPoint(1));
		TS_ASSERT_EQUALS(flow.getCharacter().getStat(1), 11);
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 5);
		for (int i = 0; i < 5; ++i)
			TS_ASSERT(flow.addBonusPoint(0));
		TS_ASSERT_EQUALS(flow.getCharacter().getStat(0), 17);
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 0);
		TS_ASSERT(!flow.addBonusPoint(0)); // no points left
		TS_ASSERT(flow.removeBonusPoint(0));
		TS_ASSERT_EQUALS(flow.getCharacter().getStat(0), 16);
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 1);
		TS_ASSERT(flow.addBonusPoint(0));
		TS_ASSERT(flow.finish());
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepDone);
	}

	void testGoldCreationOrder() {
		Wizardry7::CharacterCreationFlow flow;
		const byte humanStats[8] = { 9, 8, 8, 9, 9, 8, 8, 0 };
		flow.begin(humanStats, 20, true);
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepName);
		TS_ASSERT(flow.setName("GOLD"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepRace);
		TS_ASSERT(flow.chooseRace(0));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepSex);
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepProfession);
		TS_ASSERT(flow.chooseProfession(0));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepBonus);
	}

	void testCreationRolls() {
		Wizardry7::CharacterCreation creation;
		const byte stats[8] = { 12, 10, 10, 15, 10, 10, 10, 0 };
		creation.setStats(stats);
		creation.setBonusPoints(0);
		creation.setSex(0);
		TS_ASSERT(creation.selectClass(0)); // Fighter

		Wizardry7::WizardryRNG rng;
		rng.reset();
		const Wizardry7::CreationRolls rolls = creation.rollDerived(0, rng);
		TS_ASSERT(rolls.ageDays >= 6570 && rolls.ageDays < 7570);
		TS_ASSERT(rolls.hpMax >= 10 && rolls.hpMax <= 14);
		TS_ASSERT_EQUALS(rolls.staminaMax, 126); // (STR + VIT*2) * 3
		TS_ASSERT_EQUALS(rolls.carryingCapacity, 1755); // (VIT + STR*2) * 3 * 15
		TS_ASSERT_EQUALS(rolls.ac, 10);

		// Faerie starting gold is reduced to two-thirds.
		rng.reset();
		const Wizardry7::CreationRolls faerie = creation.rollDerived(5, rng);
		TS_ASSERT_EQUALS(faerie.carryingCapacity, 1170);
		TS_ASSERT_EQUALS(faerie.ac, 8); // Faerie -2

		// VIT above 15/17 adds VIT to ST and HP, STR above 15/17 adds STR to gold.
		const byte strongStats[8] = { 18, 10, 10, 18, 10, 10, 10, 0 };
		creation.setStats(strongStats);
		creation.setBonusPoints(0);
		TS_ASSERT(creation.selectClass(0));
		rng.reset();
		const Wizardry7::CreationRolls strong = creation.rollDerived(0, rng);
		TS_ASSERT(strong.hpMax >= 12 && strong.hpMax <= 16);
		TS_ASSERT_EQUALS(strong.staminaMax, 198);
		TS_ASSERT_EQUALS(strong.carryingCapacity, 2970);
	}

	void testCreationBonusCap() {
		Wizardry7::CharacterCreationFlow flow;
		const byte stats[8] = { 18, 10, 10, 10, 10, 10, 10, 0 };
		flow.begin(stats, 2);
		TS_ASSERT(flow.setName("CAP"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT(flow.chooseRace(0));
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT(flow.chooseProfession(0));
		TS_ASSERT(!flow.addBonusPoint(0)); // stat already at the cap
		TS_ASSERT(flow.addBonusPoint(1));
		TS_ASSERT(flow.removeBonusPoint(1));
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 2);
	}

	void testCreationRaceAndSexStats() {
		Wizardry7::CharacterCreation creation;
		const byte zero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		creation.setStats(zero);
		creation.applyRaceMinimums(0); // Human
		const byte human[8] = { 9, 8, 8, 9, 9, 8, 8, 0 };
		for (uint i = 0; i < 8; ++i)
			TS_ASSERT_EQUALS(creation.getStat(i), human[i]);

		creation.applySexModifiers(1); // Female: STR -2, PER +1, KAR/portrait +1
		TS_ASSERT_EQUALS(creation.getStat(0), 7);
		TS_ASSERT_EQUALS(creation.getStat(6), 9);
		TS_ASSERT_EQUALS(creation.getStat(7), 1);

		creation.applySexModifiers(0); // Male is a no-op
		TS_ASSERT_EQUALS(creation.getStat(0), 7);
	}

	void testCreationBonusRoll() {
		Wizardry7::WizardryRNG rng;
		rng.reset();
		const int bonus = Wizardry7::CharacterCreation::rollBonusPoints(rng);
		TS_ASSERT_EQUALS(bonus, 8); // rand(6)=1 -> 6, no lucky +8s, floor 8
	}

	void testParty() {
		Wizardry7::Party party;
		TS_ASSERT(party.addMember(0));
		TS_ASSERT(party.addMember(2));
		TS_ASSERT(party.addMember(4));
		TS_ASSERT(!party.addMember(2)); // duplicate
		TS_ASSERT_EQUALS(party.size(), 3u);
		TS_ASSERT(party.contains(4));
		TS_ASSERT(party.moveMember(2, 0));
		TS_ASSERT_EQUALS(party.getMember(0), 4);
		TS_ASSERT_EQUALS(party.getMember(1), 0);
		TS_ASSERT_EQUALS(party.getMember(2), 2);
		TS_ASSERT(party.removeMember(2));
		TS_ASSERT_EQUALS(party.size(), 2u);
		TS_ASSERT_EQUALS(party.getMember(1), 0);
		TS_ASSERT(!party.moveMember(0, 2));

		byte bytes[6];
		party.toBytes(bytes);
		TS_ASSERT_EQUALS(bytes[0], 4);
		TS_ASSERT_EQUALS(bytes[1], 0);
		TS_ASSERT_EQUALS(bytes[2], 0xFF);
		Wizardry7::Party restored;
		TS_ASSERT(restored.fromBytes(bytes, sizeof(bytes)));
		TS_ASSERT_EQUALS(restored.size(), 2u);
		TS_ASSERT_EQUALS(restored.getMember(0), 4);
		TS_ASSERT_EQUALS(restored.getMember(1), 0);

		for (byte i = 6; i < 12; ++i)
			if (party.size() < 6)
				TS_ASSERT(party.addMember(i));
		TS_ASSERT_EQUALS(party.size(), 6u);
		TS_ASSERT(!party.addMember(12));
		TS_ASSERT(!party.fromBytes(bytes, 5));
	}

	void testPartyFromRoster() {
		Common::Array<byte> data;
		data.resize(24 + 2 * 584);
		memset(data.data(), 0, data.size());
		data[0] = 0x48; // stride 584
		data[1] = 0x02;
		data[2] = 2; // slots
		data[4] = 24; // header size
		data[8] = 1;
		data[9] = 1; // slots 0 and 1 used
		memcpy(data.data() + 24, "AAA", 3);
		memcpy(data.data() + 24 + 584, "BBB", 3);
		Wizardry7::CharacterRoster roster;
		TS_ASSERT(roster.loadFromData(data.data(), data.size()));
		Wizardry7::Party party;
		party.loadFromRoster(roster);
		TS_ASSERT_EQUALS(party.size(), 2u);
		TS_ASSERT_EQUALS(party.getMember(0), 0);
		TS_ASSERT_EQUALS(party.getMember(1), 1);
	}

	void testStartingEquipment() {
		Wizardry7::StartingItem items[Wizardry7::StartingEquipment::kMaxItems];
		TS_ASSERT_EQUALS(Wizardry7::StartingEquipment::getItems(0, 0, items), 5u); // Fighter
		TS_ASSERT_EQUALS(items[0].itemId, 8);
		TS_ASSERT_EQUALS(items[1].itemId, 135);
		TS_ASSERT_EQUALS(items[4].itemId, 141);
		TS_ASSERT_EQUALS(items[0].count, 1);

		TS_ASSERT(Wizardry7::StartingEquipment::getItems(4, 0, items)); // Ranger
		TS_ASSERT_EQUALS(items[1].itemId, 33); // Willow Arrows
		TS_ASSERT_EQUALS(items[1].count, 100);
		TS_ASSERT(Wizardry7::StartingEquipment::getItems(3, 0, items)); // Thief
		TS_ASSERT_EQUALS(items[4].itemId, 27); // Bite Daggers
		TS_ASSERT_EQUALS(items[4].count, 6);
		TS_ASSERT(Wizardry7::StartingEquipment::getItems(12, 0, items)); // Monk
		TS_ASSERT_EQUALS(items[1].itemId, 47); // Shurikens
		TS_ASSERT_EQUALS(items[1].count, 15);

		// Faerie overrides the whole kit, even for a Fighter.
		TS_ASSERT(Wizardry7::StartingEquipment::getItems(0, 5, items));
		TS_ASSERT_EQUALS(items[0].itemId, 397);
		TS_ASSERT_EQUALS(items[1].itemId, 398);
		TS_ASSERT_EQUALS(items[2].itemId, 399);
		TS_ASSERT_EQUALS(items[3].itemId, 316);
		TS_ASSERT_EQUALS(items[4].itemId, 384);
		TS_ASSERT_EQUALS(Wizardry7::StartingEquipment::getItems(14, 0, items), 0u);
	}

	void testCreationResult() {
		Wizardry7::CharacterCreationFlow flow;
		const byte stats[8] = { 7, 10, 8, 10, 9, 9, 9, 11 };
		flow.begin(stats, 11);
		TS_ASSERT(flow.setName("TESTY"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT(flow.chooseRace(0));
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT(flow.chooseProfession(0)); // Fighter
		while (flow.getCharacter().getBonusPoints() > 0)
			TS_ASSERT(flow.addBonusPoint(0));

		Wizardry7::CreatedCharacter result;
		Wizardry7::WizardryRNG rng;
		rng.reset();
		TS_ASSERT(flow.buildCharacter(rng, result));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepDone);
		TS_ASSERT_EQUALS(result.name, Common::String("TESTY"));
		TS_ASSERT_EQUALS(result.race, 0);
		TS_ASSERT_EQUALS(result.sex, 0);
		TS_ASSERT_EQUALS(result.classIndex, 0);
		TS_ASSERT_EQUALS(result.rolls.ageDays >= 6570, true);
		TS_ASSERT_EQUALS(result.itemCount, 5u);
		TS_ASSERT_EQUALS(result.items[0].itemId, 8);
	}

	void testCreatedCharacterRecord() {
		Wizardry7::CharacterCreationFlow flow;
		const byte stats[8] = { 7, 10, 8, 10, 9, 9, 9, 11 };
		flow.begin(stats, 11);
		TS_ASSERT(flow.setName("TESTY"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT(flow.chooseRace(0));
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT(flow.chooseProfession(0));
		while (flow.getCharacter().getBonusPoints() > 0)
			TS_ASSERT(flow.addBonusPoint(0));
		Wizardry7::CreatedCharacter created;
		Wizardry7::WizardryRNG rng;
		rng.reset();
		TS_ASSERT(flow.buildCharacter(rng, created));

		byte record[Wizardry7::kCharacterRecordSize];
		TS_ASSERT(Wizardry7::writeCharacterRecord(created, record, sizeof(record)));
		TS_ASSERT_EQUALS(memcmp(record, "TESTY", 5), 0);
		TS_ASSERT_EQUALS(record[0x237], 0);
		TS_ASSERT_EQUALS(record[0x238], 0);
		TS_ASSERT_EQUALS(record[0x239], 0);

		Common::Array<byte> roster;
		roster.resize(24 + sizeof(record));
		memset(roster.data(), 0, roster.size());
		roster[0] = 0x48;
		roster[1] = 0x02;
		roster[2] = 1;
		roster[4] = 24;
		roster[8] = 1;
		memcpy(roster.data() + 24, record, sizeof(record));
		Wizardry7::CharacterRoster parsed;
		TS_ASSERT(parsed.loadFromData(roster.data(), roster.size()));
		const Wizardry7::SaveCharacter &c = parsed.getCharacter(0);
		TS_ASSERT_EQUALS(c.name, Common::String("TESTY"));
		TS_ASSERT_EQUALS(c.age, created.rolls.ageDays);
		TS_ASSERT_EQUALS(c.hp, created.rolls.hpMax);
		TS_ASSERT_EQUALS(c.stamina, created.rolls.staminaMax);
		TS_ASSERT_EQUALS(c.ac, created.rolls.ac);
		TS_ASSERT_EQUALS(c.inventory[0], 8);
		TS_ASSERT_EQUALS(c.inventory[1], 135);
		TS_ASSERT_EQUALS(c.inventory[4], 141);
		for (uint i = 0; i < 7; ++i)
			TS_ASSERT_EQUALS(record[0x170 + i], created.stats[i]);
		TS_ASSERT_EQUALS(record[0x177], created.portrait);
		TS_ASSERT(created.portrait >= 1 && created.portrait <= 19);
		TS_ASSERT_EQUALS(record[0x1B9], created.rolls.ac);
		TS_ASSERT_EQUALS(record[0x1BD], created.rolls.ac);
		TS_ASSERT_EQUALS(record[0x240], 1);
		TS_ASSERT_EQUALS(record[0x241], 10);
		TS_ASSERT_EQUALS(record[0x243], 1);
		TS_ASSERT_EQUALS(record[0x244], 1);
	}

	void testCreationValidationCases() {
		struct Case {
			uint8 race, sex, klass, cost;
			byte base[8], after[8];
		};
		static const Case cases[] = {
			{ 0, 1, 0, 5, { 7, 8, 8, 9, 9, 8, 9, 1 }, { 12, 8, 8, 9, 9, 8, 9, 1 } },
			{ 1, 0, 1, 2, { 7, 10, 10, 7, 9, 9, 8, 0 }, { 7, 12, 10, 7, 9, 9, 8, 0 } },
			{ 0, 1, 3, 3, { 7, 8, 8, 9, 9, 8, 9, 1 }, { 7, 8, 8, 12, 9, 8, 9, 1 } },
			{ 2, 0, 2, 3, { 11, 6, 10, 12, 7, 7, 7, 0 }, { 11, 6, 12, 12, 7, 7, 8, 0 } },
			{ 4, 1, 6, 5, { 6, 7, 6, 9, 10, 7, 14, 1 }, { 6, 7, 10, 9, 10, 8, 14, 1 } },
			{ 6, 0, 0, 0, { 12, 5, 5, 14, 8, 10, 3, 0 }, { 12, 5, 5, 14, 8, 10, 3, 0 } }
		};
		for (uint i = 0; i < ARRAYSIZE(cases); ++i) {
			Wizardry7::CharacterCreation creation;
			const byte zero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			creation.setStats(zero);
			creation.applyRaceMinimums(cases[i].race);
			creation.applySexModifiers(cases[i].sex);
			for (uint j = 0; j < 8; ++j)
				TS_ASSERT_EQUALS(creation.getStat(j), cases[i].base[j]);
			TS_ASSERT_EQUALS(creation.getClassCost(cases[i].klass), cases[i].cost);
			creation.setBonusPoints(8);
			TS_ASSERT(creation.selectClass(cases[i].klass));
			for (uint j = 0; j < 8; ++j)
				TS_ASSERT_EQUALS(creation.getStat(j), cases[i].after[j]);
			Wizardry7::StartingItem items[Wizardry7::StartingEquipment::kMaxItems];
			TS_ASSERT_EQUALS(Wizardry7::StartingEquipment::getItems(cases[i].klass, cases[i].race, items), 5u);
		}
	}

	void testInventorySlotWriter() {
		byte item[72];
		memset(item, 0, sizeof(item));
		item[0x24] = 0x32;
		item[0x38] = 2; // thrown
		item[0x39] = 1; // subtype
		item[0x37] = 4; // flag mapping -> slot bit 0x08
		item[0x28] = 7;
		item[0x29] = 9;
		item[0x3C] = 3;
		item[0x40] = 5; // AC bonus
		byte slot[12];
		TS_ASSERT(Wizardry7::writeInventorySlot(slot, 27, item, 6));
		TS_ASSERT_EQUALS(READ_LE_UINT16(slot), 27);
		TS_ASSERT_EQUALS(READ_LE_UINT16(slot + 2), 0x32);
		TS_ASSERT_EQUALS(slot[4], 2);
		TS_ASSERT_EQUALS(slot[5], 1);
		TS_ASSERT_EQUALS(slot[6], 6);
		TS_ASSERT_EQUALS(slot[7], 0x80 | 0x08);
		TS_ASSERT_EQUALS(slot[8], 7);
		TS_ASSERT_EQUALS(slot[9], 9);
		TS_ASSERT_EQUALS(slot[10], 3);
		TS_ASSERT_EQUALS(slot[11], 5);
	}

	void testProfessionGridOrder() {
		uint row, col;
		// Captured F-HUMAN set [Fighter, Mage, Priest, Thief, Ranger, Bard]:
		// Fighter/Priest/Ranger on top, Mage/Thief/Bard below (column-major).
		Wizardry7::CharacterCreation::getProfessionGridPosition(0, 6, row, col);
		TS_ASSERT_EQUALS(row, 0); TS_ASSERT_EQUALS(col, 0);
		Wizardry7::CharacterCreation::getProfessionGridPosition(1, 6, row, col);
		TS_ASSERT_EQUALS(row, 1); TS_ASSERT_EQUALS(col, 0);
		Wizardry7::CharacterCreation::getProfessionGridPosition(2, 6, row, col);
		TS_ASSERT_EQUALS(row, 0); TS_ASSERT_EQUALS(col, 1);
		Wizardry7::CharacterCreation::getProfessionGridPosition(3, 6, row, col);
		TS_ASSERT_EQUALS(row, 1); TS_ASSERT_EQUALS(col, 1);
		Wizardry7::CharacterCreation::getProfessionGridPosition(4, 6, row, col);
		TS_ASSERT_EQUALS(row, 0); TS_ASSERT_EQUALS(col, 2);
		Wizardry7::CharacterCreation::getProfessionGridPosition(5, 6, row, col);
		TS_ASSERT_EQUALS(row, 1); TS_ASSERT_EQUALS(col, 2);
	}

	void testItemRecord() {
		byte record[72];
		memset(record, 0, sizeof(record));
		memcpy(record, "SWORD =/FIRE", 12);
		record[0x16] = 0x34; record[0x17] = 0x12;
		record[0x1A] = 2; record[0x1C] = 3; record[0x1D] = 4;
		record[0x20] = 1; record[0x21] = 6;
		record[0x22] = 3; record[0x23] = 4;
		record[0x24] = 0x2C; record[0x25] = 0x01;
		record[0x28] = 0x15;
		record[0x32] = 0xFF; record[0x33] = 0x3F;
		record[0x34] = 0xFF; record[0x35] = 0x07;
		record[0x36] = 3;
		record[0x37] = 4;
		record[0x38] = 0;
		record[0x39] = 2;
		record[0x3B] = 10;
		record[0x40] = 2;

		Wizardry7::ItemInfo info;
		TS_ASSERT(Wizardry7::parseItemRecord(record, sizeof(record), info));
		TS_ASSERT_EQUALS(info.displayName, Common::String("SWORD OF FIRE"));
		Common::String line1, line2;
		Wizardry7::decodeItemNameLines(record, 22, line1, line2);
		TS_ASSERT_EQUALS(line1, Common::String("SWORD OF"));
		TS_ASSERT_EQUALS(line2, Common::String("FIRE"));
		TS_ASSERT_EQUALS(info.price, 0x1234);
		TS_ASSERT_EQUALS(info.weight, 0x012C);
		TS_ASSERT_EQUALS(info.getDefaultCharges(), 14);
		TS_ASSERT_EQUALS(info.damageDiceCount, 1);
		TS_ASSERT_EQUALS(info.damageDieSize, 6);
		TS_ASSERT_EQUALS(info.getSpellId(), 3);
		TS_ASSERT_EQUALS(info.getSpellPower(), 4);
		TS_ASSERT_EQUALS(info.kind, 0);
		TS_ASSERT_EQUALS(info.subtype, 2);
		TS_ASSERT_EQUALS(info.rank, 10);
		TS_ASSERT_EQUALS(info.acBonus, 2);
	}

	void testExperienceTable() {
		byte record[64];
		memset(record, 0, sizeof(record));
		const uint32 values[16] = {
			1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000,
			256000, 512000, 768000, 1168000, 1568000, 1968000, 2368000, 2968000
		};
		for (uint i = 0; i < 16; ++i)
			WRITE_LE_UINT32(record + i * 4, values[i]);
		Wizardry7::ExperienceTable table;
		TS_ASSERT(table.parseRecord(record, sizeof(record), 0));
		TS_ASSERT_EQUALS(table.getThreshold(0, 1), 0u);
		TS_ASSERT_EQUALS(table.getThreshold(0, 2), 1000u);
		TS_ASSERT_EQUALS(table.getThreshold(0, 16), 2368000u);
		TS_ASSERT_EQUALS(table.getThreshold(0, 17), 2968000u);
		TS_ASSERT_EQUALS(table.getThreshold(0, 18), 3568000u);
		TS_ASSERT_EQUALS(table.getLevel(0, 0), 1);
		TS_ASSERT_EQUALS(table.getLevel(0, 999), 1);
		TS_ASSERT_EQUALS(table.getLevel(0, 1000), 2);
		TS_ASSERT_EQUALS(table.getLevel(0, 2368000), 16);
		TS_ASSERT_EQUALS(table.getLevel(0, 2968000), 17);
		TS_ASSERT_EQUALS(Wizardry7::ExperienceTable::getRank(1), 0);
		TS_ASSERT_EQUALS(Wizardry7::ExperienceTable::getRank(2), 1);
		TS_ASSERT_EQUALS(Wizardry7::ExperienceTable::getRank(24), 7);
	}

	void testMazeRecord() {
		Common::Array<byte> data;
		data.resize(Wizardry7::MazeRecord::kRecordSize);
		memset(data.data(), 0, data.size());
		data[0] = 0x12; // header row0/group0/band0
		data[0x090] = 0x53; // tile A face0=3, face1=5
		data[0x291] = 0x06; // tile B face2=6
		data[0x4B0] = (0x2A & 3) << 6; // special face1=0x2A, bits 6..11
		data[0x4B1] = 0x2A >> 2;
		data[0x7B0] = 2 << 6; // flags face3=2
		data[0x8B0 + 1] = 2; // hazard layer0 face9
		data[0x9B0 + 1] = 2; // plane A face9 -> kind 2
		data[0xAB0] = 0x43; // encounter slot0 common=3, uncommon=4
		data[0xAC0] = 65;
		Wizardry7::MazeRecord maze;
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(maze.getMonster(0, 0, 0), 0x12);
		TS_ASSERT_EQUALS(maze.getTileA(0), 3);
		TS_ASSERT_EQUALS(maze.getTileA(1), 5);
		TS_ASSERT_EQUALS(maze.getTileB(2), 6);
		TS_ASSERT_EQUALS(maze.getSpecial(1), 0x2A);
		TS_ASSERT_EQUALS(maze.getFlags(3), 2);
		TS_ASSERT(maze.getHazard(0, 9));
		TS_ASSERT(!maze.getHazard(1, 9));
		TS_ASSERT_EQUALS(maze.getViewKind(0, 1, 1), 2); // face 9 is plane A
		data[0x930] = 1; // face 0 plane C -> kind 3
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(maze.getViewKind(0, 0, 0), 3);
		TS_ASSERT_EQUALS(maze.getEncounterGroup(0, false), 3);
		TS_ASSERT_EQUALS(maze.getEncounterGroup(0, true), 4);
		TS_ASSERT_EQUALS(maze.getEncounterChance(0), 65);
		for (uint i = 0; i < 16; ++i) {
			data[0x490 + i] = 50;
			data[0x4A0 + i] = 50;
		}
		data[0x490 + 2] = 10;
		data[0x4A0 + 2] = 20;
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		uint cell = 0, lx = 0, ly = 0;
		TS_ASSERT(maze.findCell(12, 23, cell, lx, ly));
		TS_ASSERT_EQUALS(cell, 2u);
		TS_ASSERT_EQUALS(lx, 2u);
		TS_ASSERT_EQUALS(ly, 3u);
		TS_ASSERT(!maze.findCell(0, 0, cell, lx, ly));
		uint8 kinds[5];
		TS_ASSERT(maze.getKindRay(2, 2, 3, 1, 0, kinds));
		maze.getEncounterChance(1);
		data[0xAC0 + 1] = 0; // no encounter possible
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		Wizardry7::WizardryRNG rng;
		rng.reset();
		TS_ASSERT_EQUALS(maze.selectEncounterMonster(1, rng), -1);
		for (uint i = 0; i < 144; ++i)
			data[i] = 42;
		data[0xAC0 + 2] = 100;
		data[0xAB0] = 0; // slot 0 group 0; slot 2 also low nibble 0
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		rng.reset();
		TS_ASSERT_EQUALS(maze.selectEncounterMonster(2, rng), 42);
	}

	void testMazePosition() {
		Wizardry7::MazePosition pos;
		pos.maze = 14; pos.a = 125; pos.b = 130;
		TS_ASSERT_EQUALS(pos.facing, 0);
		pos.stepForward(); // facing0: b+1
		TS_ASSERT_EQUALS(pos.b, 131);
		pos.stepBackward();
		TS_ASSERT_EQUALS(pos.b, 130);
		pos.turnRight();
		TS_ASSERT_EQUALS(pos.facing, 1);
		pos.stepForward(); // facing1: a+1
		TS_ASSERT_EQUALS(pos.a, 126);
		pos.turnLeft(); pos.turnLeft();
		TS_ASSERT_EQUALS(pos.facing, 3);
		pos.stepForward(); // facing3: a-1
		TS_ASSERT_EQUALS(pos.a, 125);
	}

	void testMonsterRecord() {
		byte record[232];
		memset(record, 0, sizeof(record));
		memcpy(record, "DANDIPHOOT", 10);
		memcpy(record + 0x10, "DANDIPHOOTS", 11);
		memcpy(record + 0x20, "PHOOT", 5);
		memcpy(record + 0x30, "PHOOTS", 6);
		WRITE_LE_UINT32(record + 0x40, 390);
		record[0x7A] = 5;
		WRITE_LE_UINT32(record + 0x7E, 0x0103);
		record[0x80] = 2;
		WRITE_LE_UINT32(record + 0x82, 0x0204);
		WRITE_LE_UINT32(record + 0x86, 0x0104);
		record[0xAA] = 1;
		record[0xAB] = 0;
		record[0xAC] = 25;
		record[0xAD] = 50;
		record[0xD6] = 3;
		record[0xD7] = 26;
		record[0xE2] = 1;
		Wizardry7::MonsterInfo info;
		TS_ASSERT(Wizardry7::parseMonsterRecord(record, sizeof(record), info));
		TS_ASSERT_EQUALS(info.names[0], Common::String("DANDIPHOOT"));
		TS_ASSERT_EQUALS(info.names[1], Common::String("DANDIPHOOTS"));
		TS_ASSERT_EQUALS(info.xp, 390u);
		TS_ASSERT_EQUALS(info.spawnCount, 5);
		// +0x80 is both the top byte of the level-dice u32 and the actor count.
		TS_ASSERT_EQUALS(info.levelDice, 0x020103u);
		TS_ASSERT_EQUALS(info.actorCount, 2);
		TS_ASSERT_EQUALS(info.hpDice, 0x0204u);
		TS_ASSERT_EQUALS(info.stDice, 0x0104u);
		TS_ASSERT_EQUALS(info.variantId[0], 1);
		TS_ASSERT_EQUALS(info.variantChance[0], 25);
		TS_ASSERT_EQUALS(info.bodyType, 3);
		TS_ASSERT_EQUALS(info.pictureId, 26);
		TS_ASSERT_EQUALS(info.flags, 1);
		info.variantChance[0] = info.variantChance[1] = 100;
		info.variantId[0] = 7;
		info.variantId[1] = 0; // last successful entry wins
		Wizardry7::WizardryRNG rng;
		rng.reset();
		TS_ASSERT_EQUALS(Wizardry7::selectMonsterVariant(9, info, rng), 0u);
		info.variantChance[0] = info.variantChance[1] = 0;
		rng.reset();
		TS_ASSERT_EQUALS(Wizardry7::selectMonsterVariant(9, info, rng), 9u);
	}

	void testAttackAnimation() {
		Common::Array<byte> data;
		data.resize(Wizardry7::AttackAnimation::kRecordSize);
		memset(data.data(), 0, data.size());
		data[0x00] = 2; // attack start
		data[0x04] = 3; // attack count
		data[0x08 + 1] = 7;
		data[0x12 + 1] = 4;
		data[0x30 + 1] = 2;
		data[0x26 + 5] = 9;
		data[0xA5 + 3 * 4 + 0] = 21;
		data[0xA5 + 3 * 4 + 1] = 22;
		data[0xA5 + 3 * 4 + 2] = 23;
		data[0xA5 + 3 * 4 + 3] = 24;
		WRITE_LE_UINT16(data.data() + 0x148, 0x1234);
		WRITE_LE_UINT16(data.data() + 0x14A, 0x5678);
		data[0x15B] = 0xFE;
		data[0x15C] = 0xFD;
		Wizardry7::AttackAnimation anim;
		TS_ASSERT(anim.loadFromData(data.data(), data.size()));
		TS_ASSERT_EQUALS(anim.getAttackStart(), 2);
		TS_ASSERT_EQUALS(anim.getAttackCount(), 3);
		TS_ASSERT_EQUALS(anim.getBaseDamage(1), 7);
		TS_ASSERT_EQUALS(anim.getRandomDamage(1), 4);
		TS_ASSERT_EQUALS(anim.getExtraRandomDamage(1), 2);
		TS_ASSERT_EQUALS(anim.getSpecialIndex(5), 9);
		TS_ASSERT_EQUALS(anim.getSpecialSprite(3), 21);
		TS_ASSERT_EQUALS(anim.getSpecialEffectByte(3), 22);
		TS_ASSERT_EQUALS(anim.getSeverityBase(3), 23);
		TS_ASSERT_EQUALS(anim.getSeverityRandom(3), 24);
		TS_ASSERT_EQUALS(anim.getPairA(), 0x1234);
		TS_ASSERT_EQUALS(anim.getPairB(), 0x5678);
		TS_ASSERT_EQUALS(anim.getOverrideA(), 0xFE);
		TS_ASSERT_EQUALS(anim.getOverrideB(), 0xFD);
		data[0x08] = 5;   // base damage
		data[0x12] = 0;   // no random damage
		data[0x26 + 5] = 3;
		data[0xA5 + 3 * 4 + 2] = 2;
		data[0xA5 + 3 * 4 + 3] = 0;
		TS_ASSERT(anim.loadFromData(data.data(), data.size()));
		Wizardry7::WizardryRNG rng;
		rng.reset();
		const Wizardry7::AttackRoll roll = anim.rollAttack(0, rng);
		TS_ASSERT_EQUALS(roll.damageIndex, 5);
		TS_ASSERT_EQUALS(roll.specialIndex, 3);
		TS_ASSERT_EQUALS(roll.severity, 3);
	}

	void testViewFixedPieces() {
		Common::Array<Wizardry7::ViewPiece> town = Wizardry7::ViewCompositor::fixedPieces(3, true);
		TS_ASSERT_EQUALS(town.size(), 2u);
		TS_ASSERT_EQUALS(town[0].slot, 4);
		TS_ASSERT_EQUALS(town[0].piece, 4);
		TS_ASSERT_EQUALS(town[0].sel, 4);
		TS_ASSERT_EQUALS(town[1].slot, 2);
		TS_ASSERT_EQUALS(town[1].piece, 0xAD);
		TS_ASSERT_EQUALS(town[1].sel, 0xAD);
		Common::Array<Wizardry7::ViewPiece> noMirror = Wizardry7::ViewCompositor::fixedPieces(3, false);
		TS_ASSERT_EQUALS(noMirror[0].sel, 0xFFFF);
		TS_ASSERT_EQUALS(Wizardry7::ViewCompositor::fixedPieces(1, true).size(), 2u);
		TS_ASSERT_EQUALS(Wizardry7::ViewCompositor::fixedPieces(5, true).size(), 1u);

		Wizardry7::ViewPiece piece;
		TS_ASSERT(Wizardry7::ViewCompositor::painterDirect(1, 2, 0, true, piece));
		TS_ASSERT_EQUALS(piece.slot, 1);
		TS_ASSERT_EQUALS(piece.piece, 37);
		TS_ASSERT_EQUALS(piece.sel, 37);
		TS_ASSERT(Wizardry7::ViewCompositor::painterDirect(2, 2, 0, true, piece));
		TS_ASSERT_EQUALS(piece.slot, 6);
		TS_ASSERT(Wizardry7::ViewCompositor::painterDirect(3, 2, 0, true, piece));
		TS_ASSERT_EQUALS(piece.slot, 5);
		TS_ASSERT_EQUALS(piece.sel, 0xFFFF);
		byte kinds[5][5];
		memset(kinds, 0, sizeof(kinds));
		kinds[0][0] = 1;
		kinds[1][1] = 2;
		TS_ASSERT_EQUALS(Wizardry7::ViewCompositor::painterPieces(kinds, true).size(), 2u);

		Wizardry7::ViewObjectList objects;
		Wizardry7::ViewObject a = { 5, 0xFFFF, 0xFFFF, 0xFFFF, 2, 0, 0, 0, 0xFD, 1, 3, 0, 0 };
		Wizardry7::ViewObject b = { 6, 0xFFFF, 0xFFFF, 0xFFFF, 2, 0, 0, 0, 0xFD, 1, 3, 0, 0 };
		TS_ASSERT(objects.spawn(a));
		TS_ASSERT(objects.spawn(b));
		Common::Array<Wizardry7::ViewPiece> drawn = objects.draw(2);
		TS_ASSERT_EQUALS(drawn.size(), 2u);
		TS_ASSERT_EQUALS(drawn[0].piece, 6); // reversed object order
		TS_ASSERT_EQUALS(drawn[1].piece, 5);
		Wizardry7::ViewObject stale = { 7, 8, 0xFFFF, 0xFFFF, 2, 0, 0, 0, 0xFD, 1, 76, 0, 0 };
		TS_ASSERT(objects.spawn(stale));
		drawn = objects.draw(2);
		TS_ASSERT_EQUALS(drawn.size(), 2u); // slot 76 is stale and must be skipped
	}
};
