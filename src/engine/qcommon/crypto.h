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

// The Nettle headers include the GMP header, so this include this first to cancel the warning
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4146) // "unary minus operator applied to unsigned type, result still unsigned"
#include <gmp.h>
#pragma warning(pop)
#endif

// As of GCC 16 there is a spurious warning for alignas(0) on Linux i686, although this
// is a legal no-op according to the C and C++ standards.
#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wattributes"
#endif
#include <nettle/aes.h>
#include <nettle/bignum.h>
#include <nettle/buffer.h>
#include <nettle/rsa.h>
#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic pop
#endif

// Random function used for key generation and encryption
void     qnettle_random( void *ctx, size_t length, uint8_t *dst );

#endif /* __CRYPTO_H__ */
