/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "q_shared.h"
#include "qcommon.h"

extern "C" {
    #include "findlocale/findlocale.h"
}

static Cvar::Cvar<std::string> language("language", "language for UI text", Cvar::NONE, "");

// Auto-detect the language, but not any further locale information.
// If other information such as country is desired in the future, note that FindLocale isn't very
// accurate with the country or "variant" components. A GNU-compliant locale name [1] could have
// up to 4 components, e.g. "sr_RS@latin.UTF-8", and all of them except the language are optional.
// FindLocale naively assumes that the order is always (language, country, "variant") and
// that any punctuation mark is a separator. So "C.UTF-8" would be language C, country UTF and
// variant 8.
// [1] https://www.gnu.org/software/gettext/manual/html_node/Locale-Names.html
void Trans_LoadDefaultLanguage()
{
	FL_Locale           *locale;

	// Only detect locale if no previous language set.
	if( language.Get().empty() )
	{
		FL_FindLocale( &locale, FL_MESSAGES );

		// Invalid or not found. Just use builtin language.
		if( !locale->lang || !locale->lang[0] )
		{
			language.Set( "en" );
		}
		else
		{
			language.Set( locale->lang );
		}

		FL_FreeLocale( &locale );
	}
}
