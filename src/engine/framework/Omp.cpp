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

#include <algorithm>

#include "CvarSystem.h"
#include "Omp.h"

#if defined(_OPENMP)
#include "omp.h"
#endif

#if defined(_OPENMP)
static Cvar::Range<Cvar::Cvar<int>> common_ompThreads(
	"common.ompThreads", "OpenMP threads", Cvar::NONE, 0, 0, 32 );
#endif

namespace Omp {
#if defined(_OPENMP)
static int ompMaxThreads = 1;
#endif

static int ompThreads = 1;

static void ReadMaxThreads()
{
#if defined(_OPENMP)
	ompMaxThreads = omp_get_max_threads();
#endif
}

void EnlistThreads()
{
#if defined(_OPENMP)
	omp_set_num_threads( ompThreads );
#endif
}

void SetupThreads()
{
#if defined(_OPENMP)
	if ( common_ompThreads.Get() )
	{
		ompThreads = common_ompThreads.Get();
		return;
	}

	if ( ompMaxThreads <= 4 )
	{
		ompThreads = ompMaxThreads;
		return;
	}

	if ( ompMaxThreads <= 16 )
	{
		ompThreads = ompMaxThreads - ( ompMaxThreads / 4 );
		return;
	}

	ompThreads = 16;
#endif

	EnlistThreads();
}

void Init()
{
	ReadMaxThreads();
	SetupThreads();
	EnlistThreads();
}

int GetThreads()
{
	return ompThreads;
}
}
