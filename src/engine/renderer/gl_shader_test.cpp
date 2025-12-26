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

struct u_float_A : GLUniform1f { u_float_A(GLShader* s) : GLUniform1f(s, "u_float_A", MATERIAL_OR_PUSH) {} };
struct u_float_B : GLUniform1f { u_float_B(GLShader* s) : GLUniform1f(s, "u_float_B", MATERIAL_OR_PUSH) {} };
struct u_vec2_A : GLUniform2f { u_vec2_A(GLShader* s) : GLUniform2f(s, "u_vec2_A", MATERIAL_OR_PUSH) {} };
struct u_vec3_A : GLUniform3f { u_vec3_A(GLShader* s) : GLUniform3f(s, "u_vec3_A", MATERIAL_OR_PUSH) {} };
struct u_vec3_B : GLUniform3f { u_vec3_B(GLShader* s) : GLUniform3f(s, "u_vec3_B", MATERIAL_OR_PUSH) {} };
struct u_mat4_A: GLUniformMatrix4f { u_mat4_A(GLShader* s) : GLUniformMatrix4f(s, "u_mat4_A", MATERIAL_OR_PUSH) {} };
struct u_vec4Array6_A : GLUniform4fv { u_vec4Array6_A(GLShader* s) : GLUniform4fv( s, "u_Vec4Array6_A", 6, MATERIAL_OR_PUSH ) {} };

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
                    public u_mat4_A
    {
    public:
        Shader1() : u_mat4_A(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 16u);
    ASSERT_EQ(uniforms.size(), 1);
    EXPECT_EQ(uniforms[0], Get<u_mat4_A>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 16u);
}

TEST(MaterialUniformPackingTest, TwoFloats)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_float_A,
                    public u_float_B
    {
    public:
        Shader1() : u_float_A(this), u_float_B(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 4u);
    ASSERT_EQ(uniforms.size(), 2);
    EXPECT_EQ(uniforms[0], Get<u_float_A>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 1u);
    EXPECT_EQ(uniforms[1], Get<u_float_B>(shader1));
    EXPECT_EQ(uniforms[1]->_std430Size, 3u);
}

TEST(MaterialUniformPackingTest, Vec3Handling)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_float_A,
                    public u_vec2_A,
                    public u_vec3_A,
                    public u_vec3_B
    {
    public:
        Shader1() : u_float_A(this),
                    u_vec2_A(this),
                    u_vec3_A(this),
                    u_vec3_B(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 12u);
    ASSERT_EQ(uniforms.size(), 4);
    EXPECT_EQ(uniforms[0], Get<u_vec3_A>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 3u);
    EXPECT_EQ(uniforms[1], Get<u_float_A>(shader1));
    EXPECT_EQ(uniforms[1]->_std430Size, 1u);
    EXPECT_EQ(uniforms[2], Get<u_vec3_B>(shader1));
    EXPECT_EQ(uniforms[2]->_std430Size, 4u);
    EXPECT_EQ(uniforms[3], Get<u_vec2_A>(shader1));
    EXPECT_EQ(uniforms[3]->_std430Size, 4u);
}

TEST(MaterialUniformPackingTest, Array)
{
    class Shader1 : public MaterialUniformPackingTestShaderBase,
                    public u_vec4Array6_A
    {
    public:
        Shader1() : u_vec4Array6_A(this) {}
    };

    Shader1 shader1;
    std::vector<GLUniform*> uniforms = shader1.GetUniforms();
    EXPECT_EQ(shader1.GetSTD140Size(), 24u);
    ASSERT_EQ(uniforms.size(), 1);
    EXPECT_EQ(uniforms[0], Get<u_vec4Array6_A>(shader1));
    EXPECT_EQ(uniforms[0]->_std430Size, 4u);
}

} // namespace
