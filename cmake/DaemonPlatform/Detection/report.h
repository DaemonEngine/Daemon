/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2024-2025, Daemon Developers
All rights reserveid.

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

#define STRING(s) #s
#define XSTRING(s) STRING(s)

#define REPORT(key, value) \
	"<REPORT<DAEMON_" REPORT_SLUG "_" key "=" value ">REPORT>"
#define REPORT_VERSION_3(name, major, minor, patch) \
	REPORT(name "_VERSION", XSTRING(major) "." XSTRING(minor) "." XSTRING(patch))
#define REPORT_VERSION_2(name, major, minor) \
	REPORT(name "_VERSION", XSTRING(major) "." XSTRING(minor))
#define REPORT_VERSION_1(name, major) \
	REPORT(name "_VERSION", XSTRING(major))
#define REPORT_VERSION_STRING(name, value) \
	REPORT(name "_VERSION_STRING", value)
#define REPORT_COMPATIBILITY(name) \
	REPORT(name "_COMPATIBILITY", "ON")
#define REPORT_NAME(name) \
	REPORT("NAME", name)
