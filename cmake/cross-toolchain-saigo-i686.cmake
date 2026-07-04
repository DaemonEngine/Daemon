# Daemon BSD Source Code
# Copyright (c) 2026, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# * Neither the name of the Daemon developers nor the
# names of its contributors may be used to endorse or promote products
# derived from this software without specific prior written permission.
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

set(SAIGO_ARCH "i686")
include("${CMAKE_CURRENT_LIST_DIR}/cross-toolchain-saigo-common.cmake")

if (EXISTS "${SAIGOSDK_PREFIX}/x86_64-nacl/lib32")
	# This will search libraries in paths in this order:
	# - i686-nacl/lib
	# - x86_64-nacl/lib32
	# - x86_64-nacl/lib
	# The last path is wrong but is not used when the lib is found before,
	# so it is meant to work on well-formed SDK installations.
	list(APPEND CMAKE_FIND_ROOT_PATH "${SAIGOSDK_PREFIX}/x86_64-nacl")
	set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX "32")
endif()
