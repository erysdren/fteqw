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

# vars

if(NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
	set(FTE_SYSTEM_PROCESSOR "unknown")
else()
	string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} FTE_SYSTEM_PROCESSOR)
endif()
if(WIN32 OR MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(FTE_EXECUTABLE_SUFFIX "_${FTE_SYSTEM_PROCESSOR}.exe")
else()
	set(FTE_EXECUTABLE_SUFFIX ".${FTE_SYSTEM_PROCESSOR}")
endif()

set(FTE_ROOT_DIR ${PROJECT_SOURCE_DIR})
set(FTE_ENGINE_ROOT_DIR ${FTE_ROOT_DIR}/engine)
set(FTE_ENGINE_COMMON_DIR ${FTE_ENGINE_ROOT_DIR}/common)
set(FTE_ENGINE_QCLIB_DIR ${FTE_ENGINE_ROOT_DIR}/qclib)
set(FTE_ENGINE_CLIENT_DIR ${FTE_ENGINE_ROOT_DIR}/client)
set(FTE_ENGINE_SERVER_DIR ${FTE_ENGINE_ROOT_DIR}/server)
set(FTE_ENGINE_GL_DIR ${FTE_ENGINE_ROOT_DIR}/gl)
set(FTE_ENGINE_VK_DIR ${FTE_ENGINE_ROOT_DIR}/vk)
set(FTE_PLUGINS_ROOT_DIR ${FTE_ROOT_DIR}/plugins)
set(FTE_TOOLS_ROOT_DIR ${FTE_ROOT_DIR}/tools)

# includes

include(fte_tools)
