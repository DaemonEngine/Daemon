/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2024-2025, Daemon Developers
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

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
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

#define REPORT_SLUG "COMPILER"
#include "report.h"

// GCC

#if defined(__GNUC__)
	#pragma message(REPORT_COMPATIBILITY("GCC"))
#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
	#pragma message(REPORT_VERSION_3("GCC", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__))
#endif

// Clang

#if defined(__clang__)
	#pragma message(REPORT_COMPATIBILITY("Clang"))
#endif

#if defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
	#pragma message(REPORT_VERSION_3("Clang", __clang_major__, __clang_minor__, __clang_patchlevel__))
#endif

#if defined(__clang_version__)
	#pragma message(REPORT_VERSION_STRING("Clang", __clang_version__))
#endif

// ICC

#if defined(__INTEL_COMPILER) && defined(__INTEL_COMPILER_UPDATE)
	#pragma message(REPORT_VERSION_2("ICC", __INTEL_COMPILER, __INTEL_COMPILER_UPDATE))
#elif defined(_ICC)
	#pragma message(REPORT_VERSION_1("ICC", __ICC))
#endif

// ICX

#if defined(__INTEL_CLANG_COMPILER)
	// This requires extra parsing since it's in the form 20240000 for 2024.0.0.
	#pragma message(REPORT_VERSION_1("ICX", __INTEL_CLANG_COMPILER))
#elif defined(__INTEL_LLVM_COMPILER)
	// This requires extra parsing since it's in the form 20240000 for 2024.0.0.
	#pragma message(REPORT_VERSION_1("ICX", __INTEL_LLVM_COMPILER))
#endif

// ArmClang

#if defined(__armclang_major__) && defined(__armclang_minor__)
	// There is no __armclang_patchlevel__, 23.04.1 is reported as 23.04.
	#pragma message(REPORT_VERSION_2("ARMClang", __armclang_major__, __armclang_minor__))
#endif

#if defined(__armclang_version__)
	// This string contains the version patch level and requires extra parsing.
	// #define __armclang_major__ 23
	// #define __armclang_minor__ 04
	// #define __armclang_version__ "23.04.1 (build number 14)"
	#pragma message(REPORT_VERSION_STRING("ARMClang", __armclang_version__))
#endif

// Generic

#if defined(__VERSION__)
	#pragma message(REPORT_VERSION_STRING("generic", __VERSION__))
#endif

// Selection

// There is no Zig specific version definition, we can detect its usage by parsing
// other definitions, and then use the `zig version` command to get the version:
// #define __VERSION__ "Clang 18.1.6 (https://github.com/ziglang/zig-bootstrap 98bc6bf4fc4009888d33941daf6b600d20a42a56)"
// #define __clang_version__ "18.1.6 (https://github.com/ziglang/zig-bootstrap 98bc6bf4fc4009888d33941daf6b600d20a42a56)"

// There is no Saigo specific version definition, we reuse the clang version instead
// of parsing the version strings:
// #define __VERSION__ "Clang 19.0.0git (https://chromium.googlesource.com/a/native_client/nacl-llvm-project-v10.git e25355fddbdece2ef08747ead05b7f69f3bc6dca)"
// #define __clang_version__ "19.0.0git (https://chromium.googlesource.com/a/native_client/nacl-llvm-project-v10.git e25355fddbdece2ef08747ead05b7f69f3bc6dca)"

// There is no PNaCl specific version definition, we should parse another definition:
// #define __VERSION__ "4.2.1 Compatible Clang 3.6.0 (https://chromium.googlesource.com/a/native_client/pnacl-clang.git 96b3da27dcefc9d152e51cf54280989b2206d789) (https://chromium.googlesource.com/a/native_client/pnacl-llvm.git d0089f0b008e03cfd141f05c80e3b628c2df75c1)"

// There is no AOCC specific version definition, we should parse another definition:
// #define __VERSION__ "AMD Clang 14.0.6 (CLANG: AOCC_4.0.0-Build#434 2022_10_28)"
// #define __clang_version__ "14.0.6 (CLANG: AOCC_4.0.0-Build#434 2022_10_28)"

// There is no usable AppleClang version definitions,
// It reports an old GCC version but reuses the LLVM version as its owns:
// #define __GNUC__ 4.2.1
// #define __APPLE_CC_ 6000
// #define __apple_build_version__ 10010046
// #define __VERSION__ "4.2.1 Compatible Apple LLVM 10.0.1 (clang-1001.0.46.4)"

#if defined(__INTEL_COMPILER) || defined(__ICC)
	#pragma message(REPORT_NAME("ICC")) // Intel
#elif defined(__INTEL_CLANG_COMPILER) || defined(__INTEL_LLVM_COMPILER)
	#pragma message(REPORT_NAME("ICX")) // IntelLLVM
#elif defined(__wasi__)
	#pragma message(REPORT_NAME("WASI"))
#elif defined(__saigo__)
	#pragma message(REPORT_NAME("Saigo"))
#elif defined(__pnacl__)
	#pragma message(REPORT_NAME("PNaCl"))
#elif defined(__MINGW64__) || defined(__MINGW32__)
	#pragma message(REPORT_NAME("MinGW"))
#elif defined(__armclang_major__) || defined(__armclang_version__)
	#pragma message(REPORT_NAME("ARMClang"))
#elif defined(__clang__) && (defined(__APPLE_CC__) || defined(__apple_build_version__))
	#pragma message(REPORT_NAME("AppleClang"))
#elif defined(__clang__)
	#pragma message(REPORT_NAME("Clang"))
#elif defined(__GNUC__)
	#pragma message(REPORT_NAME("GCC")) // GNU
#else
	#pragma message(REPORT_NAME("Unknown"))
#endif

// Make the compilation succeeds if architecture is supported.
int main(int argc, char** argv) {
	return 0;
}
