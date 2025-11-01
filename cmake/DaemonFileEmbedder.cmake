# Converts a text file into a C-language char array definition.
# For use in CMake script mode (cmake -P).
# Required definitions on command line:
# INPUT_FILE, OUTPUT_FILE, FILE_FORMAT, VARIABLE_NAME

# Inspired by:
# https://stackoverflow.com/questions/11813271/embed-resources-eg-shader-code-images-into-executable-library-with-cmake/27206982#27206982

file(READ ${INPUT_FILE} contents HEX)

# Translate the file content.
if ("${FILE_FORMAT}" STREQUAL "TEXT")
	# Strip \r for consistency.
	string(REGEX REPLACE "(0d)?(..)" "0x\\2," contents "${contents}") 
elseif("${FILE_FORMAT}" STREQUAL "BINARY")
	string(REGEX REPLACE "(..)" "0x\\1," contents "${contents}")
else()
	message(FATAL_ERROR "Unknown file format: ${FILE_FORMAT}")
endif()

# Add null terminator.
set(contents "${contents}0x00,") 

# Split long lines.
string(REGEX REPLACE
	"(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n"
	contents "${contents}"
)

# A bit more of beautification.
string(REGEX REPLACE ",$" ",\n" contents "${contents}")
set(DATA_VARIABLE_NAME "data_${VARIABLE_NAME}")

file(WRITE ${OUTPUT_FILE}
	"constexpr unsigned char ${DATA_VARIABLE_NAME}[] =\n"
	"{\n"
	"${contents}"
	"};\n"
	"const embeddedFileMapEntry_t ${VARIABLE_NAME} =\n"
	"{\n"
	"reinterpret_cast<const char*>( ${DATA_VARIABLE_NAME} ),\n"
	"sizeof( ${DATA_VARIABLE_NAME} ) - 1,\n"
	"};\n"
)
