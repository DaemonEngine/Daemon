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

#include "Common.h"

namespace Util {

// Exponential moving average FPS counter
//
// weight t seconds ago = e^-at
// weight of the last q seconds: integral from 0 to q of e^-at
// antiderivative -1/a e^-at
// -1/a (e^-aq - e^0) = (1 - e^-aq)/a
void UpdateFPSCounter(float halfLife, int frameMs, float& fps)
{
	if (frameMs <= 0) {
		// act as if we have found out that a frame was processed 0.5 ms in the past
		// to avoid the discontinuity of a 0-length frame
		float weight = 1.0f - exp2f(-0.0005f / halfLife);
		fps += weight / 0.0005f;
	}
	else {
		float frameLen = frameMs * 0.001f;
		float oldWeight = exp2f(-frameLen / halfLife);
		float newWeight = 1.0f - oldWeight;
		fps = oldWeight * fps + newWeight / frameLen;
	}
}

} // namespace Util
