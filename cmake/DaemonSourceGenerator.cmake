set(DAEMON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSource")

set(DAEMON_BUILDINFO_DIR "DaemonBuildInfo")
set(DAEMON_BUILDINFO_HEADER "// Automatically generated, do not modify!\n")
set(DAEMON_BUILDINFO_CPP_EXT ".cpp")
set(DAEMON_BUILDINFO_H_EXT ".h")
set(BUILDINFOLIST)

file(MAKE_DIRECTORY "${DAEMON_GENERATED_DIR}")
include_directories("${DAEMON_GENERATED_DIR}")

file(MAKE_DIRECTORY "${DAEMON_GENERATED_DIR}/${DAEMON_BUILDINFO_DIR}")

foreach(kind CPP H)
	set(DAEMON_BUILDINFO_${kind} "${DAEMON_BUILDINFO_HEADER}")
endforeach()

macro(daemon_add_buildinfo TYPE NAME VALUE)
	set(DAEMON_BUILDINFO_CPP "${DAEMON_BUILDINFO_CPP}const ${TYPE} ${NAME}=${VALUE};\n")
	set(DAEMON_BUILDINFO_H "${DAEMON_BUILDINFO_H}extern const ${TYPE} ${NAME};\n")
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
		set(DAEMON_BUILDINFO_${kind}_NAME "${NAME}${DAEMON_BUILDINFO_${kind}_EXT}")
		set(DAEMON_BUILDINFO_${kind}_PATH "${DAEMON_BUILDINFO_DIR}/${DAEMON_BUILDINFO_${kind}_NAME}")

		daemon_write_generated("${DAEMON_BUILDINFO_${kind}_PATH}" "${DAEMON_BUILDINFO_${kind}}")
		list(APPEND BUILDINFOLIST "${DAEMON_GENERATED_FILE}")
	endforeach()
endmacro()
