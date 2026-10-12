# Daemon BSD Source Code
# Copyright (c) 2013-2026, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the Daemon developers nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# TODO: Move other deps downloading code in here.

set(SAIGOSDK_BASENAME saigosdk-${DEPS_SYSTEM}-${DEPS_ARCH}_${SAIGOSDK_VERSION})
set(SAIGOSDK_ROOT ${EXTERNAL_DEPS_DIR}/${SAIGOSDK_BASENAME})
set(SAIGOSDK_ROOT "${SAIGOSDK_ROOT}" PARENT_SCOPE)

function(download_saigosdk)
    if (NOT EXISTS ${SAIGOSDK_ROOT})
        set(DEPS_FILENAME "${SAIGOSDK_BASENAME}.tar.xz")
        set(REMOTE "https://github.com/DaemonEngine/saigo-nacl-sdk/releases/download/v${SAIGOSDK_VERSION}/${DEPS_FILENAME}")
        message(STATUS "Downloading NaCl SDK from '${REMOTE}'")

        file(DOWNLOAD "${REMOTE}" "${OBJ_DIR}/${DEPS_FILENAME}"
            SHOW_PROGRESS
            STATUS DOWNLOAD_RESULT
            LOG DOWNLOAD_LOG
        )

        list(GET DOWNLOAD_RESULT 0 DOWNLOAD_STATUS)
        list(GET DOWNLOAD_RESULT 1 DOWNLOAD_STRING)

        if (NOT DOWNLOAD_STATUS EQUAL 0)
            message(FATAL_ERROR "Failed to download '${REMOTE}': This may be a network error, or your system may be an unsupported host platform.
                Status code: ${DOWNLOAD_STATUS}
                Error string: ${DOWNLOAD_STRING}
                Download log: ${DOWNLOAD_LOG}"
            )
        endif()

        message(STATUS "SDK download completed successfully")

        # Extract the downloaded archive
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xJf ${OBJ_DIR}/${DEPS_FILENAME}
            WORKING_DIRECTORY ${EXTERNAL_DEPS_DIR}
            RESULT_VARIABLE EXTRACT_RESULT
        )
        if (NOT EXTRACT_RESULT EQUAL 0)
            message(FATAL_ERROR "Could not extract ${DEPS_FILENAME}")
        endif()
    endif()
endfunction()
