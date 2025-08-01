/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// GLUtils.h

#ifndef GLUTILS_H
#define GLUTILS_H

#include "common/Common.h"
#include "GL/glew.h"

#include "tr_public.h"
#include "tr_types.h"

extern glconfig_t glConfig; // outside of TR since it shouldn't be cleared during ref re-init
extern glconfig2_t glConfig2;

void GL_CheckErrors_( const char *filename, int line );

#define GL_CheckErrors() do { if ( !glConfig.smpActive ) GL_CheckErrors_( __FILE__, __LINE__ ); } while ( false )

#endif // GLUTILS_H
