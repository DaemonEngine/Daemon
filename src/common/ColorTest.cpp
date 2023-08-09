/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2023, Daemon Developers
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
#include "Common.h"

namespace Color {
namespace {

TEST(StripColorTest, ReturningString)
{
   std::string noValidCodes =
       "^Q^q^~^xgff^x1$3^xa1W^##23456^#1j3456^#12 456^#123.56^#1234m6^#12345G^";
   EXPECT_EQ(noValidCodes, StripColors(noValidCodes));

   std::string onlyColors = "^xF1A^1^0^O^9^a^;^*^xeE2^#123Aa5^#3903ff";
   EXPECT_EQ("", StripColors(onlyColors));
}

TEST(StripColorTest, ToBuffer)
{
    char bigbuf[99];
    StripColors("^6foo^^bar", bigbuf, sizeof(bigbuf));
    EXPECT_EQ("foo^bar", std::string(bigbuf));

    char smallbuf[3];
    StripColors("ab^x123c", smallbuf, sizeof(smallbuf));
    EXPECT_EQ("ab", std::string(smallbuf));

    char justright[7];
    StripColors("dretc^#123456h", justright, sizeof(justright));
    EXPECT_EQ("dretch", std::string(justright));
}

} // namespace
} // namespace Color

