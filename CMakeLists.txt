cmake_minimum_required(VERSION 3.10)
project(fteqw-ng
	DESCRIPTION "Advanced and portable idTech engine"
	HOMEPAGE_URL "https://github.com/erysdren/fteqw"
	LANGUAGES C CXX
)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Math)
find_package(ZLIB REQUIRED)

include(CMakeDependentOption)

option(FTE_TOOLS "Build tools" ON)
cmake_dependent_option(FTE_TOOL_QTV "Build fteqtv" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_IMG "Build imgtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_IQM "Build iqmtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_ASM "Build q3asm2" ON FTE_TOOLS OFF)

set(FTE_ENGINE_ROOT_DIR ${PROJECT_SOURCE_DIR}/engine)
set(FTE_ENGINE_COMMON_DIR ${FTE_ENGINE_ROOT_DIR}/common)
set(FTE_ENGINE_QCLIB_DIR ${FTE_ENGINE_ROOT_DIR}/qclib)
set(FTE_ENGINE_CLIENT_DIR ${FTE_ENGINE_ROOT_DIR}/client)
set(FTE_ENGINE_SERVER_DIR ${FTE_ENGINE_ROOT_DIR}/server)
set(FTE_ENGINE_GL_DIR ${FTE_ENGINE_ROOT_DIR}/gl)
set(FTE_ENGINE_VK_DIR ${FTE_ENGINE_ROOT_DIR}/vk)

add_subdirectory(tools)
