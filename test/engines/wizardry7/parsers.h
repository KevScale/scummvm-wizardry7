#include "common/array.h"
#include "common/scummsys.h"
#include "graphics/screen.h"

#include "wizardry7/font.h"
#include "wizardry7/gfx.h"
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
};
