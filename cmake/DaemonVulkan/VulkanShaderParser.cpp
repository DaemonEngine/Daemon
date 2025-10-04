/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2025, Daemon Developers
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

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
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

#include <stdio.h>
#include <errno.h>

#include <string>
#include <sstream>
#include <algorithm>

std::string graphicsEnginePath = DAEMON_VULKAN_GRAPHICS_ENGINE_PATH;
std::string graphicsEngineProcessedPath = DAEMON_VULKAN_GRAPHICS_ENGINE_PROCESSED_PATH;

std::string StripLicense( const std::string& shaderText ) {
	const size_t start = shaderText.find( "/*" );

	if ( start == std::string::npos ) {
		return shaderText;
	}

	const size_t end = shaderText.find( "*/" );

	const char* src = &shaderText.c_str()[end + 2];

	uint32_t pos = end + 2;
	while ( src && ( *src == ' ' || *src == '\t' || *src == '\n' ) ) {
		src++;
		pos++;
	}

	return shaderText.substr( pos );
}

std::string ProcessInserts( const std::string& shaderText, int insertCount = 0, int lineCount = 0 ) {
	std::string out;
	std::istringstream shaderTextStream( StripLicense( shaderText ) );

	std::string line;

	int insertStartCount = insertCount;

	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		++lineCount;
		const std::string::size_type posInsert = line.find( "#insert" );
		const std::string::size_type position  = posInsert == std::string::npos ? line.find( "#include" ) : posInsert;

		if ( position == std::string::npos || line.find_first_not_of( " \t" ) != position ) {
			out += line + "\n";
			continue;
		}

		const bool useInsert = posInsert != std::string::npos;

		std::string shaderInsertPath = line.substr(
			position + ( useInsert ? 8 : 10 ),
			( useInsert ? std::string::npos : line.size() - position - 11 ) );

		if ( useInsert ) {
			shaderInsertPath += ".glsl";
		}

		// Inserted shader lines will start at 10000, 20000 etc. to easily tell them apart from the main shader code
		++insertCount;
		out += "#line " + std::to_string( insertCount * 10000 ) + "\n";
		out += "/**************************************************/\n";

		FILE* glslSource = fopen( ( graphicsEnginePath + shaderInsertPath ).c_str(), "r" );

		if ( !glslSource ) {
			char* err = strerror( errno );
			exit( 3 );
		}

		fseek( glslSource, 0, SEEK_END );
		int size = ftell( glslSource ) + 1;

		fseek( glslSource, 0, 0 );

		std::string glslSrc;
		glslSrc.resize( size );
		fread( glslSrc.data(), sizeof( char ), size, glslSource );
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		fclose( glslSource );

		out += ProcessInserts( glslSrc, insertCount, 0 );
		
		out += "/**************************************************/\n";
		out += "#line " + std::to_string( insertStartCount * 10000 + lineCount ) + "\n";
	}

	return out;
}

int main( int argc, char** argv ) {
	#ifdef _MSC_VER
		std::replace( graphicsEnginePath.begin(),          graphicsEnginePath.end(),          '/', '\\' );
		std::replace( graphicsEngineProcessedPath.begin(), graphicsEngineProcessedPath.end(), '/', '\\' );
	#endif

	for( int i = 1; i < argc; i++ ) {
		std::string path = argv[i];

		FILE* glslSource = fopen( ( graphicsEnginePath + path ).c_str(), "r");

		if( !glslSource ) {
			char* err = strerror( errno );
			return 1;
		}

		size_t nameOffset = path.rfind( "/" );
		std::string name = nameOffset == std::string::npos ? path : path.substr( nameOffset );

		FILE* processedGLSL = fopen( ( graphicsEngineProcessedPath + name ).c_str(), "w" );

		if( !processedGLSL ) {
			return 2;
		}

		fseek( glslSource, 0, SEEK_END );
		int size = ftell( glslSource );

		fseek( glslSource, 0, 0 );
		fseek( processedGLSL, 0, 0 );

		std::string glslSrc;
		glslSrc.resize( size );
		fread( glslSrc.data(), sizeof( char ), size, glslSource );
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		const std::string processedSrc = ProcessInserts( glslSrc, 0, 0 );
		fwrite( processedSrc.c_str(), sizeof( char ), processedSrc.size(), processedGLSL );

		fclose( glslSource );
		fclose( processedGLSL );
	}
}
