# Converts a text file into a C-language char array definition.
# For use in CMake script mode (cmake -P).
# Required definitions on command line:
# INPUT_FILE, OUTPUT_FILE, FILE_FORMAT, VARIABLE_NAME

# Inspired by:
# https://stackoverflow.com/questions/11813271/embed-resources-eg-shader-code-images-into-executable-library-with-cmake/27206982#27206982

file(READ ${INPUT_FILE} contents HEX)

if ("${FILE_FORMAT}" STREQUAL "TEXT")
	# Strip \r for consistency.
	string(REGEX REPLACE "(0d)?(..)" "0x\\2," contents "${contents}") 
elseif("${FILE_FORMAT}" STREQUAL "BINARY")
	string(REGEX REPLACE "(..)" "0x\\1," contents "${contents}")
else()
	message(FATAL_ERROR "Unknown file format: ${FILE_FORMAT}")
endif()

# Add null terminator.
string(REGEX REPLACE ",$" ",0x00," contents "${contents}") 

# Split long lines.
string(REGEX REPLACE
	"(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n"
	contents "${contents}"
)

string(REGEX REPLACE ",$" ",\n" contents "${contents}")

file(WRITE ${OUTPUT_FILE}
	"constexpr unsigned char ${VARIABLE_NAME}[] =\n"
	"{\n"
	"${contents}"
	"};\n"
)
