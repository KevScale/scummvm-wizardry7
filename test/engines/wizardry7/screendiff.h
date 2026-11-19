#include <string.h>

#include "common/array.h"
#include "common/stream.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "graphics/palette.h"
#include "graphics/surface.h"

#include "image/bmp.h"

#include "wizardry7/screendiff.h"

#include <cxxtest/TestSuite.h>

namespace {

const uint kWidth = 320;
const uint kHeight = 200;

void clearRgb(Common::Array<byte> &rgb, byte r, byte g, byte b) {
	for (uint i = 0; i < rgb.size(); i += 3) {
		rgb[i + 0] = r;
		rgb[i + 1] = g;
		rgb[i + 2] = b;
	}
}

void setRgb(Common::Array<byte> &rgb, uint x, uint y, byte r, byte g, byte b) {
	const uint offset = (y * kWidth + x) * 3;
	rgb[offset + 0] = r;
	rgb[offset + 1] = g;
	rgb[offset + 2] = b;
}

const Wizardry7::RegionDiff *findRegion(const Wizardry7::ScreenDiffResult &result, const char *name) {
	for (uint i = 0; i < result.regions.size(); ++i) {
		if (result.regions[i].name.equalsIgnoreCase(name))
			return &result.regions[i];
	}
	return nullptr;
}

} // End of namespace

class Wizardry7ScreenDiffTestSuite : public CxxTest::TestSuite {
public:
	void testDocumentedPanelRects() {
		// Panel geometry from docs/engine/chargen-ui.md; these rects are the
		// pixel-exact panels compared against the canon320 captures.
		struct Expect {
			Wizardry7::ScreenLayout layout;
			int x, y, width, height;
		};
		static const Expect expected[] = {
			{ Wizardry7::kLayoutCharGenSex, 68, 113, 185, 37 },
			{ Wizardry7::kLayoutCharGenRace, 51, 113, 219, 73 },
			{ Wizardry7::kLayoutCharGenProfession, 51, 113, 219, 61 },
			{ Wizardry7::kLayoutCharGenName, 54, 142, 212, 57 },
			{ Wizardry7::kLayoutCharGenBonus, 63, 130, 195, 37 }
		};

		for (uint i = 0; i < ARRAYSIZE(expected); ++i) {
			Common::Array<Wizardry7::ScreenRegion> regions;
			Wizardry7::getScreenLayoutRegions(expected[i].layout, regions);
			const Wizardry7::ScreenRegion *panel = nullptr;
			for (uint j = 0; j < regions.size(); ++j) {
				if (regions[j].name == "panel")
					panel = &regions[j];
			}
			TS_ASSERT(panel != nullptr);
			if (!panel)
				continue;
			TS_ASSERT_EQUALS(panel->rect.left, expected[i].x);
			TS_ASSERT_EQUALS(panel->rect.top, expected[i].y);
			TS_ASSERT_EQUALS(panel->rect.width(), expected[i].width);
			TS_ASSERT_EQUALS(panel->rect.height(), expected[i].height);
		}
	}

	void testLayoutsStayInsideTheFrame() {
		for (int layout = 0; layout < Wizardry7::kLayoutCount; ++layout) {
			Common::Array<Wizardry7::ScreenRegion> regions;
			Wizardry7::getScreenLayoutRegions((Wizardry7::ScreenLayout)layout, regions);
			for (uint i = 0; i < regions.size(); ++i) {
				const Common::Rect &rect = regions[i].rect;
				TS_ASSERT(rect.width() > 0);
				TS_ASSERT(rect.height() > 0);
				TS_ASSERT(rect.left >= 0);
				TS_ASSERT(rect.top >= 0);
				TS_ASSERT(rect.right <= (int)kWidth);
				TS_ASSERT(rect.bottom <= (int)kHeight);
				for (uint j = i + 1; j < regions.size(); ++j)
					TS_ASSERT(!regions[i].name.equalsIgnoreCase(regions[j].name));
			}
		}
	}

	void testLayoutNames() {
		static const char *const names[] = {
			"full", "chargen-sex", "chargen-race", "chargen-profession",
			"chargen-name", "chargen-bonus", "charmenu", "mainmenu", "review"
		};
		TS_ASSERT_EQUALS((uint)Wizardry7::kLayoutCount, (uint)ARRAYSIZE(names));
		for (uint i = 0; i < ARRAYSIZE(names); ++i) {
			Wizardry7::ScreenLayout layout = Wizardry7::kLayoutCount;
			TS_ASSERT(Wizardry7::parseScreenLayout(names[i], layout));
			TS_ASSERT_EQUALS((uint)layout, i);
			TS_ASSERT_EQUALS(Common::String(Wizardry7::getScreenLayoutName(layout)), Common::String(names[i]));
		}
		Wizardry7::ScreenLayout unknown = Wizardry7::kLayoutFull;
		TS_ASSERT(!Wizardry7::parseScreenLayout("no-such-screen", unknown));
	}

	void testGridRegions() {
		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::buildGridRegions(320, 200, 4, 5, regions);
		TS_ASSERT_EQUALS(regions.size(), (uint)20);
		TS_ASSERT_EQUALS(regions[0].name, Common::String("c0r0"));
		TS_ASSERT(regions[0].rect == Common::Rect(0, 0, 80, 40));
		TS_ASSERT_EQUALS(regions[3].name, Common::String("c3r0"));
		TS_ASSERT(regions[3].rect == Common::Rect(240, 0, 320, 40));
		TS_ASSERT_EQUALS(regions[19].name, Common::String("c3r4"));
		TS_ASSERT(regions[19].rect == Common::Rect(240, 160, 320, 200));

		// Widths that do not divide evenly must still tile the frame exactly.
		Wizardry7::buildGridRegions(320, 200, 3, 3, regions);
		TS_ASSERT_EQUALS(regions[2].rect.right, 320);
		TS_ASSERT_EQUALS(regions[2].rect.left, regions[1].rect.right);
		TS_ASSERT_EQUALS(regions[8].rect.bottom, 200);
	}

	void testParseHelpers() {
		Common::Rect rect;
		TS_ASSERT(Wizardry7::parseRegionRect("10,20,30,40", rect));
		TS_ASSERT(rect == Common::Rect(10, 20, 40, 60));
		TS_ASSERT(!Wizardry7::parseRegionRect("1,2,3", rect));
		TS_ASSERT(!Wizardry7::parseRegionRect("1,2,0,4", rect));
		TS_ASSERT(!Wizardry7::parseRegionRect("1,2,3,4x", rect));

		uint columns = 0, rows = 0;
		TS_ASSERT(Wizardry7::parseRegionGrid("4x5", columns, rows));
		TS_ASSERT_EQUALS(columns, (uint)4);
		TS_ASSERT_EQUALS(rows, (uint)5);
		TS_ASSERT(!Wizardry7::parseRegionGrid("4", columns, rows));
		TS_ASSERT(!Wizardry7::parseRegionGrid("0x5", columns, rows));
	}

	void testIdenticalFrames() {
		Common::Array<byte> reference, engine;
		reference.resize(kWidth * kHeight * 3);
		engine.resize(kWidth * kHeight * 3);
		clearRgb(reference, 0, 0, 0);
		clearRgb(engine, 0, 0, 0);

		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::buildGridRegions(kWidth, kHeight, 4, 5, regions);
		Wizardry7::ScreenDiffResult result;
		Wizardry7::ScreenDiff::compare(reference.data(), engine.data(), kWidth, kHeight,
			regions, Common::Array<Common::Rect>(), result);

		TS_ASSERT_EQUALS(result.pixels, kWidth * kHeight);
		TS_ASSERT_EQUALS(result.masked, (uint)0);
		TS_ASSERT_EQUALS(result.differing, (uint)0);
		TS_ASSERT(result.bbox.isEmpty());
		TS_ASSERT_EQUALS(result.regions.size(), (uint)20);
		for (uint i = 0; i < result.regions.size(); ++i)
			TS_ASSERT_EQUALS(result.regions[i].differing, (uint)0);
	}

	void testSinglePixelIsReportedPerRegion() {
		Common::Array<byte> reference, engine;
		reference.resize(kWidth * kHeight * 3);
		engine.resize(kWidth * kHeight * 3);
		clearRgb(reference, 0, 0, 0);
		clearRgb(engine, 0, 0, 0);
		setRgb(engine, 5, 7, 10, 0, 0);

		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::buildGridRegions(kWidth, kHeight, 4, 5, regions);
		Wizardry7::ScreenDiffResult result;
		Wizardry7::ScreenDiff::compare(reference.data(), engine.data(), kWidth, kHeight,
			regions, Common::Array<Common::Rect>(), result);

		TS_ASSERT_EQUALS(result.differing, (uint)1);
		TS_ASSERT(result.bbox == Common::Rect(5, 7, 6, 8));
		TS_ASSERT_EQUALS(result.maxDelta, (uint)10);
		TS_ASSERT_EQUALS(result.meanDelta, (uint)10);

		const Wizardry7::RegionDiff *cell = findRegion(result, "c0r0");
		TS_ASSERT(cell != nullptr);
		if (cell) {
			TS_ASSERT_EQUALS(cell->differing, (uint)1);
			TS_ASSERT(cell->bbox == Common::Rect(5, 7, 6, 8));
			TS_ASSERT_EQUALS(cell->pixels, (uint)(80 * 40));
		}
		const Wizardry7::RegionDiff *other = findRegion(result, "c1r0");
		TS_ASSERT(other != nullptr);
		if (other) {
			TS_ASSERT_EQUALS(other->differing, (uint)0);
			TS_ASSERT(other->bbox.isEmpty());
		}
	}

	void testMaskRemovesPixels() {
		Common::Array<byte> reference, engine;
		reference.resize(kWidth * kHeight * 3);
		engine.resize(kWidth * kHeight * 3);
		clearRgb(reference, 0, 0, 0);
		clearRgb(engine, 0, 0, 0);
		setRgb(engine, 5, 7, 255, 255, 255);

		Common::Array<Common::Rect> masks;
		masks.push_back(Common::Rect(0, 0, 80, 40));

		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::buildGridRegions(kWidth, kHeight, 4, 5, regions);
		Wizardry7::ScreenDiffResult result;
		Wizardry7::ScreenDiff::compare(reference.data(), engine.data(), kWidth, kHeight,
			regions, masks, result);

		TS_ASSERT_EQUALS(result.masked, (uint)(80 * 40));
		TS_ASSERT_EQUALS(result.pixels, kWidth * kHeight - 80 * 40);
		TS_ASSERT_EQUALS(result.differing, (uint)0);
		const Wizardry7::RegionDiff *cell = findRegion(result, "c0r0");
		TS_ASSERT(cell != nullptr);
		if (cell) {
			TS_ASSERT_EQUALS(cell->pixels, (uint)0);
			TS_ASSERT_EQUALS(cell->differing, (uint)0);
		}
	}

	void testRegionOutsideTheFrameIsClipped() {
		Common::Array<byte> reference, engine;
		reference.resize(kWidth * kHeight * 3);
		engine.resize(kWidth * kHeight * 3);
		clearRgb(reference, 0, 0, 0);
		clearRgb(engine, 0, 0, 0);
		setRgb(engine, 319, 199, 1, 2, 3);

		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::ScreenRegion region;
		region.name = "oversized";
		region.rect = Common::Rect(300, 180, 400, 260);
		regions.push_back(region);

		Wizardry7::ScreenDiffResult result;
		Wizardry7::ScreenDiff::compare(reference.data(), engine.data(), kWidth, kHeight,
			regions, Common::Array<Common::Rect>(), result);

		TS_ASSERT_EQUALS(result.regions.size(), (uint)1);
		TS_ASSERT(result.regions[0].rect == Common::Rect(300, 180, 320, 200));
		TS_ASSERT_EQUALS(result.regions[0].pixels, (uint)(20 * 20));
		TS_ASSERT_EQUALS(result.regions[0].differing, (uint)1);
	}

	void testReportFormatAndThreshold() {
		Common::Array<byte> reference, engine;
		reference.resize(kWidth * kHeight * 3);
		engine.resize(kWidth * kHeight * 3);
		clearRgb(reference, 0, 0, 0);
		clearRgb(engine, 0, 0, 0);
		setRgb(engine, 100, 50, 0, 0, 200);

		Common::Array<Wizardry7::ScreenRegion> regions;
		Wizardry7::getScreenLayoutRegions(Wizardry7::kLayoutCharGenSex, regions);
		Wizardry7::ScreenDiffResult result;
		Wizardry7::ScreenDiff::compare(reference.data(), engine.data(), kWidth, kHeight,
			regions, Common::Array<Common::Rect>(), result);

		const Common::String fail = Wizardry7::ScreenDiff::formatReport(result, 0);
		TS_ASSERT(fail.contains("total"));
		TS_ASSERT(fail.contains("differing=1"));
		TS_ASSERT(fail.contains("result=FAIL"));
		TS_ASSERT(fail.contains("panel"));

		const Common::String pass = Wizardry7::ScreenDiff::formatReport(result, 1);
		TS_ASSERT(pass.contains("result=PASS"));
	}

	void testExpandIndexed() {
		const byte indices[] = { 0, 1, 2, 3 };
		byte palette[768];
		memset(palette, 0, sizeof(palette));
		palette[0] = 255; palette[1] = 0; palette[2] = 0;
		palette[3] = 0; palette[4] = 255; palette[5] = 0;
		palette[6] = 0; palette[7] = 0; palette[8] = 255;
		palette[9] = 10; palette[10] = 20; palette[11] = 30;

		Common::Array<byte> rgb;
		Wizardry7::ScreenDiff::expandIndexed(indices, 4, palette, 4, 1, rgb);
		TS_ASSERT_EQUALS(rgb.size(), (uint)12);
		TS_ASSERT_EQUALS(rgb[0], (byte)255);
		TS_ASSERT_EQUALS(rgb[4], (byte)255);
		TS_ASSERT_EQUALS(rgb[8], (byte)255);
		TS_ASSERT_EQUALS(rgb[10], (byte)20);
	}

	void testBmpReferenceRoundTrip() {
		Graphics::Surface surface;
		surface.create(4, 2, Graphics::PixelFormat::createFormatCLUT8());
		TS_ASSERT(surface.getPixels() != nullptr);
		byte *pixels = (byte *)surface.getPixels();
		pixels[0] = 0;
		pixels[1] = 1;
		pixels[2] = 2;
		pixels[3] = 3;
		pixels[4] = 1;
		pixels[5] = 0;
		pixels[6] = 3;
		pixels[7] = 2;

		byte palette[768];
		memset(palette, 0, sizeof(palette));
		palette[0] = 255; palette[1] = 0; palette[2] = 0;
		palette[3] = 0; palette[4] = 255; palette[5] = 0;
		palette[6] = 0; palette[7] = 0; palette[8] = 255;
		palette[9] = 10; palette[10] = 20; palette[11] = 30;

		Common::MemoryWriteStreamDynamic out(DisposeAfterUse::NO);
		TS_ASSERT(Image::writeBMP(out, surface, palette));
		surface.free();

		Common::MemoryReadStream in(out.getData(), out.size());
		Common::Array<byte> rgb;
		uint width = 0, height = 0;
		Common::String error;
		TS_ASSERT(Wizardry7::ScreenDiff::loadReference(in, rgb, width, height, error));
		TS_ASSERT_EQUALS(width, (uint)4);
		TS_ASSERT_EQUALS(height, (uint)2);
		if (width == 4 && height == 2) {
			TS_ASSERT_EQUALS(rgb.size(), (uint)24);
			TS_ASSERT_EQUALS(rgb[0], (byte)255);
			TS_ASSERT_EQUALS(rgb[1], (byte)0);
			TS_ASSERT_EQUALS(rgb[3], (byte)0);
			TS_ASSERT_EQUALS(rgb[4], (byte)255);
			TS_ASSERT_EQUALS(rgb[6], (byte)0);
			TS_ASSERT_EQUALS(rgb[7], (byte)0);
			TS_ASSERT_EQUALS(rgb[8], (byte)255);
			TS_ASSERT_EQUALS(rgb[9], (byte)10);
			TS_ASSERT_EQUALS(rgb[10], (byte)20);
			TS_ASSERT_EQUALS(rgb[11], (byte)30);
		}
	}

	void testRegionListParsing() {
		const char *text = "# comment\n\n"
			"left 0 0 10 10\n"
			"right 10 0 10 10\n";
		Common::MemoryReadStream in((const byte *)text, (uint32)strlen(text));
		Common::Array<Wizardry7::ScreenRegion> regions;
		Common::String error;
		TS_ASSERT(Wizardry7::ScreenDiff::loadRegionList(in, regions, error));
		TS_ASSERT_EQUALS(regions.size(), (uint)2);
		TS_ASSERT_EQUALS(regions[0].name, Common::String("left"));
		TS_ASSERT(regions[0].rect == Common::Rect(0, 0, 10, 10));
		TS_ASSERT_EQUALS(regions[1].name, Common::String("right"));
		TS_ASSERT(regions[1].rect == Common::Rect(10, 0, 20, 10));

		const char *bad = "left 0 0 10\n";
		Common::MemoryReadStream badIn((const byte *)bad, (uint32)strlen(bad));
		Common::Array<Wizardry7::ScreenRegion> badRegions;
		Common::String badError;
		TS_ASSERT(!Wizardry7::ScreenDiff::loadRegionList(badIn, badRegions, badError));
		TS_ASSERT(badError.contains("line 1"));
	}
};
