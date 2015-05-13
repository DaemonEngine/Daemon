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

#include "DebugDrawSystem.h"

namespace DebugDraw {

    void Render(const LineData& line) {
    }

    void Render(const SphereData& sphere) {
    }

    template<typename T>
    struct Record {
        struct Persistent {
            Persistent(T data, Sys::SteadyClock::time_point endTime) : data(data), endTime(endTime) {}
            T data;
            Sys::SteadyClock::time_point endTime;
        };

        std::vector<Persistent> persistent;
        std::vector<std::vector<T>> flushes;

        void Frame() {
            auto now = Sys::SteadyClock::now();

            for (auto& flush : flushes) {
                for (auto& data : flush) {
                    if (data.lifetime == 0) {
                        Render(data);
                    } else {
                        persistent.emplace_back(data, now + std::chrono::milliseconds(data.lifetime));
                    }
                }
            }
            
            persistent.erase(std::remove_if(persistent.begin(), persistent.end(), [&] (Persistent& p) -> bool {
                return p.endTime < now;
            }));

            for (auto& p : persistent) {
                Render(p.data);
            }
        }
    };

    static Record<LineData> lines;
    static Record<SphereData> spheres;

    void FlushLines(std::vector<LineData> flush) {
        lines.flushes.push_back(std::move(flush));
    }

    void FlushSpheres(std::vector<SphereData> flush) {
        spheres.flushes.push_back(std::move(flush));
    }

    void Frame() {
        lines.Frame();
        spheres.Frame();
    }

}
