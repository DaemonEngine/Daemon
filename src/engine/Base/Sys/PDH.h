/*
=============================================================================
Daemon-Vulkan BSD Source Code
Copyright (c) 2025-2026 Reaper
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Reaper nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL REAPER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=============================================================================
*/

#ifndef PDH_H
#define PDH_H

#ifdef _MSC_VER
	#include "Windows.h"

	#include <Pdh.h>

	using  PdhOpenQueryAPtr               = PDH_STATUS ( * )( LPCSTR szDataSource, DWORD_PTR dwUserData, PDH_HQUERY* phQuery );
	using  PdhCloseQueryPtr               = PDH_STATUS ( * )( _Inout_ PDH_HQUERY hQuery );
	using  PdhAddCounterAPtr              = PDH_STATUS ( * )( _In_ PDH_HQUERY hQuery, _In_ LPCSTR szFullCounterPath, _In_ DWORD_PTR dwUserData,
		_Out_ PDH_HCOUNTER* phCounter );
	using  PdhCollectQueryDataPtr         = PDH_STATUS ( * )( _Inout_ PDH_HQUERY hQuery );
	using  PdhGetFormattedCounterValuePtr = PDH_STATUS ( * )( _In_ PDH_HCOUNTER hCounter, _In_ DWORD dwFormat, _Out_opt_ LPDWORD lpdwType,
		_Out_ PPDH_FMT_COUNTERVALUE pValue );

	extern bool                           pdhAvailable;

	extern PdhOpenQueryAPtr               PdhOpenQueryAf;
	extern PdhCloseQueryPtr               PdhCloseQueryf;
	extern PdhAddCounterAPtr              PdhAddCounterAf;
	extern PdhCollectQueryDataPtr         PdhCollectQueryDataf;
	extern PdhGetFormattedCounterValuePtr PdhGetFormattedCounterValuef;

	void LoadPDHFunctions();
#endif

#endif // PDH_H