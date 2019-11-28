# Dæmon

[![GitHub tag](https://img.shields.io/github/tag/DaemonEngine/Daemon.svg)](https://github.com/DaemonEngine/Daemon/tags)

[![IRC](https://img.shields.io/badge/irc-%23daemon--engine%2C%23unvanquished--dev-4cc51c.svg)](https://webchat.freenode.net/?channels=%23daemon-engine%2C%23unvanquished-dev)

| Windows | OSX | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/daemon/master.svg)](https://ci.appveyor.com/project/DolceTriade/daemon/history) | [![Travis branch](https://img.shields.io/travis/DaemonEngine/Daemon/master.svg)](https://travis-ci.org/DaemonEngine/Daemon/branches) | [![Travis branch](https://img.shields.io/travis/DaemonEngine/Daemon/master.svg)](https://travis-ci.org/DaemonEngine/Daemon/branches) |

The standalone engine that powers the multiplayer first person shooter [Unvanquished](https://github.com/Unvanquished/Unvanquished).

## Workspace requirements

To fetch and build Dæmon, you'll need:
`git`,
`cmake`,
and a C++11 compiler.

The following are actively supported:
`gcc` ≥ 4.8,
`clang` ≥ 3.5,
Visual Studio/MSVC (at least Visual Studio 2017).

## Dependencies

### Required

`zlib`,
`libgmp`,
`libnettle`,
`libcurl`,
`SDL2`,
`GLEW`,
`libpng`,
`libjpeg` ≥ 8,
`libwebp` ≥ 0.2.0,
`Freetype`,
`OpenAL`,
`libogg`,
`libvorbis`,
`libtheora`,
`libopus`,
`libopusfile`

### Optional 

`ncurses`,
`libGeoIP`

### MSYS2

`base-devel`

64-bit: `mingw-w64-x86_64-{toolchain,cmake}`  
_or_ 32-bit: `mingw-w64-i686-{toolchain,cmake}`

MSYS2 is an easy way to get MingW compiler and build dependencies, the standalone MingW on Windows also works.

## Download instructions

Daemon requires several sub-repositories to be fetched before compilation. If you have not yet cloned this repository:

```sh
git clone --recurse-submodules https://github.com/DaemonEngine/Daemon.git
```

If you have already cloned:

```sh
cd Daemon/
git submodule update --init --recursive
```

If cmake complains about missing files in `recastnavigation/` folder or similar issue then you have skipped this step.

## Build Instructions

Instead of `-j4` you can use `-jN` where `N` is your number of CPU cores to distribute compilation on them. Linux systems usually provide an handy `nproc` tool that tells the number of CPU core so you can just do `-j$(nproc)` to use all available cores.

Enter the directory before anything else:

```sh
cd Daemon/
```

### Visual Studio

  1. Run CMake.
  2. Choose your compiler.
  3. Open `Daemon.sln` and compile.

### Linux, macOS, MSYS2

Produced files will be stored in a new directory named `build`.

```sh
cmake -H. -Bbuild
cmake --build build -- -j4
```

### Linux cross-compile to Windows

For a 32-bit build use the `cross-toolchain-mingw32.cmake` toolchain file instead.

```sh
cmake -H. -Bbuild -DCMAKE_TOOLCHAIN_FILE=cmake/cross-toolchain-mingw64.cmake
cmake --build build -- -j4
```