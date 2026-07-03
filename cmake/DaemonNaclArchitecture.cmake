# Daemon BSD Source Code
# Copyright (c) 2022-2026, Daemon Developers
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

function(daemon_detect_nacl_arch target_arch)
	macro(add_nacl_arch arch_name)
		list(APPEND nacl_arch_list "${arch_name}")
	endmacro()

	# NaCl runtime is only available on architectures that have a NaCl loader.
	set(nacl_runtime_arch "amd64" "i686" "armhf")

	if (YOKAI_TARGET_SYSTEM_WINDOWS)
		if("${target_arch}" STREQUAL "amd64")
			add_nacl_arch("${target_arch}")
		elseif("${target_arch}" STREQUAL "i686")
			# Win32 requires nacl_loader-amd64.exe in order to run on Win64
			add_nacl_arch("${target_arch}")
			add_nacl_arch("amd64")
		endif()
	elseif (YOKAI_TARGET_SYSTEM_LINUX_COMPATIBILITY OR YOKAI_TARGET_SYSTEM_XDG_COMPATIBILITY)
		if ("${target_arch}" IN_LIST nacl_runtime_arch)
			add_nacl_arch("${target_arch}")
		endif()

		set(armhf_usage "arm64" "armel")
		set(box64_usage "ppc64el" "riscv64" "loong64")

		if ("${target_arch}" IN_LIST armhf_usage)
			set(DAEMON_NACL_MULTIARCH ON)

			# Load 32-bit armhf nexe on 64-bit arm64 engine on Linux with multiarch.
			# The nexe is system agnostic so there should be no difference with armel.
			add_nacl_arch("armhf")
		endif()

		if ("${target_arch}" IN_LIST box64_usage)
			option(DAEMON_NACL_BOX64_EMULATION "Use Box64 to emulate x86_64 NaCl loader on unsupported platforms" ON)

			if (DAEMON_NACL_BOX64_EMULATION)
				# Use Box64 to run x86_64 NaCl loader and amd64 nexe.
				# Box64 must be installed and available in PATH at runtime.
				add_nacl_arch("amd64")
				add_definitions(-DDAEMON_NACL_BOX64_EMULATION)
			endif()
		endif()
	elseif (YOKAI_TARGET_SYSTEM_MACOS)
		if ("${target_arch}" STREQUAL "amd64")
			add_nacl_arch("${target_arch}")
		elseif ("${target_arch}" STREQUAL "arm64")
			# You can get emulated NaCl going like this:
			# cp external_deps/macos-amd64-default_10/{nacl_loader,irt_core-amd64.nexe} build/
			add_nacl_arch("amd64")
		endif()
	endif()

	if (nacl_arch_list)
		message(STATUS "Available NaCl architectures: ${nacl_arch_list}")

		list(GET nacl_arch_list 0 nacl_arch)

		message(STATUS "Primary NaCl architecture: ${nacl_arch}")

		add_definitions(-DDAEMON_NACL_RUNTIME_ENABLED)
	else()
		set(nacl_arch "unknown")

		message(STATUS "No known NaCl architecture")
	endif()

	# The DAEMON_NACL_ARCH_NAME variable contributes to the nexe file name.
	set(DAEMON_NACL_ARCH_NAME "${nacl_arch}" PARENT_SCOPE)
	# Those names are used to copy the loader and the IRT binaries.
	set(DAEMON_NACL_ARCH_NAME_LIST "${nacl_arch_list}" PARENT_SCOPE)
endfunction()
