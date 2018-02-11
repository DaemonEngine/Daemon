# Dæmon

The standalone engine that powers the multiplayer first person shooter [Unvanquished](https://github.com/Unvanquished/Unvanquished).

## Dependencies

`zlib`, `libgmp`, `libnettle`, `libcurl`, `SDL2`, `GLEW`, `libpng`, `libjpeg` ≥ 8, `libwebp` ≥ 0.2.0, `Freetype`, `OpenAL`, `libogg`, `libvorbis`, `libtheora`, `libopus`, `libopusfile`

### Buildtime

`cmake`

### Optional 

`ncurses`, `libGeoIP`

## Build Instructions
### Visual Studio

  1. Run CMake.
  2. Choose your compiler.
  3. Open `Daemon.sln` and compile.

### Linux, Mac OS X, MSYS

  1. `mkdir build && cd build`
  2. `cmake ..`
  3. `make`¹

### Linux cross-compile to Windows

  1. `mkdir build && cd build`
  2. `cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/cross-toolchain-mingw32.cmake ..`²
  3. `make`¹

¹ *Use `make -j$(nproc)` to speed up compilation by using all CPU cores (`make -jN` for `N` threads).*  
² *Use `cross-toolchain-mingw64.cmake` for a Win64 build.*
