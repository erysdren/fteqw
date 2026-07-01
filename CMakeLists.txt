cmake_minimum_required(VERSION 3.10)
project(fteqw-ng
	DESCRIPTION "Advanced and portable idTech engine"
	HOMEPAGE_URL "https://github.com/erysdren/fteqw"
	LANGUAGES C CXX
)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# global

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# options

include(CMakeDependentOption)

option(FTE_TOOLS "Build tools" ON)
cmake_dependent_option(FTE_TOOL_QTV "Build fteqtv" OFF FTE_TOOLS OFF) # NOTE: seems unfinished
cmake_dependent_option(FTE_TOOL_IMG "Build imgtool" OFF FTE_TOOLS OFF) # NOTE: currently broken
cmake_dependent_option(FTE_TOOL_IQM "Build iqmtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_ASM "Build q3asm2" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_QCC "Build fteqcc" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_MASTER "Build ftemaster" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_HTTP "Build httpserver" ON FTE_TOOLS OFF)

option(FTE_PLUGINS "Build plugins" ON)
cmake_dependent_option(FTE_PLUGIN_COD "Build CoD formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_BULLET "Build Bullet physics plugin" ON "FTE_PLUGINS;BULLET_FOUND" OFF)
cmake_dependent_option(FTE_PLUGIN_HL2 "Build HL2 formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_JOLT "Build Jolt physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_NARBDROP "Build NarbDrop formats plugin" ON FTE_PLUGINS OFF)

option(FTE_ENGINE "Build engine" ON)
cmake_dependent_option(FTE_ENGINE_BOTH "Build server+client engine" ON FTE_ENGINE OFF)
cmake_dependent_option(FTE_ENGINE_SERVER "Build server-only engine" ON FTE_ENGINE OFF)
cmake_dependent_option(FTE_ENGINE_CLIENT "Build client-only engine" OFF FTE_ENGINE OFF) # NOTE: currently broken

set(FTE_ENGINE_SDL_VERSION_MAJOR "3" CACHE STRING "Major SDL version to use for engine")
set_property(CACHE FTE_ENGINE_SDL_VERSION_MAJOR PROPERTY STRINGS 1 2 3)

# compile options

set(FTE_COMMON_OPTIONS "")
set(FTE_COMMON_DEFINITIONS "")

# external dependencies

include(fte_dependencies)

# vars

include(fte_revision)
include(fte_system)
include(fte_directories)
include(fte_config)

# components

include(fte_tools)
include(fte_plugins)
include(fte_engine)
