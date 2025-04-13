#!/usr/bin/env bash

# Exit on undefined variable and error.
set -u -e -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
WORK_DIR="${PWD}"

# Do not reuse self-built curl from external_deps custom PATH
# to download source archives or we would get errors like:
#   curl: (1) Protocol "https" not supported or disabled in libcurl
CURL="$(command -v curl)"

# This should match the DEPS_VERSION in CMakeLists.txt.
# This is mostly to ensure the path the files end up at if you build deps yourself
# are the same as the ones when extracting from the downloaded packages.
DEPS_VERSION=10

# Package download pages
PKGCONFIG_BASEURL='https://pkg-config.freedesktop.org/releases'
NASM_BASEURL='https://www.nasm.us/pub/nasm/releasebuilds'
ZLIB_BASEURL='https://zlib.net/fossils'
GMP_BASEURL='https://gmplib.org/download/gmp'
NETTLE_BASEURL='https://mirror.cyberbits.eu/gnu/nettle'
CURL_BASEURL='https://curl.se/download'
SDL2_BASEURL='https://www.libsdl.org/release'
GLEW_BASEURL='https://github.com/nigels-com/glew/releases'
# Index: https://download.sourceforge.net/libpng/files/libpng16
PNG_BASEURL='https://sourceforge.net/projects/libpng/files/libpng16'
# Index: https://downloads.sourceforge.net/project/libjpeg-turbo
JPEG_BASEURL='https://sourceforge.net/projects/libjpeg-turbo/files'
# Index: https://storage.googleapis.com/downloads.webmproject.org/releases/webp/index.html
WEBP_BASEURL='https://storage.googleapis.com/downloads.webmproject.org/releases/webp'
OPENAL_BASEURL='https://openal-soft.org/openal-releases'
OGG_BASEURL='https://downloads.xiph.org/releases/ogg'
VORBIS_BASEURL='https://downloads.xiph.org/releases/vorbis'
OPUS_BASEURL='https://downloads.xiph.org/releases/opus'
OPUSFILE_BASEURL='https://downloads.xiph.org/releases/opus'
# No index.
NACLSDK_BASEURL='https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk'
# No index.
NACLRUNTIME_BASEURL='https://api.github.com/repos/DaemonEngine/native_client/zipball'
NCURSES_BASEURL='https://ftpmirror.gnu.org/gnu/ncurses'
WASISDK_BASEURL='https://github.com/WebAssembly/wasi-sdk/releases'
WASMTIME_BASEURL='https://github.com/bytecodealliance/wasmtime/releases'

# Package versions
PKGCONFIG_VERSION=0.29.2
NASM_VERSION=2.16.01
ZLIB_VERSION=1.2.13
GMP_VERSION=6.2.1
NETTLE_VERSION=3.8.1
CURL_VERSION=7.83.1
SDL2_VERSION=2.26.5
GLEW_VERSION=2.2.0
PNG_VERSION=1.6.39
JPEG_VERSION=2.1.5.1
WEBP_VERSION=1.3.2
OPENAL_VERSION=1.23.1
OGG_VERSION=1.3.5
VORBIS_VERSION=1.3.7
OPUS_VERSION=1.4
OPUSFILE_VERSION=0.12
NACLSDK_VERSION=44.0.2403.155
NACLRUNTIME_REVISION=2aea5fcfce504862a825920fcaea1a8426afbd6f
NCURSES_VERSION=6.2
WASISDK_VERSION=16.0
WASMTIME_VERSION=2.0.2

# Require the compiler names to be explicitly hardcoded, we should not inherit them
# from environment as we heavily cross-compile.
CC='false'
CXX='false'
# Set defaults.
LD='ld'
AR='ar'
RANLIB='ranlib'
CONFIGURE_SHARED=(--disable-shared --enable-static)
# Always reset flags, we heavily cross-compile and must not inherit any stray flag
# from environment.
CFLAGS=''
CXXFLAGS=''
CPPFLAGS=''
LDFLAGS=''

log() {
	level="${1}"; shift
	printf '%s: %s\n' "${level^^}" "${@}" >&2
	[ "${level}" != 'error' ]
}

# Extract an archive into the given subdirectory of the build dir and cd to it
# Usage: extract <filename> <directory>
extract() {
	rm -rf "${2}"
	mkdir -p "${2}"
	case "${1}" in
	*.tar.bz2)
		tar xjf "${1}" -C "${2}"
		;;
	*.tar.xz)
		tar xJf "${1}" -C "${2}"
		;;
	*.tar.gz|*.tgz)
		tar xzf "${1}" -C "${2}"
		;;
	*.zip)
		unzip -d "${2}" "${1}"
		;;
	*.cygtar.bz2)
		# Some Windows NaCl SDK packages have incorrect symlinks, so use
		# cygtar to extract them.
		"${SCRIPT_DIR}/cygtar.py" -xjf "${1}" -C "${2}"
		;;
	*.dmg)
		mkdir -p "${2}-dmg"
		hdiutil attach -mountpoint "${2}-dmg" "${1}"
		cp -R "${2}-dmg/"* "${2}/"
		hdiutil detach "${2}-dmg"
		rmdir "${2}-dmg"
		;;
	*)
		log error "Unknown archive type for ${1}"
		;;
	esac
	cd "${2}"
}

download() {
	local tarball_file="${1}"; shift

	while [ ! -f "${tarball_file}" ]; do
		if [ -z "${1:-}" ]
		then
			log error "No more mirror to download ${tarball_file} from"
		fi
		local download_url="${1}"; shift
		log status "Downloading ${download_url}"
		if ! "${CURL}" -R -L --fail -o "${tarball_file}" "${download_url}"
		then
			log warning "Failed to download ${download_url}"
			rm -f "${tarball_file}"
		fi
	done
}

# Download a file if it doesn't exist yet, and extract it into the build dir
# Usage: download <filename> <URL> <dir>
download_extract() {
	local extract_dir="${BUILD_DIR}/${1}"; shift
	local our_mirror="https://dl.unvanquished.net/deps/original/${1}"
	local tarball_file="${DOWNLOAD_DIR}/${1}"; shift

	if "${require_theirs}"
	then
		download "${tarball_file}" "${@}"
	elif "${prefer_ours}"
	then
		download "${tarball_file}" "${our_mirror}" "${@}"
	else
		download "${tarball_file}" "${@}" "${our_mirror}"
	fi

	"${download_only}" && return

	extract "${tarball_file}" "${extract_dir}"
}

# Build pkg-config
build_pkgconfig() {
	local dir_name="pkg-config-${PKGCONFIG_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract pkgconfig "${archive_name}" \
		"${PKGCONFIG_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --with-internal-glib
	make
	make install
}

# Build NASM
build_nasm() {
	case "${PLATFORM}" in
	macos-*-*)
		local dir_name="nasm-${NASM_VERSION}"
		local archive_name="${dir_name}-macosx.zip"

		download_extract nasm "${archive_name}" \
			"${NASM_BASEURL}/${NASM_VERSION}/macosx/${archive_name}"

		"${download_only}" && return

		cp "${dir_name}/nasm" "${PREFIX}/bin"
		;;
	*)
		log error 'Unsupported platform for NASM'
		;;
	esac
}

# Build zlib
build_zlib() {
	# Only the latest zlib version is provided as tar.xz, the
	# older ones in fossils/ folder are only provided as tar.gz.
	local dir_name="zlib-${ZLIB_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract zlib "${archive_name}" \
		"${ZLIB_BASEURL}/${archive_name}" \
		"https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	case "${PLATFORM}" in
	windows-*-*)
		LOC="${CFLAGS}" make -f win32/Makefile.gcc PREFIX="${HOST}-"
		make -f win32/Makefile.gcc install BINARY_PATH="${PREFIX}/bin" LIBRARY_PATH="${PREFIX}/lib" INCLUDE_PATH="${PREFIX}/include" SHARED_MODE=1
		;;
	*)
		# The default -O3 is dropped when there's user-provided CFLAGS.
		CFLAGS="${CFLAGS} -O3" ./configure --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --static --const
		make
		make install
		;;
	esac
}

# Build GMP
build_gmp() {
	local dir_name="gmp-${GMP_VERSION}"
	local archive_name="${dir_name}.tar.bz2"

	download_extract gmp "${archive_name}" \
		"${GMP_BASEURL}/${archive_name}" \
		"https://ftpmirror.gnu.org/gnu/gmp/${archive_name}" \
		"https://ftp.gnu.org/gnu/gmp/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	case "${PLATFORM}" in
	windows-*-msvc)
		# Configure script gets confused if we override the compiler. Shouldn't
		# matter since gmp doesn't use anything from libgcc.
		local CC_BACKUP="${CC}"
		local CXX_BACKUP="${CXX}"
		unset CC
		unset CXX
		;;
	esac

	# The default -O2 is dropped when there's user-provided CFLAGS.
	case "${PLATFORM}" in
	macos-*-*)
		# The assembler objects are incompatible with PIE
		CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}" --disable-assembly
		;;
	*)
		CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
		;;
	esac

	make
	make install
	case "${PLATFORM}" in
	windows-*-msvc)
		export CC="${CC_BACKUP}"
		export CXX="${CXX_BACKUP}"
		;;
	esac
}

# Build Nettle
build_nettle() {
	local dir_name="nettle-${NETTLE_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract nettle "${archive_name}" \
		"${NETTLE_BASEURL}/${archive_name}" \
		"https://ftp.gnu.org/gnu/nettle/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
	make
	make install
}

# Build cURL
build_curl() {
	local dir_name="curl-${CURL_VERSION}"
	local archive_name="${dir_name}.tar.xz"

	download_extract curl "${archive_name}" \
		"${CURL_BASEURL}/${archive_name}" \
		"https://github.com/curl/curl/releases/download/curl-${CURL_VERSION//./_}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The user-provided CFLAGS doesn't drop the default -O2
	./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --without-ssl --without-libssh2 --without-librtmp --without-libidn2 --without-brotli --without-zstd --disable-file --disable-ldap --disable-crypto-auth --disable-gopher --disable-ftp --disable-tftp --disable-dict --disable-imap --disable-mqtt --disable-smtp --disable-pop3 --disable-telnet --disable-rtsp --disable-threaded-resolver --disable-alt-svc "${CONFIGURE_SHARED[@]}"
	make
	make install
}

# Build SDL2
build_sdl2() {
	local dir_name="SDL2-${SDL2_VERSION}"

	case "${PLATFORM}" in
	windows-*-mingw)
		local archive_name="SDL2-devel-${SDL2_VERSION}-mingw.tar.gz"
		;;
	windows-*-msvc)
		local archive_name="SDL2-devel-${SDL2_VERSION}-VC.zip"
		;;
	macos-*-*)
		local archive_name="SDL2-${SDL2_VERSION}.dmg"
		;;
	*)
		local archive_name="SDL2-${SDL2_VERSION}.tar.gz"
		;;
	esac

	download_extract sdl2 "${archive_name}" \
		"${SDL2_BASEURL}/${archive_name}" \
		"https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/${archive_name}"

	"${download_only}" && return

	case "${PLATFORM}" in
	windows-*-mingw)
		cd "${dir_name}"
		make install-package arch="${HOST}" prefix="${PREFIX}"
		;;
	windows-*-msvc)
		cd "${dir_name}"
		mkdir -p "${PREFIX}/SDL2/cmake"
		cp "cmake/"* "${PREFIX}/SDL2/cmake"
		mkdir -p "${PREFIX}/SDL2/include"
		cp "include/"* "${PREFIX}/SDL2/include"

		case "${PLATFORM}" in
		*-i686-*)
			local sdl2_lib_dir='lib/x86'
			;;
		*-amd64-*)
			local sdl2_lib_dir='lib/x64'
			;;
		*)
			log error 'Unsupported platform for SDL2'
			;;
		esac

		mkdir -p "${PREFIX}/SDL2/${sdl2_lib_dir}"
		cp "${sdl2_lib_dir}/"{SDL2.lib,SDL2main.lib} "${PREFIX}/SDL2/${sdl2_lib_dir}"
		cp "${sdl2_lib_dir}/"*.dll "${PREFIX}/SDL2/${sdl2_lib_dir}"
		;;
	macos-*-*)
		rm -rf "${PREFIX}/SDL2.framework"
		cp -R "SDL2.framework" "${PREFIX}"
		;;
	*)
		cd "${dir_name}"
		# The default -O3 is dropped when there's user-provided CFLAGS.
		CFLAGS="${CFLAGS} -O3" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
		make
		make install
		# Workaround for an SDL2 CMake bug, we need to provide
		# a bin/ directory even when nothing is used from it.
		mkdir -p "${PREFIX}/bin"
		# We don't keep empty folders.
		touch "${PREFIX}/bin/.keep"
		;;
	esac
}

# Build GLEW
build_glew() {
	local dir_name="glew-${GLEW_VERSION}"
	local archive_name="${dir_name}.tgz"

	download_extract glew "${archive_name}" \
		"${GLEW_BASEURL}/download/glew-${GLEW_VERSION}/${archive_name}" \
		"https://downloads.sourceforge.net/project/glew/glew/${GLEW_VERSION}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	case "${PLATFORM}" in
	windows-*-*)
		make SYSTEM="linux-mingw${BITNESS}" GLEW_DEST="${PREFIX}" CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" STRIP="${HOST}-strip" LD="${LD}" CFLAGS.EXTRA="${CFLAGS}" LDFLAGS.EXTRA="${LDFLAGS}"
		make install SYSTEM="linux-mingw${BITNESS}" GLEW_DEST="${PREFIX}" CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" STRIP="${HOST}-strip" LD="${LD}" CFLAGS.EXTRA="${CFLAGS}" LDFLAGS.EXTRA="${LDFLAGS}"
		mv "${PREFIX}/lib/glew32.dll" "${PREFIX}/bin/"
		rm "${PREFIX}/lib/libglew32.a"
		cp lib/libglew32.dll.a "${PREFIX}/lib/"
		;;
	macos-*-*)
		make SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="${CC}" LD="${CC}" CFLAGS.EXTRA="${CFLAGS} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS}"
		make install SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="${CC}" LD="${CC}" CFLAGS.EXTRA="${CFLAGS} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS}"
		install_name_tool -id "@rpath/libGLEW.${GLEW_VERSION}.dylib" "${PREFIX}/lib/libGLEW.${GLEW_VERSION}.dylib"
		;;
	linux-*-*)
		make GLEW_DEST="${PREFIX}" CC="${CC}" LD="${CC}" CFLAGS.EXTRA="${CFLAGS}" LDFLAGS.EXTRA="${LDFLAGS}"
		make install GLEW_DEST="${PREFIX}" CC="${CC}" LD="${CC}" CFLAGS.EXTRA="${CFLAGS}" LDFLAGS.EXTRA="${LDFLAGS}" LIBDIR="${PREFIX}/lib"
		;;
	*)
		log error 'Unsupported platform for GLEW'
		;;
	esac
}

# Build PNG
build_png() {
	local dir_name="libpng-${PNG_VERSION}"
	local archive_name="${dir_name}.tar.xz"

	download_extract png "${archive_name}" \
		"${PNG_BASEURL}/${PNG_VERSION}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
	make
	make install
}

# Build JPEG
build_jpeg() {
	local dir_name="libjpeg-turbo-${JPEG_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract jpeg "${archive_name}" \
		"${JPEG_BASEURL}/${JPEG_VERSION}/${archive_name}"

	"${download_only}" && return

	case "${PLATFORM}" in
	windows-*-*)
		local SYSTEM_NAME='Windows'
		;;
	macos-*-*)
		local SYSTEM_NAME='Darwin'
		;;
	linux-*-*)
		local SYSTEM_NAME='Linux'
		;;
	*)
		# Other platforms can build but we need need to explicitly
		# set CMAKE_SYSTEM_NAME for CMAKE_CROSSCOMPILING to be set
		# and CMAKE_SYSTEM_PROCESSOR to not be ignored by cmake.
		log error 'Unsupported platform for JPEG'
		;;
	esac

	case "${PLATFORM}" in
	*-amd64-*)
		local SYSTEM_PROCESSOR='x86_64'
		# Ensure NASM is available
		nasm --help >/dev/null
		;;
	*-i686-*)
		local SYSTEM_PROCESSOR='i386'
		# Ensure NASM is available
		nasm --help >/dev/null
		;;
	*-arm64-*)
		local SYSTEM_PROCESSOR='aarch64'
		;;
	*-armhf-*)
		local SYSTEM_PROCESSOR='arm'
		;;
	*)
		log error 'Unsupported platform for JPEG'
		;;
	esac

	local jpeg_cmake_call=(cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_C_FLAGS="${CFLAGS}" -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
		-DCMAKE_SYSTEM_NAME="${SYSTEM_NAME}" -DCMAKE_SYSTEM_PROCESSOR="${SYSTEM_PROCESSOR}" \
		-DWITH_JPEG8=1)

	cd "${dir_name}"
	case "${PLATFORM}" in
	windows-*-mingw)
		"${jpeg_cmake_call[@]}" -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/../cmake/cross-toolchain-mingw${BITNESS}.cmake" -DENABLE_SHARED=0
		;;
	windows-*-msvc)
		"${jpeg_cmake_call[@]}" -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/../cmake/cross-toolchain-mingw${BITNESS}.cmake" -DENABLE_SHARED=1
		;;
	*)
		"${jpeg_cmake_call[@]}" -DENABLE_SHARED=0
		;;
	esac
	make -C build
	make -C build install
}

# Build WebP
build_webp() {
	local dir_name="libwebp-${WEBP_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract webp "${archive_name}" \
		"${WEBP_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --disable-libwebpdemux "${CONFIGURE_SHARED[@]}"
	make
	make install
}

# Build OpenAL
build_openal() {
	case "${PLATFORM}" in
	windows-*-*)
		local dir_name="openal-soft-${OPENAL_VERSION}-bin"
		local archive_name="${dir_name}.zip"
		;;
	macos-*-*|linux-*-*)
		local dir_name="openal-soft-${OPENAL_VERSION}"
		local archive_name="${dir_name}.tar.bz2"
		local openal_cmake_call=(cmake -S . -B . -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
			-DCMAKE_C_FLAGS="${CFLAGS}" -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
			-DCMAKE_BUILD_TYPE=Release -DALSOFT_EXAMPLES=OFF)
		;;
	*)
		log error 'Unsupported platform for OpenAL'
		;;
	esac

	download_extract openal "${archive_name}" \
		"${OPENAL_BASEURL}/${archive_name}" \
		"https://github.com/kcat/openal-soft/releases/download/${OPENAL_VERSION}/${archive_name}" \

	"${download_only}" && return

	case "${PLATFORM}" in
	windows-*-*)
		cd "${dir_name}"
		cp -r "include/AL" "${PREFIX}/include"
		case "${PLATFORM}" in
		*-i686-*)
			cp "libs/Win32/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win32/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		*-amd64-*)
			cp "libs/Win64/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win64/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		esac
		;;
	macos-*-*)
		cd "${dir_name}"
		"${openal_cmake_call[@]}"
		make
		make install
		install_name_tool -id "@rpath/libopenal.${OPENAL_VERSION}.dylib" "${PREFIX}/lib/libopenal.${OPENAL_VERSION}.dylib"
		;;
	linux-*-*)
		cd "${dir_name}"
		"${openal_cmake_call[@]}" -DLIBTYPE=STATIC 
		make
		make install
		;;
	esac
}

# Build Ogg
build_ogg() {
	local dir_name="libogg-${OGG_VERSION}"
	local archive_name="libogg-${OGG_VERSION}.tar.xz"

	download_extract ogg "${archive_name}" \
		"${OGG_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# This header breaks the vorbis and opusfile Mac builds
	cat <(echo '#include <stdint.h>') include/ogg/os_types.h > os_types.tmp
	mv os_types.tmp include/ogg/os_types.h
	# The user-provided CFLAGS doesn't drop the default -O2
	./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
	make
	make install
}

# Build Vorbis
build_vorbis() {
	local dir_name="libvorbis-${VORBIS_VERSION}"
	local archive_name="${dir_name}.tar.xz"

	download_extract vorbis "${archive_name}" \
		"${VORBIS_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The user-provided CFLAGS doesn't drop the default -O3
	./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}" --disable-examples
	make
	make install
}

# Build Opus
build_opus() {
	local dir_name="opus-${OPUS_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract opus "${archive_name}" \
		"${OPUS_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	case "${PLATFORM}" in
	windows-*-*)
		# With MinGW _FORTIFY_SOURCE (added by configure) can only by used with -fstack-protector enabled.
		CFLAGS="${CFLAGS} -O2 -D_FORTIFY_SOURCE=0" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
		;;
	*)
		CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}"
		;;
	esac
	make
	make install
}

# Build OpusFile
build_opusfile() {
	local dir_name="opusfile-${OPUSFILE_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract opusfile "${archive_name}" \
		"${OPUSFILE_BASEURL}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" "${CONFIGURE_SHARED[@]}" --disable-http
	make
	make install
}

# Build ncurses
build_ncurses() {
	local dir_name="ncurses-${NCURSES_VERSION}"
	local archive_name="${dir_name}.tar.gz"

	download_extract ncurses "${archive_name}" \
		"${NCURSES_BASEURL}/${archive_name}" \
		"https://ftp.gnu.org/pub/gnu/ncurses/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	# Configure terminfo search dirs based on the ones used in Debian. By default it will only look in (only) the install directory.
	CFLAGS="${CFLAGS} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --libdir="${PREFIX}/lib" --enable-widec "${CONFIGURE_SHARED[@]}" --with-terminfo-dirs=/etc/terminfo:/lib/terminfo --with-default-terminfo-dir=/usr/share/terminfo
	make
	make install
}

# "Builds" (downloads) the WASI SDK
build_wasisdk() {
	case "${PLATFORM}" in
	windows-*-*)
		local WASISDK_PLATFORM=mingw
		;;
	macos-*-*)
		local WASISDK_PLATFORM=macos
		;;
	linux-*-*)
		local WASISDK_PLATFORM=linux
		;;
	esac
	case "${PLATFORM}" in
	*-amd64-*)
		;;
	*)
		log error "wasi doesn't have release for ${PLATFORM}"
		;;
	esac

	local dir_name="wasi-sdk-${WASISDK_VERSION}"
	local archive_name="${dir_name}-${WASISDK_PLATFORM}.tar.gz"
	local WASISDK_VERSION_MAJOR="$(echo "${WASISDK_VERSION}" | cut -f1 -d'.')"

	download_extract wasisdk "${archive_name}" \
		"${WASISDK_BASEURL}/download/wasi-sdk-${WASISDK_VERSION_MAJOR}/${archive_name}"

	"${download_only}" && return

	cp -r "${dir_name}" "${PREFIX}/wasi-sdk"
}

# "Builds" (downloads) wasmtime
build_wasmtime() {
	case "${PLATFORM}" in
	windows-*-*)
		local WASMTIME_PLATFORM=windows
		local ARCHIVE_EXT=zip
		;;
	macos-*-*)
		local WASMTIME_PLATFORM=macos
		local ARCHIVE_EXT=tar.xz
		;;
	linux-*-*)
		local WASMTIME_PLATFORM=linux
		local ARCHIVE_EXT=tar.xz
		;;
	esac
	case "${PLATFORM}" in
	*-amd64-*)
		local WASMTIME_ARCH=x86_64
		;;
	linux-arm64-*|macos-arm64-*)
		local WASMTIME_ARCH=aarch64
		;;
	*)
		log error "wasmtime doesn't have release for ${PLATFORM}"
		;;
	esac

	local dir_name="wasmtime-v${WASMTIME_VERSION}-${WASMTIME_ARCH}-${WASMTIME_PLATFORM}-c-api"
	local archive_name="${folder_name}.${ARCHIVE_EXT}"

	download_extract wasmtime "${archive_name}" \
		"${WASMTIME_BASEURL}/download/v${WASMTIME_VERSION}/${archive_name}"

	"${download_only}" && return

	cd "${dir_name}"
	cp -r include/* "${PREFIX}/include"
	cp -r lib/* "${PREFIX}/lib"
}

# Build the NaCl SDK
build_naclsdk() {
	case "${PLATFORM}" in
	windows-*-*)
		local NACLSDK_PLATFORM=win
		local EXE=.exe
		local TAR_EXT=cygtar
		;;
	macos-*-*)
		local NACLSDK_PLATFORM=mac
		local EXE=
		local TAR_EXT=tar
		;;
	linux-*-*)
		local NACLSDK_PLATFORM=linux
		local EXE=
		local TAR_EXT=tar
		;;
	esac
	case "${PLATFORM}" in
	*-i686-*)
		local NACLSDK_ARCH=x86_32
		local DAEMON_ARCH=i686
		;;
	*-amd64-*)
		local NACLSDK_ARCH=x86_64
		local DAEMON_ARCH=amd64
		;;
	*-armhf-*|linux-arm64-*)
		local NACLSDK_ARCH=arm
		local DAEMON_ARCH=armhf
		;;
	esac

	local archive_name="naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}.${TAR_EXT}.bz2"

	download_extract naclsdk "${archive_name}" \
		"${NACLSDK_BASEURL}/${NACLSDK_VERSION}/naclsdk_${NACLSDK_PLATFORM}.tar.bz2"

	"${download_only}" && return

	cp pepper_*"/tools/irt_core_${NACLSDK_ARCH}.nexe" "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
	case "${PLATFORM}" in
	linux-amd64-*)
		;; # Get sel_ldr from naclruntime package
	*)
		cp pepper_*"/tools/sel_ldr_${NACLSDK_ARCH}${EXE}" "${PREFIX}/nacl_loader${EXE}"
		;;
	esac
	case "${PLATFORM}" in
	windows-i686-*|*-amd64-*)
		cp pepper_*"/toolchain/${NACLSDK_PLATFORM}_x86_newlib/bin/x86_64-nacl-gdb${EXE}" "${PREFIX}/nacl-gdb${EXE}"

		rm -rf "${PREFIX}/pnacl"

		patch -d pepper_*"/toolchain/${NACLSDK_PLATFORM}_pnacl/bin/pydir" \
			-p1 < "${SCRIPT_DIR}/naclsdk-pydir-python3.patch" >/dev/null

		cp -a pepper_*"/toolchain/${NACLSDK_PLATFORM}_pnacl" "${PREFIX}/pnacl"
		rm -rf "${PREFIX}/pnacl/bin/"{i686,x86_64}-nacl-*
		rm -rf "${PREFIX}/pnacl/arm-nacl"
		rm -rf "${PREFIX}/pnacl/arm_bc-nacl"
		rm -rf "${PREFIX}/pnacl/docs"
		rm -rf "${PREFIX}/pnacl/i686_bc-nacl"
		rm -rf "${PREFIX}/pnacl/include"
		rm -rf "${PREFIX}/pnacl/x86_64-nacl"
		rm -rf "${PREFIX}/pnacl/x86_64_bc-nacl"
	esac
	case "${PLATFORM}" in
	windows-i686-*)
		cp pepper_*"/tools/sel_ldr_x86_64.exe" "${PREFIX}/nacl_loader-amd64.exe"
		cp pepper_*"/tools/irt_core_x86_64.nexe" "${PREFIX}/irt_core-amd64.nexe"
		;;
	linux-amd64-*)
		# Fix permissions on a few files which deny access to non-owner
		chmod 644 "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
		;;
	linux-i686-*)
		cp pepper_*"/tools/nacl_helper_bootstrap_${NACLSDK_ARCH}" "${PREFIX}/nacl_helper_bootstrap"
		# Fix permissions on a few files which deny access to non-owner
		chmod 644 "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
		chmod 755 "${PREFIX}/nacl_helper_bootstrap" "${PREFIX}/nacl_loader"
		;;
	linux-armhf-*|linux-arm64-*)
		cp pepper_*"/tools/nacl_helper_bootstrap_arm" "${PREFIX}/nacl_helper_bootstrap"
		# Fix permissions on a few files which deny access to non-owner
		chmod 644 "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
		chmod 755 "${PREFIX}/nacl_helper_bootstrap" "${PREFIX}/nacl_loader"
		;;
	esac
	case "${PLATFORM}" in
	linux-arm64-*)
		mkdir -p "${PREFIX}/lib-armhf"
		cp -a pepper_*"/tools/lib/arm_trusted/lib/." "${PREFIX}/lib-armhf/."
		# Copy the library loader instead of renaming it because there may still be some
		# references to ld-linux-armhf.so.3 in binaries.
		cp "${PREFIX}/lib-armhf/ld-linux-armhf.so.3" "${PREFIX}/lib-armhf/ld-linux-armhf"
		# We can't use patchelf or 'nacl_helper_bootstrap nacl_loader' will complain:
		#   bootstrap_helper: nacl_loader: ELF file has unreasonable e_phnum=13
		sed -e 's|/lib/ld-linux-armhf.so.3|lib-armhf/ld-linux-armhf|' -i "${PREFIX}/nacl_loader"
		;;
	esac
}

# Only builds nacl_loader and nacl_helper_bootstrap for now, not IRT.
build_naclruntime() {
	case "${PLATFORM}" in
	linux-amd64-*)
		local NACL_ARCH=x86-64
		;;
	*)
		log error 'Unsupported platform for naclruntime'
		;;
	esac

	local dir_name="DaemonEngine-native_client-${NACLRUNTIME_REVISION:0:7}"
	local archive_name="native_client-${NACLRUNTIME_REVISION}.zip"

	download_extract naclruntime "${archive_name}" \
		"{$NACLRUNTIME_BASEURL}/${NACLRUNTIME_REVISION}"

	"${download_only}" && return

	cd "${dir_name}"
	python3 /usr/bin/scons --mode=opt-linux "platform=${NACL_ARCH}" werror=0 sysinfo=0 sel_ldr
	cp "scons-out/opt-linux-${NACL_ARCH}/staging/nacl_helper_bootstrap" "${PREFIX}/nacl_helper_bootstrap"
	cp "scons-out/opt-linux-${NACL_ARCH}/staging/sel_ldr" "${PREFIX}/nacl_loader"
}

# The import libraries generated by MinGW seem to have issues, so we use LLVM's version instead.
# So LLVM must be installed, e.g. 'sudo apt install llvm'
build_genlib() {
	"${download_only}" && return

	case "${PLATFORM}" in
	windows-*-msvc)
		mkdir -p "${PREFIX}/def"
		cd "${PREFIX}/def"
		for DLL_A in $(find "${PREFIX}/lib" -type f -name '*.dll.a'); do
			local DLL="$("${HOST}-dlltool" -I "${DLL_A}" 2> /dev/null || echo $(basename ${DLL_A} .dll.a).dll)"
			local DEF="$(basename ${DLL} .dll).def"
			local LIB="$(basename ${DLL_A} .dll.a).lib"

			case "${PLATFORM}" in
			*-i686-*)
				local MACHINE='i386'
				;;
			*-amd64-*)
				local MACHINE='i386:x86-64'
				;;
			*)
				log error 'Unsupported platform for genlib'
				;;
			esac

			# Using gendef from mingw-w64-tools
			gendef "${PREFIX}/bin/${DLL}"

			# Fix some issues with gendef output
			sed -i "s/\(glew.*\)@4@4/\1@4/;s/ov_halfrate_p@0/ov_halfrate_p/" "${DEF}"

			llvm-dlltool -d "${DEF}" -D "$(basename "${DLL}")" -m "${MACHINE}" -l "../lib/${LIB}"
		done
		;;
	*)
		log error 'Unsupported platform for genlib'
		;;
	esac
}

list_build() {
	local list_name="${1}"
	local package_list
	eval "package_list=(\${${list_name}_${PLATFORM//-/_}_packages})"
	for pkg in "${package_list[@]}"; do
		cd "${WORK_DIR}"
		"build_${pkg}"
	done
}

build_base() {
	list_build base
}

build_all() {
	list_build all
}

# Install all the necessary files to the location expected by CMake
build_install() {
	PKG_PREFIX="${WORK_DIR}/${PKG_BASEDIR}"
	rm -rf "${PKG_PREFIX}"
	rsync -a --link-dest="${PREFIX}" "${PREFIX}/" "${PKG_PREFIX}"

	# Ensure existence in case the selected set of deps didn't have these
	mkdir -p "${PKG_PREFIX}/bin"
	mkdir -p "${PKG_PREFIX}/include"
	mkdir -p "${PKG_PREFIX}/lib"

	# Remove all unneeded files
	rm -rf "${PKG_PREFIX}/man"
	rm -rf "${PKG_PREFIX}/def"
	rm -rf "${PKG_PREFIX}/share"
	rm -rf "${PKG_PREFIX}/lib/pkgconfig"
	find "${PKG_PREFIX}/bin" -not -type d -not -name '*.dll' -not -name '.keep' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.la' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.dll.a' -execdir bash -c 'rm -f -- "$(basename "{}" .dll.a).a"' \;
	find "${PKG_PREFIX}/lib" -name '*.dylib' -execdir bash -c 'rm -f -- "$(basename "{}" .dylib).a"' \;

	# Strip libraries
	case "${PLATFORM}" in
	windows-*-mingw)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		;;
	windows-*-msvc)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir rm -f -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.exp' -execdir rm -f -- {} \;
		;;
	esac

	# Remove empty directories
	find "${PKG_PREFIX}/" -mindepth 1 -type d -empty -delete
}

# Create a redistributable package for the dependencies
build_package() {
	cd "${WORK_DIR}"
	rm -f "${PKG_BASEDIR}.tar.xz"
	local XZ_OPT='-9'
	case "${PLATFORM}" in
	windows-*-*)
		tar --dereference -cvJf "${PKG_BASEDIR}.tar.xz" "${PKG_BASEDIR}"
		;;
	*)
		tar -cvJf "${PKG_BASEDIR}.tar.xz" "${PKG_BASEDIR}"
		;;
	esac
}

build_wipe() {
	rm -rf "${BUILD_BASEDIR}/" "${PKG_BASEDIR}/" "${PKG_BASEDIR}.tar.xz"
}

# Common setup code
common_setup() {
	HOST="${2}"
	"common_setup_${1}"
	common_setup_arch
	DOWNLOAD_DIR="${WORK_DIR}/download_cache"
	PKG_BASEDIR="${PLATFORM}_${DEPS_VERSION}"
	BUILD_BASEDIR="build-${PKG_BASEDIR}"
	BUILD_DIR="${WORK_DIR}/${BUILD_BASEDIR}"
	PREFIX="${BUILD_DIR}/prefix"
	PATH="${PREFIX}/bin:${PATH}"
	PKG_CONFIG="pkg-config"
	PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
	CPPFLAGS+=" -I${PREFIX}/include"
	LDFLAGS+=" -L${PREFIX}/lib"
	mkdir -p "${DOWNLOAD_DIR}"
	mkdir -p "${PREFIX}/bin"
	mkdir -p "${PREFIX}/include"
	mkdir -p "${PREFIX}/lib"
	export CC CXX LD AR RANLIB PKG_CONFIG PKG_CONFIG_PATH PATH CFLAGS CXXFLAGS CPPFLAGS LDFLAGS
}

common_setup_arch() {
	case "${PLATFORM}" in
	*-amd64-*)
		CFLAGS+=' -march=x86-64 -mcx16'
		CXXFLAGS+=' -march=x86-64 -mcx16'
		;;
	*-i686-*)
		CFLAGS+=' -march=i686 -msse2 -mfpmath=sse'
		CXXFLAGS+=' -march=i686 -msse2 -mfpmath=sse'
		;;
	*-arm64-*)
		CFLAGS+=' -march=armv8-a'
		CXXFLAGS+=' -march=armv8-a'
		;;
	*-armhf-*)
		CFLAGS+=' -march=armv7-a -mfpu=neon'
		CXXFLAGS+=' -march=armv7-a -mfpu=neon'
		;;
	*)
		log error 'Unsupported platform'
		;;
	esac
}

# -D__USE_MINGW_ANSI_STDIO=0 instructs MinGW to *not* use its own implementation
# of printf and instead use the system one. It's bad when MinGW uses its own
# implementation because this can cause extra DLL dependencies.
# The separate implementation exists because Microsoft's implementation at some
# point did not implement certain printf specifiers, in particular %lld (long long).
# Lua does use this one, which results in compiler warnings. But this is OK because
# the Windows build of Lua is only used in developer gamelogic builds, and Microsoft
# supports %lld since Visual Studio 2013. Also we don't build Lua anymore.
common_setup_msvc() {
	CONFIGURE_SHARED=(--enable-shared --disable-static)
	# Libtool bug prevents -static-libgcc from being set in LDFLAGS
	CC="${HOST}-gcc -static-libgcc"
	CXX="${HOST}-g++ -static-libgcc"
	LD="${HOST}-ld"
	AR="${HOST}-ar"
	RANLIB="${HOST}-ranlib"
	CFLAGS+=' -D__USE_MINGW_ANSI_STDIO=0'
}

common_setup_mingw() {
	CC="${HOST}-gcc"
	CXX="${HOST}-g++"
	LD="${HOST}-ld"
	AR="${HOST}-ar"
	RANLIB="${HOST}-ranlib"
	CFLAGS+=' -D__USE_MINGW_ANSI_STDIO=0'
}

common_setup_macos() {
	CC='clang'
	CXX='clang++'
	CFLAGS+=" -arch ${MACOS_ARCH}"
	CXXFLAGS+=" -arch ${MACOS_ARCH}"
	LDFLAGS+=" -arch ${MACOS_ARCH}"
	export CMAKE_OSX_ARCHITECTURES="${MACOS_ARCH}"
}

common_setup_linux() {
	CC="${HOST/-unknown-/-}-gcc"
	CXX="${HOST/-unknown-/-}-g++"
	CFLAGS+=' -fPIC'
	CXXFLAGS+=' -fPIC'
}

# Set up environment for 32-bit i686 Windows for Visual Studio (compile all as .dll)
setup_windows-i686-msvc() {
	BITNESS=32
	CFLAGS+=' -mpreferred-stack-boundary=2'
	CXXFLAGS+=' -mpreferred-stack-boundary=2'
	common_setup msvc i686-w64-mingw32
}

# Set up environment for 64-bit amd64 Windows for Visual Studio (compile all as .dll)
setup_windows-amd64-msvc() {
	BITNESS=64
	common_setup msvc x86_64-w64-mingw32
}

# Set up environment for 32-bit i686 Windows for MinGW (compile all as .a)
setup_windows-i686-mingw() {
	BITNESS=32
	common_setup mingw i686-w64-mingw32
}

# Set up environment for 64-bit amd64 Windows for MinGW (compile all as .a)
setup_windows-amd64-mingw() {
	BITNESS=64
	common_setup mingw x86_64-w64-mingw32
}

# Set up environment for 64-bit amd64 macOS
setup_macos-amd64-default() {
	MACOS_ARCH=x86_64
	export MACOSX_DEPLOYMENT_TARGET=10.12 # works with CMake
	common_setup macos x86_64-apple-darwin11
}

# Set up environment for 32-bit i686 Linux
setup_linux-i686-default() {
	common_setup linux i686-unknown-linux-gnu
}

# Set up environment for 64-bit amd64 Linux
setup_linux-amd64-default() {
	common_setup linux x86_64-unknown-linux-gnu
}

# Set up environment for 32-bit armhf Linux
setup_linux-armhf-default() {
	common_setup linux arm-unknown-linux-gnueabihf
}

# Set up environment for 64-bit arm Linux
setup_linux-arm64-default() {
	common_setup linux aarch64-unknown-linux-gnu
}

base_windows_amd64_msvc_packages='pkgconfig zlib gmp nettle curl sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk genlib'
all_windows_amd64_msvc_packages="${base_windows_amd64_msvc_packages}"

base_windows_i686_msvc_packages="${base_windows_amd64_msvc_packages}"
all_windows_i686_msvc_packages="${base_windows_amd64_msvc_packages}"

base_windows_amd64_mingw_packages='zlib gmp nettle curl sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk'
all_windows_amd64_mingw_packages="${base_windows_amd64_mingw_packages}"

base_windows_i686_mingw_packages="${base_windows_amd64_mingw_packages}"
all_windows_i686_mingw_packages="${base_windows_amd64_mingw_packages}"

base_macos_amd64_default_packages='pkgconfig nasm gmp nettle sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk'
all_macos_amd64_default_packages="${base_macos_amd64_default_packages}"

base_linux_amd64_default_packages='naclsdk naclruntime'
all_linux_amd64_default_packages='zlib gmp nettle curl sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk naclruntime'

base_linux_i686_default_packages="${base_linux_amd64_default_packages}"
all_linux_i686_default_packages="${all_linux_amd64_default_packages}"

base_linux_arm64_default_packages='naclsdk'
all_linux_arm64_default_packages='zlib gmp nettle curl sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk'

base_linux_armhf_default_packages="${base_linux_arm64_default_packages}"
all_linux_armhf_default_packages="${all_linux_arm64_default_packages}"

linux_build_platforms='linux-amd64-default linux-arm64-default linux-armhf-default linux-i686-default windows-amd64-mingw windows-amd64-msvc windows-i686-mingw windows-i686-msvc'
macos_build_platforms='macos-amd64-default'
all_platforms="$(echo ${linux_build_platforms} ${macos_build_platforms} | tr ' ' '\n' | sort -u | xargs echo)"

errorHelp() {
	sed -e 's/\\t/'$'\t''/g' <<-EOF
	usage: $(basename "${BASH_SOURCE[0]}") [OPTION] <PLATFORM> <PACKAGE[S]...>

	Script to build dependencies for platforms which do not provide them

	Options:
	\t--download-only — only download source packages, do not build them
	\t--prefer-ours — attempt to download from unvanquished.net first

	Platforms:
	\t${all_platforms}

	Virtual platforms:
	\tall: all platforms
	\tbuild-linux — platforms buildable on linux: ${linux_build_platforms}
	\tbuild-macos — platforms buildable on macos: ${macos_build_platforms}

	Packages:
	\tpkgconfig nasm zlib gmp nettle curl sdl2 glew png jpeg webp openal ogg vorbis opus opusfile naclsdk wasisdk wasmtime

	Virtual packages:
	\tbase — build packages for pre-built binaries to be downloaded when building the game
	\tall — build all supported packages that can possibly be involved in building the game
	\tinstall — create a stripped down version of the built packages that CMake can use
	\tpackage — create a zip/tarball of the dependencies so they can be distributed
	\twipe — remove products of build process, excepting download cache but INCLUDING installed files. Must be last

	Packages required for each platform:

	windows-amd64-msvc:
	windows-i686-msvc:
	\tbase: ${base_windows_amd64_msvc_packages}
	\tall: same

	windows-amd64-mingw:
	windows-i686-mingw:
	\tbase: ${base_windows_amd64_mingw_packages}
	\tall: same

	macos-amd64-default:
	\tbase: ${base_macos_amd64_default_packages}
	\tall: same

	linux-amd64-default:
	linux-i686-default:
	\tbase: ${base_linux_amd64_default_packages}
	\tall: ${all_linux_amd64_default_packages}

	linux-arm64-default:
	linux-armhf-default:
	\tbase: ${base_linux_arm64_default_packages}
	\tall: ${all_linux_arm64_default_packages}

	EOF
	false
}

download_only='false'
prefer_ours='false'
require_theirs='false'
while [ -n "${1:-}" ]
do
	case "${1-}" in
	'--download-only')
		download_only='true'
		shift
	;;
	'--prefer-ours')
		prefer_ours='true'
		shift
	;;
	'--require-theirs')
		require_theirs='true'
		shift
	;;
	'--'*)
		helpError
	;;
	*)
		break
	esac
done

# Usage
if [ "${#}" -lt "2" ]; then
	errorHelp
fi

# Enable parallel build
export MAKEFLAGS="-j`nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 1`"
export SCONSFLAGS="${MAKEFLAGS}"

# Setup platform
platform="${1}"; shift

platform_list=''
case "${platform}" in
'all')
	platform_list="${all_platforms}"
;;
'build-linux')
	platform_list="${linux_build_platforms}"
;;
'build-macos')
	platform_list="${macos_build_platforms}"
;;
*)
	for known_platform in ${all_platforms}
	do
		if [ "${platform}" = "${known_platform}" ]
		then
			platform_list="${platform}"
			break;
		fi
	done
	if [ -z "${platform_list}" ]
	then
		errorHelp
	fi
;;
esac

for PLATFORM in ${platform_list}
do (
	"setup_${PLATFORM}"

	# Build packages
	for pkg in "${@}"; do
		cd "${WORK_DIR}"
		"build_${pkg}"
	done
) done
