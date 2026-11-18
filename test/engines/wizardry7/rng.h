#include "wizardry7/rng.h"

#include <cxxtest/TestSuite.h>

class Wizardry7RNGTestSuite : public CxxTest::TestSuite {
public:
	void test_default_seed() {
		Wizardry7::WizardryRNG rng;
		TS_ASSERT_EQUALS(rng.getSeed(0), 3000);
		TS_ASSERT_EQUALS(rng.getSeed(1), 1);
		TS_ASSERT_EQUALS(rng.getSeed(2), 29999);
	}

	void test_rand16_sequence() {
		// Expected values from the validated reference implementation and the
		// live game trace (docs/engine/rng.md in the RE repository).
		static const uint16 expected[10] = {
			11371, 12942, 21174, 12032, 23123, 10889, 19913, 18767, 13539, 18365
		};
		Wizardry7::WizardryRNG rng;
		for (int i = 0; i < 10; ++i)
			TS_ASSERT_EQUALS(rng.rand16(), expected[i]);
	}

	void test_rand_100_sequence() {
		static const uint32 expected[20] = {
			71, 42, 74, 32, 23, 89, 13, 67, 39, 65,
			99, 70, 99, 55, 80, 30, 64, 99, 12, 14
		};
		Wizardry7::WizardryRNG rng;
		for (int i = 0; i < 20; ++i)
			TS_ASSERT_EQUALS(rng.getRandom(100), expected[i]);
	}

	void test_zero_range() {
		Wizardry7::WizardryRNG rng;
		TS_ASSERT_EQUALS(rng.getRandom(0), 0);
		TS_ASSERT_EQUALS(rng.getSeed(0), 3000);
	}

	void test_reset_seed() {
		Wizardry7::WizardryRNG rng;
		rng.reset(0x1234, 0x5678, 0x9ABC);
		TS_ASSERT_EQUALS(rng.getSeed(0), 0x1234);
		TS_ASSERT_EQUALS(rng.getSeed(1), 0x5678);
		TS_ASSERT_EQUALS(rng.getSeed(2), 0x9ABC);
	}
};
