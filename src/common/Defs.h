/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

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

#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

#ifndef ENGINE_NAME
#define ENGINE_NAME "Daemon Engine"
#endif

#ifndef ENGINE_VERSION
#define ENGINE_VERSION "0.52.1"
#endif

#ifndef ENGINE_DATE
#define ENGINE_DATE __DATE__
#endif

#define MAX_MASTER_SERVERS  5
#define AUTOEXEC_NAME       "autoexec.cfg"

#define CONFIG_NAME         "autogen.cfg"
#define KEYBINDINGS_NAME    "keybindings.cfg"
#define TEAMCONFIG_NAME     "teamconfig.cfg"

#define UNNAMED_PLAYER      "UnnamedPlayer"

/** file containing our RSA public and private keys */
#define RSAKEY_FILE        "pubkey"

#endif // COMMON_DEFS_H_
