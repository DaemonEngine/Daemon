set(DAEMON_BUILDINFO_HEADER "// Automatically generated, do not modify!\n")
set(DAEMON_BUILDINFO_CPP "${DAEMON_BUILDINFO_HEADER}")
set(DAEMON_BUILDINFO_H "${DAEMON_BUILDINFO_HEADER}")

macro(daemon_add_buildinfo TYPE NAME VALUE)
	set(DAEMON_BUILDINFO_CPP "${DAEMON_BUILDINFO_CPP}const ${TYPE} ${NAME}=${VALUE};\n")
	set(DAEMON_BUILDINFO_H "${DAEMON_BUILDINFO_H}extern const ${TYPE} ${NAME};\n")
endmacro()

set(DAEMON_BUILDINFO_DIR "${CMAKE_CURRENT_BINARY_DIR}/DaemonBuildInfo")
set(DAEMON_BUILDINFO_CPP_FILE "${DAEMON_BUILDINFO_DIR}/DaemonBuildInfo.cpp")
set(DAEMON_BUILDINFO_H_FILE "${DAEMON_BUILDINFO_DIR}/DaemonBuildInfo.h")

file(MAKE_DIRECTORY "${DAEMON_BUILDINFO_DIR}")
include_directories("${DAEMON_BUILDINFO_DIR}")

set(BUILDINFOLIST "${DAEMON_BUILDINFO_CPP_FILE}" "${DAEMON_BUILDINFO_H_FILE}")

macro(daemon_write_buildinfo)
	foreach(kind CPP H)
		if (EXISTS "${DAEMON_BUILDINFO_${kind}_FILE}")
			file(READ "${DAEMON_BUILDINFO_${kind}_FILE}" DAEMON_BUILDINFO_${kind}_READ)
		endif()

		if (NOT "${DAEMON_BUILDINFO_${kind}}" STREQUAL "${DAEMON_BUILDINFO_${kind}_READ}")
			file(WRITE "${DAEMON_BUILDINFO_${kind}_FILE}" "${DAEMON_BUILDINFO_${kind}}")
		endif()
	endforeach()
endmacro()
