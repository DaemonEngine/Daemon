# Daemon BSD Source Code
# Copyright (c) 2026, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the Daemon developers nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

macro(DefineVersionConstant name value)
    set(${name} "${value}")
    if (CMAKE_SCRIPT_MODE_FILE MATCHES versions.cmake)
        # For use by shell scripts via cmake -P mode
        message("${name}='${value}'")
    endif()
endmacro()


# DEPS_VERSION and SAIGOSDK_VERSION are used to version packages which may be auto-downloaded
# by CMake. The numbers are also included in the install path if you use the scripts to build them
# yourself, so that they end up in the same path used by the download.

# Version for the target-specific library and runtime dependencies package.
# Can be auto-downloaded by CMake or built with build.sh.
DefineVersionConstant(DEPS_VERSION 11)

# Version for the host mode Saigo NaCl SDK package.
# Can be auto-downloaded by CMake (when building NaCl gamelogic) or built with sdk-build.sh.
DefineVersionConstant(SAIGOSDK_VERSION 20261030)
# Only used by sdk-build.sh, but it's closely coupled to SAIGOSDK_VERSION.
# Can be a tag like v${SAIGOSDK_VERSION}, or a full commit hash.
DefineVersionConstant(SAIGOSDK_REF "v${SAIGOSDK_VERSION}")
