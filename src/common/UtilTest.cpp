#include <gtest/gtest.h>

#include "Util.h"

namespace Util {
	namespace {
		TEST(FPSCounterTest, FPS5000)
		{
			float counter = 0;
			for (int i = 9999; i--; ) {
				UpdateFPSCounter(1.0f, 0, counter);
				UpdateFPSCounter(1.0f, 0, counter);
				UpdateFPSCounter(1.0f, 0, counter);
				UpdateFPSCounter(1.0f, 0, counter);
				UpdateFPSCounter(1.0f, 1, counter);
			}
			EXPECT_NEAR(5000, counter, /*tolerance=*/10);
		}

		TEST(FPSCounterTest, FPS40)
		{
			float counter = 23;
			for (int i = 99; i--; )
			{
				// 2 frames every 50 ms
				UpdateFPSCounter(1.0f, 20, counter);
				UpdateFPSCounter(1.0f, 30, counter);
			}
			EXPECT_NEAR(40, counter, /*tolerance=*/1);
		}

		TEST(FPSCounterTest, LongFrame)
		{
			float counter = 60;
			UpdateFPSCounter(0.5f, 500, counter);
			// 0.5 * 60 + 0.5 * 2
			EXPECT_FLOAT_EQ(31, counter);
		}
	} // namespace
} // namespace Util
