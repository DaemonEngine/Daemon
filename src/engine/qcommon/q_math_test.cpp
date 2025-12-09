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
#include <gmock/gmock.h>

#include "common/Common.h"

// TODO: add transform tests with quats with negative real component
// the calculator I used always made it positive

namespace {

using ::testing::Pointwise;
using ::testing::FloatNear;

// Using a function for this in case we want to change how transform_t is set
// to avoid undefined behaviors with the union
transform_t MakeTransform(
    const std::array<float, 4>& quat,
    const std::array<float, 3>& translation,
    float scale)
{
    transform_t t;
    Vector4Copy(quat, t.rot);
    VectorCopy(translation, t.trans);
    t.scale = scale;
    return t;
}

void ExpectTransformEqual(
    const transform_t& t,
    const std::array<float, 4>& expectedQuat,
    const std::array<float, 3>& expectedTranslation,
    float expectedScale)
{
    const char* base = reinterpret_cast<const char*>(&t);
    std::array<float, 4> actualQuat;
    std::array<float, 3> actualTranslation;
    float actualScale;
    memcpy(&actualQuat, base + offsetof(transform_t, rot), sizeof(actualQuat));
    memcpy(&actualTranslation, base + offsetof(transform_t, trans), sizeof(actualTranslation));
    memcpy(&actualScale, base + offsetof(transform_t, scale), sizeof(actualScale));
    EXPECT_THAT(actualQuat, Pointwise(FloatNear(1e-4), expectedQuat));
    EXPECT_THAT(actualTranslation, Pointwise(FloatNear(1e-3), expectedTranslation));
    EXPECT_THAT(actualScale, FloatNear(1e-5, expectedScale));
}

TEST(QMathTransformTest, TransInit)
{
    transform_t t;
    TransInit(&t);
    ExpectTransformEqual(t, {0, 0, 0, 1}, {0, 0, 0}, 1);
}

TEST(QMathTransformTest, TransformPoint)
{
    transform_t t = MakeTransform(
        {0.3155654, 0.121273, 0.7211766, 0.6046616}, {-11, -26, 55}, 1.2);
    const vec3_t pointIn = {12, 19, -30};
    vec3_t pointOut;
    TransformPoint(&t, pointIn, pointOut);
    const vec3_t expectedPoint = {-51.8073, -10.3551, 44.3603};
    EXPECT_THAT(pointOut, Pointwise(FloatNear(0.001), expectedPoint));
}

TEST(QMathTransformTest, TransformNormalVector)
{
    const transform_t t = MakeTransform(
        {0.1641059, -0.6753088, 0.2253332, -0.6828266}, {235, 52, 42}, 8);
    const vec3_t vector = { 0.48, 0.64, 0.6 };
    vec3_t transformedVector;
    TransformNormalVector(&t, vector, transformedVector);
    const vec3_t expectedVector = {0.6462654,0.2383020,-0.7249505};
    EXPECT_THAT(transformedVector, Pointwise(FloatNear(1e-6), expectedVector));
}

TEST(QMathTransformTest, TransInitRotationQuat)
{
    const quat_t q = { -0.1423769, 0.5422508, 0.7235357, -0.402727 };
    transform_t t;
    TransInitRotationQuat(q, &t);
    ExpectTransformEqual(t, {-0.1423769, 0.5422508, 0.7235357, -0.402727}, {0, 0, 0}, 1);
}

TEST(QMathTransformTest, TransInitTranslation)
{
    const vec3_t translation = {-9, -32, -0.5};
    transform_t t;
    TransInitTranslation(translation, &t);
    ExpectTransformEqual(t, {0, 0, 0, 1}, {-9, -32, -0.5}, 1);
}

TEST(QMathTransformTest, TransInitScale)
{
    transform_t t;
    TransInitScale(4.3, &t);
    ExpectTransformEqual(t, {0, 0, 0, 1}, {0, 0, 0}, 4.3);
}

TEST(QMathTransformTest, TransInsRotationQuat)
{
    const quat_t q = {0.3694065, -0.6226631, 0.1538449, -0.6724294};
    transform_t t = MakeTransform(
        {-0.5881667, -0.3748035, 0.6614826, 0.2757227}, {1.8, -1.8, -4}, 3);
    TransInsRotationQuat(q, &t);
    ExpectTransformEqual(t, {0.8515735, 0.4151890, 0.1023027, -0.3032735}, {1.8, -1.8, -4}, 3);
}

TEST(QMathTransformTest, TransAddRotationQuat)
{
    const quat_t rot = {-0.4217441, -0.6010819, -0.3683214, -0.5702384};
    transform_t t = MakeTransform(
        {-0.1167623, -0.9282776, 0.3395844, 0.096694}, {12.5, 93.7, -14.1}, 2.9);
    TransAddRotationQuat(rot, &t);
    ExpectTransformEqual(t, {-0.5202203, 0.6574423, 0.09205336, -0.5372771},
        {-5.823749, 47.07177, 82.97640}, 2.9);
}

TEST(QMathTransformTest, TransInsScale)
{
    transform_t t = MakeTransform(
        {-0.8678937, 0.0774454, -0.2533126, -0.4202326}, {25, 25, 17}, 4.6);
    TransInsScale(3, &t);
    ExpectTransformEqual(t,
        {-0.8678937, 0.0774454, -0.2533126, -0.4202326}, {25, 25, 17}, 13.8);
}

TEST(QMathTransformTest, TransAddScale)
{
    transform_t t = MakeTransform(
        {-0.9884186, -0.0914321, 0.1088879, 0.053031}, {-9, -12, -70}, 6);
    TransAddScale(1.2, &t);
    ExpectTransformEqual(t,
        {-0.9884186, -0.0914321, 0.1088879, 0.053031}, {-10.8, -14.4, -84}, 7.2);
}

TEST(QMathTransformTest, TransInsTranslation)
{
    const vec3_t translation = {3.6, 20, 5.3};
    transform_t t = MakeTransform(
        {0.5002998, 0.3780266, 0.7196359, 0.2981948}, {-8, 28, 27}, 5.5);
    TransInsTranslation(translation, &t);
    ExpectTransformEqual(t,
        {0.5002998, 0.3780266, 0.7196359, 0.2981948}, {7.592672, -7.848988, 135.6898}, 5.5);
}

TEST(QMathTransformTest, TransAddTranslation)
{
    const vec3_t translation = { 18, -1.1, -6};
    transform_t t = MakeTransform(
        {0.3917585, -0.3411706, -0.1357459, 0.8436237}, {-23, 56, 52}, 4);
    TransAddTranslation(translation, &t);
    ExpectTransformEqual(t,
        {0.3917585, -0.3411706, -0.1357459, 0.8436237}, {-5, 54.9, 46}, 4);
}

TEST(QMathTransformTest, TransCombine)
{
    const transform_t left = MakeTransform(
        {-0.5029552, 0.4741776, -0.5393551, 0.4809239}, {-3, -50, 7}, 1.9);
    const transform_t right = MakeTransform(
        {0.4461722, 0.8836869, 0.1121932, -0.0862585}, {-25, 11, 16}, 1.6);
    transform_t combined;
    TransCombine(&right, &left, &combined);
    ExpectTransformEqual(combined,
        {0.7877796, 0.1998672, -0.5555394, -0.1755917}, {29.72799, -5.378298, -16.55849}, 3.04);
}

TEST(QMathTransformTest, TransInverse)
{
    const transform_t t = MakeTransform(
        {0.4833702, -0.42157, -0.7551386, -0.1356377}, {2.5, 3.4, 1.4}, 2.5);
    transform_t inverse;
    TransInverse(&t, &inverse);
    // This quat multiplied by -1 would also be acceptable
    ExpectTransformEqual(inverse,
        {-0.4833702, 0.42157, 0.7551386, -0.1356377}, {1.244436,1.155842,-0.5278334}, 0.4);
}

TEST(QSharedMathTest, InverseSquareRoot)
{
    constexpr float relativeTolerance = 5.0e-6;
    auto RsqrtEq = [=](float expected) { return FloatNear(expected, expected * relativeTolerance); };

    EXPECT_THAT(Q_rsqrt(1e-6), RsqrtEq(1e3));
    EXPECT_THAT(Q_rsqrt(0.036), RsqrtEq(5.270463));
    EXPECT_THAT(Q_rsqrt(0.2), RsqrtEq(2.236068));
    EXPECT_THAT(Q_rsqrt(1), RsqrtEq(1));
    EXPECT_THAT(Q_rsqrt(3), RsqrtEq(0.5773503));
    EXPECT_THAT(Q_rsqrt(29.1), RsqrtEq(0.1853760));
    EXPECT_THAT(Q_rsqrt(1e6), RsqrtEq(1e-3));
}

TEST(QSharedMathTest, FastInverseSquareRoot)
{
    constexpr float relativeTolerance = 6.50196699e-4;
    auto RsqrtEq = [=](float expected) { return FloatNear(expected, expected * relativeTolerance); };

    EXPECT_THAT(Q_rsqrt_fast(1e-6), RsqrtEq(1e3));
    EXPECT_THAT(Q_rsqrt_fast(0.036), RsqrtEq(5.270463));
    EXPECT_THAT(Q_rsqrt_fast(0.2), RsqrtEq(2.236068));
    EXPECT_THAT(Q_rsqrt_fast(1), RsqrtEq(1));
    EXPECT_THAT(Q_rsqrt_fast(3), RsqrtEq(0.5773503));
    EXPECT_THAT(Q_rsqrt_fast(29.1), RsqrtEq(0.1853760));
    EXPECT_THAT(Q_rsqrt_fast(1e6), RsqrtEq(1e-3));
}

} // namespace
