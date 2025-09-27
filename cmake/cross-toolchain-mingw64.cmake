# Target operating system and architecture
set( CMAKE_SYSTEM_NAME Windows )
set( CMAKE_SYSTEM_PROCESSOR x86_64 )

# C/C++ compilers
set( CMAKE_C_COMPILER x86_64-w64-mingw32-gcc )
set( CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-14 )
set( CMAKE_RC_COMPILER x86_64-w64-mingw32-windres )

# Find programs using host paths and headers/libraries using target paths
# FIXME: not respected when cmake invokes pkg-config
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
