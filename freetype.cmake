set(FREETYPE_DIR ${DAEMON_DIR}/libs/freetype)
set(FREETYPE_INCLUDE_DIRS ${FREETYPE_DIR}/include)
set(FREETYPE_LIBRARIES freetype)

option(FT_DISABLE_BROTLI "Disable Brotli" ON)
option(FT_DISABLE_BZIP2 "Disable bzip2" ON)
option(FT_DISABLE_HARFBUZZ "Disable HarfBuzz" ON)
option(FT_DISABLE_PNG "Disable PNG" ON)

if (PREFER_EXTERNAL_LIBS AND NOT NACL)
	set(FREETYPE_INTERNAL_ZLIB OFF)
else()
	set(FREETYPE_INTERNAL_ZLIB ON)
endif()

set(FT_DISABLE_ZLIB ${FREETYPE_INTERNAL_ZLIB} CACHE BOOL "Disable external zlib" FORCE)

add_subdirectory(${FREETYPE_DIR})

mark_as_advanced(FT_DISABLE_BROTLI)
mark_as_advanced(FT_DISABLE_BZIP2)
mark_as_advanced(FT_DISABLE_HARFBUZZ)
mark_as_advanced(FT_DISABLE_PNG)
mark_as_advanced(FT_DISABLE_ZLIB)
mark_as_advanced(FT_ENABLE_ERROR_STRINGS)
