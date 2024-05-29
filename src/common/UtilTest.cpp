/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2023-2024, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Daemon developers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include <gtest/gtest.h>

#include "Util.h"

namespace Util {
	namespace {
		TEST(UtilTest, BitCast)
		{
			ASSERT_EQ(bit_cast<float>(0x3e200000), 0.15625f);
			ASSERT_EQ(bit_cast<int>(0.15625f), 0x3e200000);

			ASSERT_EQ(bit_cast<double>(0xC037000000000000U), -23.0);
			ASSERT_EQ(bit_cast<uint64_t>(-23.0), 0xC037000000000000);
		}

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
