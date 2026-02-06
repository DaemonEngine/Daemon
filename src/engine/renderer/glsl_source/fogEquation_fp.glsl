/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

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

float FogGradientFunction(float k, float t)
{
	return 1 - exp(-k * t);
}

float FogGradientAntiderivative(float k, float t)
{
	return t + exp(-k * t) / k;
}

float GetFogGradientModifier(float k, float t0, float t1)
{
	t0 = max(0.0, t0);
	t1 = max(0.0, t1);

	float deltaT = t1 - t0;
	if (abs(deltaT) > 0.1)
	{
		return ( FogGradientAntiderivative(k, t1) - FogGradientAntiderivative(k, t0) ) / deltaT;
	}
	else
	{
		return FogGradientFunction(k, t0);
	}
}

float GetFogAlpha(float x)
{
	x = clamp(x, 0.0, 1.0);

	// sqrt(x) is bad near 0 because it increases too quickly resulting in sharp edges.
	// x ≤ 1/32: √32 * x
	// x ≥ 1/32: √x
	return min(sqrt(32.0) * x, sqrt(x));
}
