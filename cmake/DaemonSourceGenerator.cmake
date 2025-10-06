set(DAEMON_SOURCE_GENERATOR "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(current_list_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
set(DAEMON_TEXT_EMBEDDER "${current_list_dir}/cmake/EmbedText.cmake")

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

macro(daemon_write_generated generated_path generated_content)
	set(DAEMON_GENERATED_FILE ${DAEMON_GENERATED_DIR}/${generated_path})

	if (EXISTS "${DAEMON_GENERATED_FILE}")
		file(READ "${DAEMON_GENERATED_FILE}" generated_content_read)
	endif()

	if (NOT "${generated_content}" STREQUAL "${generated_content_read}")
		message(STATUS "Generating ${generated_path}")
		file(WRITE "${DAEMON_GENERATED_FILE}" "${generated_content}")
	endif()
endmacro()

macro(daemon_write_buildinfo name)
	foreach(kind CPP H)
		set(daemon_buildinfo_${kind}_name "${name}${DAEMON_GENERATED_${kind}_EXT}")
		set(daemon_buildinfo_${kind}_path "${DAEMON_BUILDINFO_SUBDIR}/${daemon_buildinfo_${kind}_name}")

		daemon_write_generated("${daemon_buildinfo_${kind}_path}" "${DAEMON_BUILDINFO_${kind}_TEXT}")
		list(APPEND BUILDINFOLIST "${DAEMON_GENERATED_FILE}")
	endforeach()
endmacro()

macro(daemon_embed_files basename slug format targetname)
	set(embed_source_dir "${slug}_EMBED_DIR")
	set(embed_source_list "${slug}_EMBED_LIST")

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

	string(APPEND embed_CPP_text
		"#include \"${embed_H_file}\"\n"
		"\n"
		"namespace ${basename} {\n"
	)

	string(APPEND embed_H_text
		"#include \"common/Common.h\"\n"
		"\n"
		"namespace ${basename} {\n"
	)

	set(embed_map_text "")

	foreach(filename ${${embed_source_list}})
		string(REGEX REPLACE "[^A-Za-z0-9]" "_" filename_symbol "${filename}")

		set(inpath "${${embed_source_dir}}/${filename}")
		set(outpath "${embed_dir}/${filename_symbol}${DAEMON_GENERATED_H_EXT}")

		add_custom_command(
			OUTPUT "${outpath}"
			COMMAND ${CMAKE_COMMAND}
				"-DINPUT_FILE=${inpath}"
				"-DOUTPUT_FILE=${outpath}"
				"-DFILE_FORMAT=${format}"
				"-DVARIABLE_NAME=${filename_symbol}"
				-P "${DAEMON_TEXT_EMBEDDER}"
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
			"extern const unsigned char ${filename_symbol}[];\n"
		)

		string(APPEND embed_map_text
			"\t{ \"${filename}\", { ${filename_symbol}, sizeof( ${filename_symbol}) - 1 } },\n"
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
		daemon_write_generated("${embed_${kind}_file}" "${embed_${kind}_text}")
	endforeach()
endmacro()
