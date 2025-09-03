/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2007-2008 Amanieu d'Antras (amanieu@gmail.com)
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include "q_shared.h"
#include "qcommon.h"

/* The Nettle headers include the GMP header, this disables the warning
on GMP alone, not the whole Nettle. We don't use GMP directly ourselves. */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4146) // "unary minus operator applied to unsigned type, result still unsigned"
#include <gmp.h>
#pragma warning(pop)
#endif

#include <nettle/bignum.h>
#include <nettle/rsa.h>
#include <nettle/buffer.h>

// Old versions of nettle have nettle_random_func taking an unsigned parameter
// instead of a size_t parameter. Detect this and use the appropriate type.
using NettleLength = std::conditional<std::is_same<nettle_random_func, void(void*, size_t, uint8_t*)>::value, size_t, unsigned>::type;

// Random function used for key generation and encryption
void     qnettle_random( void *ctx, NettleLength length, uint8_t *dst );

#endif /* __CRYPTO_H__ */
