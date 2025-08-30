/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2025, Daemon Developers
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

#include "engine/renderer/gl_shader.h"

namespace {

class MaterialUniformPackingTestShaderBase : public GLShader
{
public:
    MaterialUniformPackingTestShaderBase()
       : GLShader("MaterialUniformPackingTestShaderBase", 0, true, "", "") {}

    std::vector<GLUniform*> GetUniforms()
    {
        PostProcessUniforms();
        return _materialSystemUniforms;
    }
};

template<typename U, typename ShaderT>
GLUniform* Get(ShaderT& shader)
{
   // assume U has a single base class of GLUniform
   return reinterpret_cast<GLUniform*>(static_cast<U*>(&shader));
}

TEST(MaterialUniformPackingTest, OneMatrix)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_ModelViewMatrix //mat4
    {
    public:
        Shader1() : u_ModelViewMatrix(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 16u);
    ASSERT_EQ(uniforms.size(), 1);
    EXPECT_EQ(uniforms[0], Get<u_ModelViewMatrix>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 16u);
}

TEST(MaterialUniformPackingTest, TwoFloats)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_DeformMagnitude, //float
                    public u_InverseGamma //float
    {
    public:
        Shader1() : u_DeformMagnitude(this), u_InverseGamma(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 4u);
    ASSERT_EQ(uniforms.size(), 2);
    EXPECT_EQ(uniforms[0], Get<u_DeformMagnitude>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 1u);
    EXPECT_EQ(uniforms[1], Get<u_InverseGamma>(shader1));
    EXPECT_EQ(uniforms[1]->_std430Size, 3u);
}

TEST(MaterialUniformPackingTest, Vec3Handling)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_DeformMagnitude, //float
                    public u_SpecularExponent, //vec2
                    public u_FogColor, //vec3
                    public u_blurVec //vec3
    {
    public:
        Shader1() : u_DeformMagnitude(this),
                    u_SpecularExponent(this),
                    u_FogColor(this),
                    u_blurVec(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 12u);
    ASSERT_EQ(uniforms.size(), 4);
    EXPECT_EQ(uniforms[0], Get<u_FogColor>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 3u);
    EXPECT_EQ(uniforms[1], Get<u_DeformMagnitude>(shader1));
    EXPECT_EQ(uniforms[1]->_std430Size, 1u);
    EXPECT_EQ(uniforms[2], Get<u_blurVec>(shader1));
    EXPECT_EQ(uniforms[2]->_std430Size, 4u);
    EXPECT_EQ(uniforms[3], Get<u_SpecularExponent>(shader1));
    EXPECT_EQ(uniforms[3]->_std430Size, 4u);
}

} // namespace
