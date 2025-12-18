# Daemon BSD Source Code
# Copyright (c) 2013-2016, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Native client

option(USE_NACL_SAIGO "Use Saigo toolchain to build NaCl executables" OFF)

if (DAEMON_SYSTEM_NaCl)
  # Build nexe binary.
  if(USE_NACL_SAIGO)
    # DAEMON_NACL_ARCH is "pnacl" here, NACL_TARGET carries the architecture.
    if (NACL_TARGET STREQUAL "amd64")
      add_definitions(-DNACL_BUILD_ARCH=x86)
      add_definitions(-DNACL_BUILD_SUBARCH=64)
    elseif (NACL_TARGET STREQUAL "i686")
      add_definitions(-DNACL_BUILD_ARCH=x86)
      add_definitions(-DNACL_BUILD_SUBARCH=32)
    elseif (NACL_TARGET STREQUAL "armhf")
      add_definitions(-DNACL_BUILD_ARCH=arm)
    else()
      message(FATAL_ERROR "Unsupported architecture ${NACL_TARGET}")
    endif()
  else()
    # Those defines looks to be meaningless to produce arch-independent pexe
    # with PNaCl but they must be set to anything supported by native builds.
    # This requirement looks to be a PNaCl bug.
    # DAEMON_NACL_ARCH is "pnacl" here, NACL_TARGET is not set.
    add_definitions( -DNACL_BUILD_ARCH=x86 )
    add_definitions( -DNACL_BUILD_SUBARCH=64 )
  endif()
else()
  # Build native dll or native exe.
  if (DAEMON_SYSTEM_macOS)
    add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_FREEBSD=0 -DNACL_OSX=1 )
  elseif (DAEMON_SYSTEM_Linux)
    add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=1 -DNACL_ANDROID=0 -DNACL_FREEBSD=0 -DNACL_OSX=0 )
  elseif (DAEMON_SYSTEM_FreeBSD)
    add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_FREEBSD=1 -DNACL_OSX=0 )
  elseif (DAEMON_SYSTEM_Windows)
    add_definitions( -DNACL_WINDOWS=1 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_FREEBSD=0 -DNACL_OSX=0 )
  endif()

  if (DAEMON_NACL_ARCH_amd64)
    add_definitions( -DNACL_BUILD_ARCH=x86 )
    add_definitions( -DNACL_BUILD_SUBARCH=64 )
  elseif (DAEMON_NACL_ARCH_i686)
    add_definitions( -DNACL_BUILD_ARCH=x86 )
    add_definitions( -DNACL_BUILD_SUBARCH=32 )
  elseif (DAEMON_NACL_ARCH_armhf)
    add_definitions( -DNACL_BUILD_ARCH=arm )
  else()
    message(WARNING "Unknown NaCl architecture ${DAEMON_NACL_ARCH_NAME}")
  endif()
endif()

include_directories( ${LIB_DIR}/nacl )
