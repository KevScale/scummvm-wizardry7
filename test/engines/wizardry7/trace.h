#include "common/memstream.h"
#include "common/str.h"

#include "wizardry7/creation.h"
#include "wizardry7/maze.h"
#include "wizardry7/rng.h"
#include "wizardry7/trace.h"

#include <cxxtest/TestSuite.h>

class Wizardry7TraceTestSuite : public CxxTest::TestSuite {
public:
	// The expected lines below come from the documented generator in
	// docs/engine/rng.md: from the default seed (3000, 1, 29999) the first
	// rand(100) draws are 71, 42, 74 with the pre-call states used here.
	void testRngDrawsAreIndexed() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

		TS_ASSERT_EQUALS(rng.getRandom(100), 71u);
		TS_ASSERT_EQUALS(rng.getRandom(100), 42u);
		TS_ASSERT_EQUALS(trace.getDrawIndex(), 2u);
		TS_ASSERT_EQUALS(trace.getLineCount(), 2u);
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=100 s=3000,1,29999 -> 71"));
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 RNG n=100 s=28696,172,5566 -> 42"));
	}

	void testNoRecordingWhileDisabled() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);

		rng.getRandom(100);
		trace.addEvent("msg 100");
		TS_ASSERT_EQUALS(trace.getLineCount(), 0u);
		TS_ASSERT_EQUALS(trace.getDrawIndex(), 0u);
		TS_ASSERT(!trace.isEnabled());

		trace.start(3000, 1, 29999); // clears whatever was there
		rng.getRandom(100);
		trace.stop();
		rng.getRandom(100); // not recorded
		TS_ASSERT_EQUALS(trace.getLineCount(), 1u);
		// The draw taken while disabled still advanced the generator, so the
		// recorded line starts from the second state of the stream.
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=100 s=28696,172,5566 -> 42"));
	}

	void testEventsCarryTheDrawIndex() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

		rng.getRandom(6);
		trace.addEvent("msg 1050");
		rng.getRandom(6);
		trace.addEvent("damage target=1 amount=17");
		TS_ASSERT_EQUALS(trace.getLineCount(), 4u);
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=6 s=3000,1,29999 -> 1"));
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 msg 1050"));
		TS_ASSERT_EQUALS(trace.getLine(2), Common::String("1 RNG n=6 s=28696,172,5566 -> 0"));
		TS_ASSERT_EQUALS(trace.getLine(3), Common::String("2 damage target=1 amount=17"));
	}

	void testZeroModulusKeepsTheIndex() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

		TS_ASSERT_EQUALS(rng.getRandom(0), 0u);
		TS_ASSERT_EQUALS(trace.getDrawIndex(), 0u);
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=0 s=3000,1,29999 -> 0"));
	}

	void testReseedIsRecorded() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

		rng.getRandom(10);
		rng.reset(1, 2, 3);
		TS_ASSERT_EQUALS(trace.getLineCount(), 2u);
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 rngseed 1 2 3"));
		TS_ASSERT_EQUALS(trace.getSeed(0), 3000);
	}

	void testDumpText() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);
		rng.getRandom(100);
		trace.addEvent("msg 100");

		Common::MemoryWriteStreamDynamic out(DisposeAfterUse::NO);
		TS_ASSERT(trace.write(out));
		TS_ASSERT_EQUALS(out.size(), (int64)Common::String(
			"wizardry7-trace 1\n"
			"seed 3000 1 29999\n"
			"0 RNG n=100 s=3000,1,29999 -> 71\n"
			"1 msg 100\n").size());
		const Common::String text((const char *)out.getData(), out.size());
		TS_ASSERT_EQUALS(text, Common::String(
			"wizardry7-trace 1\n"
			"seed 3000 1 29999\n"
			"0 RNG n=100 s=3000,1,29999 -> 71\n"
			"1 msg 100\n"));
	}

	void testCreationRecordsTheResult() {
		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

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

		Wizardry7::CreatedCharacter created;
		TS_ASSERT(flow.buildCharacter(rng, created, &trace));

		// Age, class HP roll and portrait draw; then the result event.
		TS_ASSERT_EQUALS(trace.getDrawIndex(), 3u);
		TS_ASSERT_EQUALS(trace.getLineCount(), 4u);
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=1000 s=3000,1,29999 -> 371"));
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 RNG n=5 s=28696,172,5566 -> 2"));
		TS_ASSERT_EQUALS(trace.getLine(2), Common::String("2 RNG n=19 s=3438,29584,6207 -> 8"));
		TS_ASSERT_EQUALS(trace.getLine(3), Common::String("3 create race=0 sex=0 class=0 age=6941 hp=12 portrait=8"));
	}

	void testEncounterRecordsTheSelection() {
		Common::Array<byte> data;
		data.resize(Wizardry7::MazeRecord::kRecordSize);
		memset(data.data(), 0, data.size());
		for (uint i = 0; i < 144; ++i)
			data[i] = 42;
		data[0xAB0] = 0; // slot 0/2 share group 0
		data[0xAC0 + 2] = 100; // slot 2 always rolls an encounter
		Wizardry7::MazeRecord maze;
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));

		Wizardry7::TraceLog trace;
		Wizardry7::WizardryRNG rng;
		rng.setTrace(&trace);
		trace.start(3000, 1, 29999);

		TS_ASSERT_EQUALS(maze.selectEncounterMonster(2, rng, &trace), 42);
		TS_ASSERT_EQUALS(trace.getLineCount(), 4u);
		TS_ASSERT_EQUALS(trace.getLine(0), Common::String("0 RNG n=100 s=3000,1,29999 -> 71"));
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 RNG n=100 s=28696,172,5566 -> 42"));
		TS_ASSERT_EQUALS(trace.getLine(2), Common::String("2 RNG n=100 s=3438,29584,6207 -> 74"));
		TS_ASSERT_EQUALS(trace.getLine(3), Common::String("3 encounter slot=2 monster=42"));

		// A failed chance check still appears in the log.
		data[0xAC0 + 3] = 0;
		TS_ASSERT(maze.loadFromData(data.data(), data.size()));
		trace.start(3000, 1, 29999);
		TS_ASSERT_EQUALS(maze.selectEncounterMonster(3, rng, &trace), -1);
		TS_ASSERT_EQUALS(trace.getLine(1), Common::String("1 encounter slot=3 none"));
	}

	void testLineCountIsBounded() {
		Wizardry7::TraceLog trace;
		trace.start(0, 0, 0);
		for (uint i = 0; i < Wizardry7::TraceLog::kMaxLines + 2; ++i)
			trace.addEvent("noop");
		TS_ASSERT_EQUALS(trace.getLineCount(), Wizardry7::TraceLog::kMaxLines);

		Common::MemoryWriteStreamDynamic out(DisposeAfterUse::NO);
		TS_ASSERT(trace.write(out));
		const Common::String text((const char *)out.getData(), out.size());
		TS_ASSERT(text.hasSuffix(Common::String("truncated dropped=2\n")));
	}
};
