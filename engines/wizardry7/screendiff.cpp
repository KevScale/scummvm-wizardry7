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

#include "common/file.h"
#include "common/fs.h"
#include "common/str-array.h"
#include "common/stream.h"
#include "common/tokenizer.h"
#include "common/util.h"

#include "graphics/palette.h"
#include "graphics/surface.h"

#include "image/bmp.h"

#include "wizardry7/screendiff.h"

namespace Wizardry7 {

namespace {

struct RegionSpec {
	const char *name;
	int x, y, width, height;
};

// The creation panels are the modal gold panels over PCVWSCRN. Their rects are
// the panel geometry measured in docs/engine/chargen-ui.md and match the
// canon320 captures exactly; the character overlay behind them is dynamic.
static const RegionSpec kCharGenSexRegions[] = {
	{ "panel", 68, 113, 185, 37 }
};
static const RegionSpec kCharGenRaceRegions[] = {
	{ "panel", 51, 113, 219, 73 }
};
static const RegionSpec kCharGenProfessionRegions[] = {
	{ "panel", 51, 113, 219, 61 }
};
static const RegionSpec kCharGenNameRegions[] = {
	{ "panel", 54, 142, 212, 57 },
	{ "field", 147, 173, 107, 9 }
};
static const RegionSpec kCharGenBonusRegions[] = {
	{ "panel", 63, 130, 195, 37 }
};

// showCharacterMenu() clears Rect(3,187,314,198) and draws the six buttons
// there; everything above is the PCVWSCRN character view.
static const RegionSpec kCharacterMenuRegions[] = {
	{ "viewscreen", 0, 0, 320, 187 },
	{ "buttons", 3, 187, 311, 11 }
};

// showMainMenu() stacks six 107x7 buttons from (106,44) in 13 px steps.
static const RegionSpec kMainMenuRegions[] = {
	{ "buttons", 106, 44, 107, 72 }
};

// Text areas around the overlay anchors of docs/engine/review-screen.md. The
// boxes are padding around those documented positions, not measured panel art.
static const RegionSpec kReviewRegions[] = {
	{ "header", 96, 12, 120, 24 },
	{ "rank", 232, 4, 84, 24 },
	{ "stats", 112, 36, 60, 52 },
	{ "name", 8, 74, 80, 12 },
	{ "vitals", 4, 92, 180, 20 },
	{ "ac", 220, 28, 96, 70 },
	{ "inventory", 8, 112, 160, 62 }
};

struct LayoutDefinition {
	const char *name;
	const RegionSpec *specs;
	uint count;
};

static const LayoutDefinition kLayouts[kLayoutCount] = {
	{ "full", nullptr, 0 },
	{ "chargen-sex", kCharGenSexRegions, ARRAYSIZE(kCharGenSexRegions) },
	{ "chargen-race", kCharGenRaceRegions, ARRAYSIZE(kCharGenRaceRegions) },
	{ "chargen-profession", kCharGenProfessionRegions, ARRAYSIZE(kCharGenProfessionRegions) },
	{ "chargen-name", kCharGenNameRegions, ARRAYSIZE(kCharGenNameRegions) },
	{ "chargen-bonus", kCharGenBonusRegions, ARRAYSIZE(kCharGenBonusRegions) },
	{ "charmenu", kCharacterMenuRegions, ARRAYSIZE(kCharacterMenuRegions) },
	{ "mainmenu", kMainMenuRegions, ARRAYSIZE(kMainMenuRegions) },
	{ "review", kReviewRegions, ARRAYSIZE(kReviewRegions) }
};

static bool parseInt(const char *text, int &value) {
	char *end = nullptr;
	value = (int)strtol(text, &end, 0);
	return end != text && *end == 0;
}

static bool isMasked(const Common::Array<Common::Rect> &masks, int x, int y) {
	for (uint i = 0; i < masks.size(); ++i) {
		const Common::Rect &mask = masks[i];
		if (mask.contains((int16)x, (int16)y))
			return true;
	}
	return false;
}

/** Largest per-channel absolute difference of two RGB pixels. */
static uint pixelDelta(const byte *a, const byte *b) {
	uint delta = 0;
	for (uint i = 0; i < 3; ++i) {
		const uint d = a[i] > b[i] ? a[i] - b[i] : b[i] - a[i];
		if (d > delta)
			delta = d;
	}
	return delta;
}

static void extendBBox(Common::Rect &box, int x, int y) {
	if (box.isEmpty()) {
		box = Common::Rect((int16)x, (int16)y, (int16)(x + 1), (int16)(y + 1));
		return;
	}
	// Rect bounds are exclusive on the right/bottom, so grow by one pixel.
	box.left = MIN(box.left, (int16)x);
	box.top = MIN(box.top, (int16)y);
	box.right = MAX(box.right, (int16)(x + 1));
	box.bottom = MAX(box.bottom, (int16)(y + 1));
}

/**
 * Open a reference or region-list file. Absolute paths (the usual case for
 * captures outside the game directory) go through FSNode; bare names still
 * resolve against the game data path so a file next to the data works too.
 */
static Common::SeekableReadStream *openReadStream(const Common::Path &path, Common::String &error) {
	const Common::FSNode node(path);
	if (node.exists()) {
		if (node.isDirectory()) {
			error = Common::String::format("'%s' is a directory", path.toString().c_str());
			return nullptr;
		}
		Common::SeekableReadStream *stream = node.createReadStream();
		if (!stream)
			error = Common::String::format("cannot read '%s'", path.toString().c_str());
		return stream;
	}

	Common::File *file = new Common::File();
	if (file->open(path))
		return file;
	delete file;
	error = Common::String::format("cannot open '%s'", path.toString().c_str());
	return nullptr;
}

} // End of namespace

const char *getScreenLayoutName(ScreenLayout layout) {
	if (layout < 0 || layout >= kLayoutCount)
		return "full";
	return kLayouts[layout].name;
}

bool parseScreenLayout(const Common::String &name, ScreenLayout &layout) {
	for (uint i = 0; i < kLayoutCount; ++i) {
		if (name.equalsIgnoreCase(kLayouts[i].name)) {
			layout = (ScreenLayout)i;
			return true;
		}
	}
	return false;
}

void getScreenLayoutRegions(ScreenLayout layout, Common::Array<ScreenRegion> &regions) {
	regions.clear();
	if (layout < 0 || layout >= kLayoutCount)
		return;
	const LayoutDefinition &definition = kLayouts[layout];
	for (uint i = 0; i < definition.count; ++i) {
		const RegionSpec &spec = definition.specs[i];
		ScreenRegion region;
		region.name = spec.name;
		region.rect = Common::Rect(spec.x, spec.y, spec.x + spec.width, spec.y + spec.height);
		regions.push_back(region);
	}
}

void buildGridRegions(int width, int height, uint columns, uint rows,
		Common::Array<ScreenRegion> &regions) {
	regions.clear();
	if (columns == 0 || rows == 0 || width <= 0 || height <= 0)
		return;
	uint y = 0;
	for (uint row = 0; row < rows; ++row) {
		const uint nextY = (uint)(((uint64)(row + 1) * height) / rows);
		uint x = 0;
		for (uint column = 0; column < columns; ++column) {
			const uint nextX = (uint)(((uint64)(column + 1) * width) / columns);
			ScreenRegion region;
			region.name = Common::String::format("c%ur%u", column, row);
			region.rect = Common::Rect((int16)x, (int16)y, (int16)nextX, (int16)nextY);
			regions.push_back(region);
			x = nextX;
		}
		y = nextY;
	}
}

bool parseRegionRect(const char *text, Common::Rect &rect) {
	const Common::String value(text);
	const char *parse = value.c_str();
	int parts[4];
	for (uint i = 0; i < 4; ++i) {
		char *end = nullptr;
		parts[i] = (int)strtol(parse, &end, 10);
		if (end == parse)
			return false;
		if (i < 3) {
			if (*end != ',')
				return false;
			parse = end + 1;
		} else if (*end != 0) {
			return false;
		}
	}
	if (parts[2] <= 0 || parts[3] <= 0)
		return false;
	rect = Common::Rect(parts[0], parts[1], parts[0] + parts[2], parts[1] + parts[3]);
	return true;
}

bool parseRegionGrid(const char *text, uint &columns, uint &rows) {
	const Common::String value(text);
	const char *parse = value.c_str();
	char *end = nullptr;
	const long cols = strtol(parse, &end, 10);
	if (end == parse || (*end != 'x' && *end != 'X'))
		return false;
	const char *second = end + 1;
	const long count = strtol(second, &end, 10);
	if (end == second || *end != 0)
		return false;
	if (cols <= 0 || count <= 0 || cols > 320 || count > 200)
		return false;
	columns = (uint)cols;
	rows = (uint)count;
	return true;
}

bool ScreenDiff::loadReference(Common::SeekableReadStream &stream, Common::Array<byte> &rgb,
		uint &width, uint &height, Common::String &error) {
	Image::BitmapDecoder decoder;
	if (!decoder.loadStream(stream)) {
		error = "not a readable BMP image";
		return false;
	}
	const Graphics::Surface *surface = decoder.getSurface();
	if (!surface || surface->w <= 0 || surface->h <= 0) {
		error = "empty BMP image";
		return false;
	}

	const Graphics::Palette &palette = decoder.getPalette();
	const bool indexed = surface->format.bytesPerPixel == 1;
	if (indexed && palette.empty()) {
		error = "palettized BMP without a palette";
		return false;
	}

	width = surface->w;
	height = surface->h;
	rgb.resize(width * height * 3);
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			const uint32 pixel = surface->getPixel(x, y);
			byte r = 0, g = 0, b = 0;
			if (indexed) {
				if (pixel < palette.size())
					palette.get(pixel, r, g, b);
			} else {
				surface->format.colorToRGB(pixel, r, g, b);
			}
			const uint offset = (y * width + x) * 3;
			rgb[offset + 0] = r;
			rgb[offset + 1] = g;
			rgb[offset + 2] = b;
		}
	}
	return true;
}

bool ScreenDiff::loadReference(const Common::Path &path, Common::Array<byte> &rgb,
		uint &width, uint &height, Common::String &error) {
	Common::SeekableReadStream *stream = openReadStream(path, error);
	if (!stream)
		return false;
	const bool ok = loadReference(*stream, rgb, width, height, error);
	if (!ok)
		error = Common::String::format("'%s': %s", path.toString().c_str(), error.c_str());
	delete stream;
	return ok;
}

void ScreenDiff::expandIndexed(const byte *indices, uint pitch, const byte *palette,
		uint width, uint height, Common::Array<byte> &rgb) {
	rgb.resize(width * height * 3);
	for (uint y = 0; y < height; ++y) {
		const byte *row = indices + y * pitch;
		for (uint x = 0; x < width; ++x) {
			const byte index = row[x];
			const uint offset = (y * width + x) * 3;
			rgb[offset + 0] = palette[index * 3 + 0];
			rgb[offset + 1] = palette[index * 3 + 1];
			rgb[offset + 2] = palette[index * 3 + 2];
		}
	}
}

void ScreenDiff::compare(const byte *reference, const byte *engine, uint width, uint height,
		const Common::Array<ScreenRegion> &regions,
		const Common::Array<Common::Rect> &masks, ScreenDiffResult &result) {
	result.width = width;
	result.height = height;
	result.pixels = 0;
	result.masked = 0;
	result.differing = 0;
	result.maxDelta = 0;
	result.meanDelta = 0;
	result.bbox = Common::Rect();
	result.regions.clear();

	uint64 deltaSum = 0;
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			if (isMasked(masks, (int)x, (int)y)) {
				++result.masked;
				continue;
			}
			++result.pixels;
			const uint offset = (y * width + x) * 3;
			const uint delta = pixelDelta(reference + offset, engine + offset);
			if (delta == 0)
				continue;
			++result.differing;
			deltaSum += delta;
			if (delta > result.maxDelta)
				result.maxDelta = delta;
			extendBBox(result.bbox, (int)x, (int)y);
		}
	}
	if (result.differing)
		result.meanDelta = (uint)(deltaSum / result.differing);

	for (uint i = 0; i < regions.size(); ++i) {
		RegionDiff region;
		region.name = regions[i].name;
		region.rect = regions[i].rect;
		region.rect.clip(Common::Rect((int16)width, (int16)height));
		region.pixels = 0;
		region.differing = 0;
		region.maxDelta = 0;
		region.meanDelta = 0;
		region.bbox = Common::Rect();

		uint64 regionDeltaSum = 0;
		for (int y = region.rect.top; y < region.rect.bottom; ++y) {
			for (int x = region.rect.left; x < region.rect.right; ++x) {
				if (isMasked(masks, x, y))
					continue;
				++region.pixels;
				const uint offset = ((uint)y * width + (uint)x) * 3;
				const uint delta = pixelDelta(reference + offset, engine + offset);
				if (delta == 0)
					continue;
				++region.differing;
				regionDeltaSum += delta;
				if (delta > region.maxDelta)
					region.maxDelta = delta;
				extendBBox(region.bbox, x, y);
			}
		}
		if (region.differing)
			region.meanDelta = (uint)(regionDeltaSum / region.differing);
		result.regions.push_back(region);
	}
}

Common::String ScreenDiff::formatRegion(const RegionDiff &region) {
	const uint ratio = region.pixels ? (uint)(((uint64)region.differing * 10000) / region.pixels) : 0;
	Common::String box("none");
	if (!region.bbox.isEmpty()) {
		box = Common::String::format("%d,%d-%d,%d", region.bbox.left, region.bbox.top,
			region.bbox.right - 1, region.bbox.bottom - 1);
	}
	return Common::String::format("%-12s rect=%d,%d,%d,%d pixels=%u differing=%u percent=%u.%02u bbox=%s maxdelta=%u meandelta=%u",
		region.name.c_str(), region.rect.left, region.rect.top,
		region.rect.width(), region.rect.height(),
		region.pixels, region.differing, ratio / 100, ratio % 100,
		box.c_str(), region.maxDelta, region.meanDelta);
}

Common::String ScreenDiff::formatReport(const ScreenDiffResult &result, uint threshold) {
	RegionDiff total;
	total.name = "total";
	total.rect = Common::Rect((int16)result.width, (int16)result.height);
	total.pixels = result.pixels;
	total.differing = result.differing;
	total.maxDelta = result.maxDelta;
	total.meanDelta = result.meanDelta;
	total.bbox = result.bbox;

	Common::String report = Common::String::format("screen diff %ux%u: %u pixel(s) compared, %u masked, %u region(s)\n",
		result.width, result.height, result.pixels, result.masked, result.regions.size());
	report += formatRegion(total);
	report += "\n";
	for (uint i = 0; i < result.regions.size(); ++i) {
		report += formatRegion(result.regions[i]);
		report += "\n";
	}
	report += Common::String::format("threshold=%u result=%s\n", threshold,
		result.differing <= threshold ? "PASS" : "FAIL");
	return report;
}

bool ScreenDiff::loadRegionList(Common::SeekableReadStream &stream,
		Common::Array<ScreenRegion> &regions, Common::String &error) {
	regions.clear();
	uint lineNumber = 0;
	while (!stream.eos()) {
		const Common::String line = stream.readLine();
		++lineNumber;
		Common::String trimmed(line);
		trimmed.trim();
		if (trimmed.empty() || trimmed.hasPrefix("#"))
			continue;

		Common::StringTokenizer tokenizer(trimmed, " \t");
		Common::StringArray words;
		while (!tokenizer.empty())
			words.push_back(tokenizer.nextToken());
		if (words.size() != 5) {
			error = Common::String::format("line %u: expected 'name x y w h'", lineNumber);
			return false;
		}

		int values[4];
		for (uint i = 0; i < 4; ++i) {
			if (!parseInt(words[i + 1].c_str(), values[i])) {
				error = Common::String::format("line %u: bad number '%s'", lineNumber, words[i + 1].c_str());
				return false;
			}
		}
		if (values[2] <= 0 || values[3] <= 0) {
			error = Common::String::format("line %u: region has no area", lineNumber);
			return false;
		}

		ScreenRegion region;
		region.name = words[0];
		region.rect = Common::Rect(values[0], values[1], values[0] + values[2], values[1] + values[3]);
		regions.push_back(region);
	}
	if (regions.empty()) {
		error = "region list is empty";
		return false;
	}
	return true;
}

bool ScreenDiff::loadRegionList(const Common::Path &path,
		Common::Array<ScreenRegion> &regions, Common::String &error) {
	Common::SeekableReadStream *stream = openReadStream(path, error);
	if (!stream)
		return false;
	const bool ok = loadRegionList(*stream, regions, error);
	if (!ok)
		error = Common::String::format("'%s': %s", path.toString().c_str(), error.c_str());
	delete stream;
	return ok;
}

} // End of namespace Wizardry7
