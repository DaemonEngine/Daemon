/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
// CPPStandard.h

#ifndef CPPSTANDARD_H
#define CPPSTANDARD_H

enum CPPStandard_ {
  CPP14,
  CPP17,
  CPP20,
  CPP23
};

#if __cplusplus == 201402L
  const CPPStandard_ CPPStandard = CPP14;
  #define CPP_14_FEATURES
#elif __cplusplus == 201703L
  const CPPStandard_ CPPStandard = CPP17;
  #define CPP_14_FEATURES
  #define CPP_17_FEATURES
#elif __cplusplus == 202002L
  const CPPStandard_ CPPStandard = CPP20;
  #define CPP_14_FEATURES
  #define CPP_17_FEATURES
  #define CPP_20_FEATURES
#elif __cplusplus >= 202302L
  const CPPStandard_ CPPStandard = CPP23;
  #define CPP_14_FEATURES
  #define CPP_17_FEATURES
  #define CPP_20_FEATURES
  #define CPP_23_FEATURES
#endif

#endif // CPPSTANDARD_H