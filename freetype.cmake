set(FREETYPE_DIR ${DAEMON_DIR}/libs/freetype)
set(FREETYPE_INCLUDE_DIRS ${FREETYPE_DIR}/include)
set(FREETYPE_LIBRARIES freetype)

if (NACL)
	# Using Freetype's own zlib prevents the need for a zlib submodule when building the nexe cgame.
	set(FREETYPE_INTERNAL_ZLIB ON)
else()
	# Even if we can build an engine with Freetype using its internal zlib, we better rely on the
	# external zlib even if PREFER_EXTERNAL_LIBS is OFF, because then it will avoid zlib duplication
	# and share the same zlib between Freetype and the libpng.
	set(FREETYPE_INTERNAL_ZLIB OFF)
endif()

if (NOT FREETYPE_INTERNAL_ZLIB)
	find_package(ZLIB REQUIRED)
	set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARIES} ${ZLIB_LIBRARIES})
endif()

# Do not re-add the target if already set to be built.
# For example both the engine and a native game may request Freetype
# to be built, but we need to only build once for both.
if (NOT TARGET freetype)
	option(FT_DISABLE_BROTLI "Disable Brotli" ON)
	option(FT_DISABLE_BZIP2 "Disable bzip2" ON)
	option(FT_DISABLE_HARFBUZZ "Disable HarfBuzz" ON)
	option(FT_DISABLE_PNG "Disable PNG" ON)
	set(FT_DISABLE_ZLIB ${FREETYPE_INTERNAL_ZLIB} CACHE BOOL "Disable external zlib" FORCE)

	add_subdirectory(${FREETYPE_DIR})

	mark_as_advanced(FT_DISABLE_BROTLI)
	mark_as_advanced(FT_DISABLE_BZIP2)
	mark_as_advanced(FT_DISABLE_HARFBUZZ)
	mark_as_advanced(FT_DISABLE_PNG)
	mark_as_advanced(FT_DISABLE_ZLIB)
	mark_as_advanced(FT_ENABLE_ERROR_STRINGS)
endif()
