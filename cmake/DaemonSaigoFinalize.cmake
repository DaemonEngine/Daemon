# Daemon BSD Source Code
# Copyright (c) 2023-2026, Daemon Developers
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

function(saigo_finalize dir module arch)
	set(SUBPROJECT_NEXE "${dir}/${module}${NACL_EXECUTABLE_SUFFIX}")
	set(NEXE "${dir}/../${module}-${arch}${NACL_EXECUTABLE_SUFFIX}")
	set(STRIPPED_NEXE "${dir}/../${module}-${arch}-stripped${NACL_EXECUTABLE_SUFFIX}")

	add_custom_command(
		OUTPUT "${NEXE}"
		COMMENT "Copying ${module} (${arch})"
		DEPENDS "${SUBPROJECT_NEXE}"
		COMMAND
			${CMAKE_COMMAND}
			-E copy
			"${SUBPROJECT_NEXE}"
			"${NEXE}"
	)

	add_custom_command(
		OUTPUT "${STRIPPED_NEXE}"
		COMMENT "Stripping ${module} (${arch})"
		DEPENDS "${NEXE}"
		COMMAND
			"${CMAKE_STRIP}"
			-s
			"${NEXE}"
			-o "${STRIPPED_NEXE}"
	)

	add_custom_target("${module}-${arch}" ALL DEPENDS "${STRIPPED_NEXE}")
	add_dependencies("${module}-${arch}" "${module}-nacl")
endfunction()
