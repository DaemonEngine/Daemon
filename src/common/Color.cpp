/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#include "Common.h"

#include "../engine/qcommon/q_unicode.h"

namespace Color {

static Color g_color_table[ 32 ] =
{
	{ 0.20f, 0.20f, 0.20f, 1.00f }, // 0 - black    0
	{ 1.00f, 0.00f, 0.00f, 1.00f }, // 1 - red      1
	{ 0.00f, 1.00f, 0.00f, 1.00f }, // 2 - green    2
	{ 1.00f, 1.00f, 0.00f, 1.00f }, // 3 - yellow   3
	{ 0.00f, 0.00f, 1.00f, 1.00f }, // 4 - blue     4
	{ 0.00f, 1.00f, 1.00f, 1.00f }, // 5 - cyan     5
	{ 1.00f, 0.00f, 1.00f, 1.00f }, // 6 - purple   6
	{ 1.00f, 1.00f, 1.00f, 1.00f }, // 7 - white    7
	{ 1.00f, 0.50f, 0.00f, 1.00f }, // 8 - orange   8
	{ 0.50f, 0.50f, 0.50f, 1.00f }, // 9 - md.grey    9
	{ 0.75f, 0.75f, 0.75f, 1.00f }, // : - lt.grey    10    // lt grey for names
	{ 0.75f, 0.75f, 0.75f, 1.00f }, // ; - lt.grey    11
	{ 0.00f, 0.50f, 0.00f, 1.00f }, // < - md.green   12
	{ 0.50f, 0.50f, 0.00f, 1.00f }, // = - md.yellow  13
	{ 0.00f, 0.00f, 0.50f, 1.00f }, // > - md.blue    14
	{ 0.50f, 0.00f, 0.00f, 1.00f }, // ? - md.red   15
	{ 0.50f, 0.25f, 0.00f, 1.00f }, // @ - md.orange  16
	{ 1.00f, 0.60f, 0.10f, 1.00f }, // A - lt.orange  17
	{ 0.00f, 0.50f, 0.50f, 1.00f }, // B - md.cyan    18
	{ 0.50f, 0.00f, 0.50f, 1.00f }, // C - md.purple  19
	{ 0.00f, 0.50f, 1.00f, 1.00f }, // D        20
	{ 0.50f, 0.00f, 1.00f, 1.00f }, // E        21
	{ 0.20f, 0.60f, 0.80f, 1.00f }, // F        22
	{ 0.80f, 1.00f, 0.80f, 1.00f }, // G        23
	{ 0.00f, 0.40f, 0.20f, 1.00f }, // H        24
	{ 1.00f, 0.00f, 0.20f, 1.00f }, // I        25
	{ 0.70f, 0.10f, 0.10f, 1.00f }, // J        26
	{ 0.60f, 0.20f, 0.00f, 1.00f }, // K        27
	{ 0.80f, 0.60f, 0.20f, 1.00f }, // L        28
	{ 0.60f, 0.60f, 0.20f, 1.00f }, // M        29
	{ 1.00f, 1.00f, 0.75f, 1.00f }, // N        30
	{ 1.00f, 1.00f, 0.50f, 1.00f }, // O        31
};

int StrlenNocolor( const char *string )
{
	int len = 0;
	for ( const auto& token : Parser( string ) )
	{
		if ( token.Type() == Token::TokenType::CHARACTER || token.Type() == Token::TokenType::ESCAPE )
		{
			len++;
		}
	}

	return len;
}

void StripColors( const char *in, char *out, size_t len )
{
	bool preserveColor = false;

	for ( const auto& token : Parser( in ) )
	{
		if ( token.Type() == Token::TokenType::CONTROL )
		{
			if ( token.RawToken().size() != 1 )
			{
				Log::Warn( "Invalid control token of length %d", token.RawToken().size() );
				continue;
			}
			switch ( token.RawToken()[0] )
			{
				case Constants::DECOLOR_ON:
					preserveColor = false;
					break;
				case Constants::DECOLOR_OFF:
					preserveColor = true;
					break;
			}
			continue;
		}
		Str::StringView text = ( preserveColor && token.Type() == Token::TokenType::COLOR ) ?
			token.RawToken() : token.PlainText();
		if ( text.size() >= len )
		{
			break;
		}
		std::copy( text.begin(), text.end(), out );
		out += text.size();
		len -= text.size();
	}

	*out = '\0';
}

std::string StripColors( Str::StringRef input )
{
	std::string output;
	output.reserve( input.size() );

	bool preserveColor = false;

	for ( const auto& token : Parser( input.c_str() ) )
	{
		if ( token.Type() == Token::TokenType::CONTROL )
		{
			if ( token.RawToken().size() != 1 )
			{
				Log::Warn( "Invalid control token of length %d", token.RawToken().size() );
				continue;
			}
			switch ( token.RawToken()[0] )
			{
				case Constants::DECOLOR_ON:
					preserveColor = false;
					break;
				case Constants::DECOLOR_OFF:
					preserveColor = true;
					break;
			}
			continue;
		}
		Str::StringView text = ( preserveColor && token.Type() == Token::TokenType::COLOR ) ?
			token.RawToken() : token.PlainText();
		output.append( text.begin(), text.end() );
	}

	return output;
}

namespace detail {

const Color& Indexed( int index )
{
    if ( index < 0 )
        index *= -1;
    return g_color_table[index%32];
}

CONSTEXPR_FUNCTION bool Has8Bits( uint8_t v )
{
	return ( v / 0x11 * 0x11 ) != v;
}

std::string ToString ( const Color32Bit& color )
{
    std::string text = "^";

	if ( Has8Bits( color.Red() ) || Has8Bits( color.Green() ) || Has8Bits( color.Blue() ) )
	{
		text += '#';
		text += Str::HexDigit((color.Red() >> 4) & 0xf);
		text += Str::HexDigit(color.Red() & 0xf);
		text += Str::HexDigit((color.Green() >> 4) & 0xf);
		text += Str::HexDigit(color.Green() & 0xf);
		text += Str::HexDigit((color.Blue() >> 4) & 0xf);
		text += Str::HexDigit(color.Blue() & 0xf);
	}
	else
	{
		text += 'x';
		text += Str::HexDigit(color.Red() & 0xf);
		text += Str::HexDigit(color.Green() & 0xf);
		text += Str::HexDigit(color.Blue() & 0xf);
	}

	return text;

}

} // namespace detail

TokenIterator::value_type TokenIterator::NextToken(const char* input)
{
    if ( !input || *input == '\0' )
    {
        return value_type();
    }

    if ( input[0] == Constants::ESCAPE )
    {
        if ( input[1] == Constants::ESCAPE )
        {
            return value_type( input, input+2, value_type::TokenType::ESCAPE );
        }
        else if ( input[1] == Constants::NULL_COLOR )
        {
            return value_type( input, input+2, parent->DefaultColor() );
        }
        else if ( Str::ctoupper( input[1] ) >= '0' && Str::ctoupper( input[1] ) < 'P' )
        {
            return value_type( input, input+2, detail::Indexed( input[1] - '0' ) );
        }
        else if ( Str::ctolower( input[1] ) == 'x' && Str::cisxdigit( input[2] ) &&
                  Str::cisxdigit( input[3] ) && Str::cisxdigit( input[4] ) )
        {
            return value_type( input, input+5, Color(
                Str::GetHex( input[2] ) / 15.f,
                Str::GetHex( input[3] ) / 15.f,
                Str::GetHex( input[4] ) / 15.f,
                1
            ) );
        }
        else if ( input[1] == '#' )
        {
            bool long_hex = true;
            for ( int i = 0; i < 6; i++ )
            {
                if ( !Str::cisxdigit( input[i+2] ) )
                {
                    long_hex = false;
                    break;
                }
            }
            if ( long_hex )
            {
                return value_type( input, input+8, Color(
                    ( (Str::GetHex( input[2] ) << 4) | Str::GetHex( input[3] ) ) / 255.f,
                    ( (Str::GetHex( input[4] ) << 4) | Str::GetHex( input[5] ) ) / 255.f,
                    ( (Str::GetHex( input[6] ) << 4) | Str::GetHex( input[7] ) ) / 255.f,
                    1
                ) );
            }
        }
    }
	else if ( input[0] == Constants::DECOLOR_ON || input[0] == Constants::DECOLOR_OFF )
	{
		return value_type( input, input + 1, value_type::TokenType::CONTROL );
	}

    return value_type( input, input + Q_UTF8_Width( input ), value_type::TokenType::CHARACTER );
}

} // namespace Color
