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
#include <algorithm>
#include <vector>
#include <unordered_set>

#define MAX_TOKEN_CHARS 16384

static char com_token[MAX_TOKEN_CHARS];
static char com_parsename[MAX_TOKEN_CHARS];
static int  com_lines;

static const char* punctuation[] =
{
	"+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
	"&&", "||", "<=", ">=", "==", "!=",
	nullptr
};

static const char* SkipWhitespace( const char* data, bool* hasNewLines, std::string* skippedText ) {
	int c;

	while ( ( c = *data & 0xFF ) <= ' ' ) {
		if ( !c ) {
			return nullptr;
		}

		if ( c == '\n' ) {
			com_lines++;
			*hasNewLines = true;
		}

		if ( skippedText ) {
			*skippedText += c;
		}

		data++;
	}

	return data;
}

const char* COM_ParseExt2( const char** data_p, bool allowLineBreaks, std::string* skippedText = nullptr ) {
	int        c = 0, len;
	bool   hasNewLines = false;
	const char* data;
	const char** punc;

	if ( !data_p ) {
		exit( 3 );
	}

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = nullptr;
		return com_token;
	}

	// skip whitespace
	while ( true ) {
		data = SkipWhitespace( data, &hasNewLines, skippedText );

		if ( !data ) {
			*data_p = nullptr;
			return com_token;
		}

		if ( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		if ( skippedText ) {
			// *skippedText += c;
		}

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			data += 2;

			while ( *data && *data != '\n' ) {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			data += 2;

			while ( *data && ( *data != '*' || data[1] != '/' ) ) {
				data++;
			}

			if ( *data ) {
				data += 2;
			}
		} else {
			// a real token to parse
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;

		while ( true ) {
			c = *data++;

			if ( ( c == '\\' ) && ( *data == '\"' ) ) {
				// allow quoted strings to use \" to indicate the " character
				data++;
			} else if ( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p = ( char* ) data;
				return com_token;
			} else if ( *data == '\n' ) {
				com_lines++;
			}

			if ( len < MAX_TOKEN_CHARS - 1 ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// check for a number
	// is this parsing of negative numbers going to cause expression problems
	if ( ( c >= '0' && c <= '9' ) ||
		( c == '-' && data[1] >= '0' && data[1] <= '9' ) ||
		( c == '.' && data[1] >= '0' && data[1] <= '9' ) ||
		( c == '-' && data[1] == '.' && data[2] >= '0' && data[2] <= '9' ) ) {
		do {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				com_token[len] = c;
				len++;
			}

			data++;

			c = *data;
		} while ( ( c >= '0' && c <= '9' ) || c == '.' );

		// parse the exponent
		if ( c == 'e' || c == 'E' ) {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				com_token[len] = c;
				len++;
			}

			data++;
			c = *data;

			if ( c == '-' || c == '+' ) {
				if ( len < MAX_TOKEN_CHARS - 1 ) {
					com_token[len] = c;
					len++;
				}

				data++;
				c = *data;
			}

			do {
				if ( len < MAX_TOKEN_CHARS - 1 ) {
					com_token[len] = c;
					len++;
				}

				data++;

				c = *data;
			} while ( c >= '0' && c <= '9' );
		}

		if ( len == MAX_TOKEN_CHARS ) {
			len = 0;
		}

		com_token[len] = 0;

		*data_p = ( char* ) data;
		return com_token;
	}

	// check for a regular word
	// we still allow forward and back slashes in name tokens for pathnames
	// and also colons for drive letters
	if ( ( c >= 'a' && c <= 'z' ) ||
		( c >= 'A' && c <= 'Z' ) ||
		( c == '_' ) ||
		( c == '/' ) ||
		( c == '\\' ) ||
		( c == '$' ) || ( c == '*' ) ) // Tr3B - for bad shader strings
	{
		do {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				com_token[len] = c;
				len++;
			}

			data++;

			c = *data;
		} while
			( ( c >= 'a' && c <= 'z' ) ||
				( c >= 'A' && c <= 'Z' ) ||
				( c == '_' ) ||
				( c == '-' ) ||
				( c >= '0' && c <= '9' ) ||
				( c == '/' ) ||
				( c == '\\' ) ||
				( c == ':' ) ||
				( c == '.' ) ||
				( c == '$' ) ||
				( c == '*' ) ||
				( c == '@' ) );

		if ( len == MAX_TOKEN_CHARS ) {
			len = 0;
		}

		com_token[len] = 0;

		*data_p = ( char* ) data;
		return com_token;
	}

	// check for multi-character punctuation token
	for ( punc = punctuation; *punc; punc++ ) {
		int l;
		int j;

		l = strlen( *punc );

		for ( j = 0; j < l; j++ ) {
			if ( data[j] != ( *punc )[j] ) {
				break;
			}
		}

		if ( j == l ) {
			// a valid multi-character punctuation
			memcpy( com_token, *punc, l );
			com_token[l] = 0;
			data += l;
			*data_p = ( char* ) data;
			return com_token;
		}
	}

	// single character punctuation
	com_token[0] = *data;
	com_token[1] = 0;
	data++;
	*data_p = ( char* ) data;

	return com_token;
}

std::string SkipRestOfLine( const char** data ) {
	const char* p = *data;

	int  c;

	std::string out;
	while ( ( c = *p++ ) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}

		out += c;
	}

	*data = p;

	return out + "\n";
}

int Q_strnicmp( const char* string1, const char* string2, int n ) {
	int c1, c2;

	if ( string1 == nullptr ) {
		return ( string2 == nullptr ) ? 0 : -1;
	} else if ( string2 == nullptr ) {
		return 1;
	}

	do {
		c1 = *string1++;
		c2 = *string2++;

		if ( !n-- ) {
			return 0; // Strings are equal until end point
		}

		if ( c1 != c2 ) {
			if ( c1 >= 'a' && c1 <= 'z' ) {
				c1 -= ( 'a' - 'A' );
			}

			if ( c2 >= 'a' && c2 <= 'z' ) {
				c2 -= ( 'a' - 'A' );
			}

			if ( c1 != c2 ) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while ( c1 );

	return 0; // Strings are equal
}

bool cisdigit( int c ) {
	return c >= '0' && c <= '9';
}

bool cislower( int c ) {
	return c >= 'a' && c <= 'z';
}

int ctoupper( int c ) {
	if ( cislower( c ) )
		return c - 'a' + 'A';
	else
		return c;
}

int Q_stricmp( const char* s1, const char* s2 ) {
	if ( !s1 || !s2 ) {
		return Q_strnicmp( s1, s2, 0 );
	}
	while ( *s1 ) {
		if ( *s1 != *s2 ) {
			int uc1 = ctoupper( *s1 );
			int uc2 = ctoupper( *s2 );
			if ( uc1 < uc2 ) return -1;
			if ( uc1 > uc2 ) return 1;
		}
		s1++;
		s2++;
	}
	return *s2 == '\0' ? 0 : -1;
}

const char* Q_stristr( const char* s, const char* find ) {
	char   c, sc;
	size_t len;

	if ( ( c = *find++ ) != 0 ) {
		if ( c >= 'a' && c <= 'z' ) {
			c -= ( 'a' - 'A' );
		}

		len = strlen( find );

		do {
			do {
				if ( ( sc = *s++ ) == 0 ) {
					return nullptr;
				}

				if ( sc >= 'a' && sc <= 'z' ) {
					sc -= ( 'a' - 'A' );
				}
			} while ( sc != c );
		} while ( Q_strnicmp( s, find, len ) != 0 );

		s--;
	}

	return s;
}

std::string ParseEnum( const char** text ) {
	std::string out;

	bool foundClosingBrace = false;
	while ( true ) {
		const char* token = COM_ParseExt2( text, false, &out );

		if ( !token || !strlen( *text ) ) {
			exit( 4 );
		}

		if ( foundClosingBrace && *token == ';' ) {
			return out + ";\n\n";
		}

		if ( *token == '}' ) {
			foundClosingBrace = true;
			// break;
		}

		out += token;

		// out += " ";
		// out += token;
	}
}

char* Q_strlwr( char* s1 ) {
	char* s;

	for ( s = s1; *s; ++s ) {
		if ( ( 'A' <= *s ) && ( *s <= 'Z' ) ) {
			*s -= 'A' - 'a';
		}
	}

	return s1;
}

char* Q_strupr( char* s1 ) {
	char* cp;

	for ( cp = s1; *cp; ++cp ) {
		if ( ( 'a' <= *cp ) && ( *cp <= 'z' ) ) {
			*cp += 'A' - 'a';
		}
	}

	return s1;
}

std::string ParseStruct( const char** text ) {
	std::string out;
	std::string name;

	bool foundClosingBrace = false;
	bool foundName = false;
	bool baseStruct = false;
	while ( true ) {
		const char* token = COM_ParseExt2( text, true );

		if ( !token || !strlen( *text ) ) {
			exit( 4 );
		}

		if ( !foundName ) {
			name = token;
			foundName = true;

			if ( Q_stristr( token, "VkBaseInStructure" ) || Q_stristr( token, "VkBaseOutStructure" ) ) {
				baseStruct = true;
			}

			out += " ";
			out += token;

			continue;
		}

		if ( foundClosingBrace && *token == ';' ) {
			return out + ";\n\n";
		}

		if ( *token == '}' ) {
			foundClosingBrace = true;
			out = out.erase( out.size() - 1, 1 );
			// break;
		}

		if ( !baseStruct && Q_stristr( token, "VkStructureType" ) ) {
			out += "\n\tconst VkStructureType ";
		} else if ( !baseStruct && Q_stristr( token, "sType" ) ) {
			out += "sType = VK_STRUCTURE_TYPE";

			for ( const char* symbol = name.c_str() + 2; symbol < name.c_str() + name.size(); symbol++ ) {
				char s[2];
				s[0] = *symbol;
				s[1] = '\0';

				if ( !cislower( *symbol ) || cisdigit( *symbol ) ) {
					out += "_";
				}

				char s2[2];
				s2[0] = *symbol;
				s2[1] = '\0';

				Q_strupr( s2 );

				out += s2;
			}
		} else if( *token == ';' ) {
			out += ";\n\t";
		} else {
			out += " ";
			out += token;
		}
	}
}

std::vector<std::string> functionNames;
std::vector<std::string> physicalDeviceFunctionNames;
std::vector<std::string> functionDeclarations;

std::string ParseFunction( const char** text ) {
	std::string out;
	std::string out2;

	bool found = false;
	bool physicalDeviceRequired = false;
	while ( true ) {
		const char* token = COM_ParseExt2( text, false );

		if ( !token || !strlen( *text ) ) {
			exit( 4 );
		}

		if ( !found && Q_stristr( token, "PFN_" ) ) {
			out = token + 5;
			found = true;
		} else if ( *token == ';' ) {
			if ( found ) {
				if ( physicalDeviceRequired ) {
					physicalDeviceFunctionNames.emplace_back( out );
				} else {
					functionNames.emplace_back( out );
				}
			}

			return out2 + ";\n\n";
		} else if ( Q_stristr( token, "VkPhysicalDevice" ) ) {
			physicalDeviceRequired = true;
		}

		out2 += token;
		out2 += " ";
	}
}

void ParseFunctionDeclaration( const char** text ) {
	std::string out = "VKAPI_ATTR ";

	while ( true ) {
		const char* token = COM_ParseExt2( text, true );

		if ( !token || !strlen( *text ) ) {
			exit( 5 );
		}

		if ( *token == ';' ) {
			functionDeclarations.emplace_back( out.substr( 0, out.size() - 1 ) + ";\n\n" );
			return;
		}

		out += token;
		out += " ";
	}
}

std::string ParseTypedef( const char** text ) {
	std::string out;

	bool foundClosingBrace = false;
	while ( true ) {
		const char* token = COM_ParseExt2( text, false );

		if ( !token || !strlen( *text ) ) {
			exit( 4 );
		}

		if ( Q_stristr( token, "enum" ) ) {
			return "enum " + ParseEnum( text );
		} else if ( Q_stristr( token, "struct" ) ) {
			return "struct " + ParseStruct( text );
		} else if ( Q_stristr( token, "union" ) ) {
			return "union " + ParseStruct( text );
		} else {
			out += token;
			out += " " + ParseFunction( text );
			return out;
		}
	}
}

std::string ParseHeader( const std::string& header ) {
	std::string functionDefs;
	functionDefs.reserve( header.size() );

	std::string out;
	out.reserve( header.size() );

	const char* hdr = header.c_str();
	const char** text = &hdr;
	
	while ( true ) {
		const char* token = COM_ParseExt2( text, false, &out );

		if ( !token || !*text || !strlen( *text ) ) {
			break;
		}

		if ( *token == '#' ) {
			out += "#" + SkipRestOfLine(text);
		} else if ( Q_stristr( token, "typedef" ) ) {
			out += "typedef " + ParseTypedef( text ) + "\n";
		} else if ( *token == ';' ) {
			out += ";\n";
		} else {
			out += token;
		}
		/* else if ( Q_stristr( token, "VKAPI_ATTR" ) ) {
			ParseFunctionDeclaration( text );
		} */
	}

	return out;
}

void WriteFunctionDefs( FILE* vulkanLoaderHeader, FILE* vulkanLoaderSource, FILE* vulkanSource ) {
	static const char* headerStart =
		"// Auto-generated by VulkanHeaderParser, do not modify\n\n"
		"#ifndef VULKAN_LOADER_H\n"
		"#define VULKAN_LOADER_H\n\n"
		"#include <vulkan/vulkan.h>\n\n";

	static const char* headerEnd =
		"#endif\0";

	fseek( vulkanLoaderHeader, 0, 0 );
	fseek( vulkanLoaderSource, 0, 0 );
	fseek( vulkanSource, 0, 0 );

	fwrite( headerStart, sizeof( char ), strlen( headerStart ), vulkanLoaderHeader );

	static const char* sourceStart =
		"// Auto-generated by VulkanHeaderParser, do not modify\n\n"

		"#ifdef _MSC_VER\n"
		"\t#include <windows.h>\n"
		"#else\n"
		"\t#include <dlfcn.h>\n"
		"#endif\n\n"

		"#include \"../GraphicsCore/Vulkan.h\"\n\n"
		"#include \"VulkanLoadFunctions.h\"\n\n"

		"#ifdef _MSC_VER\n"
		"\tHMODULE libVulkan;\n"
		"#else\n"
		"\tvoid*   libVulkan;\n"
		"#endif\n\n"

		"void VulkanLoaderInit() {\n"

		"\t#ifdef _MSC_VER\n"
		"\t\tlibVulkan = LoadLibrary( \"vulkan-1.dll\" );\n"
		"\t\tvkGetInstanceProcAddr = ( PFN_vkGetInstanceProcAddr ) GetProcAddress( libVulkan, \"vkGetInstanceProcAddr\" );\n"
		"\t#else\n"
		"\t\tlibVulkan = dlopen( \"libvulkan.so\", RTLD_NOW );\n"
		"\t\tvkGetInstanceProcAddr = dlsym( libVulkan, \"vkGetInstanceProcAddr\" );\n"
		"\t#endif\n\n"

		"\tvkEnumerateInstanceVersion = ( PFN_vkEnumerateInstanceVersion ) vkGetInstanceProcAddr( nullptr, \"vkEnumerateInstanceVersion\" );\n\n"
		"\tvkEnumerateInstanceExtensionProperties = ( PFN_vkEnumerateInstanceExtensionProperties ) vkGetInstanceProcAddr( nullptr, \"vkEnumerateInstanceExtensionProperties\" );\n\n"
		"\tvkEnumerateInstanceLayerProperties = ( PFN_vkEnumerateInstanceLayerProperties ) vkGetInstanceProcAddr( nullptr, \"vkEnumerateInstanceLayerProperties\" );\n\n"
		"\tvkCreateInstance = ( PFN_vkCreateInstance ) vkGetInstanceProcAddr( nullptr, \"vkCreateInstance\" );\n"

		"}\n\n"

		"void VulkanLoaderFree() {\n"

		"\t#ifdef _MSC_VER\n"
		"\t\tFreeLibrary( libVulkan );\n"
		"\t#else\n"
		"\t\tdlclose( libVulkan );\n"
		"\t#endif\n"

		"}\n\n"

		"void VulkanLoadInstanceFunctions( VkInstance instance ) {\n"

		"\tvkGetDeviceProcAddr = ( PFN_vkGetDeviceProcAddr ) vkGetInstanceProcAddr( instance, \"vkGetDeviceProcAddr\" );\n\n";

	static const char* sourceFunctionEnd =
		"}\n\n";

	static const char* sourceDeviceLoadFunction =
		"void VulkanLoadDeviceFunctions( VkDevice device, VkInstance instance ) {\n";

	static const char* sourceEnd =
		"}\0";

	fwrite( sourceStart, sizeof( char ), strlen( sourceStart ), vulkanLoaderSource );

	std::unordered_set<std::string> loadedDefs {
		"vkGetInstanceProcAddr", "vkGetDeviceProcAddr",
		"vkEnumerateInstanceVersion",
		"vkEnumerateInstanceExtensionProperties",
		"vkEnumerateInstanceLayerProperties",
		"vkCreateInstance"
	};

	/*
	static const char* instFunction =
		"VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr( VkInstance instance, const char* pName ); \n\n"
		"VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr( VkDevice device, const char* pName );\n\n"
		"VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion( uint32_t * pApiVersion );\n\n"
		"VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties( const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties );\n\n"
		"VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties( uint32_t * pPropertyCount, VkLayerProperties* pProperties );\n\n"
		"VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance( const VkInstanceCreateInfo * pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance );\n\n";
	*/

	static const char* instFunction =
		"extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr; \n\n"
		"extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;\n\n"
		"extern PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;\n\n"
		"extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;\n\n"
		"extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;\n\n"
		"extern PFN_vkCreateInstance vkCreateInstance;\n\n";

	static const char* instFunctionSrc =
		"PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr; \n\n"
		"PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;\n\n"
		"PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;\n\n"
		"PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;\n\n"
		"PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;\n\n"
		"PFN_vkCreateInstance vkCreateInstance;\n\n";

	fwrite( instFunction, sizeof( char ), strlen( instFunction ), vulkanLoaderHeader );

	fwrite( instFunctionSrc, sizeof( char ), strlen( instFunctionSrc ), vulkanLoaderSource );

	static const char* srcStart =
		"// Auto-generated by VulkanHeaderParser, do not modify\n\n"

		"#include \"Vulkan.h\"\n\n";

	fwrite( srcStart, sizeof( char ), strlen( srcStart ), vulkanSource );

	for ( const std::string& name : physicalDeviceFunctionNames ) {
		if ( loadedDefs.find( name ) != loadedDefs.end() ) {
			continue;
		}

		loadedDefs.insert( name );

		const std::string nameDeclaration = "extern PFN_" + name + " " + name + ";\n\n";

		fwrite( nameDeclaration.c_str(), sizeof( char ), nameDeclaration.size(), vulkanLoaderHeader );

		const std::string nameDef = "PFN_" + name + " " + name + ";\n\n";

		fwrite( nameDef.c_str(), sizeof( char ), nameDef.size(), vulkanSource );

		const std::string nameLoad =
			"\t" + name +
			" = ( " + "PFN_" + name + " ) vkGetInstanceProcAddr( instance, \"" + name + "\" );\n\n";

		fwrite( nameLoad.c_str(), sizeof( char ), nameLoad.size(), vulkanLoaderSource );
	}

	fwrite( sourceFunctionEnd, sizeof( char ), strlen( sourceFunctionEnd ), vulkanLoaderSource );

	fwrite( sourceDeviceLoadFunction, sizeof( char ), strlen( sourceDeviceLoadFunction ), vulkanLoaderSource );

	for ( const std::string& name : functionNames ) {
		if ( loadedDefs.find( name ) != loadedDefs.end() ) {
			continue;
		}

		loadedDefs.insert( name );

		const std::string nameDeclaration = "extern PFN_" + name + " " + name + ";\n\n";

		fwrite( nameDeclaration.c_str(), sizeof( char ), nameDeclaration.size(), vulkanLoaderHeader );

		const std::string nameDef = "PFN_" + name + " " + name + ";\n\n";

		fwrite( nameDef.c_str(), sizeof( char ), nameDef.size(), vulkanSource );

		const std::string nameLoad =
			"\t" + name +
			" = ( " + "PFN_" + name + " ) vkGetDeviceProcAddr( device, \"" + name + "\" );\n\n";

		fwrite( nameLoad.c_str(), sizeof( char ), nameLoad.size(), vulkanLoaderSource );
	}

	for ( const std::string& name : functionDeclarations ) {
		// fwrite( name.c_str(), sizeof( char ), name.size(), vulkanLoaderHeader );
	}

	fwrite( headerEnd, sizeof( char ), strlen( headerEnd ), vulkanLoaderHeader );

	fwrite( sourceEnd, sizeof( char ), strlen( sourceEnd ), vulkanLoaderSource );
}

int main( int argc, char** argv ) {
	std::string headerPath = DAEMON_VULKAN_HEADER_PATH;
	headerPath += "vulkan_core.h";

	std::string loaderHeaderPath = DAEMON_VULKAN_LOADER_PATH;
	loaderHeaderPath += "../GraphicsCore/Vulkan.h";

	std::string loaderProcessedHeaderPath = DAEMON_VULKAN_LOADER_PATH;
	loaderProcessedHeaderPath += "../VulkanLoader/vulkan_core.h";

	std::string loaderSourcePath = DAEMON_VULKAN_LOADER_PATH;
	loaderSourcePath += "VulkanLoadFunctions.cpp";

	std::string sourcePath = DAEMON_VULKAN_LOADER_PATH;
	sourcePath += "Vulkan.cpp";

	// std::string loaderFunctionLoadHeaderPath = DAEMON_VULKAN_LOADER_PATH;
	// loaderFunctionLoadHeaderPath += "VulkanLoadFunctions.h";
	
	#ifdef _MSC_VER
		std::replace( headerPath.begin(), headerPath.end(), '/', '\\' );
		std::replace( loaderHeaderPath.begin(), loaderHeaderPath.end(), '/', '\\' );
		std::replace( loaderSourcePath.begin(), loaderSourcePath.end(), '/', '\\' );
		std::replace( sourcePath.begin(), sourcePath.end(), '/', '\\' );
		std::replace( loaderProcessedHeaderPath.begin(), loaderProcessedHeaderPath.end(), '/', '\\' );
		// std::replace( loaderFunctionLoadHeaderPath.begin(), loaderFunctionLoadHeaderPath.end(), '/', '\\' );
	#endif

	FILE* vulkanHeader = fopen( headerPath.c_str(), "r" );

	if( !vulkanHeader ) {
		char* err = strerror( errno );
		return 1;
	}

	FILE* vulkanLoaderHeader = fopen( loaderHeaderPath.c_str(), "w");

	if( !vulkanLoaderHeader ) {
		return 2;
	}

	FILE* vulkanLoaderProcessedHeader = fopen( loaderProcessedHeaderPath.c_str(), "w" );

	if( !vulkanLoaderProcessedHeader ) {
		return 2;
	}

	FILE* vulkanLoaderSource = fopen( loaderSourcePath.c_str(), "w" );

	if( !vulkanLoaderSource ) {
		return 2;
	}

	FILE* vulkanSource = fopen( sourcePath.c_str(), "w" );

	if ( !vulkanSource ) {
		return 2;
	}

	/* FILE* loaderFunctionLoadHeader = fopen( loaderFunctionLoadHeaderPath.c_str(), "w" );

	if ( !loaderFunctionLoadHeader ) {
		return 2;
	} */

	fseek( vulkanHeader, 0, SEEK_END );
	int size = ftell( vulkanHeader ) * 2;
	fseek( vulkanHeader, 0, 0 );
	fseek( vulkanLoaderHeader, 0, 0 );
	fseek( vulkanLoaderSource, 0, 0 );

	std::string vulkanHdr;

	vulkanHdr.resize( size );
	fread( vulkanHdr.data(), sizeof( char ), size, vulkanHeader );

	fclose( vulkanHeader );

	printf( vulkanHdr.c_str() );

	fwrite( ParseHeader( vulkanHdr ).c_str(), sizeof( char ), size, vulkanLoaderProcessedHeader );

	WriteFunctionDefs( vulkanLoaderHeader, vulkanLoaderSource, vulkanSource );

	fclose( vulkanLoaderHeader );
	fclose( vulkanLoaderProcessedHeader );
	fclose( vulkanLoaderSource );
	fclose( vulkanSource );
	// fclose( loaderFunctionLoadHeader );
}
