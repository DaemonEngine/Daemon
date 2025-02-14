set(DAEMON_BUILTIN_HEADER "// Automatically generated, do not modify!\n")
set(DAEMON_BUILTIN_C "${DAEMON_BUILTIN_HEADER}")
set(DAEMON_BUILTIN_H "${DAEMON_BUILTIN_HEADER}")

macro(daemon_add_builtin TYPE NAME VALUE)
	set(DAEMON_BUILTIN_C "${DAEMON_BUILTIN_C}const ${TYPE} ${NAME}=${VALUE};\n")
	set(DAEMON_BUILTIN_H "${DAEMON_BUILTIN_H}extern const ${TYPE} ${NAME};\n")
endmacro()

set(DAEMON_BUILTIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/builtin")
set(DAEMON_BUILTIN_C_FILE "${DAEMON_BUILTIN_DIR}/builtin.cpp")
set(DAEMON_BUILTIN_H_FILE "${DAEMON_BUILTIN_DIR}/builtin.h")

file(MAKE_DIRECTORY "${DAEMON_BUILTIN_DIR}")
include_directories("${DAEMON_BUILTIN_DIR}")

set(BUILTINLIST "${DAEMON_BUILTIN_C_FILE}" "${DAEMON_BUILTIN_H_FILE}")

macro(daemon_write_builtin)
	foreach(kind C H)
		if (EXISTS "${DAEMON_BUILTIN_${kind}_FILE}")
			file(READ "${DAEMON_BUILTIN_${kind}_FILE}" DAEMON_BUILTIN_${kind}_READ)
		endif()

		if (NOT "${DAEMON_BUILTIN_${kind}}" STREQUAL "${DAEMON_BUILTIN_${kind}_READ}")
			file(WRITE "${DAEMON_BUILTIN_${kind}_FILE}" "${DAEMON_BUILTIN_${kind}}")
		endif()
	endforeach()
endmacro()
