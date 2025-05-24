# FTEQW

***That magical Quake/QuakeWorld engine***

## Features

- Single & multi-player support.
- Runs Quake, Quake 2, Quake 3, Hexen 2, and many more FTE-only mods and games
- Hybrid NetQuake/QuakeWorld capabilities
- Vast amount of map, model & image formats are supported
- Advanced console, with descriptions & autocompletion
- Plugin support, enabling use of FFMPEG, Bullet/ODE physics & more
- Extensive suite of QuakeC/entity debugging features
- Deep integration with FTEQCC, which can even be executed in-game
- Support for split-screen local multiplayer
- Voice-chat via Opus & Speex
- Support for hundreds of players on a single server
- Works on Windows, Linux, OpenBSD, Haiku... & more!
- New features are added all the time in cooperation with modders!

# Documentation

FTEQW is a very large engine with a huge swath of features, and much of it is
still poorly documented. Here are some of the existing docs:

- [Console](./specs/console.txt)
- [CSQC Guide For Idiots](./specs/csqc_for_idiots.txt)
- [FTE Manifest Files](./specs/fte_manifests.txt)
- [Hosting](./specs/hosting.txt)
- [Multiprogs](./specs/multiprogs.txt)
- [Particles](./specs/particles.txt)
- [Realtime Lights](./specs/rtlights.txt)
- [Skeletal Animation](./specs/skeletal.txt)
- [Splitscreen](./specs/splitscreen.txt)

# Building

FTEQW has two coexisting build systems, one using a makefile and the other
using CMake. Fully portable builds should use the makefile.

## makefile

It is always recommended to build the `makelibs` target before building any
engine components. FTEQW's makefile is designed for GNU Make, and will not work
with BSD Make.

Note that the `-rel` suffix can be replaced with `-dbg` for debug builds.

```sh
cd engine

# build all static dependencies
make makelibs

# build server binary
make sv-rel

# build fteqcc
make qcc-rel

# build fteqccgui (windows only)
make qccgui-rel

# build fteqccgui with syntax highlighting (windows only)
make qccgui-scintilla

# build engine binary
# variations:
#  gl-rel : Use OpenGL renderer only
#  vk-rel : Use Vulkan renderer only
#  d3d-rel : Use DirectX renderer only
#  m-rel : Use all available renderers
make m-rel

# build plugins
make plugins-rel
```

Various platform targets can be built by setting the `FTE_TARGET` variable,
such as:

```sh
# build static dependencies for win32 target, usually for cross compiling
make makelibs FTE_TARGET=win32

# build static dependencies for win64 target, usually for cross compiling
make makelibs FTE_TARGET=win64

# build engine binary for native target with SDL2
make m-rel FTE_TARGET=SDL2

# build engine binary for win64 target with SDL2, usually for cross compiling
make m-rel FTE_TARGET=win64_SDL2
```

**IMPORTANT**: for each possible value of `FTE_TARGET` you wish to use, you
must build the `makelibs` target again with `FTE_TARGET` properly set, as the
build system will not do so automatically.

Also note that cross-compiling usually requires you to have the appropirate
cross-compiling system packages installed, like `mingw-w64` on Debian and
`mingw-w64-gcc` on Arch Linux.

Support for web-based [Emscripten](https://emscripten.org/) builds is also
available using the `web-rel` and `web-dbg` targets.

## CMake

The CMake build system is moreso intended for package maintainers and people
who are alright with the engine depending on system packages being available.
Use the makefile if you want fully portable "runs anywhere" builds.

```sh
# configure
cmake -Bbuild -S. -DCMAKE_BUILD_TYPE=Release

# build server binary
cmake --build build --target fteqw-sv

# build fteqcc
cmake --build build --target fteqcc

# build engine binary
cmake --build build --target fteqw

# build plugins
cmake --build build --target fteqw
```
