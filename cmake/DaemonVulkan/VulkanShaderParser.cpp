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

std::string graphicsCorePath            = DAEMON_VULKAN_GRAPHICS_CORE_PATH;
std::string graphicsEnginePath          = DAEMON_VULKAN_GRAPHICS_ENGINE_PATH;
std::string graphicsSharedPath          = DAEMON_VULKAN_GRAPHICS_SHARED_PATH;
std::string graphicsEngineProcessedPath = DAEMON_VULKAN_GRAPHICS_ENGINE_PROCESSED_PATH;

std::string srcPath      = graphicsEngineProcessedPath + "processed/";
std::string spirvPath    = graphicsEngineProcessedPath + "spirv/";
std::string spirvBinPath = graphicsEngineProcessedPath + "bin/";

struct File {
	FILE* file;
	uint32_t size;

	File( const std::string& path, const char* mode ) {
		file = fopen( path.c_str(), mode );

		if ( !file ) {
			printf( "Failed to open file: %s, mode: %s", path.c_str(), mode );
			printf( strerror( errno ) );
			exit( 1 );
		}

		fseek( file, 0, SEEK_END );
		size = ftell( file );

		fseek( file, 0, 0 );
	}

	~File() {
		fclose( file );
	}

	std::string ReadAll() {
		std::string data;
		data.resize( size );
		fread( data.data(), sizeof( char ), size, file );

		return data;
	}

	std::vector<unsigned char> ReadAllUByte() {
		std::vector<unsigned char> data;
		data.resize( size );
		fread( data.data(), sizeof( char ), size, file );

		return data;
	}
};

std::string StripLicense( const std::string& shaderText ) {
	const uint32_t start = shaderText.find( "/*" );

	if ( start == std::string::npos ) {
		return shaderText;
	}

	const uint32_t end = shaderText.find( "*/" );

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

		File glslSource { graphicsEnginePath + shaderInsertPath, "r" };

		std::string glslSrc = glslSource.ReadAll();
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		out += ProcessInserts( glslSrc, stage, insertCount, 0 );
		
		out += "/**************************************************/\n";
		out += "#line " + std::to_string( insertStartCount * 10000 + lineCount ) + "\n";
	}

	return out;
}

int main( int argc, char** argv ) {
	#ifdef _MSC_VER
		std::replace( graphicsCorePath.begin(),            graphicsCorePath.end(),            '/', '\\' );
		std::replace( graphicsEnginePath.begin(),          graphicsEnginePath.end(),          '/', '\\' );
		std::replace( graphicsSharedPath.begin(),          graphicsSharedPath.end(),          '/', '\\' );
		std::replace( graphicsEngineProcessedPath.begin(), graphicsEngineProcessedPath.end(), '/', '\\' );
		std::replace( srcPath.begin(),                     srcPath.end(),                     '/', '\\' );
		std::replace( spirvPath.begin(),                   spirvPath.end(),                   '/', '\\' );
		std::replace( spirvBinPath.begin(),                spirvBinPath.end(),                '/', '\\' );
	#endif

	File spirvBinH { graphicsCorePath   + "RenderGraph/SPIRVBin.h", "w" };
	File spirvH    { graphicsCorePath   + "RenderGraph/SPIRV.h",    "w" };
	File spirvIDs  { graphicsSharedPath + "SPIRVIDs.h",             "w" };

	fprintf( spirvBinH.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#include \"../../Math/NumberTypes.h\"\n\n"
	);

	fprintf( spirvH.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#ifndef SPIRV_H\n"
		"#define SPIRV_H\n\n"
		"#include \"../../Math/NumberTypes.h\"\n\n"
		"#include \"../Vulkan.h\"\n\n"
		"#include \"SPIRVBin.h\"\n\n"
		"struct SPIRVModule {\n"
		"\tconst uint32*               code;\n"
		"\tconst uint32                size;\n"
		"\tconst VkShaderStageFlagBits stage;\n"
		"};\n\n"
		"constexpr uint32 spirvCount = %u;\n\n"
		"const SPIRVModule SPIRVBin[] = {\n",
		argc - 3
	);

	fprintf( spirvIDs.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#ifndef SPIRV_IDS_H\n"
		"#define SPIRV_IDS_H\n\n"
	);

	const std::string baseSpirvOptions = argv[1];

	const bool spirvAsm = !stricmp( argv[2], "ON" );

	for( int i = 3; i < argc; i++ ) {
		std::string path = argv[i];

		File glslSource { graphicsEnginePath + path, "r" };

		size_t nameOffset = path.rfind( "/" );
		std::string name      = nameOffset == std::string::npos ? path : path.substr( nameOffset );
		std::string nameNoExt = name.substr( 0, name.rfind( "." ) );

		Stage stage = FRAGMENT;
		{
			File processedGLSL { srcPath + name, "w" };

			std::string glslSrc = glslSource.ReadAll();
			glslSrc.resize( strlen( glslSrc.c_str() ) );
			glslSrc.shrink_to_fit();

			const std::string processedSrc = ProcessInserts( glslSrc, &stage, 0, 0 );
			fwrite( processedSrc.c_str(), sizeof( char ), processedSrc.size(), processedGLSL.file );
		}

		std::string spirvOptions = baseSpirvOptions;
		const char* stageStr;
		switch ( stage ) {
			case VERTEX:
				spirvOptions += " -S vert";
				stageStr      = "VK_SHADER_STAGE_VERTEX_BIT";
				break;
			case FRAGMENT:
				spirvOptions += " -S frag";
				stageStr      = "VK_SHADER_STAGE_FRAGMENT_BIT";
				break;
			case COMPUTE:
			default:
				spirvOptions += " -S comp";
				stageStr      = "VK_SHADER_STAGE_COMPUTE_BIT";
				break;
		}

		std::string binPath = spirvBinPath + nameNoExt + ".spirvBin";

		spirvOptions += " -V " + srcPath + name + " -o " + binPath;
		if ( spirvAsm ) {
			spirvOptions += " -H > " + spirvPath + nameNoExt + ".spirv";
		}

		int res = system( spirvOptions.c_str() );

		fprintf( spirvBinH.file, "constexpr unsigned char %sBin[] = {", nameNoExt.c_str() );

		uint32_t binSize;
		{
			File spirvBin { binPath, "rb" };

			std::vector<unsigned char> spirvBinData = spirvBin.ReadAllUByte();
			binSize = spirvBin.size;

			for ( unsigned char* c = spirvBinData.data(); c < spirvBinData.data() + spirvBinData.size() - 1; c++ ) {
				fprintf( spirvBinH.file, "0x%02x,", *c );
			}
			fprintf( spirvBinH.file, "0x%02x", spirvBinData.back() );
		}

		fprintf( spirvBinH.file, " };" );

		fprintf( spirvH.file,    "\t{ ( uint32* ) %s, %u, %s }", ( nameNoExt + "Bin" ).c_str(), binSize, stageStr );

		fprintf( spirvIDs.file,  "constexpr uint32 %s = %u;\n\n", nameNoExt.c_str(), i - 3 );

		if ( i < argc - 1 ) {
			fprintf( spirvBinH.file, "\n" );
			fprintf( spirvH.file,    ",\n" );
		}
	}

	fprintf( spirvH.file, "\n};\n\n" );
	fprintf( spirvH.file, "#endif // SPIRV_H" );

	fprintf( spirvIDs.file, "#endif // SPIRV_IDS_H" );
}
