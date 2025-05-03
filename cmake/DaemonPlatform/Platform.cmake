include(${CMAKE_CURRENT_LIST_DIR}/BuildInfo.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/System.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Architecture.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Compiler.cmake)

if (NACL AND DAEMON_CXX_COMPILER_Saigo)
	# Saigo clang reports weird errors when building some Unvanquished cgame and sgame arm nexe with PIE.
	# Saigo clang crashes when building Unvanquished amd64 cgame with PIE, sgame builds properly though.
	set(NACL_PIE 0)
else()
	set(NACL_PIE 1)
endif()
