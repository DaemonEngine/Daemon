# Converts a text file into a C-language char array definition.
# For use in CMake script mode (cmake -P).
# Required definitions on command line: INPUT_FILE, OUTPUT_FILE, VARIABLE_NAME, TEXT_MODE

# Inspired by https://stackoverflow.com/questions/11813271/embed-resources-eg-shader-code-images-into-executable-library-with-cmake/27206982#27206982
file(READ ${INPUT_FILE} contents HEX)
if (TEXT_MODE)
    string(REGEX REPLACE "(0d)?(..)" "0x\\2," contents ${contents}) # Strip \r for consistency
else()
    string(REGEX REPLACE "(..)" "0x\\1," contents ${contents})
endif()
file(WRITE ${OUTPUT_FILE} "const unsigned char ${VARIABLE_NAME}[] = {${contents}};\n")
