/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2022, Daemon Developers
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
#include <gmock/gmock.h>
#include "String.h"

namespace Str {
namespace {
using ::testing::Eq;

TEST(IsIPrefix, EqualArguments)
{
    // The case with two literals could behave differently, since the pointers are the same
    EXPECT_TRUE(IsIPrefix("", ""));
    EXPECT_TRUE(IsIPrefix("", std::string()));
    EXPECT_TRUE(IsIPrefix("DIRECT, indirect, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES",
                          "DIRECT, INDIRECT, INCIDENTAL, SPECIAL, exemplary, OR CONSEQUENTIAL DAMAGES"));
}

TEST(IsIPrefix, NotPrefix)
{
    EXPECT_FALSE(IsIPrefix("tnhhtnshtnshtns", ""));
    EXPECT_FALSE(IsIPrefix("Redistributions in binary form must", "redistributions in binary form mus"));
    EXPECT_FALSE(IsIPrefix("IN NO EVENT SHALL DAEMON DEVELOPERS", "IN NO EVENT SHALL DAEMON DE.................."));
}

TEST(IsIPrefix, ProperPrefix)
{
    EXPECT_TRUE(IsIPrefix("", "%"));
    EXPECT_TRUE(IsIPrefix("proCUREMENT OF SUBSTITUTE", "PROCUREMENT of SUBSTITUTE GOODS"));
}

TEST(IsISuffix, EqualArguments)
{
    // The case with two literals could behave differently, since the pointers are the same
    EXPECT_TRUE(IsISuffix("", ""));
    EXPECT_TRUE(IsISuffix("", std::string()));
    EXPECT_TRUE(IsIPrefix("DIRECT, indirect, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES",
                          "DIRECT, INDIRECT, INCIDENTAL, SPECIAL, exemplary, OR CONSEQUENTIAL DAMAGES"));
}

TEST(IsISuffix, NotSuffix)
{
    EXPECT_FALSE(IsISuffix("tnhhtnshtnshtns", ""));
    EXPECT_FALSE(IsISuffix("Redistributions in binary form must", "redistributions in binary form mus"));
    EXPECT_FALSE(IsISuffix("daemon DEVELOPERS", "IN NO EVENT SHALL DEVELOPERS"));
}

TEST(IsISuffix, ProperSuffix)
{
    EXPECT_TRUE(IsISuffix("", "%"));
    EXPECT_TRUE(IsISuffix("of SUBSTITUTE GOODS", "PROCUREMENT OF SUBSTITUTE goodS"));
}

TEST(LongestIPrefixSize, EqualArguments)
{
    EXPECT_THAT(LongestIPrefixSize("", ""), Eq(0));
    EXPECT_THAT(LongestIPrefixSize("ITNESS for A PARTICULAR", "ITNESS FOR A parTICULAR"), Eq(23));
}

TEST(LongestIPrefixSize, OneArgumentIsPrefix)
{
    EXPECT_THAT(LongestIPrefixSize("", "dh"), Eq(0));
    EXPECT_THAT(LongestIPrefixSize("dh", ""), Eq(0));
    EXPECT_THAT(LongestIPrefixSize("EVEN IF ADVISED OF", "even if advised"), Eq(15));
    EXPECT_THAT(LongestIPrefixSize("even if advised", "EVEN IF ADVISED OF"), Eq(15));
}

TEST(LongestIPrefixSize, MismatchedArguments)
{
    EXPECT_THAT(LongestIPrefixSize("cat", "mat"), Eq(0));
    EXPECT_THAT(LongestIPrefixSize("granger", "GRavity"), Eq(3));
}

} // namespace
} // namespace Cmd

