#include "common/array.h"
#include "common/scummsys.h"
#include "graphics/screen.h"

#include "wizardry7/creation.h"
#include "wizardry7/rng.h"
#include "wizardry7/font.h"
#include "wizardry7/gfx.h"
#include "wizardry7/party.h"
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
		record[0x22] = 50;  // gold
		record[0x40] = 7;   // ac
		record[0x236] = 4;
		record[0x237] = 3;
		record[0x238] = 1; // female
		record[0x239] = 2; // priest
		record[0x23A] = 3; // alignment

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
		TS_ASSERT_EQUALS(c.ac, 7u);
		TS_ASSERT_EQUALS(c.race, 3);
		TS_ASSERT_EQUALS(c.gender, 1);
		TS_ASSERT_EQUALS(c.klass, 2);
		TS_ASSERT_EQUALS(c.alignment, 3);
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
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepSex);

		TS_ASSERT(!flow.chooseRace(0)); // wrong step
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepRace);
		TS_ASSERT(flow.chooseRace(0)); // human
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepProfession);

		TS_ASSERT(!flow.chooseProfession(7)); // not affordable
		TS_ASSERT(flow.chooseProfession(0)); // fighter
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepName);
		TS_ASSERT_EQUALS(flow.getCharacter().getBonusPoints(), 6);

		TS_ASSERT(!flow.setName("")); // empty names rejected
		TS_ASSERT(flow.setName("TESTY"));
		TS_ASSERT(flow.finishName());
		TS_ASSERT_EQUALS(flow.getStep(), Wizardry7::CharacterCreationFlow::kStepBonus);
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

		// Faerie starting gold is reduced to two-thirds.
		rng.reset();
		const Wizardry7::CreationRolls faerie = creation.rollDerived(5, rng);
		TS_ASSERT_EQUALS(faerie.carryingCapacity, 1170);

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
		TS_ASSERT(flow.chooseSex(0));
		TS_ASSERT(flow.chooseRace(0));
		TS_ASSERT(flow.chooseProfession(0));
		TS_ASSERT(flow.setName("CAP"));
		TS_ASSERT(flow.finishName());
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

		creation.applySexModifiers(1); // Female: STR -2, PER +1, KAR +1
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
};
