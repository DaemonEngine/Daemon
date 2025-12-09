# Dæmon

Dæmon is the standalone engine that powers the multiplayer first person shooter [Unvanquished](https://unvanquished.net).

[![GitHub tag](https://img.shields.io/github/tag/DaemonEngine/Daemon.svg)](https://github.com/DaemonEngine/Daemon/tags)

[![IRC](https://img.shields.io/badge/irc-%23unvanquished--dev-9cf.svg)](https://web.libera.chat/#unvanquished-dev)

| Windows | macOS | Linux |
|---------|-----|-------|
| [![AppVeyor branch](https://img.shields.io/appveyor/ci/DolceTriade/daemon/master.svg)](https://ci.appveyor.com/project/DolceTriade/daemon/history) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/51482765-8c0b-4b28-a82c-09554ed6887e/1/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Daemon/_build?definitionId=1) | [![Azure branch](https://img.shields.io/azure-devops/build/UnvanquishedDevelopment/51482765-8c0b-4b28-a82c-09554ed6887e/1/master.svg)](https://dev.azure.com/UnvanquishedDevelopment/Daemon/_build?definitionId=1) |

ℹ️ We provide ready-to-use downloads for the Unvanquished game on the Unvanquished [download page](https://unvanquished.net/download/), builds of the Dæmon engine are included.

ℹ️ The repository of the source code for the game logic of Unvanquished can be found [there](https://github.com/Unvanquished/Unvanquished).

## Workspace requirements

To fetch and build Dæmon, you'll need:
`git`,
`cmake`,
and a C++14 compiler.

The following are actively supported:
`gcc` ≥ 9,
`clang` ≥ 11,
Visual Studio/MSVC (at least Visual Studio 2019).

## Dependencies

Required:
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
`libopus`,
`libopusfile`.

Optional:
`ncurses`.

### MSYS2

MSYS2 is the recommended way to build using MinGW on a Windows host.

Required packages for 64-bit: `mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-cmake`, `make`  
Required packages for 32-bit: `mingw-w64-i686-gcc`, `mingw-w64-i686-cmake`, `make`

## Downloading the sources for the game engine

Daemon requires several sub-repositories to be fetched before compilation. If you have not yet cloned this repository:

```sh
git clone --recurse-submodules https://github.com/DaemonEngine/Daemon.git
```

If you have already cloned:

```sh
cd Daemon/
git submodule update --init --recursive
```

ℹ️ If cmake complains about missing files in `libs/crunch/` folder or similar issue then you have skipped this step.

## Build Instructions

💡️ Instead of `-j4` you can use `-jN` where `N` is your number of CPU cores to distribute compilation on them. Linux systems usually provide a handy `nproc` tool that tells the number of CPU core so you can just do `-j$(nproc)` to use all available cores.

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

💡️ For a 32-bit build use the `cross-toolchain-mingw32.cmake` toolchain file instead.

```sh
cmake -H. -Bbuild -DCMAKE_TOOLCHAIN_FILE=cmake/cross-toolchain-mingw64.cmake
cmake --build build -- -j4
```

## Running a game

ℹ️ On Windows you'll have to use `daemon.exe` and `daemonded.exe` instead of `./daemon` and `./daemonded`, everything else will be the same.

To run a game you would need a `pkg/` folder full of `.dpk` files provided by the Dæmon-based game you want to run. This `pkg/` folder has to be stored next to the `daemon` binary.

You then run the game this way:

```
./daemon
```

If you want to run a dedicated server, you may want to use the non-graphical `daemonded` server binary and start a map this way:

```
./daemonded +map <mapname>
```

Map names and other options may be [specific to the game](https://github.com/Unvanquished/Unvanquished#configuring-the-server).
