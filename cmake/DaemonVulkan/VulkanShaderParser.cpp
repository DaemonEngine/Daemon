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
#include <unordered_map>
#include <algorithm>

#include <cstdlib>

std::string graphicsEnginePath          = DAEMON_VULKAN_GRAPHICS_ENGINE_PATH;
std::string graphicsEngineProcessedPath = DAEMON_VULKAN_GRAPHICS_ENGINE_PROCESSED_PATH;

std::string srcPath      = graphicsEngineProcessedPath + "processed/";
std::string spirvPath    = graphicsEngineProcessedPath + "spirv/";
std::string spirvBinPath = graphicsEngineProcessedPath + "bin/";

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

enum Stage {
	FRAGMENT,
	VERTEX,
	COMPUTE
};

const std::unordered_map<std::string, Stage> stageKeywords = {
	{ "local_size_x", COMPUTE },
	{ "local_size_y", COMPUTE },
	{ "local_size_z", COMPUTE },
	{ "gl_Position",  VERTEX  },
};

std::string ProcessInserts( const std::string& shaderText, Stage* stage, int insertCount = 0, int lineCount = 0 ) {
	std::string out;
	std::istringstream shaderTextStream( StripLicense( shaderText ) );

	std::string line;

	int insertStartCount = insertCount;

	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		++lineCount;
		const std::string::size_type posInsert = line.find( "#insert" );
		const std::string::size_type position  = posInsert == std::string::npos ? line.find( "#include" ) : posInsert;

		if ( !*stage ) {
			for ( const std::pair<std::string, Stage>& pair : stageKeywords ) {
				if ( line.find( pair.first ) != std::string::npos ) {
					*stage = pair.second;
					break;
				}
			}
		}

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

		out += ProcessInserts( glslSrc, stage, insertCount, 0 );
		
		out += "/**************************************************/\n";
		out += "#line " + std::to_string( insertStartCount * 10000 + lineCount ) + "\n";
	}

	return out;
}

int main( int argc, char** argv ) {
	#ifdef _MSC_VER
		std::replace( graphicsEnginePath.begin(),          graphicsEnginePath.end(),          '/', '\\' );
		std::replace( graphicsEngineProcessedPath.begin(), graphicsEngineProcessedPath.end(), '/', '\\' );
		std::replace( srcPath.begin(),                     srcPath.end(),                     '/', '\\' );
		std::replace( spirvPath.begin(),                   spirvPath.end(),                   '/', '\\' );
		std::replace( spirvBinPath.begin(),                spirvBinPath.end(),                '/', '\\' );
	#endif

	/* FILE* shaderStages = fopen( ( graphicsEngineProcessedPath + "ShaderStages" ).c_str(), "w" );

	if ( !shaderStages ) {
		char* err = strerror( errno );
		return 1;
	}

	fseek( shaderStages, 0, 0 ); */

	const std::string baseSpirvOptions = argv[1];

	const bool spirvAsm = !stricmp( argv[2], "ON" );

	for( int i = 3; i < argc; i++ ) {
		std::string path = argv[i];

		FILE* glslSource = fopen( ( graphicsEnginePath + path ).c_str(), "r" );

		if( !glslSource ) {
			char* err = strerror( errno );
			return 1;
		}

		size_t nameOffset = path.rfind( "/" );
		std::string name      = nameOffset == std::string::npos ? path : path.substr( nameOffset );
		std::string nameNoExt = name.substr( 0, name.rfind( "." ) );

		FILE* processedGLSL = fopen( ( srcPath + name ).c_str(), "w" );

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

		Stage stage = FRAGMENT;
		const std::string processedSrc = ProcessInserts( glslSrc, &stage, 0, 0 );
		fwrite( processedSrc.c_str(), sizeof( char ), processedSrc.size(), processedGLSL );

		std::string spirvOptions = baseSpirvOptions;
		switch ( stage ) {
			case VERTEX:
				spirvOptions += " -S vert";
				// fwrite( "vert", sizeof( char ), 4, shaderStages );
				break;
			case FRAGMENT:
				spirvOptions += " -S frag";
				// fwrite( "frag", sizeof( char ), 4, shaderStages );
				break;
			case COMPUTE:
				spirvOptions += " -S comp";
				// fwrite( "comp", sizeof( char ), 4, shaderStages );
				break;
		}

		fclose( glslSource );
		fclose( processedGLSL );

		spirvOptions += " -V " + srcPath + name + " -o " + spirvBinPath + nameNoExt + ".spirvBin";
		if ( spirvAsm ) {
			spirvOptions += " -H > " + spirvPath + nameNoExt + ".spirv";
		}

		int r = system( spirvOptions.c_str() );

		if ( i < argc - 1 ) {
			// fwrite( "\n", sizeof( char ), 1, shaderStages );
		}
	}

	// fclose( shaderStages );
}
