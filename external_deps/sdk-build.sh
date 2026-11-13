#!/usr/bin/env bash

# Exit on undefined variable and error.
set -u -e -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
WORK_DIR="${PWD}"
DOWNLOAD_DIR="${WORK_DIR}/download_cache"

# Define SAIGOSDK_VERSION/SAIGOSDK_REF
versions_out="$(cmake -P "${SCRIPT_DIR}/versions.cmake" 2>&1)"
eval "${versions_out}"

REPO_BASENAME=saigo-nacl-sdk
REPO_URL="https://github.com/DaemonEngine/${REPO_BASENAME}"
SRC_ARCHIVE_NAME="saigosdk-${SAIGOSDK_REF}.tar.gz"

usage() {
	cat <<-EOF
	Usage: $0 <os>-<arch> (build | install | package | wipe)...
	<os>-<arch> should normally be your current native platform.
	For a list of supported platforms see ${REPO_URL}/tree/master/tools/release/conf
	Actions:
	  build - build all tools and libraries in the SDK
	  install - install final build products to ${WORK_DIR}/saigosdk-<os>-<arch>_${SAIGOSDK_VERSION}/
	  wipe - delete build products (except download cache) AND install directory
	  download - download a pre-built SDK
	If you want to build only a subset of packages, prefer using the
	${REPO_URL} CMake build directly.
	Some global configuration options are available via environment
	variables; see ${REPO_URL}/tree/master/tools/release/build
	EOF
}

download() {
	local archive_name="$1"
	local url="$2"
	test -f "${DOWNLOAD_DIR}/${archive_name}" && return
	mkdir -p "${DOWNLOAD_DIR}"
	curl --fail -L "${url}" -o "${DOWNLOAD_DIR}/${archive_name}"
}

if [ "$#" -lt 2 ]
then
	usage
	exit 1
fi

PLATFORM="$1"
shift

if grep -q -E '^[0-9a-f]{40}$' <<< "${SAIGOSDK_REF}"
then
	SRC_DOWNLOAD_URL="https://github.com/DaemonEngine/saigo-nacl-sdk/archive/${SAIGOSDK_REF}.tar.gz"
	TARBALL_OUTER_DIR="${REPO_BASENAME}-${SAIGOSDK_REF}"
elif grep -q -E '^v[0-9]' <<< "${SAIGOSDK_REF}"
then
	# The 'v' from the tag name is mysteriously removed???
	TARBALL_OUTER_DIR="${REPO_BASENAME}-${SAIGOSDK_REF:1}"
	SRC_DOWNLOAD_URL="https://github.com/DaemonEngine/saigo-nacl-sdk/archive/refs/tags/${SAIGOSDK_REF}.tar.gz"
fi

INSTALL_DIR="${WORK_DIR}/saigosdk-${PLATFORM}_${SAIGOSDK_VERSION}"
BUILD_DIR="${WORK_DIR}/build-saigosdk_${SAIGOSDK_VERSION}"
REPO_DIR="${BUILD_DIR}/${TARBALL_OUTER_DIR}"

for target in "$@"
do
	case "${target}" in
	build)
		download "${SRC_ARCHIVE_NAME}" "${SRC_DOWNLOAD_URL}"
		mkdir -p "${BUILD_DIR}"
		tar xzf "${DOWNLOAD_DIR}/${SRC_ARCHIVE_NAME}" -C "${BUILD_DIR}"
		(
			cd "${REPO_DIR}"
			tools/release/build "${PLATFORM}"
		)
		;;
	install)
		cp -R "${REPO_DIR}/build/${PLATFORM}/install/." "${INSTALL_DIR}/"
		;;
	wipe)
		rm -rf "${BUILD_DIR}" "${INSTALL_DIR}"
		;;
	download)
		BIN_ARCHIVE_NAME="saigosdk-${PLATFORM}_${SAIGOSDK_VERSION}.tar.xz"
		download "${BIN_ARCHIVE_NAME}" \
			"https://github.com/DaemonEngine/saigo-nacl-sdk/releases/download/v${SAIGOSDK_VERSION}/${BIN_ARCHIVE_NAME}"
		tar xJf "${DOWNLOAD_DIR}/${BIN_ARCHIVE_NAME}" -C "${WORK_DIR}"
		;;
	*)
		usage
		exit 1
		;;
	esac
done
