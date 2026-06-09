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

function(daemon_detect_nacl_arch)
	set(arch_name "${YOKAI_TARGET_ARCH_NAME}")
	set(nacl_arch "${arch_name}")

	if (YOKAI_TARGET_SYSTEM_LINUX_COMPATIBILITY OR YOKAI_TARGET_SYSTEM_XDG_COMPATIBILITY)
		set(armhf_usage "arm64;armel")
		if ("${arch_name}" IN_LIST armhf_usage)
		# Load 32-bit armhf nexe on 64-bit arm64 engine on Linux with multiarch.
		# The nexe is system agnostic so there should be no difference with armel.
		set(nacl_arch "armhf")

		set(box64_usage ppc64el riscv64)
		if ("${arch_name}" IN_LIST box64_usage)
			option(DAEMON_NACL_BOX64_EMULATION "Use Box64 to emulate x86_64 NaCl loader on unsupported platforms" ON)
			if (DAEMON_NACL_BOX64_EMULATION)
				# Use Box64 to run x86_64 NaCl loader and amd64 nexe.
				# Box64 must be installed and available in PATH at runtime.
				set(nacl_arch "amd64")
				add_definitions(-DDAEMON_NACL_BOX64_EMULATION)
			endif()
		endif()
	endif()

	elseif (DAEMON_TARGET_SYSTEM_MACOS)
		if ("${arch_name}" STREQUAL "arm64")
			# You can get emulated NaCl going like this:
			# cp external_deps/macos-amd64-default_10/{nacl_loader,irt_core-amd64.nexe} build/
			set(nacl_arch "amd64")
		endif()
	endif()

	string(TOUPPER "${nacl_arch_name}" nacl_arch_name_upper)

	# The DAEMON_NACL_ARCH_NAME variable contributes to the nexe file name.
	set(DAEMON_NACL_ARCH_NAME "${nacl_arch}" PARENT_SCOPE)
	set(DAEMON_NACL_ARCH_NAME_UPPER "${nacl_arch_upper}" PARENT_SCOPE)

	# NaCl runtime is only available on architectures that have a NaCl loader.
	set(nacl_runtime_arch amd64 i686 armhf)
	if ("${nacl_arch}" IN_LIST nacl_runtime_arch)
		add_definitions(-DDAEMON_NACL_RUNTIME_ENABLED)
	endif()
endfunction()

daemon_detect_nacl_arch()

# Makes possible to do that in CMake code:
# > if (DAEMON_NACL_ARCH_ARMHF)
set("DAEMON_NACL_ARCH_${DAEMON_NACL_ARCH_NAME_UPPER}" ON)

# Add printable strings to the executable.
yokai_add_buildinfo("char*" "DAEMON_NACL_ARCH_STRING" "\"${DAEMON_NACL_ARCH_NAME}\"")
