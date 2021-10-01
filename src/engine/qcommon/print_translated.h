/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// Function body for translated text printing from cgame/ui & server code
// Macros required:
//   TRANSLATE_FUNC            - name of the translation function, called from this code
//   PLURAL_TRANSLATE_FUNC     - name of the translation function for plurals, called from this code
// If needed:
//   Cmd_Argv
//   Cmd_Argc

#if !defined TRANSLATE_FUNC && !defined PLURAL_TRANSLATE_FUNC
#error No translation function? Fail!
#endif

static const char *TranslateText_Internal( bool plural, int firstTextArg )
{
	static char str[ MAX_STRING_CHARS ];
	char        buf[ MAX_STRING_CHARS ];
	const char  *in;
	unsigned i = 0;

	if ( plural )
	{
		int        number = atoi( Cmd_Argv( firstTextArg ) );
		const char *text = Cmd_Argv( ++firstTextArg );

		Q_strncpyz( buf, PLURAL_TRANSLATE_FUNC( text, text, number ), sizeof( buf ) );
	}
	else
	{
		Q_strncpyz( buf, TRANSLATE_FUNC( Cmd_Argv( firstTextArg ) ), sizeof( buf ) );
	}

	int maxArgnum = Cmd_Argc() - firstTextArg - 1;

	in = buf;

	while( *in && i < sizeof( str ) - 1 )
	{
		if ( *in != '$' ) // regular character
		{
			str[ i++ ] = *in++;
			continue;
		}

		++in;

		if( *in == '$' ) // escaped '$'
		{
			str[ i++ ] = *in++;
			continue;
		}

		if ( !Str::cisdigit( *in ) )
		{
			str[ i++ ] = '$'; // stray '$', treat as normal char
			continue;
		}

		const char* number = in;
		do { ++in; } while ( Str::cisdigit( *in ) );
		int argnum = atoi( number );

		if ( argnum <= 0 || argnum > maxArgnum )
		{
			str[ i++ ] = '$'; // substitution parsing failed
			in = number;
			continue;
		}

		const char* substitution;

		if ( in[ 0 ] == '$' )
		{
			in += 1;
			substitution = Cmd_Argv( argnum + firstTextArg );
		}
		else if ( in[ 0 ] == 't' && in[ 1 ] == '$' )
		{
			in += 2;
			substitution = TRANSLATE_FUNC( Cmd_Argv( argnum + firstTextArg ) );
		}
		else
		{
			str[ i++ ] = '$'; // substitution parsing failed
			in = number; // go back and print everything literally
			continue;
		}

		Q_strncpyz( str + i, substitution, sizeof( str ) - i );
		i += strlen( str + i );
	}

	str[ i ] = '\0';
	return str;
}
