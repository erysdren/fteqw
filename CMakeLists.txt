cmake_minimum_required(VERSION 3.10)
project(fteqw-ng
	DESCRIPTION "Advanced and portable idTech engine"
	HOMEPAGE_URL "https://github.com/erysdren/fteqw"
	LANGUAGES C CXX
)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# external dependencies

find_package(Math)
find_package(ZLIB REQUIRED)

# options

include(CMakeDependentOption)

option(FTE_TOOLS "Build tools" ON)
cmake_dependent_option(FTE_TOOL_QTV "Build fteqtv" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_IMG "Build imgtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_IQM "Build iqmtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_ASM "Build q3asm2" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_QCC "Build fteqcc" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_MASTER "Build ftemaster" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_HTTP "Build httpserver" ON FTE_TOOLS OFF)

# vars

include(fte_revision)
include(fte_system)
include(fte_directories)

# components

include(fte_tools)
