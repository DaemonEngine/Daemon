# - Find GLEW library
# Find the native GLEW headers and libraries.
#
#  GLEW_INCLUDE_DIRS - where to find glew.h, etc.
#  GLEW_LIBRARIES    - List of libraries when using GLEW.
#  GLEW_FOUND        - True if GLEW is found.

#=============================================================================
#Copyright 2000-2009 Kitware, Inc., Insight Software Consortium
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#* Redistributions of source code must retain the above copyright notice, 
#this list of conditions and the following disclaimer.
#
#* Redistributions in binary form must reproduce the above copyright notice, 
#this list of conditions and the following disclaimer in the documentation 
#and/or other materials provided with the distribution.
#
#* Neither the names of Kitware, Inc., the Insight Software Consortium, nor 
#the names of their contributors may be used to endorse or promote products 
#derived from this software without specific prior written  permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
#ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
#LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
#INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
#ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
#POSSIBILITY OF SUCH DAMAGE.
#=============================================================================
# Look for the header file.
FIND_PATH(GLEW_INCLUDE_DIR NAMES GL/glew.h)
MARK_AS_ADVANCED(GLEW_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(GLEW_LIBRARY NAMES GLEW glew32 glew glew32s PATH_SUFFIXES lib64 lib)
MARK_AS_ADVANCED(GLEW_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set GLEW_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)

SET(GLEW_LIBRARIES ${GLEW_LIBRARY})
SET(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
