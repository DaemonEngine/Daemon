#!/usr/bin/env bash

# Exit on undefined variable and error.
set -u -e -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
WORK_DIR="${PWD}"
DOWNLOAD_DIR="${WORK_DIR}/download_cache"

# This should match the SAIGOSDK_VERSION in CMakeLists.txt.
# This is mostly to ensure the path the files end up at if you build deps yourself
# are the same as the ones when extracting from the downloaded packages.
SAIGOSDK_VERSION=20261030
# Can be a tag like v${SAIGOSDK_VERSION}, or a full commit hash
SAIGOSDK_REF="v${SAIGOSDK_VERSION}"

REPO_BASENAME=saigo-nacl-sdk
REPO_URL="https://github.com/DaemonEngine/${REPO_BASENAME}"
ARCHIVE_PATH="${DOWNLOAD_DIR}/saigosdk-${SAIGOSDK_REF}.tar.gz"

usage() {
	cat <<-EOF
	Usage: $0 <os>-<arch> (build | install | package | wipe)...
	<os>-<arch> should normally be your current native platform.
	For a list of supported platforms see ${REPO_URL}/tree/master/tools/release/conf
	Actions:
	  build - build all tools and libraries in the SDK
	  install - install final build products to ${WORK_DIR}/saigosdk-<os>-<arch>_${SAIGOSDK_VERSION}/
	  wipe - delete build products (except download cache) AND install directory
	If you want to build only a subset of packages, prefer using the
	${REPO_URL} CMake build directly.
	Some global configuration options are available via environment
	variables; see ${REPO_URL}/tree/master/tools/release/build
	EOF
}

download() {
	test -f "${ARCHIVE_PATH}" && return
	mkdir -p "${DOWNLOAD_DIR}"
	curl --fail -L "${DOWNLOAD_URL}" -o "${ARCHIVE_PATH}"
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
	DOWNLOAD_URL="https://github.com/DaemonEngine/saigo-nacl-sdk/archive/${SAIGOSDK_REF}.tar.gz"
	TARBALL_OUTER_DIR="${REPO_BASENAME}-${SAIGOSDK_REF}"
elif grep -q -E '^v[0-9]' <<< "${SAIGOSDK_REF}"
then
	# The 'v' from the tag name is mysteriously removed???
	TARBALL_OUTER_DIR="${REPO_BASENAME}-${SAIGOSDK_REF:1}"
	DOWNLOAD_URL="https://github.com/DaemonEngine/saigo-nacl-sdk/archive/refs/tags/${SAIGOSDK_REF}.tar.gz"
fi

INSTALL_DIR="${WORK_DIR}/saigosdk-${PLATFORM}_${SAIGOSDK_VERSION}"
BUILD_DIR="${WORK_DIR}/build-saigosdk_${SAIGOSDK_VERSION}"
REPO_DIR="${BUILD_DIR}/${TARBALL_OUTER_DIR}"

for target in "$@"
do
	case "${target}" in
	build)
		download
		mkdir -p "${BUILD_DIR}"
		tar xzf "${ARCHIVE_PATH}" -C "${BUILD_DIR}"
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
	*)
		usage
		exit 1
		;;
	esac
done
