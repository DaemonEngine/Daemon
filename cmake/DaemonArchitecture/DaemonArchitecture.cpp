/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2022, Daemon Developers
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

/* The qprocessordetection.h file doesn't detect endianness for some
platforms including ppc64, but we know how to do it for them. */

#include <stdint.h>
#include "../../src/common/Endian.h"

/* qprocessordetection.h will print an error if it fails to detect
endianness and while it is not already set, so the else clause is
outsourced to that qprocessordetection.h file instead. */

#if defined(Q3_BIG_ENDIAN)
	#define Q_BYTE_ORDER Q_BIG_ENDIAN
#elif defined(Q3_LITTLE_ENDIAN)
	#define Q_BYTE_ORDER Q_LITTLE_ENDIAN
#endif

/* This source file includes qprocessordetection.h from Qt:

- https://github.com/qt/qtbase/blob/dev/src/corelib/global/qprocessordetection.h
  https://raw.githubusercontent.com/qt/qtbase/dev/src/corelib/global/qprocessordetection.h

Always update by downloading new version from Qt, do not edit by hand.

This source file and the qprocessordetection.h header do not contribute to the
produced engine and game binaries in any way that can be subject to copyright. */

#include "qprocessordetection.h"

/* The architecture names are loosely inspired by Debian conventions:
	https://wiki.debian.org/ArchitectureSpecificsMemo

Except we don't have the same technical debt so we don't have
to name i686 as i386 for backward compatibility purpose neither
care of platform name variants that are meant to distinguish
platform variants we cannot support anyway. */

/* PNaCl virtual machines. */
#if defined(__native_client__)
	#pragma message("DAEMON_ARCH_nacl")

/* Wasm virtual machines, work in progress. */
#elif defined(Q_PROCESSOR_WASM)
	#pragma message("DAEMON_ARCH_wasm")

/* Devices like:
  - IBM PC compatibles and derivatives,
  - Apple Intel-based mac,
  - Steam Deck, Atari VCS consoles… */

#elif defined(Q_PROCESSOR_X86_64)
	#pragma message("DAEMON_ARCH_amd64")

#elif defined(Q_PROCESSOR_X86_32)
	// Assume at least i686. Detecting older revisions would be unlikely to work here
	// because the revisions are likely configured by flags, but this file is "compiled"
	// without most command-line flags.
	#pragma message("DAEMON_ARCH_i686")

/* Devices like:
 - Raspberry Pi,
 - Apple M1-based mac,
 - Android phones and tablets… */

#elif defined(Q_PROCESSOR_ARM_64)
	#pragma message("DAEMON_ARCH_arm64")

#elif defined(Q_PROCESSOR_ARM_32) && defined(__ARM_PCS_VFP)
	#pragma message("DAEMON_ARCH_armhf")

#elif defined(Q_PROCESSOR_ARM_32) && !defined(__ARM_PCS_VFP)
	#pragma message("DAEMON_ARCH_armel")

/* Devices like:
 - Raptor Computing Systems Talos, Blackbird… */

#elif defined(Q_PROCESSOR_POWER_64) && Q_BYTE_ORDER == Q_BIG_ENDIAN
	#pragma message("DAEMON_ARCH_ppc64")

#elif defined(Q_PROCESSOR_POWER_64) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	#pragma message("DAEMON_ARCH_ppc64el")

/* Devices like:
 - SiFive HiFive Unmatched, Horse Creek… */

#elif defined(Q_PROCESSOR_RISCV_64)
	#pragma message("DAEMON_ARCH_riscv64")

#else
	#pragma message("DAEMON_ARCH_unknown")
#endif

// Make the compilation succeeds if architecture is supported.
int main(int argc, char** argv) {
	return 0;
}
