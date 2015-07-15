set(PLATFORM_PREFIX ${DEPS_DIR}/pnacl/bin)
set(PLATFORM_TRIPLET "pnacl")
if (WIN32)
  set(PNACL_BIN_EXT ".bat")
else()
  set(PNACL_BIN_EXT "")
endif()
set(PLATFORM_EXE_SUFFIX ".pexe")

set(CMAKE_SYSTEM_NAME "Generic")

include(CMakeForceCompiler)
CMAKE_FORCE_C_COMPILER(   "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang${PNACL_BIN_EXT}" Clang)
CMAKE_FORCE_CXX_COMPILER( "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-clang++${PNACL_BIN_EXT}" Clang)
set(CMAKE_AR              "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ar${PNACL_BIN_EXT}" CACHE FILEPATH "Archiver" FORCE)
set(CMAKE_RANLIB          "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-ranlib${PNACL_BIN_EXT}")
set(CMAKE_FIND_ROOT_PATH  "${PLATFORM_PREFIX}/../le32-nacl")

SET(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 1)
SET(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
SET(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
SET(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")
set(CMAKE_C_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_AR> rc <TARGET> <LINK_FLAGS> <OBJECTS>")

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

set(NACL ON)

set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")

function(pnacl_finalize target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMENT "Finalising ${target}"
    COMMAND
      "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-finalize${PNACL_BIN_EXT}"
      "$<TARGET_FILE:${target}>"
  )
endfunction()

set(NACL_TRANSLATE_OPTIONS
  --allow-llvm-bitcode-input # FIXME: finalize as part of the build process
  --pnacl-allow-exceptions
  $<$<CONFIG:None>:-O3>
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:RelWithDebInfo>:-O2>
  $<$<CONFIG:MinSizeRel>:-O2>
)

function(pnacl_translate target arch suffix)
  cmake_policy(PUSH)
    if (POLICY CMP0026)
      cmake_policy(SET CMP0026 OLD)
    endif()
    get_target_property(FILE ${target} LOCATION)
  cmake_policy(POP)
  get_filename_component(DIRNAME ${FILE} PATH)
  get_filename_component(BASENAME ${FILE} NAME_WE)
  add_custom_command(
    OUTPUT ${DIRNAME}/${BASENAME}${suffix}.nexe
    COMMENT "Translating ${target} (${arch})"
    DEPENDS ${FILE}
    COMMAND
      "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-translate${PNACL_BIN_EXT}"
      ${NACL_TRANSLATE_OPTIONS}
      -arch ${arch}
      ${FILE}
      -o ${DIRNAME}/${BASENAME}${suffix}.nexe
  )
  add_custom_command(
    OUTPUT ${DIRNAME}/${BASENAME}${suffix}-stripped.nexe
    COMMENT "Stripping ${target} (${arch})"
    DEPENDS ${DIRNAME}/${BASENAME}${suffix}.nexe
    COMMAND
      "${PLATFORM_PREFIX}/${PLATFORM_TRIPLET}-strip${PNACL_BIN_EXT}"
      -s
      ${DIRNAME}/${BASENAME}${suffix}.nexe
      -o ${DIRNAME}/${BASENAME}${suffix}-stripped.nexe
  )
  add_custom_target(${target}${suffix} ALL
      DEPENDS ${DIRNAME}/${BASENAME}${suffix}-stripped.nexe
  )
  add_dependencies(${target}${suffix} ${target})
endfunction()
