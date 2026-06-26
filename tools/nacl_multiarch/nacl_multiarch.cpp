/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2022, Daemon Developers
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

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include "YokaiBuildInfo/DaemonEngine.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("nacl_multiarch-%s: missing command line\n", DAEMON_NACL_ARCH_STRING);
		return 1;
	}

	char *directory = dirname(strdup(argv[0]));

	int err = chdir(directory);

	if (err != 0) {
		return err;
	}

	char env[100];
	sprintf(env, "LD_LIBRARY_PATH=libs-linux-%s", DAEMON_NACL_ARCH_STRING);

	err = putenv(strdup(env));

	if (err != 0) {
		return err;
	}

	execv(argv[1], &argv[1]);

	// The execv() function returns only if an error has occurred.
	printf("nacl_multiarch-%s: %s\n", DAEMON_NACL_ARCH_STRING, strerror(errno));
	return 1;
}
