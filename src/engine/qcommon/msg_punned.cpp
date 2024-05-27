/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// This file should be built with the -fno-strict-aliasing flag.

#include "qcommon.h"

void MSG_WriteDeltaFloat( msg_t *msg, float oldV, float newV )
{
	if ( oldV == newV )
	{
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, * ( int * ) &newV, 32 );
}

float MSG_ReadDeltaFloat( msg_t *msg, float oldV )
{
	if ( MSG_ReadBits( msg, 1 ) )
	{
		float newV;

		* ( int * ) &newV = MSG_ReadBits( msg, 32 );
		return newV;
	}

	return oldV;
}

