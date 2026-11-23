/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2026, Daemon Developers
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

#include "cm_patch.h"

namespace {

// Box position test against a single surface collide, equivalent to CM_PositionTestInSurfaceCollide
// for an AABB with half-size r centered at `origin`
bool BoxInSurfaceCollide(const cSurfaceCollide_t* sc, const vec3_t origin, float r)
{
    auto outside = [&](const cPlane_t& cplane, bool flip) {
        plane_t plane = cplane.plane;
        if (flip) {
            VectorNegate(plane.normal, plane.normal);
            plane.dist = -plane.dist;
        }
        float offset = r * (fabsf(plane.normal[0]) + fabsf(plane.normal[1]) + fabsf(plane.normal[2]));
        return DotProduct(plane.normal, origin) - plane.dist > offset;
    };

    for (int i = 0; i < sc->numFacets; i++) {
        const cFacet_t& facet = sc->facets[i];
        if (outside(sc->planes[facet.surfacePlane], false)) {
            continue;
        }
        int j;
        for (j = 0; j < facet.numBorders; j++) {
            if (outside(sc->planes[facet.borderPlanes[j]], facet.borderInward[j])) {
                break;
            }
        }
        if (j == facet.numBorders) {
            return true;
        }
    }
    return false;
}

// Generates the collision facets for a patch and checks that points sampled on all the triangles
// of the subdivided grid (which is the surface the facets approximate) collide with a small box.
// Points are packed as concatenated rows, as in CM_GeneratePatchCollide.
void ExpectNoHolesInPatch(int width, int height, const std::vector<std::array<float, 3>>& controlPoints)
{
    ASSERT_EQ(controlPoints.size(), size_t(width * height));
    auto points = std::make_unique<vec3_t[]>(controlPoints.size());
    for (size_t i = 0; i < controlPoints.size(); i++) {
        VectorCopy(controlPoints[i].data(), points[i]);
    }

    // Reproduce the subdivision of CM_GeneratePatchCollide to get the same final grid points
    cGrid_t grid;
    grid.width = width;
    grid.height = height;
    grid.wrapWidth = false;
    grid.wrapHeight = false;
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            VectorCopy(points[j * width + i], grid.points[i][j]);
        }
    }
    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);
    CM_TransposeGrid(&grid);
    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);

    const cSurfaceCollide_t* sc = CM_GeneratePatchCollide(width, height, points.get());

    constexpr float BOX_HALF_SIZE = 1.0f;
    constexpr int N = 6;
    int misses = 0;
    for (int i = 0; i < grid.width - 1; i++) {
        for (int j = 0; j < grid.height - 1; j++) {
            for (int tri = 0; tri < 2; tri++) {
                const float* a = tri ? grid.points[i + 1][j + 1] : grid.points[i][j];
                const float* b = tri ? grid.points[i][j + 1] : grid.points[i + 1][j];
                const float* c = tri ? grid.points[i][j] : grid.points[i + 1][j + 1];
                for (int u = 0; u <= N; u++) {
                    for (int v = 0; u + v <= N; v++) {
                        vec3_t q;
                        for (int k = 0; k < 3; k++) {
                            q[k] = (float(N - u - v) * a[k] + float(u) * b[k] + float(v) * c[k]) / N;
                        }
                        if (!BoxInSurfaceCollide(sc, q, BOX_HALF_SIZE)) {
                            misses++;
                            ADD_FAILURE() << "hole in patch at " << q[0] << " " << q[1] << " " << q[2];
                            if (misses > 5) return;
                        }
                    }
                }
            }
        }
    }
}

// staratcs example from https://github.com/DaemonEngine/Daemon/issues/1307
TEST(PatchCollideTest, NoHolesStaratcs)
{
    ExpectNoHolesInPatch(3, 3, {
        {-3176, -816, -1316}, {-3176, -816, -1372}, {-3198, -816, -1384},
        {-3176, -926.69, -1313.75}, {-3176, -987.80, -1380.93}, {-3198, -997.47, -1388.04},
        {-3176, -928, -1204}, {-3176, -984, -1204}, {-3198, -998, -1204},
    });
}

// From map metro-b1-2. Inside-facing side of tunnel arch
TEST(PatchCollideTest, NoHolesMetro)
{
    ExpectNoHolesInPatch(3, 7, {
        {-512, 152, 168}, {-512, 168, 168}, {-512, 194, 168}, {-512, 256, 168}, {-512, 318, 168}, {-512, 344, 168}, {-512, 360, 168},
        {-512, 152, 122}, {-512, 168, 122}, {-512, 192, 150.5}, {-512, 256, 160}, {-512, 320, 150.5}, {-512, 344, 122}, {-512, 360, 122},
        {-512, 152, 0}, {-512, 168, 0}, {-512, 186, 114}, {-512, 256, 152}, {-512, 326, 114}, {-512, 344, 0}, {-512, 360, 0},
    });
}

} // namespace
