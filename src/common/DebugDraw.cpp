/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2015, Daemon Developers
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

#include "Common.h"

// TODO remove that when in common
#include "DebugDraw.h"

namespace DebugDraw {

    void FlushLines(std::vector<LineData>);
    void FlushSpheres(std::vector<SphereData>);

    static std::vector<LineData> lines;
    static std::vector<SphereData> spheres;
    static LineData dummyLine;
    static SphereData dummySphere;

    void Flush() {
        //TODO
#ifndef BUILD_VM
        FlushLines(std::move(lines));
        FlushSpheres(std::move(spheres));
#endif
    }

    Drawer::Drawer(Str::StringRef name): enabled("debugDraw." + name, "should " + name + " debug be drawn", Cvar::CHEAT, false) {
    }

    Drawer::LineContinuation Drawer::AddLine(Vec3 start, Vec3 end) {
        if (enabled.Get()) {
            lines.emplace_back(start, end);
            return LineContinuation(lines.back());
        } else {
            return LineContinuation(dummyLine);
        }
    }

    Drawer::SphereContinuation Drawer::AddSphere(Vec3 center, float radius) {
        if (enabled.Get()) {
            spheres.emplace_back(center, radius);
            return SphereContinuation(spheres.back());
        } else {
            return SphereContinuation(dummySphere);
        }
    }

    Drawer::LineContinuation::LineContinuation(LineData& data) : BaseContinuation(data) {
    }
 
    Drawer::LineContinuation Drawer::LineContinuation::Width(float width) {
        data.width = width;
        return *this;
    }

    Drawer::SphereContinuation::SphereContinuation(SphereData& data) : BaseContinuation(data) {
    }
}
