/*
===========================================================================
Copyright (C) 2012-2013 Unvanquished Devleopers

This file is part of Daemon source code.

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
===========================================================================
*/

#ifndef Q_UNICODE_H_
#define Q_UNICODE_H_

constexpr int MIN_PRINTABLE_ASCII = 32;
constexpr int MAX_PRINTABLE_ASCII = 126;
constexpr int UNICODE_MAX_CODE_POINT = 0x10FFFF;

int Q_UTF8_Width( const char *str );
int Q_UTF8_WidthCP( int ch );
int Q_UTF8_Strlen( const char *str );
bool Q_UTF8_ContByte( char c );
unsigned long Q_UTF8_CodePoint( const char *str );
char *Q_UTF8_Encode( unsigned long codepoint );

bool Q_Unicode_IsAlpha( int ch );
bool Q_Unicode_IsUpper( int ch );
bool Q_Unicode_IsLower( int ch );
bool Q_Unicode_IsIdeo( int ch );
bool Q_Unicode_IsAlphaOrIdeo( int ch );
bool Q_Unicode_IsAlphaOrIdeoOrDigit( int ch );
bool Q_Unicode_IsPrivateUse( int ch );

int Q_Unicode_ToUpper( int ch );
int Q_Unicode_ToLower( int ch );

#endif /* Q_UNICODE_H_ */
