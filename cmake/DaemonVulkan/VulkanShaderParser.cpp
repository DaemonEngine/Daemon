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
#include <unordered_set>
#include <vector>
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
	FILE*    file;
	uint32_t size;

	File( const std::string& path, const char* mode ) {
		file = fopen( path.c_str(), mode );

		if ( !file ) {
			printf( "Failed to open file: %s, mode: %s\n", path.c_str(), mode );
			printf( strerror( errno ) );
			exit( 1 );
		}

		fseek( file, 0, SEEK_END );
		size = ftell( file );

		fseek( file, 0, 0 );
	}

	File( const std::string& path, const std::string& path2, const char* mode ) {
		file = fopen( path.c_str(), mode );

		if ( !file ) {
			file = fopen( path2.c_str(), mode );

			if( !file ) {
				printf( "Failed to open file: %s, mode: %s", path.c_str(), mode );
				printf( strerror( errno ) );
				exit( 1 );
			}
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
	const uint64_t start = shaderText.find( "/*" );

	if ( start == std::string::npos ) {
		return shaderText;
	}

	const uint64_t end = shaderText.find( "*/" );

	const char* src = &shaderText.c_str()[end + 2];

	uint64_t pos = end + 2;
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

struct Image {
	std::string src;
	std::string replace;
};

static std::unordered_map<std::string, Image> images;
static uint32_t imageID = 0;

void ProcessImages( const std::string& shaderText ) {
	std::string out;
	std::istringstream shaderTextStream( StripLicense( shaderText ) );

	std::string line;

	bool skip              = false;
	bool pushConst         = false;
	bool pushConstStart    = false;

	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		if ( line.find( "#ifdef __cplusplus" ) != std::string::npos ) {
			skip = true;
			continue;
		}

		if ( skip ) {
			if ( line.find( "#endif" ) != std::string::npos ) {
				skip = false;
			}

			continue;
		}

		enum Img {
			IMG_NONE,
			IMG_2D,
			IMG_3D,
			IMG_CUBE
		};

		Img      img       = IMG_NONE;
		uint32_t imgOffset = 0;

		if ( line.find( "Image2D" ) != std::string::npos ) {
			img       = IMG_2D;
			imgOffset = line.find_first_not_of( "Image2D" );
		} else if ( line.find( "Image3D" ) != std::string::npos ) {
			img       = IMG_3D;
			imgOffset = line.find_first_not_of( "Image3D" );
		} else if ( line.find( "ImageCube" ) != std::string::npos ) {
			img       = IMG_CUBE;
			imgOffset = line.find_first_not_of( "ImageCube" );
		}

		if ( img != IMG_NONE ) {
			uint32_t    offset  = imgOffset + 1;
			std::string format  = line.substr( offset, line.find( " ", offset + 1 ) - offset );

			std::transform( format.begin(), format.end(), format.begin(), ::toupper );

			std::string image   = std::to_string( imageID ) + ", " + format + ", ";

			if ( line.find( "swapchain" ) != std::string::npos ) {
				offset          = line.find( " ", offset + 10 ) + 1;

				image          += line.substr( offset, line.find( " ", offset + 1 ) - offset ) + ", 0, 0, 0, ";
			} else {
				image          += "0.0f, ";

				for ( int i = 0; i < ( img == IMG_2D ? 2 : 3 ); i++ ) {
					offset      = line.find( " ", offset + 1 ) + 1;

					image      += line.substr( offset, line.find( " ", offset + 1 ) - offset ) + ", ";
				}

				if ( img == IMG_2D ) {
					image      += "0, ";
				}
			}

			image              += line.find( "nomips" ) == std::string::npos ? "true, " : "false, ";

			image              += img == IMG_CUBE ? "true" : "false";

			offset              = line.find_last_of( " " ) + 1;
			std::string name    = line.substr( offset, line.size() - offset - 1 );

			images[name]        = { image, "images[" + std::to_string( imageID ) + "]" };

			imageID++;

			continue;
		}

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

		if ( shaderInsertPath == "Images.glsl" ) {
			continue;
		}

		File glslSource { graphicsEnginePath + shaderInsertPath, graphicsSharedPath + shaderInsertPath, "r" };

		std::string glslSrc = glslSource.ReadAll();
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		ProcessImages( glslSrc );
	}
}

static std::unordered_set<std::string> inserts;

std::string ProcessInserts( const std::string& shaderText, Stage* stage, uint32_t* pushConstSize,
	int insertCount = 0, int lineCount = 0 ) {
	std::string out;
	std::istringstream shaderTextStream( StripLicense( shaderText ) );

	std::string line;

	int insertStartCount = insertCount;
	bool skip              = false;
	bool pushConst         = false;
	bool pushConstStart    = false;

	while ( std::getline( shaderTextStream, line, '\n' ) ) {
		lineCount++;

		if ( line.find( "#ifdef __cplusplus" ) != std::string::npos ) {
			skip = true;
			continue;
		}

		if ( skip ) {
			if ( line.find( "#endif" ) != std::string::npos ) {
				skip = false;
			}

			continue;
		}

		if ( line.find( "push_constant" ) != std::string::npos ) {
			pushConst      = true;
			pushConstStart = true;
		}

		if ( pushConst ) {
			if ( line.find( "}" ) != std::string::npos ) {
				pushConst = false;
			} else if ( pushConstStart ) {
				pushConstStart = false;
			} else {
				const uint32_t start = line.find_first_not_of( " \t" );
				const uint32_t end   = std::min( line.find( " ", start ), line.find( "\t", start ) );

				const std::string type = line.substr( start, end - start );

				if ( type == "uint" || type == "uint32" || type == "float" ) {
					*pushConstSize += 4;
				} else {
					// Assumed BDA
					*pushConstSize += 8;
				}
			}
		}

		if ( line.find( "Image2D" ) != std::string::npos || line.find( "Image3D" ) != std::string::npos
			|| line.find( "ImageCube" ) != std::string::npos ) {
			continue;
		}

		uint64_t longestName = 0;
		uint64_t bestOffset  = std::string::npos;

		Image    bestImage;

		for ( const std::pair<std::string, Image>& image : images ) {
			uint64_t offset = line.find( image.first );

			if ( offset != std::string::npos ) {
				longestName = std::max( longestName, image.first.size() );
				bestOffset  = offset;

				bestImage   = image.second;
			}
		}

		if ( bestOffset != std::string::npos ) {
			line = line.substr( 0, bestOffset ) + bestImage.replace + line.substr( bestOffset + longestName );
		}

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

		if ( inserts.contains( shaderInsertPath ) ) {
			continue;
		}

		inserts.insert( shaderInsertPath );

		// Inserted shader lines will start at 10000, 20000 etc. to easily tell them apart from the main shader code
		insertCount++;
		out += "#line " + std::to_string( insertCount * 10000 ) + "\n";
		out += "/**************************************************/\n";

		File glslSource { graphicsEnginePath + shaderInsertPath, graphicsSharedPath + shaderInsertPath, "r" };

		std::string glslSrc = glslSource.ReadAll();
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		out += ProcessInserts( glslSrc, stage, pushConstSize, insertCount, 0 );
		
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

	File spirvBinH  { graphicsCorePath   + "ExecutionGraph/SPIRVBin.h", "w" };
	File spirvH     { graphicsCorePath   + "ExecutionGraph/SPIRV.h",    "w" };
	File spirvIDs   { graphicsSharedPath + "SPIRVIDs.h",                "w" };

	fprintf( spirvBinH.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#include \"../../Math/NumberTypes.h\"\n\n"
	);

	fprintf( spirvH.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#ifndef SPIRV_H\n"
		"#define SPIRV_H\n\n"
		"#include <string>\n\n"
		"#include <unordered_map>\n\n"
		"#include \"../../Math/NumberTypes.h\"\n\n"
		"#include \"../Vulkan.h\"\n\n"
		"#include \"SPIRVBin.h\"\n\n"
		"#include \"../../GraphicsShared/SPIRVIDs.h\"\n\n"
		"enum SPIRVType {\n"
		"\tSPIRV_COMPUTE,\n"
		"\tSPIRV_VERTEX,\n"
		"\tSPIRV_FRAGMENT\n"
		"};\n\n"
		"struct SPIRVModule {\n"
		"\tconst uint32*   code;\n"
		"\tconst SPIRVType type;\n"
		"\tconst uint32    size;\n"
		"\tconst uint32    pushConstSize;\n"
		"};\n\n"
		"constexpr uint32 spirvCount = %u;\n\n"
		"const SPIRVModule SPIRVBin[] = {\n",
		argc - 3
	);

	std::vector<uint32_t> pushConstSizes;

	fprintf( spirvIDs.file,
		"// Auto-generated by VulkanShaderParser, do not modify\n\n"
		"#ifndef SPIRV_IDS_H\n"
		"#define SPIRV_IDS_H\n\n"
	);

	const std::string baseSpirvOptions = argv[1];

	const bool spirvAsm = !stricmp( argv[2], "ON" );

	std::vector<std::string> SPIRVMap;

	SPIRVMap.reserve( argc - 3 );

	for( int i = 3; i < argc; i++ ) {
		std::string path = argv[i];

		File glslSource { graphicsEnginePath + path, "r" };

		std::string glslSrc = glslSource.ReadAll();
		glslSrc.resize( strlen( glslSrc.c_str() ) );
		glslSrc.shrink_to_fit();

		ProcessImages( glslSrc );
	}

	{
		File imageBinds { graphicsEnginePath + "Images.glsl", "w" };

		fprintf( imageBinds.file, "// Auto-generated by VulkanShaderParser, do not modify\n\n" );

		fprintf( imageBinds.file, "struct ImageCfg {\n" );
		fprintf( imageBinds.file, "\tuint  id;\n" );
		fprintf( imageBinds.file, "\tuint  format;\n" );
		fprintf( imageBinds.file, "\tfloat relativeSize;\n" );
		fprintf( imageBinds.file, "\tuint  width;\n" );
		fprintf( imageBinds.file, "\tuint  height;\n" );
		fprintf( imageBinds.file, "\tuint  depth;\n" );
		fprintf( imageBinds.file, "\tbool  useMips;\n" );
		fprintf( imageBinds.file, "\tbool  cube;\n" );
		fprintf( imageBinds.file, "};\n\n" );

		uint32_t imageCount = images.size();

		fprintf( imageBinds.file, "ImageCfg imageConfigs[%u] = ImageCfg[%u] (\n", imageCount, imageCount );

		uint32_t i = 0;
		for ( const std::pair<std::string, Image>& image : images ) {
			fprintf( imageBinds.file, "\tImageCfg( %s )%s\n", image.second.src.c_str(), i < imageCount - 1 ? "," : "" );
			i++;
		}

		fprintf( imageBinds.file, ");\n\n" );

		fprintf( imageBinds.file, "const uint imageCount = %u;", imageCount );
	}

	for( int i = 3; i < argc; i++ ) {
		std::string path = argv[i];

		File glslSource { graphicsEnginePath + path, "r" };

		size_t      nameOffset = path.rfind( "/" );
		std::string name       = nameOffset == std::string::npos ? path : path.substr( nameOffset );
		std::string nameNoExt  = name.substr( 0, name.rfind( "." ) );

		Stage stage            = FRAGMENT;
		uint32_t pushConstID   = 0;
		{
			File processedGLSL { srcPath + name, "w" };

			std::string glslSrc = glslSource.ReadAll();
			glslSrc.resize( strlen( glslSrc.c_str() ) );
			glslSrc.shrink_to_fit();

			uint32_t pushConstSize = 0;
			const std::string processedSrc = ProcessInserts( glslSrc, &stage, &pushConstSize, 0, 0 );
			fwrite( processedSrc.c_str(), sizeof( char ), processedSrc.size(), processedGLSL.file );

			std::vector<uint32_t>::iterator it = std::find( pushConstSizes.begin(), pushConstSizes.end(), pushConstSize );
			if ( it == pushConstSizes.end() ) {
				pushConstID = pushConstSizes.size();
				pushConstSizes.push_back( pushConstSize );
			} else {
				pushConstID = it - pushConstSizes.begin();
			}
		}

		std::string spirvOptions = baseSpirvOptions;
		std::string spirvType;
		switch ( stage ) {
			case VERTEX:
				spirvOptions += " -S vert";
				spirvType     = "SPIRV_VERTEX";
				break;
			case FRAGMENT:
				spirvOptions += " -S frag";
				spirvType     = "SPIRV_FRAGMENT";
				break;
			case COMPUTE:
			default:
				spirvOptions += " -S comp";
				spirvType     = "SPIRV_COMPUTE";
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

		fprintf( spirvH.file,    "\t{ ( uint32* ) %s, %s, %u, %u }",
			( nameNoExt + "Bin" ).c_str(), spirvType.c_str(), binSize, pushConstID);
		
		SPIRVMap.push_back( nameNoExt );

		fprintf( spirvIDs.file,  "constexpr uint32 %s = %u;\n\n", nameNoExt.c_str(), i - 3 );

		if ( i < argc - 1 ) {
			fprintf( spirvBinH.file, "\n" );
			fprintf( spirvH.file,    ",\n" );
		}

		inserts.clear();
	}

	fprintf( spirvH.file, "\n};\n\n" );
	
	fprintf( spirvH.file, "const std::unordered_map<std::string, uint32> SPIRVMap {\n" );
	
	for( int i = 0; i < argc - 3; i++ ) {
		fprintf( spirvH.file, "\t{ \"%s\", %s }%s\n", SPIRVMap[i].c_str(), SPIRVMap[i].c_str(), i < argc - 4 ? "," : "" );
	}

	fprintf( spirvH.file, "};\n\n" );

	fprintf( spirvH.file, "constexpr uint32 pushConstSizesCount = %u;\n\n", pushConstSizes.size() );

	fprintf( spirvH.file, "const uint32 pushConstSizes[] = { " );

	if ( pushConstSizes.size() ) {
		for ( uint32_t i = 0; i < pushConstSizes.size() - 1; i++ ) {
			fprintf( spirvH.file, "%u, ", pushConstSizes[i] );
		}

		fprintf( spirvH.file, "%u };\n\n", pushConstSizes.back() );
	}

	fprintf( spirvH.file, "#endif // SPIRV_H" );

	fprintf( spirvIDs.file, "#endif // SPIRV_IDS_H" );
}