set(DAEMON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSource")

set(DAEMON_BUILDINFO_DIR "${DAEMON_GENERATED_DIR}/DaemonBuildInfo")
set(DAEMON_BUILDINFO_HEADER "// Automatically generated, do not modify!\n")
set(DAEMON_BUILDINFO_CPP_EXT ".cpp")
set(DAEMON_BUILDINFO_H_EXT ".h")
set(BUILDINFOLIST)

file(MAKE_DIRECTORY "${DAEMON_GENERATED_DIR}")
include_directories("${DAEMON_GENERATED_DIR}")

file(MAKE_DIRECTORY ${DAEMON_BUILDINFO_DIR})

foreach(kind CPP H)
	set(DAEMON_BUILDINFO_${kind} "${DAEMON_BUILDINFO_HEADER}")
endforeach()

macro(daemon_add_buildinfo TYPE NAME VALUE)
	set(DAEMON_BUILDINFO_CPP "${DAEMON_BUILDINFO_CPP}const ${TYPE} ${NAME}=${VALUE};\n")
	set(DAEMON_BUILDINFO_H "${DAEMON_BUILDINFO_H}extern const ${TYPE} ${NAME};\n")
endmacro()

macro(daemon_write_buildinfo NAME)
	foreach(kind CPP H)
		set(DAEMON_BUILDINFO_${kind}_NAME "${NAME}${DAEMON_BUILDINFO_${kind}_EXT}")
		set(DAEMON_BUILDINFO_${kind}_PATH "${DAEMON_BUILDINFO_DIR}/${DAEMON_BUILDINFO_${kind}_NAME}")

		file(GENERATE OUTPUT "${DAEMON_BUILDINFO_${kind}_PATH}" CONTENT "${DAEMON_BUILDINFO_${kind}}")
		list(APPEND BUILDINFOLIST "${DAEMON_BUILDINFO_${kind}_PATH}")
	endforeach()
endmacro()
