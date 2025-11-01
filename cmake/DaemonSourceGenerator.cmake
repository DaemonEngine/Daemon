# Daemon BSD Source Code
# Copyright (c) 2025, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(DAEMON_SOURCE_GENERATOR "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(current_list_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
set(DAEMON_FILE_EMBEDDER "${current_list_dir}/DaemonFileEmbedder.cmake")

set(DAEMON_GENERATED_SUBDIR "GeneratedSource")
set(DAEMON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${DAEMON_GENERATED_SUBDIR}")

set(DAEMON_BUILDINFO_SUBDIR "DaemonBuildInfo")
set(DAEMON_EMBEDDED_SUBDIR "DaemonEmbeddedFiles")

set(DAEMON_BUILDINFO_DIR "${DAEMON_GENERATED_DIR}/${DAEMON_BUILDINFO_SUBDIR}")
set(DAEMON_EMBEDDED_DIR "${DAEMON_GENERATED_DIR}/${DAEMON_EMBEDDED_SUBDIR}")

file(MAKE_DIRECTORY "${DAEMON_GENERATED_DIR}")
include_directories("${DAEMON_GENERATED_DIR}")

file(MAKE_DIRECTORY "${DAEMON_BUILDINFO_DIR}")
file(MAKE_DIRECTORY "${DAEMON_EMBEDDED_DIR}")

set(DAEMON_GENERATED_HEADER "// Automatically generated, do not modify!\n")
set(DAEMON_GENERATED_CPP_EXT ".cpp")
set(DAEMON_GENERATED_H_EXT ".h")

set(BUILDINFOLIST)

foreach(kind CPP H)
	set(DAEMON_BUILDINFO_${kind}_TEXT "${DAEMON_GENERATED_HEADER}")
endforeach()

macro(daemon_add_buildinfo type name value)
	string(APPEND DAEMON_BUILDINFO_CPP_TEXT "const ${type} ${name}=${value};\n")
	string(APPEND DAEMON_BUILDINFO_H_TEXT "extern const ${type} ${name};\n")
endmacro()

macro(daemon_write_buildinfo name)
	foreach(kind CPP H)
		set(buildinfo_file_path "${DAEMON_BUILDINFO_DIR}/${name}${DAEMON_GENERATED_${kind}_EXT}")

		file(GENERATE OUTPUT "${buildinfo_file_path}" CONTENT "${DAEMON_BUILDINFO_${kind}_TEXT}")
		list(APPEND BUILDINFOLIST "${buildinfo_file_path}")
	endforeach()
endmacro()

macro(daemon_embed_files basename dir list format targetname)
	set(embed_subdir "${DAEMON_EMBEDDED_SUBDIR}/${basename}")
	set(embed_dir "${DAEMON_GENERATED_DIR}/${embed_subdir}")

	file(MAKE_DIRECTORY "${embed_dir}")

	foreach(kind CPP H)
		set(embed_${kind}_basename "${basename}${DAEMON_GENERATED_${kind}_EXT}")
		set(embed_${kind}_src_file "${DAEMON_EMBEDDED_DIR}/${embed_${kind}_basename}")
		set(embed_${kind}_file "${DAEMON_EMBEDDED_SUBDIR}/${embed_${kind}_basename}")
		set(embed_${kind}_text "${DAEMON_GENERATED_HEADER}")
		set_property(SOURCE "${embed_${kind}_src_file}" APPEND PROPERTY OBJECT_DEPENDS "${DAEMON_SOURCE_GENERATOR}")
		set_property(TARGET "${targetname}" APPEND PROPERTY SOURCES "${embed_${kind}_src_file}")
	endforeach()

	if (NOT DAEMON_EMBEDDED_FILES_HEADER)
		set(DAEMON_EMBEDDED_FILES_HEADER "${DAEMON_EMBEDDED_SUBDIR}/DaemonEmbeddedFiles.h")

		string(APPEND embed_header_text
			"#ifndef DAEMON_EMBEDDED_FILES_H_\n"
			"#define DAEMON_EMBEDDED_FILES_H_\n"
			"#include <unordered_map>\n"
			"#include <string>\n"
			"\n"
			"struct embeddedFileMapEntry_t\n"
			"{\n"
			"	const char* data;\n"
			"	size_t size;\n"
			"};\n"
			"\n"
			"using embeddedFileMap_t = std::unordered_map<std::string, const embeddedFileMapEntry_t>;\n"
			"#endif // DAEMON_EMBEDDED_FILES_H_\n"
		)

		set(embed_header_file "${DAEMON_GENERATED_DIR}/${DAEMON_EMBEDDED_FILES_HEADER}")
		file(GENERATE OUTPUT "${embed_header_file}" CONTENT "${embed_header_text}")
	endif()

	string(APPEND embed_CPP_text
		"#include \"${embed_H_file}\"\n"
		"\n"
		"namespace ${basename} {\n"
	)

	string(APPEND embed_H_text
		"#include \"${DAEMON_EMBEDDED_FILES_HEADER}\"\n"
		"\n"
		"namespace ${basename} {\n"
	)

	set(embed_map_text "")

	foreach(filename ${list})
		string(REGEX REPLACE "[^A-Za-z0-9]" "_" filename_symbol "${filename}")

		set(inpath "${dir}/${filename}")
		set(outpath "${embed_dir}/${filename_symbol}${DAEMON_GENERATED_H_EXT}")

		add_custom_command(
			OUTPUT "${outpath}"
			COMMAND ${CMAKE_COMMAND}
				"-DINPUT_FILE=${inpath}"
				"-DOUTPUT_FILE=${outpath}"
				"-DFILE_FORMAT=${format}"
				"-DVARIABLE_NAME=${filename_symbol}"
				-P "${DAEMON_FILE_EMBEDDER}"
			MAIN_DEPENDENCY ${inpath}
			DEPENDS
				"${DAEMON_FILE_EMBEDDER}"
				"${DAEMON_SOURCE_GENERATOR}"
		)

		set_property(TARGET "${targetname}" APPEND PROPERTY SOURCES "${outpath}")

		string(APPEND embed_CPP_text
			"#include \"${basename}/${filename_symbol}.h\"\n"
		)

		string(APPEND embed_H_text
			"extern const embeddedFileMapEntry_t ${filename_symbol};\n"
		)

		string(APPEND embed_map_text
			"\t{ \"${filename}\", ${filename_symbol} },\n"
		)
	endforeach()

	string(APPEND embed_CPP_text
		"\n"
		"const embeddedFileMap_t FileMap\n{\n"
		"${embed_map_text}"
		"};\n"
		"}"
	)

	string(APPEND embed_H_text
		"extern const embeddedFileMap_t FileMap;\n"
		"};\n"
	)

	foreach(kind CPP H)
		set(embed_file "${DAEMON_GENERATED_DIR}/${embed_${kind}_file}")
		file(GENERATE OUTPUT "${embed_file}" CONTENT "${embed_${kind}_text}")
	endforeach()
endmacro()
