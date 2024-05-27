/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2024, Daemon Developers
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

#include "common/Common.h"

namespace Math {
namespace {

// Uncomment this and the tests should fail with GCC or Clang in release mode :-)
//#define IsFinite std::isfinite

TEST(IsFiniteTest, Float)
{
    ASSERT_TRUE(IsFinite(-0.0f));
    ASSERT_TRUE(IsFinite(std::numeric_limits<float>::max()));
    ASSERT_TRUE(IsFinite(std::numeric_limits<float>::min()));
    ASSERT_TRUE(IsFinite(123.45f));

    ASSERT_FALSE(IsFinite(std::stof("nan")));
    ASSERT_FALSE(IsFinite(std::stof("inf")));
    ASSERT_FALSE(IsFinite(std::stof("-inf")));
}

TEST(IsFiniteTest, Double)
{
    ASSERT_TRUE(IsFinite(-0.0));
    ASSERT_TRUE(IsFinite(std::numeric_limits<double>::max()));
    ASSERT_TRUE(IsFinite(std::numeric_limits<double>::min()));
    ASSERT_TRUE(IsFinite(123.45));

    ASSERT_FALSE(IsFinite(std::stod("nan")));
    ASSERT_FALSE(IsFinite(std::stod("inf")));
    ASSERT_FALSE(IsFinite(std::stod("-inf")));
}

} // namespace Math
} // namespace
