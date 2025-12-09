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
#include "engine/framework/CvarSystem.h"

namespace Cvar {
namespace {

TEST(ModifiedCvarTest, Normal)
{
    Modified<Cvar<int>> cv("test_modified", "desc", NONE, 3);
    Util::optional<int> modified = cv.GetModifiedValue();
    ASSERT_TRUE(modified); // modified flag is true on birth
    ASSERT_EQ(modified.value(), 3);

    // now cleared
    ASSERT_FALSE(cv.GetModifiedValue());
    ASSERT_FALSE(cv.GetModifiedValue());

    // test setting via the cvar object
    cv.Set(9);
    modified = cv.GetModifiedValue();
    ASSERT_TRUE(modified);
    ASSERT_EQ(modified.value(), 9);

    ASSERT_FALSE(cv.GetModifiedValue());

    // test setting externally
    SetValue("test_modified", "1");
    modified = cv.GetModifiedValue();
    ASSERT_TRUE(modified);
    ASSERT_EQ(modified.value(), 1);

    ASSERT_FALSE(cv.GetModifiedValue());

    // test that invalid set is ignored
    SetValue("test_modified", "a");
    ASSERT_FALSE(cv.GetModifiedValue());
}

TEST(ModifiedCvarTest, Latch)
{
    Modified<Cvar<float>> cv("test_modifiedLatched", "desc", NONE, 1.5f);
    Latch(cv);
    ASSERT_TRUE(cv.GetModifiedValue());
    ASSERT_FALSE(cv.GetModifiedValue());

    cv.Set(-5.0f);
    ASSERT_EQ(cv.Get(), 1.5f);

    // Clear latch flag. It's probably true now because the implementation of latched cvars
    // sets them to the new value and back to the current value, but let's not make this
    // part of the contract.
    cv.GetModifiedValue();

    // Make sure modified flag is set upon unlatching
    Latch(cv);
    Util::optional<float> modified = cv.GetModifiedValue();
    ASSERT_TRUE(modified);
    ASSERT_EQ(modified.value(), -5.0f);
}

} // namespace Cvar
} // namespace
