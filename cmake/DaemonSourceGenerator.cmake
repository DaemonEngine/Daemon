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

macro(daemon_add_buildinfo TYPE NAME VALUE)
	string(APPEND DAEMON_BUILDINFO_CPP_TEXT "const ${TYPE} ${NAME}=${VALUE};\n")
	string(APPEND DAEMON_BUILDINFO_H_TEXT "extern const ${TYPE} ${NAME};\n")
endmacro()

macro(daemon_write_generated GENERATED_PATH GENERATED_CONTENT)
	set(DAEMON_GENERATED_FILE ${DAEMON_GENERATED_DIR}/${GENERATED_PATH})

	if (EXISTS "${DAEMON_GENERATED_FILE}")
		file(READ "${DAEMON_GENERATED_FILE}" GENERATED_CONTENT_READ)
	endif()

	if (NOT "${GENERATED_CONTENT}" STREQUAL "${GENERATED_CONTENT_READ}")
		message(STATUS "Generating ${GENERATED_PATH}")
		file(WRITE "${DAEMON_GENERATED_FILE}" "${GENERATED_CONTENT}")
	endif()
endmacro()

macro(daemon_write_buildinfo NAME)
	foreach(kind CPP H)
		set(DAEMON_BUILDINFO_${kind}_NAME "${NAME}${DAEMON_GENERATED_${kind}_EXT}")
		set(DAEMON_BUILDINFO_${kind}_PATH "${DAEMON_BUILDINFO_SUBDIR}/${DAEMON_BUILDINFO_${kind}_NAME}")

		daemon_write_generated("${DAEMON_BUILDINFO_${kind}_PATH}" "${DAEMON_BUILDINFO_${kind}_TEXT}")
		list(APPEND BUILDINFOLIST "${DAEMON_GENERATED_FILE}")
	endforeach()
endmacro()

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
		string(REGEX REPLACE "[^A-Za-z0-9]" "_" filename_symbol "${filename}")

		set(inpath "${${EMBED_SOURCE_DIR}}/${filename}")
		set(outpath "${EMBED_DIR}/${filename_symbol}${DAEMON_GENERATED_H_EXT}")

		add_custom_command(
			OUTPUT "${outpath}"
			COMMAND ${CMAKE_COMMAND}
				"-DINPUT_FILE=${inpath}"
				"-DOUTPUT_FILE=${outpath}"
				"-DFILE_FORMAT=${FORMAT}"
				"-DVARIABLE_NAME=${filename_symbol}"
				-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedText.cmake"
			MAIN_DEPENDENCY ${inpath}
		)

		set_property(TARGET "${TARGETNAME}" APPEND PROPERTY SOURCES "${outpath}")

		string(APPEND EMBED_CPP_TEXT
			"#include \"${BASENAME}/${filename_symbol}.h\"\n")
		string(APPEND EMBED_MAP_TEXT
			"\t{ \"${filename}\", "
			"std::string(reinterpret_cast<const char *>( ${filename_symbol} ), "
			"sizeof( ${filename_symbol} )) },\n")
	endforeach()

	string(APPEND EMBED_CPP_TEXT
		"\n"
		"namespace ${BASENAME} {\n"
		"const std::unordered_map<std::string, std::string> FileMap\n{\n"
		"${EMBED_MAP_TEXT}"
		"};\n"
		"}"
	)

	string(APPEND EMBED_H_TEXT
		"#include \"common/Common.h\"\n"
		"\n"
		"namespace ${BASENAME} {\n"
		"extern const std::unordered_map<std::string, std::string> FileMap;\n"
		"};\n"
	)

	foreach(kind CPP H)
		daemon_write_generated("${EMBED_${kind}_FILE}" "${EMBED_${kind}_TEXT}")
	endforeach()
endmacro()
