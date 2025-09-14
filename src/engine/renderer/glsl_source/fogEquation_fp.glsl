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

float FogTable(float s)
{
	return sqrt(floor(s * 255.0) / 255);
}

float FogFactor(float s, float t)
{
	s -= 1.0f / 512.0f;

	if ( s < 0 )
	{
		return 0;
	}

	if ( t < 1.0f / 32.0f )
	{
		return 0;
	}

	if ( t < 31.0f / 32.0f )
	{
		s *= ( t - 1.0f / 32.0f ) / ( 30.0f / 32.0f );
	}

	s *= 8;

	if ( s > 1.0f )
	{
		s = 1.0f;
	}

	return FogTable(s);
}

float GetFogAlpha(float s, float t)
{
	float sfloor = floor(s * 256 + 0.5) - 0.5;
	float sceil = sfloor + 1;
	sfloor = clamp(sfloor, 0.5, 255.5) / 256;
	sceil = clamp(sceil, 0.5, 255.5) / 256;
	float smix = sfloor < sceil ? (s - sfloor) * 256 : 0.5;

	float tfloor = floor(t * 32 + 0.5) - 0.5;
	float tceil = tfloor + 1;
	tfloor = clamp(tfloor, 0.5, 31.5) / 32;
	tceil = clamp(tceil, 0.5, 31.5) / 32;
	float tmix = tfloor < tceil ? (t - tfloor) * 32 : 0.5;

	float f00 = FogFactor(sfloor, tfloor);
	float f01 = FogFactor(sfloor, tceil);
	float f10 = FogFactor(sceil, tfloor);
	float f11 = FogFactor(sceil, tceil);
	return mix(mix(f00, f01, tmix), mix(f10, f11, tmix), smix);
}
