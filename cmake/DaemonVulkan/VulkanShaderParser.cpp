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
		size = fread( data.data(), sizeof( char ), size, file );

		data.resize( size );
		data.shrink_to_fit();

		return data;
	}

	std::vector<unsigned char> ReadAllUByte() {
		std::vector<unsigned char> data;
		data.resize( size );
		fread( data.data(), sizeof( char ), size, file );

		return data;
	}
};

struct StringView {
	const char* memory;
	uint32_t    size;

	StringView& operator++() {
		( *this ) += 1;
		return *this;
	}

	StringView operator++( int ) {
		StringView t = *this;
		( *this )++;
		return t;
	}

	StringView& operator+=( const uint32_t offset ) {
		size -= offset;
		memory += offset;
		return *this;
	}
};

bool operator==( const StringView& lhs, const char* rhs ) {
	if ( !lhs.size || !lhs.memory && rhs ) {
		return false;
	}

	if ( lhs.size && !rhs ) {
		return false;
	}

	const uint32_t strSize = strlen( rhs );

	if ( lhs.size != strSize ) {
		return false;
	}

	for ( const char* c = lhs.memory, *c2 = rhs; c < lhs.memory + lhs.size; c++, c2++ ) {
		if ( *c != *c2 ) {
			return false;
		}
	}

	return true;
}

static uint32_t newLines;

StringView Parse( StringView& data, std::string* outStr = nullptr, const char* allowedSymbols = "_/\\$~-@.#" ) {
	const char* text = data.memory;

	newLines = 0;

	if ( !text ) {
		return {};
	}

	const char* current = text;

	if ( !current ) {
		return { text };
	}

	if ( outStr ) {
		outStr->reserve( data.size );
		*outStr = "";
	}

	uint32_t offset = 0;

	int c;

	while ( *current ) {
		// Whitespace
		while ( ( c = *current & 0xFF ) <= ' ' ) {
			if ( !c ) {
				break;
			}

			if ( c == '\n' ) {
				newLines++;
			}

			current++;
			offset++;

			if ( outStr ) {
				outStr->push_back( c );
			}
		}

		if ( !current ) {
			return {};
		}

		c = *current;

		// Comments
		if ( c == '/' && current[1] == '/' ) {
			current += 2;
			offset  += 2;

			newLines++;

			while ( *current && *current != '\n' ) {
				current++;
				offset++;
			}
		} else if ( c == '/' && current[1] == '*' ) {
			current += 2;
			offset  += 2;

			while ( *current && ( *current != '*' || current[1] != '/' ) ) {
				if ( c == '\n' ) {
					newLines++;
				}

				current++;
				offset++;
			}

			if ( *current ) {
				current += 2;
				offset  += 2;
			}
		} else {
			break;
		}
	}

	uint32_t size = 0;

	// Quoted strings
	if ( c == '\"' ) {
		current++;
		offset++;

		while ( true ) {
			c = *current;

			current++;

			if ( c == '\n' ) {
				newLines++;
			}

			if ( outStr ) {
				outStr->push_back( c );
			}

			if ( ( c == '\\' ) && ( *current == '\"' ) ) {
				// Allow quoted strings to use \" to indicate the " character
				current++;
			} else if ( c == '\"' || !c ) {
				data += offset + size + 1;
				return { text + offset, size };
			}

			size++;
		}
	}

	bool id = false;
	while ( true ) {
		c = *current;

		while ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ) {
			if ( outStr ) {
				outStr->push_back( c );
			}

			current++;
			size++;
			c = *current;

			id = true;
		}

		const char* allowedSymbol;

		for ( allowedSymbol = allowedSymbols; allowedSymbol < allowedSymbols + strlen( allowedSymbols ); allowedSymbol++ ) {
			if ( c == *allowedSymbol ) {
				if ( outStr ) {
					outStr->push_back( c );
				}

				current++;
				size++;

				id = true;

				break;
			}
		}

		c = *current;

		if ( !*allowedSymbol ) {
			break;
		}
	}

	if ( id ) {
		data += offset + size;
		return { text + offset, size };
	}

	// Single character punctuation / EOF
	data += offset + ( c ? 1 : 0 );

	if ( outStr && c ) {
		outStr->push_back( c );
	}

	return { text + offset, c ? 1u : 0u };
}

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
static uint32_t imageID  = 18;

static std::unordered_map<std::string, std::string> buffers;
static uint32_t bufferID = 0;

void SkipCPPIfDef( StringView& data ) {
	StringView v    = data;
	bool       skip = false;

	do {
		StringView o = Parse( v );

		if ( o == "#ifdef" ) {
			o = Parse( v );

			if ( o == "__cplusplus" ) {
				skip = true;
				continue;
			}
		} else if ( !skip ) {
			return;
		}

		if ( skip && o == "#endif" ) {
			data = v;
			return;
		}
	} while ( v.size );
}

void SkipScope( StringView& data ) {
	StringView v = data;
	bool       skip = false;

	do {
		StringView o = Parse( v );

		if ( o == "{" ) {
			skip = true;
			continue;
		} else if ( !skip ) {
			return;
		}

		if ( skip && o == "}" ) {
			data = v;
			return;
		}
	} while ( v.size );
}

void SkipRestOfLine( StringView& data ) {
	int c;

	while ( ( c = *data.memory & 0xFF ) != '\n' ) {
		if ( !c ) {
			return;
		}

		data.memory++;
		data.size--;
	}
}

enum Img {
	IMG_NONE,
	IMG_2D,
	IMG_3D,
	IMG_CUBE
};

void ParseImage( StringView& data, Img type ) {
	StringView o = Parse( data );

	std::string name { o.memory, o.size };

	if ( images.find( name ) != images.end() ) {
		SkipScope( data );
		SkipRestOfLine( data );

		return;
	}

	o = Parse( data );

	if ( o != "{" ) {
		return;
	}

	o = Parse( data );

	std::string format { o.memory, o.size };

	std::transform( format.begin(), format.end(), format.begin(), ::toupper );

	std::string image = std::to_string( imageID ) + ", " + format + ", ";

	o = Parse( data );

	if ( o == "rel" ) {
		o = Parse( data );

		image += std::string { o.memory, o.size } + ", 0, 0, 0, ";
	} else {
		image += "0.0f, " + std::string { o.memory, o.size } + ", ";

		o = Parse( data );

		image += std::string { o.memory, o.size } + ", ";

		if ( type == IMG_2D ) {
			image += "0, ";
		} else {
			o = Parse( data );

			image += std::string { o.memory, o.size } + ", ";
		}
	}

	o = Parse( data );

	if ( o == "nomips" ) {
		image += "false, ";
	} else {
		image += "true, ";
	}

	image += type == IMG_CUBE ? "true" : "false";

	SkipRestOfLine( data );

	images[name] = { image, "images[" + std::to_string( imageID ) + "]" };

	imageID++;
}

void ParseBuffer( StringView& data ) {
	StringView o = Parse( data );

	std::string name { o.memory, o.size };

	if ( buffers.find( name ) != buffers.end() ) {
		SkipScope( data );
		SkipRestOfLine( data );

		return;
	}

	o = Parse( data );

	if ( o != "{" ) {
		return;
	}

	std::string buffer = std::to_string( bufferID ) + ", ";

	o = Parse( data );

	if ( o == "rel" ) {
		o = Parse( data );

		buffer += std::string { o.memory, o.size } + ", 0, ";
	} else {
		buffer += "0.0f, " + std::string { o.memory, o.size } + ", ";
	}

	o = Parse( data );

	// usage
	if ( o != "}" ) {
		buffer += std::string { o.memory, o.size };
	} else {
		buffer += "0";
	}

	SkipRestOfLine( data );

	buffers[name] = buffer;

	bufferID++;
}

void ProcessImagesBuffers( const std::string& shaderText ) {
	StringView v { shaderText.c_str(), shaderText.size() };

	do {
		SkipCPPIfDef( v );

		StringView o = Parse( v );

		if ( o == "Image2D" ) {
			ParseImage( v, IMG_2D );
			continue;
		} else if ( o == "Image3D" ) {
			ParseImage( v, IMG_3D );
			continue;
		} else if ( o == "ImageCube" ) {
			ParseImage( v, IMG_CUBE );
			continue;
		}

		if ( o == "Buffer" ) {
			ParseBuffer( v );
			continue;
		}

		if ( o == "#" ) {
			o = Parse( v );

			if ( o == "include" || o == "insert" ) {
				o = Parse( v );

				if ( o == "Images.glsl" || o == "Buffers.glsl" ) {
					continue;
				}

				File glslSource {
					graphicsEnginePath + std::string { o.memory, o.size },
					graphicsSharedPath + std::string { o.memory, o.size },
					"r"
				};

				std::string glslSrc = glslSource.ReadAll();

				glslSrc.resize( strlen( glslSrc.c_str() ) );
				glslSrc.shrink_to_fit();

				ProcessImagesBuffers( glslSrc );
			}
		}
	} while ( v.size );
}

static std::unordered_set<std::string> inserts;

struct BufferPushIDs {
	uint32_t count = 0;
	uint32_t ids[16];
};

struct std::unordered_map<uint32_t, BufferPushIDs> bufferPushIDs;
static uint32_t currentSPIRVID = 0;

static std::unordered_set<std::string> bufferPointers;
static std::unordered_set<std::string> bufferPointerTypes;
static std::string extensions;

std::string ProcessInserts( const std::string& shaderText, Stage* stage, uint32_t* pushConstSize,
	int insertCount = 0, int lineCount = 0 ) {
	std::string out;

	int insertStartCount = insertCount;

	StringView v { shaderText.c_str(), shaderText.size() };

	uint32_t offset   = 0;
	uint32_t lastSize = v.size;

	static uint32_t stackDepth = 0;

	do {
		SkipCPPIfDef( v );

		uint32_t insertLastSize = shaderText.size() - v.size;

		std::string outStr;
		StringView o = Parse( v, &outStr );

		lineCount += newLines;

		if ( o == "Image2D" ) {
			ParseImage( v, IMG_2D );
			continue;
		} else if ( o == "Image3D" ) {
			ParseImage( v, IMG_3D );
			continue;
		} else if ( o == "ImageCube" ) {
			ParseImage( v, IMG_CUBE );
			continue;
		}

		if ( o == "Buffer" ) {
			ParseBuffer( v );
			continue;
		}

		if ( o == "push_constant" ) {
			out += outStr;

			while ( o != "{" ) {
				o = Parse( v, &outStr );

				out += outStr;
			}

			while ( true ) {
				o = Parse( v, &outStr );

				if ( o == "}" ) {
					out += outStr;

					break;
				}

				bool constPointer = o == "const";

				if ( constPointer ) {
					o = Parse( v, &outStr );
				}

				out += outStr;

				std::string type { o.memory, o.size };

				o = Parse( v, &outStr );

				bool pointer = o == "*";

				if ( pointer ) {
					o = Parse( v, &outStr );

					out += constPointer ? "_ConstPointer" : "_Pointer";

					std::string pointerType = type + ( constPointer ? "_ConstPointer" : "_Pointer" );

					if ( !bufferPointerTypes.contains( pointerType ) ) {
						static const std::string bufPointer = "layout ( scalar, buffer_reference, buffer_reference_align = 4 ) ";

						out = bufPointer
							+ ( constPointer ? "restrict readonly buffer " : "restrict buffer " )
							+ pointerType + " {\n\t"
							+ type
							+ " memory[];\n};\n\n"
							+ out;

						bufferPointerTypes.insert( pointerType );
					}
				}

				out += outStr;

				std::string name { o.memory, o.size };

				if ( pointer ) {
					bufferPointers.insert( name );
				}

				o = Parse( v, &outStr ); // ;

				out += outStr;

				if ( type == "uint" || type == "uint32" || type == "float" ) {
					*pushConstSize += 4;
				} else if ( buffers.find( name ) != buffers.end() ) {
					*pushConstSize += 8;

					BufferPushIDs& pushIDs = bufferPushIDs[currentSPIRVID];

					pushIDs.ids[pushIDs.count] = stoi( buffers[name].substr( 0, buffers[name].find( "," ) ) );
					pushIDs.count++;
				} else {
					// Assumed BDA
					*pushConstSize += 8;
				}
			};

			continue;
		}

		if ( o == "#extension" ) {
			extensions += outStr;

			do {
				o = Parse( v, &outStr );

				extensions += outStr;
			} while ( o != "require" );

			continue;
		}

		if ( o == "#include" || o == "#insert" ) {
			o = Parse( v );

			std::string shaderInsertPath { o.memory, o.size };

			if ( inserts.contains( shaderInsertPath ) ) {
				continue;
			}

			inserts.insert( shaderInsertPath );

			File glslSource {
				graphicsEnginePath + shaderInsertPath,
				graphicsSharedPath + shaderInsertPath,
				"r"
			};

			std::string glslSrc = glslSource.ReadAll();

			glslSrc.resize( strlen( glslSrc.c_str() ) );
			glslSrc.shrink_to_fit();

			lastSize = v.size;

			// Inserted shader lines will start at 10000, 20000 etc. to easily tell them apart from the main shader code
			insertCount++;
			out += "\n#line " + std::to_string( insertCount * 10000 ) + "\n";
			out += "/**************************************************/\n";

			stackDepth++;

			out += ProcessInserts( glslSrc, stage, pushConstSize, insertCount, 0 );

			stackDepth--;

			out += "\n\n/**************************************************/\n";
			out += "#line " + std::to_string( insertStartCount * 10000 + lineCount ) + "\n";

			continue;
		}

		std::string chk { o.memory, o.size };

		if ( o.size ) {
			StringView bufView = Parse( o, nullptr, "" );

			std::string bufferName { bufView.memory, bufView.size };

			if ( bufferPointers.contains( bufferName ) ) {
				out += " push." + bufferName + ".memory";

				continue;
			}
		}

		if ( !*stage && stageKeywords.contains( chk ) ) {
			*stage = stageKeywords.at( chk );
		}

		out += outStr;
	} while ( v.size );

	return stackDepth ? out : extensions + "\n\n" + out;
}

std::string BufferIDsToString( const BufferPushIDs& buffer ) {
	std::string out;

	for ( uint32_t i = 0; i < buffer.count; i++ ) {
		out += std::to_string( buffer.ids[i] ) + ( i < 15 ? ", " : "" );
	}

	for ( uint32_t i = buffer.count; i < 16; i++ ) {
		out += i < 15 ? "0, " : "0";
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

		ProcessImagesBuffers( glslSrc );
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
			fprintf( imageBinds.file, "\tImageCfg( %s )%s // %s\n", image.second.src.c_str(), i < imageCount - 1 ? "," : "",
			                                                        image.first.c_str() );
			i++;
		}

		fprintf( imageBinds.file, ");\n\n" );

		fprintf( imageBinds.file, "const uint imageCount = %u;", imageCount );
	}

	for( int i = 3; i < argc; i++ ) {
		bufferPointers.clear();
		bufferPointerTypes.clear();

		std::string path = argv[i];

		File glslSource { graphicsEnginePath + path, "r" };

		size_t      nameOffset = path.rfind( "/" );
		std::string name       = nameOffset == std::string::npos ? path : path.substr( nameOffset );
		std::string nameNoExt  = name.substr( 0, name.rfind( "." ) );

		Stage stage            = FRAGMENT;
		uint32_t pushConstID   = 0;

		currentSPIRVID         = i - 3;

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

	{
		File bufferBinds { graphicsEnginePath + "Buffers.glsl", "w" };

		fprintf( bufferBinds.file, "// Auto-generated by VulkanShaderParser, do not modify\n\n" );

		fprintf( bufferBinds.file, "struct BufferCfg {\n" );
		fprintf( bufferBinds.file, "\tuint   id;\n" );
		fprintf( bufferBinds.file, "\tfloat  relativeSize;\n" );
		fprintf( bufferBinds.file, "\tuint64 size;\n" );
		fprintf( bufferBinds.file, "\tuint   usage;\n" );
		fprintf( bufferBinds.file, "};\n\n" );

		uint32_t bufferCount = buffers.size();

		fprintf( bufferBinds.file, "BufferCfg bufferConfigs[%u] = BufferCfg[%u] (\n", bufferCount, bufferCount );

		uint32_t i = 0;
		for ( const std::pair<std::string, std::string>& buffer : buffers ) {
			fprintf( bufferBinds.file, "\tBufferCfg( %s )%s // %s\n", buffer.second.c_str(), i < bufferCount - 1 ? "," : "",
			                                                          buffer.first.c_str() );
			i++;
		}

		fprintf( bufferBinds.file, ");\n\n" );

		fprintf( bufferBinds.file, "const uint bufferCount = %u;\n\n", bufferCount );

		fprintf( bufferBinds.file, "struct SPIRVBufferCfg {\n" );
		fprintf( bufferBinds.file, "\tuint id;\n" );
		fprintf( bufferBinds.file, "\tuint count;\n" );
		fprintf( bufferBinds.file, "\tuint buffers[16];\n" );
		fprintf( bufferBinds.file, "};\n\n" );

		fprintf( bufferBinds.file, "SPIRVBufferCfg SPIRVBufferConfigs[%u] = SPIRVBufferCfg[%u] (\n", argc - 3, argc - 3 );

		for ( int i = 0; i < argc - 3; i++ ) {
			if ( !bufferPushIDs.contains( i ) ) {
				static constexpr BufferPushIDs emptyBufferIDs {};

				fprintf( bufferBinds.file, "\tSPIRVBufferCfg( %i, 0, uint[16] ( %s ) )%s\n", i, BufferIDsToString( emptyBufferIDs ).c_str(),
					i < argc - 4 ? "," : "" );

				continue;
			}

			fprintf( bufferBinds.file, "\tSPIRVBufferCfg( %i, %u, uint[16] ( %s ) )%s\n", i, bufferPushIDs[i].count,
				BufferIDsToString( bufferPushIDs[i] ).c_str(), i < argc - 4 ? "," : "" );
		}

		fprintf( bufferBinds.file, ");\n\n" );

		fprintf( bufferBinds.file, "const uint SPIRVCount = %u;", argc - 3 );
	}

	fprintf( spirvH.file, "\n};\n\n" );
	
	fprintf( spirvH.file, "const std::unordered_map<std::string, uint32> SPIRVMap {\n" );
	
	for ( int i = 0; i < argc - 3; i++ ) {
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