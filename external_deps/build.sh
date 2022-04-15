#!/usr/bin/env bash

# Exit on error
# Error on undefined variable
set -e
set -u

# /!\ Please do not use bash associative arrays,
# obsolete macOS GPLv2 bash 3.2.57 does not support them.

# /!\ Please do not use bash arrays for command,
# platform, and package lists,
# obsolete macOS GPLv2 bash 3.2.57 does not fully support them,
# and bash arrays do not preserve ordering anyway.

# When declaring a platform for a package, prefix it with : character
# to mark it as optional.

# Dependencies version. This number must be updated every time the
# version numbers below change, or packages are added/removed, or
# packages are rebuilt.
DEPS_VERSION=6

# Package versions
PKGCONFIG_VERSION=0.29.2
NASM_VERSION=2.15.05
ZLIB_VERSION=1.2.11
GMP_VERSION=6.2.0
NETTLE_VERSION=3.6
CURL_VERSION=7.73.0
SDL2_VERSION=2.0.12
GLEW_VERSION=2.2.0
PNG_VERSION=1.6.37
JPEG_VERSION=2.0.5
WEBP_VERSION=1.1.0
FREETYPE_VERSION=2.10.4
OPENAL_VERSION=1.21.1
OGG_VERSION=1.3.4
VORBIS_VERSION=1.3.7
OPUS_VERSION=1.3.1
OPUSFILE_VERSION=0.12
LUA_VERSION=5.4.1
NACLSDK_VERSION=44.0.2403.155
NCURSES_VERSION=6.2
WASISDK_VERSION=12.0
WASMTIME_VERSION=0.28.0

error() { echo "ERROR: ${@}" >&2; exit 1; }

to_lines() { echo "${@// /$'\n'}"; }

# Implements kind of associative array of command name and command description.
#   register_command install 'Create a stripped down version of the built packages that CMake can use.'
# is like:
#   command['install']='Create a stripped down version of the built packages that CMake can use.'
register_command() {
	if ! to_lines "${commands:-}" | egrep -q "^${1}"; then
		commands+="${commands:+ }${1}"
		eval "command_${1}='${2}'"
	fi
}

# Implement a kind of associative array of platform name and platform description.
#   register_platform linux-amd64-default 'Linux amd64 native compilation'
# is like:
#   platform['linux-amd64-default']='Linux amd64 native compilation'
register_platform() {
	# Bash variable names can't contain any hyphen.
	local platform="${1//-/_}"
	if ! to_lines "${platforms:-}" | egrep -q "^${platform}"; then
		platforms+="${platforms:+ }${platform}"
		eval "platform_${platform}='${2}'"
	fi
}

# Implement a kind of associative array of platform name and associative array
# of sorted list of required, optional, unused and all packages.
#  register_package pkgconfig :linux-amd64-default :windows-i686-mingw :windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
# is like:
#   package['linux-amd64-default']['optional'].append('pkgconfig')
#   package['windows-i686-mingw']['optional'].append('pkgconfig')
#   package['windows-amd64-mingw']['optional'].append('pkgconfig')
#   package['windows-i686-msvc']['required'].append('pkgconfig')
#   package['windows-amd64-msvc']['required'].append('pkgconfig')
#   package['macos-amd64-default']['required'].append('pkgconfig')
#   package['linux-amd64-default']['all'].append('pkgconfig')
#   package['windows-i686-mingw']['all'].append('pkgconfig')
#   package['windows-amd64-mingw']['all'].append('pkgconfig')
#   package['windows-i686-msvc']['all'].append('pkgconfig')
#   package['windows-amd64-msvc']['all'].append('pkgconfig')
#   package['macos-amd64-default']['all'].append('pkgconfig')
# if the package wasn't required by any platform, it would also do:
#   package['unused'].append('pkgconfig')
# It also checks for the platform having been declared before.
register_package() {
	local package="${1}"; shift
	if ! to_lines "${packages:-}" | egrep -q "^${package}$"; then
		packages+="${packages:+ }${package}"
	fi
	while [ -n "${1:-}" ]; do
		local platform="${1//-/_}"
		local selection='required'
		if echo "${platform}" | egrep -q '^:'; then
			platform="${platform:1}"
			selection='optional'
		else
			local used='true'
		fi
		if ! to_lines "${platforms:-}" | egrep -q "^${platform}$"; then
			error "Unknown platform ${platform}"
		elif ! to_lines "${package_platforms:-}" | egrep -q "^${platform}$"; then
			package_platforms+="${package_platforms:+ }${platform}"
		fi
		eval "all_packages_${platform}+=\"\${all_packages_${platform}:+ }${package}\""
		eval "${selection}_packages_${platform}+=\"\${${selection}_packages_${platform}:+ }${package}\""
		shift
	done
	if [ -z "${used:-}" ]; then
		packages_unused+="${packages_unused:+ }${package}"
	fi
}

# Extract an archive into the given subdirectory of the build dir and cd to it
# Usage: extract <filename> <directory>
extract() {
	echo "Extracting: ${1}"
	rm -rf "${BUILD_DIR}/${2}"
	mkdir -p "${BUILD_DIR}/${2}"
	case "${1}" in
	*.tar.bz2)
		tar xjf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.tar.xz)
		tar xJf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.tar.gz|*.tgz)
		tar xzf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.zip)
		unzip -d "${BUILD_DIR}/${2}" "${DOWNLOAD_DIR}/${1}"
		;;
	*.cygtar.bz2)
		# Some Windows NaCl SDK packages have incorrect symlinks, so use
		# cygtar to extract them.
		"${SCRIPT_DIR}/cygtar.py" -xjf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.dmg)
		# Mounting .dmg files requires filesystem locking feature.
		# If building on NFS and locking features does not work,
		# one may emulate locking feature with locallocks mount option.
		mkdir -p "${BUILD_DIR}/${2}-dmg"
		hdiutil attach -mountpoint "${BUILD_DIR}/${2}-dmg" "${DOWNLOAD_DIR}/${1}"
		cp -R "${BUILD_DIR}/${2}-dmg/"* "${BUILD_DIR}/${2}/"
		hdiutil detach "${BUILD_DIR}/${2}-dmg"
		rmdir "${BUILD_DIR}/${2}-dmg"
		;;
	*)
		error "Unknown archive type for ${1}"
		;;
	esac
	cd "${BUILD_DIR}/${2}"
}

# Download a file if it doesn't exist yet, and extract it into the build dir
# Usage: download <filename> <URL> <dir>
download() {
	echo "Downloading: ${2}"
	if [ ! -f "${DOWNLOAD_DIR}/${1}" ]; then
		# Retry download with wget if curl fails because
		# macOS curl, Homebrew curl and own build curl were
		# seen as failing to download Ogg tarball from Xiph.
		curl -L --fail -o "${DOWNLOAD_DIR}/${1}" "${2}" \
		|| wget --continue -O "${DOWNLOAD_DIR}/${1}" "${2}"
	fi
	extract "${1}" "${3}"
}

# Common setup code
setup_common() {
	WORK_DIR="${PWD}"
	SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
	DOWNLOAD_DIR="${WORK_DIR}/download_cache"
	BUILD_DIR="${WORK_DIR}/build-${PLATFORM}_${DEPS_VERSION}"
	PREFIX="${BUILD_DIR}/prefix"
	export PATH="${PATH}:${PREFIX}/bin"
	export PKG_CONFIG="pkg-config"
	export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
	export CPPFLAGS="${CPPFLAGS:-} -I${PREFIX}/include"
	export LDFLAGS="${LDFLAGS:-} -L${PREFIX}/lib"
	export MAKEFLAGS="-j$(nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 1)"
	mkdir -p "${DOWNLOAD_DIR}"
	mkdir -p "${PREFIX}"
	mkdir -p "${PREFIX}/bin"
	mkdir -p "${PREFIX}/include"
	mkdir -p "${PREFIX}/lib"
}

# Set up environment for 32-bit Windows for Visual Studio (compile all as .dll)
register_platform windows-i686-msvc 'Windows i686 native compilation'
setup_windows_i686_msvc() {
	HOST=i686-w64-mingw32
	CROSS="${HOST}-"
	BITNESS=32
	MSVC_SHARED=(--enable-shared --disable-static)
	# Libtool bug prevents -static-libgcc from being set in LDFLAGS
	export CC="i686-w64-mingw32-gcc -static-libgcc"
	export CXX="i686-w64-mingw32-g++ -static-libgcc"
	export CFLAGS="-msse2 -mpreferred-stack-boundary=2 -D__USE_MINGW_ANSI_STDIO=0"
	export CXXFLAGS="-msse2 -mpreferred-stack-boundary=2"
	setup_common
}

# Set up environment for 64-bit Windows for Visual Studio (compile all as .dll)
register_platform windows-amd64-msvc 'Windows amd64 native compilation'
setup_windows_amd64_msvc() {
	HOST=x86_64-w64-mingw32
	CROSS="${HOST}-"
	BITNESS=64
	MSVC_SHARED=(--enable-shared --disable-static)
	# Libtool bug prevents -static-libgcc from being set in LDFLAGS
	export CC="x86_64-w64-mingw32-gcc -static-libgcc"
	export CXX="x86_64-w64-mingw32-g++ -static-libgcc"
	export CFLAGS="-D__USE_MINGW_ANSI_STDIO=0"
	setup_common
}

# Set up environment for 32-bit Windows for MinGW (compile all as .a)
register_platform windows-i686-mingw 'Windows i686 MingW compilation or cross-compilation from Linux'
setup_windows_i686_mingw() {
	HOST=i686-w64-mingw32
	CROSS="${HOST}-"
	BITNESS=32
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m32 -msse2 -D__USE_MINGW_ANSI_STDIO=0"
	export CXXFLAGS="-m32 -msse2"
	setup_common
}

# Set up environment for 64-bit Windows for MinGW (compile all as .a)
register_platform windows-amd64-mingw 'Windows amd64 MingW compilation or cross-compilation from Linux'
setup_windows_amd64_mingw() {
	HOST=x86_64-w64-mingw32
	CROSS="${HOST}-"
	BITNESS=64
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64 -D__USE_MINGW_ANSI_STDIO=0"
	export CXXFLAGS="-m64"
	setup_common
}

# Set up environment for Mac OS X 64-bit
register_platform macos-amd64-default 'macOS amd64 native compilation'
setup_macos_amd64_default() {
	HOST=x86_64-apple-darwin11
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export MACOSX_DEPLOYMENT_TARGET=10.9
	export CC=clang
	export CXX=clang++
	export CFLAGS="-arch x86_64"
	export CXXFLAGS="-arch x86_64"
	export LDFLAGS="-arch x86_64"
	setup_common
}

# Set up environment for 64-bit Linux
register_platform linux-amd64-default 'Linux amd64 native compilation'
setup_linux_amd64_default() {
	HOST=x86_64-unknown-linux-gnu
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64 -fPIC"
	export CXXFLAGS="-m64 -fPIC"
	export LDFLAGS="-m64"
	setup_common
}

# Build pkg-config
register_package pkgconfig :linux-amd64-default :windows-i686-mingw :windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_pkgconfig() {
	download "pkg-config-${PKGCONFIG_VERSION}.tar.gz" "http://pkgconfig.freedesktop.org/releases/pkg-config-${PKGCONFIG_VERSION}.tar.gz" pkgconfig
	cd "pkg-config-${PKGCONFIG_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" --with-internal-glib
	make
	make install
}

# Build NASM
register_package nasm windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_nasm() {
	case "${PLATFORM}" in
	macos-*-*)
		download "nasm-${NASM_VERSION}-macosx.zip" "https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/macosx/nasm-${NASM_VERSION}-macosx.zip" nasm
		cp "nasm-${NASM_VERSION}/nasm" "${PREFIX}/bin"
		;;
	windows-*-*)
		download "nasm-${NASM_VERSION}-win32.zip" "https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/win32/nasm-${NASM_VERSION}-win32.zip" nasm
		cp "nasm-${NASM_VERSION}/nasm.exe" "${PREFIX}/bin"
		;;
	*)
		error "Unsupported platform for NASM"
		;;
	esac
}

# Build zlib
register_package zlib linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc :macos-amd64-default
build_zlib() {
	download "zlib-${ZLIB_VERSION}.tar.gz" "https://zlib.net/zlib-${ZLIB_VERSION}.tar.gz" zlib
	cd "zlib-${ZLIB_VERSION}"
	case "${PLATFORM}" in
	windows-*-*)
		LOC="${CFLAGS:-}" make -f win32/Makefile.gcc PREFIX="${CROSS}"
		make -f win32/Makefile.gcc install BINARY_PATH="${PREFIX}/bin" LIBRARY_PATH="${PREFIX}/lib" INCLUDE_PATH="${PREFIX}/include" SHARED_MODE=1
		;;
	macos-*-*|linux-*-*)
		./configure --prefix="${PREFIX}" --static --const
		make
		make install
		;;
	*)
		error "Unsupported platform for zlib"
		;;
	esac
}

# Build GMP
register_package gmp :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_gmp() {
	download "gmp-${GMP_VERSION}.tar.bz2" "https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.bz2" gmp
	cd "gmp-${GMP_VERSION}"
	case "${PLATFORM}" in
	*-*-msvc)
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
		CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-assembly
		;;
	*)
		CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
		;;
	esac

	make
	make install
	case "${PLATFORM}" in
	msvc*)
		export CC="${CC_BACKUP}"
		export CXX="${CXX_BACKUP}"
		;;
	esac
}

# Build Nettle
register_package nettle :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_nettle() {
	# download "nettle-${NETTLE_VERSION}.tar.gz" "https://www.lysator.liu.se/~nisse/archive/nettle-${NETTLE_VERSION}.tar.gz" nettle
	download "nettle-${NETTLE_VERSION}.tar.gz" "https://ftp.gnu.org/gnu/nettle/nettle-${NETTLE_VERSION}.tar.gz" nettle
	cd "nettle-${NETTLE_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build cURL
register_package curl :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc :macos-amd64-default
build_curl() {
	download "curl-${CURL_VERSION}.tar.bz2" "https://curl.haxx.se/download/curl-${CURL_VERSION}.tar.bz2" curl
	cd "curl-${CURL_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" --without-ssl --without-libssh2 --without-librtmp --without-libidn --disable-file --disable-ldap --disable-crypto-auth --disable-threaded-resolver ${MSVC_SHARED[@]}
	make
	make install
}

# Build SDL2
register_package sdl2 :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_sdl2() {
	case "${PLATFORM}" in
	windows-*-mingw)
		download "SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" "https://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" sdl2
		cd "SDL2-${SDL2_VERSION}"
		make install-package arch="${HOST}" prefix="${PREFIX}"
		;;
	windows-*-msvc)
		download "SDL2-devel-${SDL2_VERSION}-VC.zip" "https://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-VC.zip" sdl2
		cd "SDL2-${SDL2_VERSION}"
		mkdir -p "${PREFIX}/include/SDL2"
		cp include/* "${PREFIX}/include/SDL2"
		case "${PLATFORM}" in
		windows-i686-msvc)
			cp lib/x86/*.lib "${PREFIX}/lib"
			cp lib/x86/*.dll "${PREFIX}/bin"
			;;
		windows-amd64-msvc)
			cp lib/x64/*.lib "${PREFIX}/lib"
			cp lib/x64/*.dll "${PREFIX}/bin"
			;;
		esac
		;;
	macos-*-*)
		download "SDL2-${SDL2_VERSION}.dmg" "https://libsdl.org/release/SDL2-${SDL2_VERSION}.dmg" sdl2
		# macOS produces weird issue on NFS:
		# > cp: cannot overwrite directory external_deps/build-macos-amd64-default-5/prefix/SDL2.framework/Headers with non-directory SDL2.framework/Headers
		# while both look to be directories, so it's better to clean-up
		# before to prevent any issue occurring when overwriting.
		if [ -d "${PREFIX}/SDL2.framework" ]; then
			rm -r "${PREFIX}/SDL2.framework"
		fi
		cp -R 'SDL2.framework' "${PREFIX}"
		;;
	linux-*-*)
		download "SDL2-${SDL2_VERSION}.tar.gz" "https://www.libsdl.org/release/SDL2-${SDL2_VERSION}.tar.gz" sdl2
		cd "SDL2-${SDL2_VERSION}"
		# The default -O3 is dropped when there's user-provided CFLAGS.
		CFLAGS="${CFLAGS:-} -O3" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
		make
		make install
		;;
	*)
		error "Unsupported platform for GLEW"
		;;
	esac
}

# Build GLEW
register_package glew :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_glew() {
	download "glew-${GLEW_VERSION}.tgz" "https://downloads.sourceforge.net/project/glew/glew/${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz" glew
	cd "glew-${GLEW_VERSION}"
	case "${PLATFORM}" in
	windows-*-*)
		make SYSTEM="linux-mingw${BITNESS}" GLEW_DEST="${PREFIX}" CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" STRIP="${CROSS}strip" LD="${CROSS}ld" CFLAGS.EXTRA="${CFLAGS:-}" LDFLAGS.EXTRA="${LDFLAGS:-}"
		make install SYSTEM="linux-mingw${BITNESS}" GLEW_DEST="${PREFIX}" CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" STRIP="${CROSS}strip" LD="${CROSS}ld" CFLAGS.EXTRA="${CFLAGS:-}" LDFLAGS.EXTRA="${LDFLAGS:-}"
		mv "${PREFIX}/lib/glew32.dll" "${PREFIX}/bin/"
		rm "${PREFIX}/lib/libglew32.a"
		cp lib/libglew32.dll.a "${PREFIX}/lib/"
		;;
	macos-*-*)
		make SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		make install SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		install_name_tool -id "@rpath/libGLEW.${GLEW_VERSION}.dylib" "${PREFIX}/lib/libGLEW.${GLEW_VERSION}.dylib"
		;;
	linux-*-*)
		make GLEW_DEST="${PREFIX}"
		make install GLEW_DEST="${PREFIX}"
		;;
	*)
		error "Unsupported platform for GLEW"
		;;
	esac
}

# Build PNG
register_package png :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_png() {
	download "libpng-${PNG_VERSION}.tar.gz" "https://download.sourceforge.net/libpng/libpng-${PNG_VERSION}.tar.gz" png
	cd "libpng-${PNG_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build JPEG
register_package jpeg :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_jpeg() {
	echo $PATH
	download "libjpeg-turbo-${JPEG_VERSION}.tar.gz" "https://downloads.sourceforge.net/project/libjpeg-turbo/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz" jpeg
	cd "libjpeg-turbo-${JPEG_VERSION}"
	case "${PLATFORM}" in
	windows-*-mingw)
		cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/../cmake/cross-toolchain-mingw${BITNESS}.cmake" -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DWITH_JPEG8=1 -DENABLE_SHARED=0
		;;
	windows-*-msvc)
		CFLAGS="${CFLAGS:-} " cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/../cmake/cross-toolchain-mingw${BITNESS}.cmake" -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DWITH_JPEG8=1 -DENABLE_SHARED=1
		;;
	macos-*-*)
		# A newer version of nasm is required for 64-bit
		local NASM="${PREFIX}/bin/nasm"
		cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DWITH_JPEG8=1 -DENABLE_SHARED=0 -DCMAKE_ASM_NASM_COMPILER="${NASM}"
		;;
	linux-*-*)
		cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DWITH_JPEG8=1 -DENABLE_SHARED=0
		;;
	*)
		error "Unsupported platform for OpenAL"
		;;
	esac
	cmake --build build
	cmake --install build
}

# Build WebP
register_package webp :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_webp() {
	download "libwebp-${WEBP_VERSION}.tar.gz" "https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-${WEBP_VERSION}.tar.gz" webp
	cd "libwebp-${WEBP_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --disable-libwebpdemux ${MSVC_SHARED[@]}
	make
	make install
}

# Build FreeType
register_package freetype :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_freetype() {
	download "freetype-${FREETYPE_VERSION}.tar.gz" "https://download.savannah.gnu.org/releases/freetype/freetype-${FREETYPE_VERSION}.tar.gz" freetype
	cd "freetype-${FREETYPE_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --without-bzip2 --without-png --with-harfbuzz=no --with-brotli=no
	make
	make install
	cp -a "${PREFIX}/include/freetype2" "${PREFIX}/include/freetype"
	mv "${PREFIX}/include/freetype" "${PREFIX}/include/freetype2/freetype"
}

# Build OpenAL
register_package openal :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_openal() {
	case "${PLATFORM}" in
	windows-*-*)
		download "openal-soft-${OPENAL_VERSION}-bin.zip" "https://openal-soft.org/openal-binaries/openal-soft-${OPENAL_VERSION}-bin.zip" openal
		cd "openal-soft-${OPENAL_VERSION}-bin"
		cp -r "include/AL" "${PREFIX}/include"
		case "${PLATFORM}" in
		windows-i686-*)
			cp "libs/Win32/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win32/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		windows-amd64-*)
			cp "libs/Win64/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win64/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		esac
		;;
	macos-*-*)
		download "openal-soft-${OPENAL_VERSION}.tar.bz2" "https://openal-soft.org/openal-releases/openal-soft-${OPENAL_VERSION}.tar.bz2" openal
		cd "openal-soft-${OPENAL_VERSION}"
		cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" -DCMAKE_BUILD_TYPE=Release -DALSOFT_EXAMPLES=OFF
		make
		make install
		install_name_tool -id "@rpath/libopenal.${OPENAL_VERSION}.dylib" "${PREFIX}/lib/libopenal.${OPENAL_VERSION}.dylib"
		;;
	linux-*-*)
		download "openal-soft-${OPENAL_VERSION}.tar.bz2" "https://openal-soft.org/openal-releases/openal-soft-${OPENAL_VERSION}.tar.bz2" openal
		cd "openal-soft-${OPENAL_VERSION}"
		cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DALSOFT_EXAMPLES=OFF -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=Release .
		make
		make install
		;;
	*)
		error "Unsupported platform for OpenAL"
		;;
	esac
}

# Build Ogg
register_package ogg :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_ogg() {
	download "libogg-${OGG_VERSION}.tar.gz" "https://downloads.xiph.org/releases/ogg/libogg-${OGG_VERSION}.tar.gz" ogg
	cd "libogg-${OGG_VERSION}"
	# This header breaks the vorbis and opusfile Mac builds
	cat <(echo '#include <stdint.h>') include/ogg/os_types.h > os_types.tmp
	mv os_types.tmp include/ogg/os_types.h
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build Vorbis
register_package vorbis :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_vorbis() {
	download "libvorbis-${VORBIS_VERSION}.tar.gz" "https://downloads.xiph.org/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.gz" vorbis
	cd "libvorbis-${VORBIS_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-examples
	make
	make install
}

# Build Opus
register_package opus :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_opus() {
	download "opus-${OPUS_VERSION}.tar.gz" "https://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz" opus
	cd "opus-${OPUS_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	case "${PLATFORM}" in
	windows-*-*)
		# With MinGW _FORTIFY_SOURCE (added by configure) can only by used with -fstack-protector enabled.
		CFLAGS="${CFLAGS:-} -O2 -D_FORTIFY_SOURCE=0" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
		;;
	*)
		CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
		;;
	esac
	make
	make install
}

# Build OpusFile
register_package opusfile :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_opusfile() {
	download "opusfile-${OPUSFILE_VERSION}.tar.gz" "https://downloads.xiph.org/releases/opus/opusfile-${OPUSFILE_VERSION}.tar.gz" opusfile
	cd "opusfile-${OPUSFILE_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-http
	make
	make install
}

# Build Lua
register_package lua :linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_lua() {
	download "lua-${LUA_VERSION}.tar.gz" "https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz" lua
	cd "lua-${LUA_VERSION}"
	case "${PLATFORM}" in
	windows-*-*)
		local LUA_PLATFORM=mingw
		;;
	macos-*-*)
		local LUA_PLATFORM=macosx
		;;
	linux-*-*)
		local LUA_PLATFORM=linux
		;;
	*)
		error "Unsupported platform for Lua"
		;;
	esac
	make "${LUA_PLATFORM}" CC="${CC:-${CROSS}gcc}" AR="${CROSS}ar rcu" RANLIB="${CROSS}ranlib" MYCFLAGS="${CFLAGS:-}" MYLDFLAGS="${LDFLAGS}"
	case "${PLATFORM}" in
	windows-*-mingw)
		make install TO_BIN="lua.exe luac.exe" TO_LIB="liblua.a" INSTALL_TOP="${PREFIX}"
		;;
	windows-*-msvc)
		make install TO_BIN="lua.exe luac.exe lua54.dll" TO_LIB="liblua.a" INSTALL_TOP="${PREFIX}"
		touch "${PREFIX}/lib/lua54.dll.a"
		;;
	*)
		make install INSTALL_TOP="${PREFIX}"
		;;
	esac
}

# Build ncurses
register_package ncurses :linux-amd64-default :windows-i686-mingw :windows-amd64-mingw :windows-i686-msvc :windows-amd64-msvc :macos-amd64-default
build_ncurses() {
	download "ncurses-${NCURSES_VERSION}.tar.gz" "https://ftp.gnu.org/pub/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" ncurses
	cd "ncurses-${NCURSES_VERSION}"
	# The default -O2 is dropped when there's user-provided CFLAGS.
	# Configure terminfo search dirs based on the ones used in Debian. By default it will only look in (only) the install directory.
	CFLAGS="${CFLAGS:-} -O2" ./configure --host="${HOST}" --prefix="${PREFIX}" --enable-widec ${MSVC_SHARED[@]} --with-terminfo-dirs=/etc/terminfo:/lib/terminfo --with-default-terminfo-dir=/usr/share/terminfo
	make
	make install
}

# "Builds" (downloads) the WASI SDK
register_package wasisdk :linux-amd64-default :windows-i686-mingw :windows-amd64-mingw :windows-i686-msvc :windows-amd64-msvc :macos-amd64-default
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
	local WASISDK_VERSION_MAJOR="$(echo "${WASISDK_VERSION}" | cut -f1 -d'.')"
	download "wasi-sdk_${WASISDK_PLATFORM}.tar.gz" "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASISDK_VERSION_MAJOR}/wasi-sdk-${WASISDK_VERSION}-${WASISDK_PLATFORM}.tar.gz" wasisdk
	cp -r "wasi-sdk-${WASISDK_VERSION}" "${PREFIX}/wasi-sdk"
}

# "Builds" (downloads) wasmtime
register_package wasmtime :linux-amd64-default :windows-i686-mingw :windows-amd64-mingw :windows-i686-msvc :windows-amd64-msvc :macos-amd64-default
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
	*-i686-*)
		error "wasmtime doesn't have release for x86"
		;;
	*-amd64-*)
		local WASMTIME_ARCH=x86_64
		;;
	esac
	download "wasmtime_${WASMTIME_PLATFORM}.${ARCHIVE_EXT}" "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASMTIME_VERSION}/wasmtime-v${WASMTIME_VERSION}-${WASMTIME_ARCH}-${WASMTIME_PLATFORM}-c-api.${ARCHIVE_EXT}" wasmtime
	cd "wasmtime-v${WASMTIME_VERSION}-${WASMTIME_ARCH}-${WASMTIME_PLATFORM}-c-api"
	cp -r include/* "${PREFIX}/include"
	cp -r lib/* "${PREFIX}/lib"
}

# Build the NaCl SDK
register_package naclsdk linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
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
		local DAEMON_ARCH=x86
		;;
	*-amd64-*)
		local NACLSDK_ARCH=x86_64
		local DAEMON_ARCH=x86_64
		;;
	esac
	download "naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}.${TAR_EXT}.bz2" "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/${NACLSDK_VERSION}/naclsdk_${NACLSDK_PLATFORM}.tar.bz2" naclsdk
	cp pepper_*"/tools/sel_ldr_${NACLSDK_ARCH}${EXE}" "${PREFIX}/sel_ldr${EXE}"
	cp pepper_*"/tools/irt_core_${NACLSDK_ARCH}.nexe" "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
	cp pepper_*"/toolchain/${NACLSDK_PLATFORM}_x86_newlib/bin/x86_64-nacl-gdb${EXE}" "${PREFIX}/nacl-gdb${EXE}"
	rm -rf "${PREFIX}/pnacl"
	cp -a pepper_*"/toolchain/${NACLSDK_PLATFORM}_pnacl" "${PREFIX}/pnacl"
	rm -rf "${PREFIX}/pnacl/bin/"{i686,x86_64}-nacl-*
	rm -rf "${PREFIX}/pnacl/arm-nacl"
	rm -rf "${PREFIX}/pnacl/arm_bc-nacl"
	rm -rf "${PREFIX}/pnacl/docs"
	rm -rf "${PREFIX}/pnacl/i686_bc-nacl"
	rm -rf "${PREFIX}/pnacl/include"
	rm -rf "${PREFIX}/pnacl/x86_64-nacl"
	rm -rf "${PREFIX}/pnacl/x86_64_bc-nacl"
	case "${PLATFORM}" in
	windows-i686-*)
		cp pepper_*"/tools/sel_ldr_x86_64.exe" "${PREFIX}/sel_ldr64.exe"
		cp pepper_*"/tools/irt_core_x86_64.nexe" "${PREFIX}/irt_core-x86_64.nexe"
		;;
	linux-amd64-*)
		cp pepper_*"/tools/nacl_helper_bootstrap_x86_64" "${PREFIX}/nacl_helper_bootstrap"
		# Fix permissions on a few files which deny access to non-owner
		chmod 644 "${PREFIX}/irt_core-x86_64.nexe"
		chmod 755 "${PREFIX}/nacl_helper_bootstrap" "${PREFIX}/sel_ldr"
		;;
	esac
}

register_package naclports linux-amd64-default windows-i686-mingw windows-amd64-mingw windows-i686-msvc windows-amd64-msvc macos-amd64-default
build_naclports() {
	download "naclports-${NACLSDK_VERSION}.tar.bz2" "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/${NACLSDK_VERSION}/naclports.tar.bz2" naclports
	mkdir -p "${PREFIX}/pnacl_deps/"{include,lib}
	cp pepper_*"/ports/include/"{lauxlib.h,lua.h,lua.hpp,luaconf.h,lualib.h} "${PREFIX}/pnacl_deps/include"
	cp -a pepper_*"/ports/include/freetype2" "${PREFIX}/pnacl_deps/include"
	cp pepper_*"/ports/lib/newlib_pnacl/Release/"{liblua.a,libfreetype.a,libpng16.a} "${PREFIX}/pnacl_deps/lib"
}

# For MSVC, we need to use the Microsoft LIB tool to generate import libraries,
# the import libraries generated by MinGW seem to have issues. Instead we
# generate a .bat file to be run using the Visual Studio tools command shell.
register_package gendef :windows-i686-msvc :windows-amd64-msvc
build_gendef() {
	case "${PLATFORM}" in
	windows-*-msvc)
		mkdir -p "${PREFIX}/def"
		cd "${PREFIX}/def"
		echo 'cd /d "%~dp0"' > "${PREFIX}/genlib.bat"
		for DLL_A in "${PREFIX}"/lib/*.dll.a; do
			local DLL="$(${CROSS}dlltool -I "${DLL_A}" 2> /dev/null || echo $(basename ${DLL_A} .dll.a).dll)"
			local DEF="$(basename ${DLL} .dll).def"
			local LIB="$(basename ${DLL_A} .dll.a).lib"
			local MACHINE="$([ "${PLATFORM}" = windows-i686-msvc ] && echo x86 || echo x64)"

			# Using gendef from mingw-w64-tools
			gendef "${PREFIX}/bin/${DLL}"

			# Fix some issues with gendef output
			local TMP_FILE="$(mktemp /tmp/config.XXXXXXXXXX)"
			sed "s/\(glew.*\)@4@4/\1@4/" "${DEF}" > "${TMP_FILE}"
			sed "s/ov_halfrate_p@0/ov_halfrate_p/" "${TMP_FILE}" > "${DEF}"
			rm -f "${TMP_FILE}"

			echo "lib /def:def\\${DEF} /machine:${MACHINE} /out:lib\\${LIB}" >> "${PREFIX}/genlib.bat"
		done
		;;
	*)
		error "Unsupported platform for gendef"
		;;
	esac
}

build_selection() {
	local package; for package in $(eval "echo \"\${${1}_packages_${PLATFORM//-/_}}\""); do
		run_build "${package}"
	done
}

run_setup() {
	if ! to_lines "${platforms}" | egrep -q "^${PLATFORM//-/_}$"; then
		error "Unknown platform ${PLATFORM}"
	fi
	echo "Setting up: ${PLATFORM}"
	"setup_${PLATFORM//-/_}"
}

run_build() {
	if ! to_lines "${packages}" | egrep -q "^${1}$"; then
		error "Unknown package ${1}"
	fi
	echo "Building: ${1}"
	cd "${WORK_DIR}"
	"build_${1}"
}

# Install all the necessary files to the location expected by CMake
register_command install 'Create a stripped down version of the built packages that CMake can use.'
run_install() {
	echo "Installing: ${PLATFORM}"
	PKG_PREFIX="${WORK_DIR}/${PLATFORM}_${DEPS_VERSION}"
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
	rm -f "${PKG_PREFIX}/genlib.bat"
	rm -rf "${PKG_PREFIX}/lib/cmake"
	rm -rf "${PKG_PREFIX}/lib/pkgconfig"
	find "${PKG_PREFIX}/bin" -not -type d -not -name '*.dll' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.la' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.dll.a' -execdir bash -c 'rm -f -- "$(basename "{}" .dll.a).a"' \;
	find "${PKG_PREFIX}/lib" -name '*.dylib' -execdir bash -c 'rm -f -- "$(basename "{}" .dylib).a"' \;

	# Strip libraries
	case "${PLATFORM}" in
	windows-*-mingw)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		;;
	windows-*-msvc)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir rm -f -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.exp' -execdir rm -f -- {} \;
		;;
	esac

	# Remove empty directories
	find "${PKG_PREFIX}/" -mindepth 1 -type d -empty -delete
}

# Create a redistributable package for the dependencies
register_command package 'Create a zip/tarball of the dependencies so they can be distributed.'
run_package() {
	echo "Packaging: ${PLATFORM}"
	cd "${WORK_DIR}"
	case "${PLATFORM}" in
	windows-*-*)
		rm -f "${PLATFORM}_${DEPS_VERSION}.zip"
		zip -r "${PLATFORM}_${DEPS_VERSION}.zip" "${PLATFORM}_${DEPS_VERSION}"
		;;
	*)
		rm -f "${PLATFORM}_${DEPS_VERSION}.tar.bz2"
		tar cvjf "${PLATFORM}_${DEPS_VERSION}.tar.bz2" "${PLATFORM}_${DEPS_VERSION}"
		;;
	esac
}

register_command clean 'Remove products of build process, excepting download cache. Must be last.'
run_clean() {
	echo "Cleaning: ${PLATFORM}"
	local NAME="${PLATFORM}-${DEPS_VERSION}"
	rm -rf "build-${NAME}/" "${NAME}/" "${NAME}.zip"
}

print_commands() {
	local command; for command in ${commands}; do
		printf '\t%s: ' "${command}"
		eval "echo \"\${command_${command}}\""
	done
}

print_platforms() {
	local platform; for platform in $(to_lines "${platforms}" | sort -u); do
		printf '\t%s: ' "${platform//_/-}"
		eval "echo \"\${platform_${platform}}\""
	done
}

print_selection_packages_per_platform() {
	local platform; for platform in $(to_lines "${package_platforms}" | sort -u); do
		printf '\t%s: ' "${platform//_/-}"
		eval "echo \"\${${1}_packages_${platform}}\""
	done
}

print_help() {
	local basename="$(basename "${0}")"
	local tab=$'\t'
	cat <<-EOF
	Usage: ${basename} [PLATFORM] [SELECTION]… [PACKAGE]… [COMMAND]…

	Script to build dependencies for platforms which do not provide them.

	Platforms:
	$(print_platforms)

	Selections:
	${tab}required  (build required packages for the given platform)
	${tab}optional  (build optional packages for the given platform)
	${tab}all       (build all available packages for the given platform)

	Packages:
	${tab}${packages}

	Required packages per platform:
	$(print_selection_packages_per_platform required)

	Optional packages per platform:
	$(print_selection_packages_per_platform optional)

	Unused packages:
	${tab}${packages_unused:-}

	Commands:
	$(print_commands)

	Example:
	${tab}${basename} linux-amd64-default required ncurses install package clean

	EOF
}

# Usage
if [ "${1:-}" = '-h' -o "${1:-}" = '--help' ]; then
	print_help
	exit
elif [ "${#}" -lt "1" ]; then
	error 'Missing platform name'
elif [ "${#}" -lt "2" ]; then
	error 'Missing package name or command name'
fi

# Setup platform
PLATFORM="${1}"; shift
run_setup

# Build packages, selection or run commands
while [ -n "${1:-}" ]; do
	case "${1}" in
	install|package|clean)
		"run_${1}"
		;;
	required|optional|all)
		build_selection "${1}"
		;;
	*)
		run_build "${1}"
		;;
	esac
	shift
done

# /!\ Please do not remove this line, it makes easy
# to figure out if the script silently failed or not.
echo 'Done'
