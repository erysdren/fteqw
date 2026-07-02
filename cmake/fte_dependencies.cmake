
include(FetchContent)

find_package(Math)

if(FTE_VENDOR_DEPENDENCIES)
	set(ZLIB_USE_STATIC_LIBS ON)
	FetchContent_Declare(ZLIB
		URL "https://zlib.net/zlib-1.3.2.tar.gz"
		URL_HASH MD5=a1e6c958597af3c67d162995a342138a
		FIND_PACKAGE_ARGS
	)
	FetchContent_MakeAvailable(ZLIB)
else()
	find_package(ZLIB REQUIRED)
endif()

if(FTE_ENGINE_BOTH OR FTE_ENGINE_CLIENT)
	if(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "1")
		if(FTE_VENDOR_DEPENDENCIES)
			message(FATAL_ERROR "Vendoring SDL 1.2 is currently unsupported")
		else()
			find_package(SDL REQUIRED)
		endif()
		set(FTE_ENGINE_USE_SDL TRUE)
	elseif(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "2")
		if(FTE_VENDOR_DEPENDENCIES)
			FetchContent_Declare(SDL2
				GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
				GIT_TAG "release-2.32.10"
				FIND_PACKAGE_ARGS
			)
			FetchContent_MakeAvailable(SDL2)
		else()
			find_package(SDL2 REQUIRED)
		endif()
		set(FTE_ENGINE_USE_SDL TRUE)
	elseif(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "3")
		if(FTE_VENDOR_DEPENDENCIES)
			FetchContent_Declare(SDL3
				GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
				GIT_TAG "release-3.4.12"
				FIND_PACKAGE_ARGS
			)
			FetchContent_MakeAvailable(SDL3)
		else()
			find_package(SDL3 REQUIRED)
		endif()
		set(FTE_ENGINE_USE_SDL TRUE)
	else()
		set(FTE_ENGINE_USE_SDL FALSE)
	endif()
	if(FTE_ENGINE_RENDERER STREQUAL "gl")
		find_package(OpenGL REQUIRED)
	elseif(FTE_ENGINE_RENDERER STREQUAL "vk")
		find_package(Vulkan REQUIRED)
	endif()
	if(FTE_VENDOR_DEPENDENCIES)
		FetchContent_Declare(Freetype
			URL "https://download.savannah.gnu.org/releases/freetype/freetype-2.14.3.tar.gz"
			URL_HASH MD5=c8333525a49e3caf08f427f1a4b01f35
		)
		FetchContent_MakeAvailable(Freetype)
		list(APPEND FTE_COMMON_DEFINITIONS AVAIL_FREETYPE)
	else()
		find_package(Freetype)
		if(Freetype_FOUND)
			add_library(freetype ALIAS Freetype::Freetype)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_FREETYPE)
		else()
			message(WARNING "Freetype not found, TTF fonts will not render")
		endif()
	endif()
	if(FTE_VENDOR_DEPENDENCIES)
		FetchContent_Declare(Ogg
			URL "https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-1.3.5.tar.gz"
			URL_HASH MD5=3267127fe8d7ba77d3e00cb9d7ad578d
			FIND_PACKAGE_ARGS
		)
		FetchContent_MakeAvailable(Ogg)
		FetchContent_Declare(Vorbis
			URL "https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-1.3.7.tar.gz"
			URL_HASH MD5=9b8034da6edc1a17d18b9bc4542015c7
			FIND_PACKAGE_ARGS
		)
		FetchContent_MakeAvailable(Vorbis)
		list(APPEND FTE_COMMON_DEFINITIONS AVAIL_OGGVORBIS)
	else()
		find_package(Ogg)
		find_package(Vorbis)
		if(Ogg_FOUND AND Vorbis_FOUND)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_OGGVORBIS)
		else()
			message(WARNING "Ogg/Vorbis not found")
		endif()
	endif()
endif()
