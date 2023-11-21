set(PLATFORM_PREFIX "${DEPS_DIR}/saigo_newlib/bin")
set(PLATFORM_TRIPLET "${SAIGO_ARCH}-nacl")
set(PLATFORM_EXE_SUFFIX ".nexe")

set(CMAKE_SYSTEM_NAME "Generic")

set(CMAKE_C_COMPILER "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang")
set(CMAKE_CXX_COMPILER "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang++")
set(CMAKE_AR "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ar" CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ranlib")
set(SAIGO_STRIP "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-strip")
set(CMAKE_FIND_ROOT_PATH "${PLATFORM_PREFIX}/../${PLATFORM_TRIPLET}")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
# These commands can fail on windows if there is a space at the beginning
set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# Copy-pasted from the PNaCl toolchain, it's not sure we need it.
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

set(NACL ON)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")

function(saigo_finalize dir module arch)
	set(SUBPROJECT_NEXE ${dir}/nacl-vms-${arch}/${module}.nexe)
	set(NEXE ${dir}/${module}-${arch}.nexe)
	set(STRIPPED_NEXE ${dir}/${module}-${arch}-stripped.nexe)

	add_custom_command(
		OUTPUT ${NEXE}
		COMMENT "Copying ${module} (${arch})"
		DEPENDS ${SUBPROJECT_NEXE}
		COMMAND
			${CMAKE_COMMAND}
			-E copy
			${SUBPROJECT_NEXE}
			${NEXE}
	)

	add_custom_command(
		OUTPUT ${STRIPPED_NEXE}
		COMMENT "Stripping ${module} (${arch})"
		DEPENDS ${NEXE}
		COMMAND
			"${SAIGO_STRIP}"
			-s
			${NEXE}
			-o ${STRIPPED_NEXE}
	)

	add_custom_target(${module}-${arch} ALL DEPENDS ${STRIPPED_NEXE})
	add_dependencies(${module}-${arch} ${module}-nacl)
endfunction()
