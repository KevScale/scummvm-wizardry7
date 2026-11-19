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
 */

#ifndef WIZARDRY7_TEST_DATADIR_H
#define WIZARDRY7_TEST_DATADIR_H

#include "common/archive.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/path.h"
#include "common/str.h"
#include "common/stream.h"

#include "wizardry7/datadir.h"

#include <cxxtest/TestSuite.h>

#include "../system/null_osystem.h"

#if NULL_OSYSTEM_IS_AVAILABLE
#define WIZ7_DATADIR_TEST_AVAILABLE 1
#else
#define WIZ7_DATADIR_TEST_AVAILABLE 0
#endif

/**
 * Launcher path handling: Wizardry Gold keeps its data in Basedata/, and the
 * game must be addable both from the install root and from Basedata/ itself.
 *
 * The tests build small directory trees under the test build directory
 * (test/engine-data, gitignored); the files are empty placeholders, so no game
 * data is needed.
 */
class Wizardry7DataDirectoryTestSuite : public CxxTest::TestSuite {
public:
	void setUp() {
#if WIZ7_DATADIR_TEST_AVAILABLE
		Common::install_null_g_system();
		SearchMan.clear();
#endif
	}

	void tearDown() {
#if WIZ7_DATADIR_TEST_AVAILABLE
		SearchMan.clear();
		Common::uninstall_null_g_system();
#endif
	}

	void testInstallRoot() {
		if (!haveTree())
			return;

		// The launcher was pointed at the install root: the data files are one
		// directory down, in Basedata/.
		const Common::FSNode gamePath = treeNode("root");
		const Common::FSNode data = Wizardry7::findDataDirectory(gamePath);
		TS_ASSERT(data.exists());
		TS_ASSERT_EQUALS(data.getName(), Common::String("Basedata"));

		// Engine::initializePath() registers the game path itself.
		SearchMan.addDirectory(gamePath);
		TS_ASSERT(Wizardry7::registerDataDirectory(gamePath));
		TS_ASSERT(SearchMan.hasArchive("Basedata"));
		TS_ASSERT(canOpen("MSG.GLD"));
		TS_ASSERT(canOpen("SCENARIO.GLD"));

		// A second registration must not add the directory again.
		TS_ASSERT(!Wizardry7::registerDataDirectory(gamePath));
	}

	void testBasedataDirectory() {
		if (!haveTree())
			return;

		// The launcher was pointed at Basedata/ itself: the data files are in
		// the selected directory, so only the game path is registered.
		const Common::FSNode gamePath = treeNode("basedata");
		const Common::FSNode data = Wizardry7::findDataDirectory(gamePath);
		TS_ASSERT(data.exists());
		TS_ASSERT(data.getPath() == gamePath.getPath());

		SearchMan.addDirectory(gamePath);
		TS_ASSERT(!Wizardry7::registerDataDirectory(gamePath));
		TS_ASSERT(canOpen("MSG.GLD"));
		TS_ASSERT(canOpen("SCENARIO.GLD"));
	}

	void testBasedataDirectoryCase() {
		if (!haveTree())
			return;

		// Directory names copied from a case-insensitive file system may be
		// lower-case; the detector matches the name case-insensitively too.
		const Common::FSNode gamePath = treeNode("lower");
		const Common::FSNode data = Wizardry7::findDataDirectory(gamePath);
		TS_ASSERT(data.exists());
		TS_ASSERT_EQUALS(data.getName(), Common::String("basedata"));

		SearchMan.addDirectory(gamePath);
		TS_ASSERT(Wizardry7::registerDataDirectory(gamePath));
		TS_ASSERT(SearchMan.hasArchive("basedata"));
		TS_ASSERT(canOpen("MSG.GLD"));
	}

	void testDosDirectory() {
		if (!haveTree())
			return;

		// The DOS build is flat: there is nothing to add below the game path.
		const Common::FSNode gamePath = treeNode("dos");
		const Common::FSNode data = Wizardry7::findDataDirectory(gamePath);
		TS_ASSERT(data.exists());
		TS_ASSERT(data.getPath() == gamePath.getPath());

		SearchMan.addDirectory(gamePath);
		TS_ASSERT(!Wizardry7::registerDataDirectory(gamePath));
		TS_ASSERT(canOpen("MSG.DBS"));
		TS_ASSERT(canOpen("SCENARIO.DBS"));
	}

	void testEmptyDirectory() {
		if (!haveTree())
			return;

		const Common::FSNode gamePath = treeNode("empty");
		TS_ASSERT(!Wizardry7::findDataDirectory(gamePath).exists());
		TS_ASSERT(!Wizardry7::registerDataDirectory(gamePath));
	}

private:
	/** Directory holding the trees scanned by the tests (gitignored build dir). */
	static Common::Path scratchDirectory() {
		return Common::Path("test/engine-data").appendComponent("wiz7-datadir");
	}

	static Common::FSNode treeNode(const char *name) {
		return Common::FSNode(scratchDirectory().appendComponent(name));
	}

	static bool createDirectory(const Common::FSNode &node) {
		return node.exists() || node.createDirectory();
	}

	static bool createEmptyFile(const Common::FSNode &directory, const char *name) {
		Common::FSNode file = directory.getChild(name);
		if (file.exists())
			return true;

		Common::SeekableWriteStream *stream = file.createWriteStream();
		if (!stream)
			return false;
		stream->writeByte(0);
		delete stream;
		return true;
	}

	static bool buildTree() {
		const Common::FSNode root = treeNode("root");
		const Common::FSNode basedata = treeNode("basedata");
		const Common::FSNode lower = treeNode("lower");
		const Common::FSNode dos = treeNode("dos");

		return createDirectory(Common::FSNode("test/engine-data")) &&
			createDirectory(Common::FSNode(scratchDirectory())) &&
			createDirectory(root) &&
			createDirectory(root.getChild("Basedata")) &&
			createDirectory(basedata) &&
			createDirectory(lower) &&
			createDirectory(lower.getChild("basedata")) &&
			createDirectory(dos) &&
			createDirectory(treeNode("empty")) &&
			createEmptyFile(root.getChild("Basedata"), "MSG.GLD") &&
			createEmptyFile(root.getChild("Basedata"), "SCENARIO.GLD") &&
			createEmptyFile(basedata, "MSG.GLD") &&
			createEmptyFile(basedata, "SCENARIO.GLD") &&
			createEmptyFile(lower.getChild("basedata"), "MSG.GLD") &&
			createEmptyFile(dos, "MSG.DBS") &&
			createEmptyFile(dos, "SCENARIO.DBS");
	}

	/** Build the trees once; CxxTest runs without exceptions, so skip instead. */
	static bool haveTree() {
		static const bool built = buildTree();
		if (!built)
			TS_WARN(Common::String("cannot create test tree under ") + scratchDirectory().toString());
		return built;
	}

	static bool canOpen(const char *name) {
		Common::File file;
		return file.open(Common::Path(name));
	}
};

#endif // WIZARDRY7_TEST_DATADIR_H
