/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2021, Daemon Developers
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

#include <GL/osmesa.h>

#include "framework/System.h"
#include "framework/Application.h"
#include "framework/CommandSystem.h"
#include "tr_local.h"




void GL_VertexAttribsState(uint32_t) {}
bool CL_WWWBadChecksum(const char*) { return false; }
void R_SyncRenderThread() {}
void GLSL_InitGPUShaders();
void GLimp_InitExtensions();

namespace Application {

using BufferType = char[4];
static ALIGNED(4, BufferType) buffer; // 1x1, 4 bytes per pixel

class ShaderTestApplication : public Application {
    public:
        void Frame() override {
            OSMesaContext context;
            if (!(context = OSMesaCreateContext(OSMESA_RGBA, nullptr)) ||
                !OSMesaMakeCurrent(context, buffer, GL_UNSIGNED_BYTE, 1, 1)) {
                 Sys::Error("Can't initialize OSMesa");
            }
            R_RegisterCvars();
            GLimp_InitExtensions();
            glConfig2.shadingLanguageVersion = 120; // TODO make a cvar
            GLSL_InitGPUShaders();
            Sys::Quit("GLSL shaders compiled successfully");
        }
};

INSTANTIATE_APPLICATION(ShaderTestApplication)

}
