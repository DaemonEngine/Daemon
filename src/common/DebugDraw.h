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

#ifndef COMMON_DEBUG_DRAW_H_
#define COMMON_DEBUG_DRAW_H_

//TODO doc
//TODO more primitives
//TODO lifetime
//TODO DoDrawCode();

namespace DebugDraw {

    void Flush();

    struct BaseData;
    struct LineData;
    struct SphereData;

    class Drawer {
    public:
        Drawer(Str::StringRef name);

        template<typename Child,typename Data>
        struct BaseContinuation {
            BaseContinuation(Data& data);
     
            Child Lifetime(int lifetime);
            Child Color(Vec4 color);
     
        protected:
            Data& data;
        };
     
        struct LineContinuation : public BaseContinuation<LineContinuation, LineData> {
            LineContinuation(LineData& data);
     
            LineContinuation Width(float width);
        };

        struct SphereContinuation : public BaseContinuation<LineContinuation, SphereData> {
            SphereContinuation(SphereData& data);
        };

        LineContinuation AddLine(Vec3 start, Vec3 end);
        SphereContinuation AddSphere(Vec3 center, float radius);

    private:
        Cvar::Cvar<bool> enabled;
    };

    //

    struct BaseData {
        BaseData(): lifetime(0.0f), color(1.0f, 0.0f, 0.0f, 1.0f) {}
        int lifetime;
        Vec4 color;
    };

    struct LineData : public BaseData {
        LineData() : width(1.0f) {}
        LineData(Vec3 start, Vec3 end) : start(start), end(end), width(1.0f) {}
        Vec3 start;
        Vec3 end;
        float width;
    };

    struct SphereData : public BaseData {
        SphereData() {}
        SphereData(Vec3 center, float radius) : center(center), radius(radius) {}
        Vec3 center;
        float radius;
    };

    // Engine functions available everywhere

    void FlushLines(std::vector<LineData>);
    void FlushSpheres(std::vector<SphereData>);

    // Implementation of the templates

    template<typename Child, typename Data>
    Drawer::BaseContinuation<Child, Data>::BaseContinuation(Data& data): data(data) {
        static_assert(sizeof(BaseContinuation) == sizeof(Child), "");
    }
 
    template<typename Child, typename Data>
    Child Drawer::BaseContinuation<Child, Data>::Lifetime(int lifetime) {
        data.lifetime = lifetime;
        return *reinterpret_cast<Child*>(this);
    }
 
    template<typename Child, typename Data>
    Child Drawer::BaseContinuation<Child, Data>::Color(Vec4 color) {
        data.color = color;
        return *reinterpret_cast<Child*>(this);
    }
}

#endif // COMMON_DEBUG_DRAW_H_
