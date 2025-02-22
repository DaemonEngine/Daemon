# Daemon BSD Source Code
# Copyright (c) 2025, Daemon Developers
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

# Work around https://github.com/DaemonEngine/Daemon/issues/398. The Visual Studio CMake
# integration, which uses a (single-config) Ninja generator, sets the configuration name
# to an arbitrary name instead of "Release", "Debug" etc. So then $<CONFIG:Debug> etc.
# doesn't work since it doesn't correspond to CMAKE_BUILD_TYPE.
get_property(are_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (are_multi_config)
    set(DEBUG_GENEXP_COND "$<CONFIG:Debug>")
    set(MINSIZEREL_GENEXP_COND "$<CONFIG:MinSizeRel>")
    set(RELWITHDEBINFO_GENEXP_COND "$<CONFIG:RelWithDebInfo>")
    set(RELEASE_GENEXP_COND "$<CONFIG:Release>")
else()
    set(DEBUG_GENEXP_COND "0")
    set(MINSIZEREL_GENEXP_COND "0")
    set(RELWITHDEBINFO_GENEXP_COND "0")
    set(RELEASE_GENEXP_COND "0")
    string(TOUPPER ${CMAKE_BUILD_TYPE} build_type_upper)
    set(${build_type_upper}_GENEXP_COND "1")
endif()
