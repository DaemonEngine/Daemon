# ===========================================================================
#
# Daemon BSD Source Code
# Copyright (c) 2025 Daemon Developers
# All rights reserved.
#
# This file is part of the Daemon BSD Source Code (Daemon Source Code).
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 	* Redistributions of source code must retain the above copyright
# 	  notice, this list of conditions and the following disclaimer.
# 	* Redistributions in binary form must reproduce the above copyright
# 	  notice, this list of conditions and the following disclaimer in the
# 	  documentation and/or other materials provided with the distribution.
# 	* Neither the name of the Daemon developers nor the
# 	  names of its contributors may be used to endorse or promote products
# 	  derived from this software without specific prior written permission.
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
#
# ===========================================================================

function(GenerateEmbedFile srcPath dstPath filename_symbol format target mode srcList headerList baseName filename)
	add_custom_command(
		OUTPUT ${dstPath}
		COMMAND ${CMAKE_COMMAND}
			"-DINPUT_FILE=${srcPath}"
			"-DOUTPUT_FILE=${dstPath}"
			"-DFILE_FORMAT=${format}"
			"-DVARIABLE_NAME=${filename_symbol}"
			"-DEMBED_MODE=${mode}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedText.cmake"
		MAIN_DEPENDENCY ${srcPath}
	)

	target_sources(${target} PRIVATE ${dstPath})

	string(APPEND srcList
		"#include \"${baseName}/${filename_symbol}.h\"\n")
	string(APPEND headerList
		"\t{ \"${filename}\", "
		"std::string(reinterpret_cast<const char *>( ${filename_symbol} ), "
		"sizeof( ${filename_symbol} )) },\n")
endfunction()

function(GenerateEmbedFilesConstexpr srcPaths dstPath format target)
	set(first TRUE)
	foreach(srcPath IN LISTS srcPaths)
		get_filename_component(filename "${srcPath}" NAME_WE)

		if(first)
			set(mode WRITE)
			set(first FALSE)
		else()
			set(mode APPEND)
		endif()
		
		set(cmd ${CMAKE_COMMAND}
			"-DINPUT_FILE=${srcPath}"
			"-DOUTPUT_FILE=${dstPath}"
			"-DFILE_FORMAT=${format}"
			"-DVARIABLE_NAME=${filename}"
			"-DEMBED_MODE=${mode}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedText.cmake" )
		list(APPEND cmdList ${cmd})
	endforeach()

	add_custom_command(
		OUTPUT ${dstPath}
		COMMAND ${cmdList}
		DEPENDS ${srcPaths} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DaemonVulkan.cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DaemonEmbed.cmake
	)

	target_sources(${target} PRIVATE ${dstPath})
endfunction()

macro(daemon_embed_files BASENAME SLUG FORMAT TARGETNAME)
	set(EMBED_SOURCE_DIR "${SLUG}_EMBED_DIR")
	set(EMBED_SOURCE_LIST "${SLUG}_EMBED_LIST")

	set(EMBED_SUBDIR "${DAEMON_EMBEDDED_SUBDIR}/${BASENAME}")
	set(EMBED_DIR "${DAEMON_GENERATED_DIR}/${EMBED_SUBDIR}")

	foreach(kind CPP H)
		set(EMBED_${kind}_FILE "${DAEMON_EMBEDDED_SUBDIR}/${BASENAME}${DAEMON_GENERATED_${kind}_EXT}")
		set(EMBED_${kind}_TEXT "${DAEMON_GENERATED_HEADER}")
	endforeach()

	string(APPEND EMBED_CPP_TEXT "#include \"${EMBED_H_FILE}\"\n\n")

	set(EMBED_MAP_TEXT "")

	foreach(filename ${${EMBED_SOURCE_LIST}})
		string(REPLACE "/" "_" filename_symbol "${filename}")
		string(REPLACE "." "_" filename_symbol "${filename_symbol}")

		set(inpath "${${EMBED_SOURCE_DIR}}/${filename}")
		set(outpath "${EMBED_DIR}/${filename_symbol}${DAEMON_GENERATED_H_EXT}")

		GenerateEmbedFile(${inpath} ${outpath} ${filename_symbol} ${FORMAT} ${TARGETNAME} WRITE EMBED_CPP_TEXT EMBED_MAP_TEXT TRUE ${BASENAME} ${filename})
	endforeach()

	string(APPEND EMBED_CPP_TEXT
		"\n"
		"namespace ${BASENAME} {\n"
		"const std::unordered_map<std::string, std::string> FileMap\n{\n"
		"${EMBED_MAP_TEXT}\n"
		"};\n"
		"\n"
		"bool HasFile(Str::StringRef filename)\n"
		"{\n"
		"\treturn FileMap.find(filename) != FileMap.end();\n"
		"}\n"
		"\n"
		"const std::string ReadFile(Str::StringRef filename)\n"
		"{\n"
		"	auto it = FileMap.find(filename);\n"
		"	if (it != FileMap.end())\n"
		"		return it->second;\n"
		"	return \"\";\n"
		"}\n"
		"}"
	)

	string(APPEND EMBED_H_TEXT
		"#include \"common/Common.h\"\n"
		"\n"
		"namespace ${BASENAME} {\n"
		"extern const std::unordered_map<std::string, std::string> FileMap;\n"
		"bool HasFile(Str::StringRef filename);\n"
		"const std::string ReadFile(Str::StringRef filename);\n"
		"};\n"
	)

	foreach(kind CPP H)
		daemon_write_generated("${EMBED_${kind}_FILE}" "${EMBED_${kind}_TEXT}")
	endforeach()
endmacro()