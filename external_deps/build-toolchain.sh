#! /usr/bin/env bash

# Exit on undefined variable and error.
set -u -e -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
WORK_DIR="${PWD}"

# This should match the TOOLCHAINS_VERSION in build.sh.
TOOLCHAINS_VERSION='1'

# Package versions
TOOLCHAIN_CTNG_VERSION='1.25.0'

setup_linux-amd64() {
	TOOLCHAIN_SYSTEM='linux'
	TOOLCHAIN_TARGET='x86_64-unknown-linux-gnu'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_GLIBC_VERSION='2.17'
	TOOLCHAIN_LINUX_VERSION='3.2'
}

setup_linux-i686() {
	TOOLCHAIN_SYSTEM='linux'
	TOOLCHAIN_TARGET='i686-ubuntu14.04-linux-gnu'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_GLIBC_VERSION='2.17'
	TOOLCHAIN_LINUX_VERSION='2.6.32'
}

setup_linux-arm64() {
	TOOLCHAIN_SYSTEM='linux'
	TOOLCHAIN_TARGET='aarch64-unknown-linux-gnu'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_GLIBC_VERSION='2.17'
	TOOLCHAIN_LINUX_VERSION='3.10'
}

setup_linux-armhf() {
	TOOLCHAIN_SYSTEM='linux'
	TOOLCHAIN_TARGET='arm-unknown-linux-gnueabi'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_GLIBC_VERSION='2.17'
	TOOLCHAIN_LINUX_VERSION='3.2'
}

setup_windows-amd64() {
	TOOLCHAIN_SYSTEM='windows'
	TOOLCHAIN_TARGET='x86_64-w64-mingw32'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_MINGW_VERSION='4.0'
	TOOLCHAIN_THREAD_IMPLEMENTATION='posix'
}

setup_windows-i686() {
	TOOLCHAIN_SYSTEM='windows'
	TOOLCHAIN_TARGET='i686-w64-mingw32'
	TOOLCHAIN_GCC_VERSION='11'
	TOOLCHAIN_MINGW_VERSION='4.0'
	TOOLCHAIN_THREAD_IMPLEMENTATION='posix'
}

build_toolchain() {
	toolchain_platform="${1}"

	mkdir -p "${TOOLCHAINS_BUILD_DIR}"
	cd "${TOOLCHAINS_BUILD_DIR}"

	"setup_${toolchain_platform}"

	local toolchain_prefix="${WORK_DIR}/${toolchain_platform}"
	mkdir -p "${toolchain_prefix}"

	ct-ng "${TOOLCHAIN_TARGET}"

	cat >> '.config' <<-EOF
	CT_PREFIX_DIR="${toolchain_prefix}"
	CT_LOCAL_TARBALLS_DIR="${TOOLCHAIN_DOWNLOAD_CACHE}"
	CT_DEBUG_GDB=n
	CT_GDB_GDBSERVER=n
	GCC_V_${TOOLCHAIN_GCC_VERSION}=y
	EOF

	case "${TOOLCHAIN_SYSTEM}" in
		'linux')
			cat >> '.config' <<-EOF
			CT_GLIBC_V_${TOOLCHAIN_GLIBC_VERSION//./_}=y
			CT_LINUX_V_${TOOLCHAIN_LINUX_VERSION//./_}=y
			EOF
			;;
		'windows')
			cat >> '.config' <<-EOF
			MINGW_W64_V_V${TOOLCHAIN_MINGW_VERSION//./_}=y
			CT_THREADS="${TOOLCHAIN_THREAD_IMPLEMENTATION}"
			EOF
			;;
	esac

	ct-ng upgradeconfig

	# Workaround for zlib upstream having deleted zlib 1.2.12 archive.
	sed -e 's|CT_ZLIB_VERSION="1.2.12"|CT_ZLIB_VERSION="1.2.13"|' -i '.config'

	ct-ng build
}

build_ctng() {
	local ctng_dir="crosstool-ng-${TOOLCHAIN_CTNG_VERSION}"
	local ctng_tar="${ctng_dir}.tar.xz"
	local ctng_tar_path="${TOOLCHAIN_DOWNLOAD_CACHE}/${ctng_tar}"
	local ctng_tar_url="http://crosstool-ng.org/download/crosstool-ng/${ctng_tar}"

	local ctng_prefix="${TOOLCHAINS_BUILD_DIR}/ct-ng"
	mkdir -p "${ctng_prefix}"

	if [ ! -f "${ctng_prefix}/bin/ct-ng" ]
	then
		if [ ! -f "${ctng_tar_path}" ]
		then
			wget -O "${ctng_tar_path}" "${ctng_tar_url}"
		fi

		local source_parent_dir="${TOOLCHAINS_BUILD_DIR}/.build/src"
		mkdir -p "${source_parent_dir}"
		local ctng_source_dir="${source_parent_dir}/${ctng_dir}"
		rm -rf "${ctng_source_dir}"
		tar -C "${source_parent_dir}" -xJf "${ctng_tar_path}"

		( cd "${ctng_source_dir}" ; ./configure --prefix="${ctng_prefix}" )
		make -C "${ctng_source_dir}" -j"$(nproc)"
		make -C "${ctng_source_dir}" install
	fi

	export PATH="${ctng_prefix}/bin:${PATH}"
}

print_help() {
	sed -e 's/\\t/\t/' <<-EOF
	usage: $(basename "${0}") <platform>

	Script to build cross-compiler toolchains for various platforms.

	Platforms:
	\twindows-amd64 windows-i686 linux-amd64 linux-i686 linux-arm64 linux-armhf

	EOF
}

error() {
	echo "ERROR: ${1}" >&2
	false
}

main() {
	if [ -z "${1:-}" ]
	then
		error 'Missing platform' || true
		print_help
		return
	fi

	local platform_list=()

	while [ -n "${1:-}" ]
	do
		case "${1:-}" in
			'-h'|'--help')
				print_help
				exit
				;;
			'linux-amd64')
				;;
			'linux-i686')
				;;
			'linux-arm64')
				;;
			'linux-armhf')
				;;
			'windows-amd64')
				;;
			'windows-i686')
				;;
			*)
				error "Unsupported platform: ${1}"
			;;
		esac

		platform_list+="${1}"

		shift
	done

	TOOLCHAIN_DOWNLOAD_CACHE="${WORK_DIR}/download_cache"
	mkdir -p "${TOOLCHAIN_DOWNLOAD_CACHE}"

	TOOLCHAINS_BUILD_DIR="${WORK_DIR}/toolchains_${TOOLCHAINS_VERSION}"
	mkdir -p "${TOOLCHAINS_BUILD_DIR}"
	cd "${TOOLCHAINS_BUILD_DIR}"

	build_ctng

	local platform
	for platform in "${platform_list[@]}"
	do
		build_toolchain "${platform}"
	done
}

main "${@}"
