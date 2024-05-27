/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// This file should be built with the -fno-strict-aliasing flag.

#include "tr_local.h"

unsigned int VertexCoordGenerateHash( const vec3_t xyz )
{
	unsigned int hash = 0;

#ifndef HASH_USE_EPSILON
	hash += ~( * ( ( unsigned int * ) &xyz[ 0 ] ) << 15 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 0 ] ) >> 10 );
	hash += ( * ( ( unsigned int * ) &xyz[ 1 ] ) << 3 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 1 ] ) >> 6 );
	hash += ~( * ( ( unsigned int * ) &xyz[ 2 ] ) << 11 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 2 ] ) >> 16 );
#else
	vec3_t xyz_epsilonspace;

	VectorScale( xyz, HASH_XYZ_EPSILONSPACE_MULTIPLIER, xyz_epsilonspace );
	xyz_epsilonspace[ 0 ] = floor( xyz_epsilonspace[ 0 ] );
	xyz_epsilonspace[ 1 ] = floor( xyz_epsilonspace[ 1 ] );
	xyz_epsilonspace[ 2 ] = floor( xyz_epsilonspace[ 2 ] );

	hash += ~( * ( ( unsigned int * ) &xyz_epsilonspace[ 0 ] ) << 15 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 0 ] ) >> 10 );
	hash += ( * ( ( unsigned int * ) &xyz_epsilonspace[ 1 ] ) << 3 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 1 ] ) >> 6 );
	hash += ~( * ( ( unsigned int * ) &xyz_epsilonspace[ 2 ] ) << 11 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 2 ] ) >> 16 );

#endif

	hash = hash % ( HASHTABLE_SIZE );
	return hash;
}
