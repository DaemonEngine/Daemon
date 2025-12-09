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
#include <gmock/gmock.h>

#include "cm_public.h"
#include "common/FileSystem.h"

namespace {

using ::testing::FloatNear;
using ::testing::Pointwise;

constexpr int contentmask = ~0;
constexpr int skipmask = 0;

// To check whether some patch tests are really hitting patches and not brushes,
// it can be helpful to run the tests with `-set cm_noCurves 1` on the command line
class TraceTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        const FS::PakInfo* pak = FS::FindPak("testdata", "src");
        if (!pak) {
            FAIL() << "Test data not available - some tests will be skipped. Please add daemon/pkg/ to the pak path";
        }
        FS::PakPath::LoadPak(*pak);
        CM_LoadMap("plat23_1.13.4");
    }

    static void TearDownTestSuite()
    {
        CM_ClearMap();
    }
};

// There is non-intersecting brush with fraction exactly 0 and an intersecting at the start brush
TEST_F(TraceTest, StartsolidWithZeroFraction)
{
    trace_t tr;
    vec3_t start{ -243.4, 1708.8, 27.1 };
    vec3_t end{ -180, 1700.8, 27.1 };
    vec3_t mins{ -23, -23, -22 };
    vec3_t maxs{ 23, 23, 14 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_TRUE(tr.startsolid);
    EXPECT_FALSE(tr.allsolid);
    EXPECT_FLOAT_EQ(tr.fraction, 0.0f);
}

TEST_F(TraceTest, StartsolidWithZeroFraction2)
{
    trace_t tr;
    vec3_t start{ 143.9, 1880.8, -126.7 };
    vec3_t end{ 160.9, 1863.8, -126.7 };
    vec3_t mins{ -32, -32, -22 };
    vec3_t maxs{ 32, 32, 70 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_TRUE(tr.allsolid);
}

// box starts overlapping a facet then gets out of it (going through the front side)
TEST_F(TraceTest, StartInPatch)
{
    trace_t tr;
    vec3_t start{ 1617, 2020, 115 };
    vec3_t end{ 1617, 2020, 125 };
    vec3_t mins{ -1, -1, -1 };
    vec3_t maxs{ 1, 1, 5 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_FALSE(tr.startsolid); // startsolid not implemented for patces
    EXPECT_EQ(1.0f, tr.fraction);
}

// interior side of facet is non-colliding
// start position is completely inside the facets, not overlapping any edges
TEST_F(TraceTest, GoThroughPatchBackSide)
{
    trace_t tr;
    vec3_t start{ 1617, 2020, 125 };
    vec3_t end{ 1617, 2020, 80 };
    vec3_t mins{ -1, -1, -1 };
    vec3_t maxs{ 1, 1, 5 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_FALSE(tr.startsolid);
    EXPECT_EQ(1.0f, tr.fraction);
}

// once inside a facet you can move freely
TEST_F(TraceTest, AllInPatch)
{
    trace_t tr;
    vec3_t start{ 1774.7, 1113.7, 150.1 };
    vec3_t end{ 1784.7, 1113.7, 150.1 };
    vec3_t mins{ -32, -32, -22 };
    vec3_t maxs{ 32, 32, 70 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_FALSE(tr.startsolid);
    EXPECT_EQ(1.0f, tr.fraction);
}

// The patch planes (produced from a cGrid_t) come out fairly differently if 80-bit x87 math is used
constexpr float PATCH_PLANE_NORMAL_ATOL = 5.0e-6;
constexpr float PATCH_PLANE_DIST_ATOL = 8.0e-3;
constexpr float PATCH_TRACE_FRACTION_ATOL = 3.0e-6;

TEST_F(TraceTest, PointHitPatch)
{
    trace_t tr;
    vec3_t start{ -1990, 1855, 111 };
    vec3_t end{ -1990, 1855, 150 };

    CM_BoxTrace(&tr, start, end, nullptr, nullptr, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_FALSE(tr.startsolid);
    EXPECT_NEAR(tr.fraction, 0.426183, PATCH_TRACE_FRACTION_ATOL);
    EXPECT_EQ(tr.contents, CONTENTS_SOLID);
    const vec3_t expectedPlaneNormal = {0, .2425355, -0.970142};
    EXPECT_THAT(tr.plane.normal, Pointwise(FloatNear(PATCH_PLANE_NORMAL_ATOL), expectedPlaneNormal));
    EXPECT_NEAR(tr.plane.dist, 325.9677, PATCH_PLANE_DIST_ATOL);
}

TEST_F(TraceTest, BoxHitPatch)
{
    trace_t tr;
    vec3_t start{ -1990, 1855, 70 };
    vec3_t end{ -1990, 1855, 150 };
    vec3_t mins{ -9, -9, -30 };
    vec3_t maxs{ 9, 9, 40 };

    CM_BoxTrace(&tr, start, end, mins, maxs, CM_InlineModel(0), contentmask, skipmask, traceType_t::TT_AABB);
    EXPECT_EQ(CM_CheckTraceConsistency(start, end, contentmask, skipmask, tr), "");

    EXPECT_FALSE(tr.startsolid);
    EXPECT_NEAR(tr.fraction, 0.192139, PATCH_TRACE_FRACTION_ATOL);
    EXPECT_EQ(tr.contents, CONTENTS_SOLID);
    const vec3_t expectedPlaneNormal = {0, .2425355, -0.970142};
    EXPECT_THAT(tr.plane.normal, Pointwise(FloatNear(PATCH_PLANE_NORMAL_ATOL), expectedPlaneNormal));
    EXPECT_NEAR(tr.plane.dist, 362.105, PATCH_PLANE_DIST_ATOL);
}

} // namespace
