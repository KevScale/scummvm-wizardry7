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

#ifndef WIZARDRY7_SCREENDIFF_H
#define WIZARDRY7_SCREENDIFF_H

#include "common/array.h"
#include "common/path.h"
#include "common/rect.h"
#include "common/str.h"

namespace Common {
class SeekableReadStream;
}

namespace Wizardry7 {

/** A named rectangle of the 320x200 frame. */
struct ScreenRegion {
	Common::String name;
	Common::Rect rect;
};

/**
 * Screens whose panels have a decoded layout. The sets list only the areas
 * worth reporting separately; the frame itself is always reported as "total".
 */
enum ScreenLayout {
	kLayoutFull = 0,
	kLayoutCharGenSex,
	kLayoutCharGenRace,
	kLayoutCharGenProfession,
	kLayoutCharGenName,
	kLayoutCharGenBonus,
	kLayoutCharacterMenu,
	kLayoutMainMenu,
	kLayoutReview,
	kLayoutCount
};

const char *getScreenLayoutName(ScreenLayout layout);

/** Accepts "full", "chargen-sex", ..., "charmenu", "mainmenu", "review". */
bool parseScreenLayout(const Common::String &name, ScreenLayout &layout);

void getScreenLayoutRegions(ScreenLayout layout, Common::Array<ScreenRegion> &regions);

/** Split width x height into columns x rows cells named c<col>r<row>. */
void buildGridRegions(int width, int height, uint columns, uint rows,
		Common::Array<ScreenRegion> &regions);

/** Parse "x,y,w,h" into a rectangle. */
bool parseRegionRect(const char *text, Common::Rect &rect);

/** Parse "COLUMNSxROWS" (e.g. 4x5). */
bool parseRegionGrid(const char *text, uint &columns, uint &rows);

/**
 * Compare result for one region. @p pixels counts the pixels actually
 * compared, i.e. the region minus any masked areas.
 */
struct RegionDiff {
	Common::String name;
	Common::Rect rect;
	uint pixels;
	uint differing;
	uint maxDelta;  // largest per-channel absolute difference of a differing pixel
	uint meanDelta; // mean of maxDelta over the differing pixels
	Common::Rect bbox; // union of the differing pixels, empty when none differ
};

struct ScreenDiffResult {
	uint width, height;
	uint pixels;   // frame pixels compared (masks excluded)
	uint masked;   // frame pixels skipped because of a mask
	uint differing;
	uint maxDelta;
	uint meanDelta;
	Common::Rect bbox;
	Common::Array<RegionDiff> regions;
};

class ScreenDiff {
public:
	/** Decode a BMP reference (8, 24 or 32 bpp) into RGB888. */
	static bool loadReference(Common::SeekableReadStream &stream, Common::Array<byte> &rgb,
			uint &width, uint &height, Common::String &error);
	static bool loadReference(const Common::Path &path, Common::Array<byte> &rgb,
			uint &width, uint &height, Common::String &error);

	/** Expand a CLUT8 buffer to RGB888 with a 768-byte palette. */
	static void expandIndexed(const byte *indices, uint pitch, const byte *palette,
			uint width, uint height, Common::Array<byte> &rgb);

	/**
	 * Compare two RGB888 buffers. Every region gets its own differing-pixel
	 * count, bounding box and colour distance; @p masks are excluded from all
	 * regions and from the frame totals.
	 */
	static void compare(const byte *reference, const byte *engine, uint width, uint height,
			const Common::Array<ScreenRegion> &regions,
			const Common::Array<Common::Rect> &masks, ScreenDiffResult &result);

	/** Human- and tool-readable report, one "region ..." line per region. */
	static Common::String formatReport(const ScreenDiffResult &result, uint threshold);

	/** One report line, also used for the frame summary. */
	static Common::String formatRegion(const RegionDiff &region);

	/** Parse a region list ("name x y w h" per line, # comments). */
	static bool loadRegionList(Common::SeekableReadStream &stream,
			Common::Array<ScreenRegion> &regions, Common::String &error);
	static bool loadRegionList(const Common::Path &path,
			Common::Array<ScreenRegion> &regions, Common::String &error);
};

} // End of namespace Wizardry7

#endif // WIZARDRY7_SCREENDIFF_H
