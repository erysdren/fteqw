cmake_minimum_required(VERSION 4.0)
project(FTEQuake LANGUAGES C)
list(PREPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

cmake_language(DEFER CALL set CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
cmake_language(DEFER CALL include CPack)

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(FetchContent)

include(FTEGitRevision)
include(FTEPlugin)

set(OpenSSL_USE_STATIC_LIBS YES)
set(OpenGL_GL_PREFERENCE LEGACY)

find_package(ccache 4.11)

find_package(Wayland COMPONENTS Client EGL)

find_package(Fontconfig)
find_package(VorbisFile)
find_package(Freetype)
find_package(OSSAudio)
find_package(OpenSSL)
find_package(Threads)
find_package(Vulkan)
find_package(GnuTLS)
find_package(OpenGL)
find_package(DBus1)
find_package(BZip2)
find_package(ALSA)
find_package(JPEG)
find_package(ZLIB)
find_package(PNG)
find_package(X11)

find_package(SDL2)

FetchContent_Declare(
  epoll-shim
  GIT_REPOSITORY https://github.com/jiixyj/epoll-shim.git
  GIT_TAG master)

cmake_dependent_option(FTE_ENGINE "Compile fteqw engine binary" ON
  [[NOT CMAKE_SYSTEM_NAME STREQUAL "ANDROID"]] OFF)
cmake_dependent_option(FTE_ENGINE_LIBRARY "Compile fteqw engine shared library" ON
  [[CMAKE_SYSTEM_NAME STREQUAL "Android"]] OFF)
cmake_dependent_option(FTE_USE_DBUS "Link against libdbus" ON "DBus1_FOUND" OFF)

option(FTE_ENGINE_SERVER "Compile fteqw-sv (server only) engine binary" ON)
option(FTE_ENGINE_CLIENT "Compile dteqw-cl (client only) engine binary" ON)

option(FTE_USE_SDL "Force the use of SDL instead of using native builds")

option(FTE_USE_VORBISFILE "Link against vorbisfile" ON)
option(FTE_USE_FREETYPE "Link against libfreetype" ON)
option(FTE_USE_GNUTLS "Link against gnutls" ON)
option(FTE_USE_BZIP2 "Link against libbzip2" ON)
option(FTE_USE_JPEG "Link against libjpeg" ON)
option(FTE_USE_ZLIB "Link against zlib" ON)
option(FTE_USE_PNG "Link against libpng" ON)

# Change to FTE_USE_DRACO?
option(FTE_DEP_DRACO "Link against libdraco (apache2)")

option(FTE_PLUG_NAMEMAKER "Compile namemaker plugin")
option(FTE_PLUG_TIMIDITY "Compile timidity audio plugin" ON)
option(FTE_PLUG_OPENSSL "Compile OpenSSL")
option(FTE_PLUG_OPENXR "Compile OpenXR plugin (for VR support)" ON)
option(FTE_PLUG_FFMPEG "Compile ffmpeg media plugin" ON)
option(FTE_PLUG_BULLET "Compile bullet rigid body physics" ON)
option(FTE_PLUG_MODELS "Compile models formats plugin" ON)
option(FTE_PLUG_QUAKE3 "Compile Quake 3 plugin" ON)
option(FTE_PLUG_EZHUD "Compile MoreQuakeWorld HUD Plugin" ON)
option(FTE_PLUG_X11SV "Compile X11 server plugin")
option(FTE_PLUG_XMPP "Compile xmpp/jabber instant-messenger plugin" ON)
option(FTE_PLUG_CEF "Compile libcef (webbrowser) plugin" ON)
option(FTE_PLUG_HL2 "Compile support for HL2 file formats" ON)
option(FTE_PLUG_COD "Compile Call of Duty" ON)
option(FTE_PLUG_IRC "Compile irc plugin" ON)
option(FTE_PLUG_MPQ "Compile mpq junk")
option(FTE_PLUG_QU "Compile Quake Injector plugin." ON)

option(FTE_MENU_SYS "Compile system menu" ON)
option(FTE_CSADDON "Compile CS Addon" ON)

option(FTE_TOOL_HTTPSV "Compile small http server" ON)
option(FTE_TOOL_MASTER "Compile master server" ON)
option(FTE_TOOL_IMAGE "Compile Image Tool" ON)
option(FTE_TOOL_QCCGUI "Compile gui qc compiler" ON)
option(FTE_TOOL_QCVM "Compile standalone qcvm")
option(FTE_TOOL_QCC "Compile CLI qc compiler" ON)
option(FTE_TOOL_QTV "Compile qtv server" ON)
option(FTE_TOOL_IQM "Compile IQM Tool" ON)

set(FTE_INSTALL_BINDIR games CACHE PATH "Binary dir to install to.")
set(FTE_INSTALL_LIBDIR fteqw CACHE PATH "Binary dir to install to.")
set(FTE_BUILD_CONFIG ${PROJECT_SOURCE_DIR}/engine/common/config_fteqw.h CACHE FILEPATH
  "Which build config file to use to control supported features.")

if (NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION $<CONFIG:Release,RelWithDebInfo>)
endif()

if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()

if (NOT DEFINED CMAKE_COMPILE_WARNINGS_AS_ERROR)
  set(CMAKE_COMPILE_WARNING_AS_ERROR $<CONFIG:Debug>)
endif()

if (TARGET ccache::ccache)
  set(CMAKE_CXX_COMPILER_LAUNCHER $<TARGET_PROPERTY:ccache::ccache,IMPORTED_LOCATION>)
  set(CMAKE_C_COMPILER_LAUNCHER $<TARGET_PROPERTY:ccache::ccache,IMPORTED_LOCATION>)
endif()

#plugins need visibility hidden in order to avoid conflicts with function names that match the engine.
#this is consistent with how windows works so no great loss.
#plus it means that gcc can inline more (with LTO), including optimising args.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_CXX_EXTENSIONS $<CONFIG:Debug>)
set(CMAKE_C_EXTENSIONS $<CONFIG:Debug>)

include_directories(
  engine/common
  engine/client
  engine/qclib
  engine/gl
  engine/server
  engine
)

add_compile_definitions(
  $<$<BOOL:${FTE_LITTLE_ENDIAN}>:FTE_LITTLE_ENDIAN>
  $<$<BOOL:${FTE_BIG_ENDIAN}>:FTE_BIG_ENDIAN>
)

# libepoll-shim needs to be installed on the BSDs and Mac OSX to get 
# some of the server code to compile and work correctly on those platforms - Brad
block (SCOPE_FOR VARIABLES PROPAGATE EPOLL_INC_DIR)
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_TESTS OFF)
  if (CMAKE_SYSTEM_NAME MATCHES "BSD|Darwin")
    FetchContent_MakeAvailable(epoll-shim)
  endif()
  set(EPOLL_INC_DIR "${epoll-shim_SOURCE_DIR}/include")
endblock()

#[[ End Options ]]

add_library(fteqw-system-common INTERFACE)
add_library(fteqw-server-common INTERFACE)
add_library(fteqw-qcc-common INTERFACE)
add_library(fteqw-qtv-common INTERFACE)
add_library(fteqw-common INTERFACE)

# TODO: Use $<AND> with value
target_compile_definitions(fteqw-common
  INTERFACE
    CONFIG_FILE_NAME=${FTE_BUILD_CONFIG}
    $<IF:$<TARGET_EXISTS:Freetype::Freetype>,LIBFREETYPE_STATIC,NO_FREETYPE>
    $<IF:$<TARGET_EXISTS:OpenGL::OpenGL>,GLQUAKE,NO_OPENGL>
    $<IF:$<TARGET_EXISTS:JPEG::JPEG>,LIBJPEG_STATIC,NO_JPEG>
    $<IF:$<TARGET_EXISTS:PNG::PNG>,LIBPNG_STATIC,NO_PNG>
    $<IF:$<TARGET_EXISTS:ZLIB::ZLIB>,ZLIB_STATIC,NO_ZLIB>
    $<$<TARGET_EXISTS:BZIP2::BZIP2>:BZLIB_STATIC>
    $<$<TARGET_EXISTS:BZIP2::BZIP2>:AVAIL_BZLIB>
)

target_link_libraries(fteqw-common
  INTERFACE
    $<TARGET_NAME_IF_EXISTS:Freetype::Freetype>
    $<TARGET_NAME_IF_EXISTS:OpenGL::OpenGL>
    $<TARGET_NAME_IF_EXISTS:BZIP2::BZIP2>
    $<TARGET_NAME_IF_EXISTS:JPEG::JPEG>
    $<TARGET_NAME_IF_EXISTS:ZLIB::ZLIB>
    $<TARGET_NAME_IF_EXISTS:PNG::PNG>
)

target_link_libraries(fteqw-server-common INTERFACE fteqw-common)
target_link_libraries(fteqw-qcc-common INTERFACE fteqw-common)
target_link_libraries(fteqw-qtv-common INTERFACE fteqw-common)

target_link_libraries(fteqw-system-common
  INTERFACE
    $<$<NOT:$<PLATFORM_ID:Windows>>:m>)

list(APPEND FTEQTV_LIBS fteqw-qtv-common)
list(APPEND FTEQCC_LIBS fteqw-qcc-common)
list(APPEND FTESV_LIBS fteqw-server-common)
list(APPEND FTE_LIBS fteqw-common)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND DBus1_FOUND)
  include_directories( ${DBus1_INCLUDE_DIRS} )
  list(APPEND FTE_LIB_DEFINES HAVE_DBUS)
  list(APPEND FTE_LIBS ${DBus1_LIBRARIES})
  message(WARNING "libdbus-1 library NOT available. Who cares?")
endif()

if(TARGET Freetype::Freetype)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  list(APPEND FTE_LIBS ${FREETYPE_LIBRARIES})

  find_package(Fontconfig)
  if(Fontconfig_FOUND)
    include_directories(${Fontconfig_INCLUDE_DIRS})
    list(APPEND FTE_LIB_DEFINES LIBFONTCONFIG_STATIC)
    list(APPEND FTE_LIBS ${Fontconfig_LIBRARIES})
  endif()
endif()

#this is just for headers.

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
if(VULKAN_INCLUDE_DIR)
  include_directories( ${VULKAN_INCLUDE_DIR} )
  list(APPEND FTE_DEFINES VKQUAKE) #no libs required, thankfully
else()
  message(WARNING "Vulkan headers NOT available.")
endif()

if(FTE_DEP_VORBISFILE)
  find_library(VORBISFILE_LIBRARY NAMES vorbisfile)
endif()
if(NOT VORBISFILE_LIBRARY)
  include_directories( ${VORBISFILE_INCLUDE_DIRS} )
  message(WARNING "libvorbisfile library NOT available. Who listens to the bgm anyway?")
  list(APPEND FTE_LIB_DEFINES NO_OGG)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wno-pointer-sign
    $<$<CONFIG:Debug>:-Wall>
    $<$<CONFIG:Debug>:-Wno-pointer-sign>
    $<$<CONFIG:Debug>:-Wno-unknown-pragmas>
    $<$<CONFIG:Debug>:-Wno-format-zero-length>
    $<$<CONFIG:Debug>:-Wno-strict-aliasing>
    $<$<CONFIG:Debug>:-Wno-error=cpp>)
endif()

# TODO: Make a genexpr
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
  add_compile_options(
    -Wstrict-prototypes           #
    -Wold-style-definition        # k&r c is weird and can't cope with 64bit types.
    -Wmissing-parameter-type      # k&r c is weird and can't cope with 64bit types.
    -Wold-style-declaration       #
    -Wpointer-arith               # void* stuff
    -Wvla                         # msvc doesn't support vla
    -Wdeclaration-after-statement # msvc doesn't allow defs after statements, and they're so very tempting...
    -Wformat-truncation=1         #
  #  -Wmissing-prototypes          # for finding missing statics.
  #  -Wno-unused-function          #
  #  -Wswitch-enum                 # to warn about omitted enums despite default.
  #  -march=native                 # might as well do this, public builds use the regular Makefile.
    $<$<AND:$<CONFIG:Debug>,$<NOT:$<PLATFORM_ID:Windows>>>:-fstack-protector-strong>
  )

  add_link_options($<$<BOOL:${FTE_USE_SDL}>:LINKER:--warn-common>)
endif()

add_compile_definitions(
  $<$<CONFIG:Debug>:_DEBUG>
  _FILE_OFFSET_BITS=64
  FTE_LIBRARY_PATH=${CMAKE_INSTALL_FULL_LIBDIR}/${FTE_INSTALL_LIBDIR}
  FTE_DATA_DIR=${CMAKE_INSTALL_FULL_DATAROOTDIR}
)

if(FTE_DEP_GNUTLS)
  if(NOT GNUTLS_FOUND)
    message(WARNING "gnutls library NOT available. HTTPS/DTLS will not be available.")
    list(APPEND FTE_LIB_DEFINES NO_GNUTLS)
  else()
    if(WIN32)
      set(GNUTLS_STATIC true CACHE BOOL "Link gnutls statically.") #usually as an .so though. :/
    else()
      set(GNUTLS_STATIC false CACHE BOOL "Link gnutls statically.") #usually as an .so though. :/
    endif()
    if(GNUTLS_STATIC)
      list(APPEND FTE_LIB_DEFINES GNUTLS_STATIC)
      list(APPEND FTE_LIBS ${GNUTLS_LIBRARY})
      list(APPEND FTESV_LIBS ${GNUTLS_LIBRARY})
    endif()
  endif()
endif()
if(WIN32)
  set(fte_dep_winsspi true cache bool "link against winsspi(schannel)")
  if(NOT FTE_DEP_WINSSPI)
    list(APPEND FTE_DEFINES NO_WINSSPI)
  endif()
endif()

if(ANDROID)
# include_directories(  ${FREETYPE_INCLUDE_DIRS} )

  list(APPEND FTE_DEFINES ANDROID VKQUAKE MULTITHREAD)
  list(APPEND FTE_LIBS android log EGL ${SYS_LIBS} ${CMAKE_DL_LIBS})
  set(fte.arch.srcs
    engine/client/sys_droid.c
    engine/common/sys_linux_threads.c
    engine/client/snd_droid.c
    engine/client/cd_null.c
    engine/gl/gl_viddroid.c
  )
elseif(WIN32 AND NOT FTE_USE_SDL)
  include_directories(engine/libs engine/libs/freetype2/include)
# link_directories(engine/libs/mingw64-libs)

  # engine/server/sv_sys_win.c

  list(APPEND FTE_LIBS ole32 gdi32 wsock32 winmm dxguid)
  list(APPEND FTE_DEFINES D3D9QUAKE D3D11QUAKE)
  file(GLOB direct3d.sources CONFIGURE_DEPENDS "engine/d3d/*.c")
  set(fte.arch.srcs 
    engine/client/winquake.rc
    engine/common/sys_win_threads.c
    engine/common/net_ssl_winsspi.c
    engine/common/net_ssl_gnutls.c
    engine/common/fs_win32.c
    engine/client/cd_win.c
    engine/client/in_win.c
    engine/client/snd_al.c
    engine/client/snd_directx.c
    engine/client/snd_wasapi.c
    engine/client/snd_win.c
    engine/client/snd_xaudio.c
    engine/client/sys_win.c

    engine/gl/gl_vidnt.c

    ${direct3d.sources}
  )

  list(APPEND FTESV_LIBS wsock32 winmm)
  set(FTESV_ARCH_FILES 
    engine/client/winquake.rc
    engine/common/sys_win_threads.c
    engine/common/net_ssl_winsspi.c
    engine/common/net_ssl_gnutls.c
    engine/common/fs_win32.c
    engine/server/sv_sys_win.c
  )
elseif(UNIX AND NOT FTE_USE_SDL)  #linux(ish)
        #openbsd will have issues with snd_linux.c

  #linux-only packages
  if(ALSA_FOUND)
    list(APPEND FTE_LIB_DEFINES AUDIO_ALSA AUDIO_PULSE)
  endif()

  if(X11_FOUND)
    if (NOT X11_Xcursor_FOUND)
      list(APPEND FTE_LIB_DEFINES NO_X11_CURSOR)
      message(WARNING "Xcursor library NOT available.")
    endif()
    if (NOT X11_Xrandr_FOUND)
      list(APPEND FTE_LIB_DEFINES NO_X11_RANDR)
      message(WARNING "Xrandr library NOT available.")
    endif()
    if (NOT X11_Xscreensaver_FOUND)
      list(APPEND FTE_LIB_DEFINES NO_X11_XSS)
      message(WARNING "Xss library NOT available.")
    endif()
  else()
    message(WARNING "x11 library NOT available.")
    list(APPEND FTE_LIB_DEFINES NO_X11)
  endif()

  list(APPEND FTE_DEFINES DYNAMIC_SDL MULTITHREAD)
  list(APPEND FTE_LIBS ${SYS_LIBS} ${CMAKE_DL_LIBS} Threads::Threads ${SDL2_LIBRARIES})
  set(fte.arch_files ${fte_arch.srcs}
    engine/client/sys_linux.c
    engine/common/sys_linux_threads.c
    engine/common/net_ssl_gnutls.c

    engine/client/snd_al.c
    engine/client/snd_alsa.c
    engine/client/snd_linux.c
    engine/client/snd_pulse.c
    engine/client/snd_sdl.c #we use SDL audio even without sys_sdl, because of pulseaudio fucking over alsa, alsa fucking over oss3, and oss4 not being used. Either way, openal should be the default anyway.

    engine/client/cd_linux.c
    engine/gl/gl_vidlinuxglx.c
    engine/gl/gl_videgl.c

#   engine/gl/gl_vidrpi.c
#   engine/gl/gl_vidwayland.c
  )

  #openbsd uses a libossaudio library for all the oss stuff, use that to ensure that we still get sound
  find_library(
    OSSAUDIO_LIBRARY
    NAMES ossaudio
  )
  if(OSSAUDIO_LIBRARY)
    list(APPEND FTE_LIBS ${OSSAUDIO_LIBRARY})
  endif()

  if(NOT TARGET X11::xkbcommon)
    message(WARNING "xkbcommon library not found, needed for wayland to be usable.")
    unset(WAYLAND_CLIENT_LIBRARY)
  endif()
  if(WAYLAND_CLIENT_LIBRARY AND WAYLAND_EGL)
#   list(APPEND FTE_LIBS ${WAYLAND_CLIENT_LIBRARY} ${HAVE_XKBCOMMON} ${WAYLAND_EGL})
#   list(APPEND FTE_DEFINES STATIC_WAYLAND)

    list(APPEND FTE_DEFINES WAYLANDQUAKE USE_EGL)
    set(fte.arch_files ${fte_arch.srcs}
      engine/gl/gl_vidwayland.c
    )
  else()
    message(WARNING "Wayland library NOT available. X11 will live forever anyway.")
    if(NOT X11_FOUND)
      message(WARNING "No renderers supported!")
      set(FTE_NO_RENDERERS 1)
    endif()
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "BSD|Darwin")
    find_library(epoll-shim REQUIRED)
    include_directories(${EPOLL_INC_DIR})
    list(APPEND FTESV_LIBS epoll-shim)
    list(APPEND FTESV_DEFINES HAVE_EPOLL)
  endif()

  list(APPEND FTESV_DEFINES MULTITHREAD)
  set(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
    engine/server/sv_sys_unix.c
    engine/common/sys_linux_threads.c
    engine/common/net_ssl_gnutls.c
  )
  list(APPEND FTESV_LIBS ${SYS_LIBS} ${CMAKE_DL_LIBS} pthread)

else() #SDL
# find_package(Freetype REQUIRED)
# include_directories(engine/libs engine/libs/freetype2/include)

  find_package(PkgConfig REQUIRED)
  pkg_search_module(sdl2 REQUIRED sdl2)

  include_directories(${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

  #SDL2.0.7 supports vulkan, so lets use it.
  list(APPEND FTE_DEFINES FTE_SDL;MULTITHREAD)
  list(APPEND FTE_LIBS ${SYS_LIBS} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES})
  set(fte.arch.srcs
    engine/client/sys_sdl.c
    engine/client/snd_al.c
    engine/client/snd_sdl.c
    engine/client/in_sdl.c
    engine/client/cd_sdl.c
    engine/gl/gl_vidsdl.c
  )

  list(APPEND FTESV_DEFINES MULTITHREAD)
  list(APPEND FTESV_LIBS ${SYS_LIBS} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES})

  if(WIN32)
    list(APPEND FTE_LIBS wsock32 gdi32 ole32)
    list(APPEND FTE_DEFINES NO_DIRECTX)
    set(fte.arch_files ${fte_arch.srcs}
      engine/client/winquake.rc
      engine/common/net_ssl_winsspi.c
    )
    set(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
      engine/client/winquake.rc
      engine/common/net_ssl_winsspi.c
      engine/server/sv_sys_win.c
    )
  else()
    set(fte.arch_files ${fte_arch.srcs}
      engine/common/net_ssl_gnutls.c
    )
    set(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
      engine/common/net_ssl_gnutls.c
      engine/common/sys_linux_threads.c
      engine/server/sv_sys_unix.c
    )
    list(APPEND FTESV_LIBS pthread)
  endif()
endif()

file(GLOB fte.vk.sources CONFIGURE_DEPENDS "engine/vk/*.c")

set(fte.gl.srcs
#These are GL-specific, but can be left even if no gl is supported.
  engine/gl/gl_backend.c
  engine/gl/gl_bloom.c
  engine/gl/gl_draw.c
  engine/gl/gl_rmain.c
  engine/gl/gl_rmisc.c
  engine/gl/gl_rsurf.c
  engine/gl/gl_screen.c
  engine/gl/gl_vidcommon.c
  engine/gl/glmod_doom.c
)

set(fte.qcvm.srcs
  engine/qclib/comprout.c
  engine/qclib/initlib.c
  engine/qclib/pr_edict.c
  engine/qclib/pr_exec.c
  engine/qclib/pr_multi.c
  engine/qclib/qcc_cmdlib.c
  engine/qclib/qcc_pr_comp.c
  engine/qclib/qcc_pr_lex.c
# engine/qclib/decomp.c
# engine/qclib/packager.c
# engine/qclib/pr_x86.c
# engine/qclib/qccgui.c
# engine/qclib/qccguistuff.c
# engine/qclib/qcctui.c
  engine/qclib/qccmain.c
  engine/qclib/qcd_main.c
  engine/qclib/qcdecomp.c
)
set(fte.common.srcs
  #these files are common to both server-only and client+server builds.
  engine/common/cmd.c
  engine/common/com_mesh.c
  engine/common/com_bih.c
  engine/common/common.c
  engine/common/json.c
  engine/common/crc.c
  engine/common/cvar.c
  engine/common/fs.c
  engine/common/fs_dzip.c
  engine/common/fs_pak.c
  engine/common/fs_stdio.c
  engine/common/fs_xz.c
  engine/common/fs_zip.c
  engine/common/gl_q2bsp.c
  engine/common/huff.c
  engine/common/log.c
  engine/common/mathlib.c
  engine/common/md4.c
  engine/common/md5.c
  engine/common/net_chan.c
  engine/common/net_ice.c
  engine/common/net_wins.c
  engine/common/plugin.c
  engine/common/pmove.c
  engine/common/pmovetst.c
  engine/common/pr_bgcmd.c
  engine/common/q1bsp.c
  engine/common/q2pmove.c
  engine/common/qvm.c
  engine/common/sha1.c
  engine/common/sha2.c
  engine/common/translate.c
  engine/common/zone.c

  #important headers
  engine/common/bothdefs.h
  engine/common/config_fteqw.h
  engine/common/config_minimal.h
  engine/common/config_nocompat.h
  engine/common/config_wastes.h
  engine/common/config_freecs.h
  engine/common/config_fteqw_noweb.h

  #useless headers that I'll never search for
  engine/client/api_menu.h
  engine/client/cdaudio.h
  engine/client/client.h
  engine/client/cl_ignore.h
  engine/client/cl_master.h
  engine/client/input.h
  engine/client/keys.h
  engine/client/menu.h
  engine/client/merged.h
  engine/client/modelgen.h
  engine/client/quakedef.h
  engine/client/render.h
  engine/client/sbar.h
  engine/client/screen.h
  engine/client/sound.h
  engine/client/spritegn.h
# engine/client/sys_plugfte.h
  engine/client/vid.h
  engine/client/view.h
  engine/client/wad.h
# engine/client/winquake.h
  engine/common/bothdefs.h
  engine/common/bspfile.h
  engine/common/cmd.h
  engine/common/com_mesh.h
  engine/common/common.h
  engine/common/console.h
  engine/common/cvar.h
  engine/common/fs.h
  engine/common/mathlib.h
  engine/common/net.h
  engine/common/netinc.h
  engine/common/particles.h
  engine/common/pmove.h
  engine/common/pr_common.h
  engine/common/protocol.h
  engine/common/sys.h
  engine/common/translate.h
  engine/common/ui_public.h
  engine/common/vm.h
  engine/common/world.h
  engine/common/zone.h
  engine/gl/gl_draw.h
  engine/gl/gl_model.h
  engine/gl/glquake.h
  engine/gl/glsupp.h
  engine/gl/gl_terrain.h
  engine/gl/gl_videgl.h
  engine/gl/model_hl.h
  engine/gl/shader.h
  engine/http/iweb.h
  engine/qclib/cmdlib.h
  engine/qclib/execloop.h
  engine/qclib/gui.h
  engine/qclib/hash.h
  engine/qclib/pr_comp.h
  engine/qclib/progsint.h
  engine/qclib/progslib.h
  engine/qclib/progtype.h
  engine/qclib/qcc.h
  engine/qclib/qcd.h
  engine/server/progdefs.h
  engine/server/progs.h
  engine/server/q2game.h
  engine/server/server.h
  #engine/server/svhl_gcapi.h
  engine/server/sv_sql.h
  #engine/sw/sw.h
  #engine/sw/sw_spans.h
  engine/vk/vkrenderer.h
  engine/web/ftejslib.h


  #sigh
  engine/client/pr_skelobj.c
  engine/client/m_download.c
  engine/client/net_master.c
  engine/client/r_d3.c

  #these are here because of hitmodel etc
  engine/gl/gl_heightmap.c
  engine/gl/gl_hlmdl.c
  engine/gl/gl_model.c

  engine/server/sv_move.c
  engine/server/sv_phys.c
  engine/server/world.c

  ${fte.qcvm.srcs}
  engine/qclib/hash.c

  engine/http/httpclient.c
)

set(fte.server.srcs
  engine/server/net_preparse.c
  engine/server/pr_cmds.c
  engine/server/pr_lua.c
  engine/server/pr_q1qvm.c
  engine/server/savegame.c
  engine/server/sv_ccmds.c
  engine/server/sv_chat.c
  engine/server/sv_cluster.c
  engine/server/sv_demo.c
  engine/server/sv_ents.c
  engine/server/sv_init.c
  engine/server/sv_main.c
  engine/server/sv_master.c
  engine/server/sv_mvd.c
  engine/server/sv_nchan.c
  engine/server/sv_rankin.c
  engine/server/sv_send.c
  engine/server/sv_sql.c
  engine/server/sv_user.c
# engine/server/svhl_game.c
# engine/server/svhl_phys.c
# engine/server/svhl_world.c
  engine/server/svq2_ents.c
  engine/server/svq2_game.c
)

#these files are only in the client
set(fte.client.srcs
  engine/client/cl_cam.c
  engine/client/cl_demo.c
  engine/client/cl_ents.c
  engine/client/cl_ignore.c
  engine/client/cl_input.c
  engine/client/cl_main.c
  engine/client/cl_parse.c
  engine/client/cl_pred.c
  engine/client/cl_screen.c
  engine/client/cl_tent.c
# engine/client/clhl_game.c
  engine/client/clq2_cin.c
  engine/client/clq2_ents.c
  engine/client/console.c
  engine/client/fragstats.c
  engine/client/image.c
  engine/client/in_generic.c
  engine/client/keys.c
  engine/client/m_items.c
  engine/client/m_master.c
  engine/client/m_mp3.c
  engine/client/m_multi.c
  engine/client/m_options.c
  engine/client/m_script.c
  engine/client/m_native.c
  engine/client/m_single.c
  engine/client/menu.c
  engine/client/p_classic.c
  engine/client/p_null.c
  engine/client/p_script.c
  engine/client/pr_clcmd.c
  engine/client/pr_csqc.c
  engine/client/pr_menu.c
  engine/client/r_2d.c
  engine/client/r_d3.c
  engine/client/r_part.c
  engine/client/r_partset.c
  engine/client/r_surf.c
  engine/client/renderer.c
  engine/client/renderque.c
  engine/client/roq_read.c
  engine/client/sbar.c
  engine/client/skin.c
  engine/client/snd_dma.c
  engine/client/snd_mem.c
  engine/client/snd_mix.c
  engine/client/snd_mp3.c
  engine/client/snd_ov.c
  engine/client/textedit.c
  engine/client/valid.c
  engine/client/view.c
  engine/client/wad.c
  engine/client/zqtp.c


#these are generic renderer files and no longer gl-specific (for the most part)
  engine/gl/gl_alias.c
  engine/gl/gl_font.c
  engine/gl/gl_ngraph.c
  engine/gl/gl_rlight.c
  engine/gl/gl_shader.c
  engine/gl/gl_shadow.c
  engine/gl/gl_warp.c
  engine/gl/ltface.c

#these are renderer-specific
  engine/client/vid_headless.c
  ${fte.gl.srcs}
  ${fte.vk.sources}
)

set(fte.q3.srcs
  plugins/quake3/botlib/be_aas_bspq3.c
  plugins/quake3/botlib/be_aas_entity.c
  plugins/quake3/botlib/be_aas_move.c
  plugins/quake3/botlib/be_aas_routealt.c
  plugins/quake3/botlib/be_ai_char.c
  plugins/quake3/botlib/be_ai_goal.c
  plugins/quake3/botlib/be_ai_weight.c
  plugins/quake3/botlib/l_crc.c
  plugins/quake3/botlib/l_memory.c
  plugins/quake3/botlib/l_struct.c
  plugins/quake3/botlib/be_aas_cluster.c
  plugins/quake3/botlib/be_aas_file.c
  plugins/quake3/botlib/be_aas_optimize.c
  plugins/quake3/botlib/be_aas_route.c
  plugins/quake3/botlib/be_ai_chat.c
  plugins/quake3/botlib/be_ai_move.c
  plugins/quake3/botlib/be_ea.c
  plugins/quake3/botlib/l_libvar.c
  plugins/quake3/botlib/l_precomp.c
  plugins/quake3/botlib/be_aas_debug.c
  plugins/quake3/botlib/be_aas_main.c
  plugins/quake3/botlib/be_aas_reach.c
  plugins/quake3/botlib/be_aas_sample.c
  plugins/quake3/botlib/be_ai_gen.c
  plugins/quake3/botlib/be_ai_weap.c
  plugins/quake3/botlib/be_interface.c
  plugins/quake3/botlib/l_log.c
  plugins/quake3/botlib/l_script.c
  plugins/quake3/botlib/standalone.c

  plugins/quake3/clq3_cg.c
  plugins/quake3/clq3_ui.c
  plugins/quake3/clq3_parse.c
  plugins/quake3/svq3_game.c
  plugins/quake3/q3common.c
  plugins/quake3/q3common.h

  plugins/quake3/clq3defs.h
  plugins/quake3/q3g_public.h
)

#For annoying compressed gltf2 files.
if(FTE_DEP_DRACO)
  find_library(
    DRACO_LIBRARY
    NAMES draco
  )
  if(DRACO_LIBRARY)
    set(DRACO_FILES plugins/models/draco.cpp)
    set(DRACO_CFLAGS HAVE_DRACO)

    list(APPEND fte.common_files ${draco.srcs})
    list(APPEND FTE_LIB_DEFINES ${DRACO_CFLAGS})
    list(APPEND FTE_LIBS ${DRACO_LIBRARY})
    list(APPEND FTESV_LIBS ${DRACO_LIBRARY})
  else()
    message(WARNING "draco library not found, needed for GLTF's KHR_draco_mesh_compression to be usable.")
  endif()
endif()


if(FTE_PLUG_QUAKE3)
    #define the modules and make sure they're linked (one generic, one for server-only builds.
    add_library(plug_quake3 MODULE ${fte.q3.srcs} plugins/plugin.c)
    target_compile_definitions(plug_quake3
       PRIVATE
        ${FTE_LIB_DEFINES}
        ${FTE_REVISON}
        BOTLIB
        BOTLIB_STATIC
        FTEPLUGIN)
    target_link_libraries(plug_quake3 ${SYS_LIBS})
    embed_plugin_meta(quake3 "Quake3 Compat" "Provides compatability with Quake3's gamecode.")
endif()

#still a wip, so disabled by default
if(FTE_PLUG_COD)
  add_library(plug_cod MODULE
    plugins/cod/codmod.c
    plugins/cod/codbsp.c
    plugins/cod/codmat.c
    plugins/cod/codiwi.c
    #plugins/cod/codff.c
    plugins/plugin.c)
  target_compile_definitions(plug_cod
    PRIVATE
      ${FTE_LIB_DEFINES}
      ${FTE_REVISON}
      FTEPLUGIN)
  target_link_libraries(plug_cod ${SYS_LIBS}
    #${ZLIB_LIBRARIES}
    )
  embed_plugin_meta(cod "CoD Formats" "Provides compatability with Call Of Duty's file formats.")
endif()

file(STRINGS "${FTE_BUILD_CONFIG}" BULLET_INTERNAL REGEX "^#define[\t ]+USE_INTERNAL_BULLET")
if(BULLET_INTERNAL)
  #Built-in bullet physics plugin...
  find_package(Bullet REQUIRED)
  list(APPEND fte.common.srcs plugins/bullet/bulletplug.cpp)
  include_directories( ${BULLET_INCLUDE_DIRS} )
  list(APPEND FTE_LIBS ${BULLET_LIBRARIES})
  list(APPEND FTESV_LIBS ${BULLET_LIBRARIES})
else()
  #Bullet Physics library plugin
  if(FTE_PLUG_BULLET)
    find_package(Bullet)
    if (BULLET_FOUND)
      add_library(plug_bullet MODULE
        plugins/plugin.c
        plugins/bullet/bulletplug.cpp
      )
      target_include_directories(plug_bullet PUBLIC ${BULLET_INCLUDE_DIRS})
      target_compile_definitions(plug_bullet
        PRIVATE
          FTEPLUGIN
          ${FTE_LIB_DEFINES})
      target_link_libraries(plug_bullet ${SYS_LIBS} ${BULLET_LIBRARIES})

      embed_plugin_meta(bullet "Bullet Physics Plugin" "Provides Rigid Body Physics.")
    else()
      message(WARNING "bullet library not detected, skipping plugin")
    endif()
  endif()
endif()

#ODE Physics library plugin
set(FTE_PLUG_ODE true CACHE BOOL "Compile ODE rigid body physics plugin.")
set_property(CACHE FTE_PLUG_ODE PROPERTY STRINGS false true static)
if(FTE_PLUG_ODE)
  find_path(LIBODE_INCLUDE_DIR ode/ode.h)
  if (LIBODE_INCLUDE_DIR)
    find_library(LIBODE_LIBRARY ode)
  endif()
  if (LIBODE_LIBRARY)
    if (FTE_PLUG_ODE STREQUAL "static")
      #set (fte.common_files ${fte_common.srcs} engine/common/com_phys_ode.c)
      set(FTE_LIB_DEFINES "${FTE_LIB_DEFINES};USE_INTERNAL_ODE;ODE_STATIC")
      list(APPEND FTE_LIBS ${LIBODE_LIBRARY})
      list(APPEND FTESV_LIBS ${LIBODE_LIBRARY})
      list(APPEND FTE_INCLUDES ${ODE_INCLUDE_DIRS})
    else()
      add_library(plug_ode MODULE
        plugins/plugin.c
        engine/common/com_phys_ode.c
        engine/common/mathlib.c
      )
      target_include_directories(plug_ode PUBLIC ${ODE_INCLUDE_DIRS})
      target_compile_definitions(plug_ode
        PRIVATE
          FTEPLUGIN
          ODE_STATIC)
      target_link_libraries(plug_ode ${SYS_LIBS} ${LIBODE_LIBRARY})

      embed_plugin_meta(ode "ODE Physics" "Provides Rigid Body Physics behaviours.")
    endif()
  else()
    message(WARNING "ODE library not found, no ode plugin for you")
  endif()
endif()

# android sucks. everything is a library. so we build the engine as a shared
# library and completely ignore dedicated servers+tools
if(FTE_ENGINE_FTEDROID)
  add_fte_plugin(ftedroied)
  target_sources(fte-plugin-ftedroid PRIVATE
    ${fte.arch.srcs}
    ${fte.common.srcs}
    ${fte.client.srcs}
  )
  target_compile_definitions(ftedroid
     PRIVATE ${FTE_LIB_DEFINES} ${FTE_DEFINES} ${FTE_REVISON})
  target_include_directories(ftedroid PUBLIC ${FTE_INCLUDES})
  target_link_libraries(ftedroid ${FTE_LIBS})
  list(APPEND INSTALLTARGS ftedroid)
endif()
if(FTE_ENGINE)
    add_executable(fteqw WIN32
      ${fte.arch.srcs}
      ${fte.common.srcs}
      ${fte.client.srcs}
      ${fte.server.srcs}
    )
    if(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
      find_library(epoll-shim REQUIRED)
      list(APPEND FTE_INCLUDES "${EPOLL_INC_DIR}")
      list(APPEND FTE_LIBS epoll-shim)
      list(APPEND FTE_DEFINES HAVE_EPOLL)
    endif()
    target_compile_definitions(fteqw
      PRIVATE
        ${FTE_LIB_DEFINES}
        ${FTE_DEFINES}
        ${FTE_REVISON})
    target_include_directories(fteqw PUBLIC ${FTE_INCLUDES})
    target_link_libraries(fteqw ${FTE_LIBS})
    list(APPEND INSTALLTARGS fteqw)

    add_custom_target(fteqw-i18n ALL
      DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot"
      COMMAND xgettext
        --output=${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot
        -k
        -kCon_TPrintf
        -kCon_SafeTPrintf
        -kNetchan_OutOfBandTPrintf:4
        -kSV_OutOfBandTPrintf:4 -klangtext
        -kSV_TPrintToClient:3
        -kSV_ClientTPrintf:3
        -kSV_BroadcastTPrintf:2
        -kCVARAFCD:6
        -kCVARAFD:5
        -kCVARFCD:5
        -kCVARAD:4
        -kCVARFD:4
        -kCVARCD:4
        -kCVARD:3
        -kCmd_AddCommandD:3
        -kCmd_AddCommandAD:4
        -kMenu_Prompt:4:5:6
        -kHost_EndGame
        $<TARGET_PROPERTY:fteqw-i18n,I18N_SOURCES>
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Generating i18n files"
      COMMAND_EXPAND_LISTS
      VERBATIM)
    set_property(TARGET fteqw-i18n APPEND
      PROPERTY
        I18N_SOURCES
          ${fte.arch.srcs}
          ${fte.common.srcs}
          ${fte.client.srcs}
          ${fte.server.srcs})
  endif()

  if(FTE_ENGINE_SERVER)
    add_executable(fteqw-sv
      ${FTESV_ARCH_FILES}
      ${fte.common.srcs}
      ${fte.server.srcs}
    )
    target_compile_definitions(fteqw-sv PRIVATE
      SERVERONLY
      ${FTE_LIB_DEFINES}
      ${FTESV_DEFINES}
      ${FTE_REVISON})
    target_include_directories(fteqw-sv PUBLIC ${FTE_INCLUDES})
    target_link_libraries(fteqw-sv ${FTESV_LIBS})
    list(APPEND INSTALLTARGS fteqw-sv)
  endif()

  if(FTE_ENGINE_CLIENT)
    add_executable(fteqw-cl
      ${fte.arch.srcs}
      ${fte.common.srcs}
      ${fte.client.srcs}
    )
    target_compile_definitions(fteqw-cl
      PRIVATE
        CLIENTONLY
        ${FTE_LIB_DEFINES}
        ${FTE_DEFINES}
        ${FTE_REVISON})
    target_include_directories(fteqw-cl PUBLIC ${FTE_INCLUDES})
    target_link_libraries(fteqw-cl ${FTE_LIBS})
    list(APPEND INSTALLTARGS fteqw-cl)
  endif()

  if((FTE_ENGINE OR FTE_ENGINE_CLIENT) AND SDL2_FOUND)
    set(FTE_SDL2 FTE_SDL)
    set(FTE_SDL2_INCLUDES ${SDL2_INCLUDE_DIRS})
  endif()

  if(FTE_TOOL_IQM)
    add_executable(iqmtool
      iqm/iqm.cpp
      plugins/models/gltf.c
      engine/common/json.c
      engine/client/image.c
      imgtool.c
      iqm/iqm.h
      ${DRACO_FILES}
    )
    target_compile_definitions(iqmtool
      PRIVATE
        IQMTOOL
        ${DRACO_CFLAGS}
        ${FTE_LIB_DEFINES}
        ${FTE_REVISON})
    target_link_libraries(iqmtool
      PRIVATE
        ${CMAKE_DL_LIBS}
        ${DRACO_LIBRARY}
        $<TARGET_NAME_IF_EXISTS:JPEG::JPEG>
        $<TARGET_NAME_IF_EXISTS:PNG::PNG>)
    list(APPEND INSTALLTARGS iqmtool)
  endif()

  if(FTE_TOOL_IMAGE)
    add_executable(imgtool
      engine/client/image.c
      imgtool.c
    )
    target_include_directories(imgtool PUBLIC ${FTE_SDL2_INCLUDES})
    target_compile_definitions(imgtool
       PRIVATE IMGTOOL ${FTE_LIB_DEFINES} ${FTE_DEFINES} ${FTE_REVISON} ${FTE_SDL2})
    target_link_libraries(imgtool ${FTE_LIBS})
    list(APPEND INSTALLTARGS imgtool)
  endif()

  if(FTE_TOOL_QTV)
    add_executable(qtv
      fteqtv/netchan.c
      fteqtv/parse.c
      fteqtv/msg.c
      fteqtv/qw.c
      fteqtv/source.c
      fteqtv/bsp.c
      fteqtv/rcon.c
      fteqtv/relay.c
      fteqtv/mdfour.c
      engine/common/md5.c
      fteqtv/crc.c
      fteqtv/control.c
      fteqtv/forward.c
      fteqtv/pmove.c
      fteqtv/menu.c
      fteqtv/httpsv.c
      fteqtv/libqtvc/glibc_sucks.c
      engine/common/sha1.c
    )
    target_compile_definitions(qtv
      PRIVATE
        ${FTE_REVISON}
        ${FTE_LIB_DEFINES})
    target_link_libraries(qtv
      $<$<PLATFORM_ID:Windows>:ws2_32>
      $<$<PLATFORM_ID:Windows>:winmm>
      ${SYS_LIBS}
      ${FTEQV_LIBS})
    if(CMAKE_SYSTEM_NAME MATCHES "BSD|Darwin")
      # Add Epoll-shim for the kqueue Unixes here - Brad
      find_library(epoll-shim REQUIRED)
      target_include_directories(qtv PUBLIC "${EPOLL_INC_DIR}")
      target_link_libraries(qtv epoll-shim ${SYS_LIBS} ${FTEQTV_LIBS})
      target_compile_definitions(qtv
        PRIVATE
          ${FTE_REVISON}
          ${FTE_LIB_DEFINES}
          HAVE_EPOLL)
    endif()
    list(APPEND INSTALLTARGS qtv)
  endif()

  if(FTE_TOOL_MASTER AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_executable(ftemaster
      ${FTESV_ARCH_FILES}
      engine/server/sv_master.c
      engine/common/net_ice.c   #for the stun responses.
      engine/common/net_wins.c
      engine/common/cvar.c
      engine/common/cmd.c
      engine/common/sha1.c    #for websockets
      engine/common/sha2.c    #for fingerprints
      engine/http/httpclient.c  #for the pipe stuff
      engine/common/log.c
      engine/common/fs.c
      engine/common/fs_stdio.c
      engine/common/common.c
      engine/common/translate.c
      engine/common/zone.c
      engine/qclib/hash.c
    )
    target_compile_definitions(ftemaster
      PRIVATE
        MASTERONLY
        ${FTE_LIB_DEFINES}
        ${FTESV_DEFINES}
        ${FTE_REVISON})
    target_link_libraries(ftemaster PRIVATE ${FTESV_LIBS})
    list(APPEND INSTALLTARGS ftemaster)
  endif()

  if(FTE_TOOL_HTTPSV)
    add_executable(httpserver
      engine/common/fs_stdio.c
      engine/http/httpserver.c
      engine/http/iwebiface.c
      engine/http/ftpserver.c
    )
    target_compile_definitions(httpserver
       PRIVATE
        WEBSERVER WEBSVONLY
        ${FTE_REVISON})
    target_link_libraries(httpserver
      PRIVATE
        $<$<PLATFORM_ID:Windows>:ws2_32>)
    if(CMAKE_SYSTEM_NAME MATCHES "BSD|Darwin")
      find_library(epoll-shim REQUIRED)
      target_include_directories(httpserver PUBLIC "${EPOLL_INC_DIR}")
      target_link_libraries(httpserver epoll-shim)
      target_compile_definitions(httpserver
        PRIVATE
          WEBSERVER
          WEBSVONLY
          ${FTE_REVISON}
          HAVE_EPOLL)
    endif()
    #list(APPEND INSTALLTARGS httpserver)
  endif()

  if(FTE_TOOL_QCVM)
    add_executable(qcvm
      engine/qclib/test.c

      engine/qclib/hash.c
      ${fte.qcvm.srcs}
    )
    target_compile_definitions(qcvm
      PRIVATE
        ${FTE_LIB_DEFINES}
        ${FTE_REVISON})
    target_link_libraries(qcvm PRIVATE ${FTEQCC_LIBS} ${SYS_LIBS})
    list(APPEND INSTALLTARGS qcvm)
  endif()

  if(FTE_TOOL_QCC)
    add_executable(fteqcc
      engine/qclib/qcctui.c
      engine/qclib/comprout.c
      engine/qclib/hash.c
      engine/qclib/qcc_cmdlib.c
      engine/qclib/qcc_pr_comp.c
      engine/qclib/qcc_pr_lex.c
      engine/qclib/qccmain.c
      engine/qclib/qcd_main.c
      engine/qclib/decomp.c
      engine/qclib/packager.c
    )
    target_compile_definitions(fteqcc
      PRIVATE
        ${FTE_LIB_DEFINES}
        ${FTE_REVISON})
    target_link_libraries(fteqcc ${FTEQCC_LIBS} ${SYS_LIBS})
    list(APPEND INSTALLTARGS fteqcc)
  endif()

  if(FTE_TOOL_QCCGUI)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      add_executable(fteqccgui WIN32
        engine/qclib/qccgui.c
        engine/qclib/qccguistuff.c
        engine/qclib/comprout.c
        engine/qclib/hash.c
        engine/qclib/qcc_cmdlib.c
        engine/qclib/qcc_pr_comp.c
        engine/qclib/qcc_pr_lex.c
        engine/qclib/qccmain.c
        engine/qclib/decomp.c
        engine/qclib/packager.c
        engine/qclib/qcd_main.c
      )
      target_compile_definitions(fteqccgui
        PRIVATE
          ${FTE_LIB_DEFINES} ${FTE_REVISON})
      target_link_libraries(fteqccgui ${FTEQCC_LIBS} shlwapi ole32 comctl32 comdlg32)
      list(APPEND INSTALLTARGS fteqccgui)
    else()
      find_package(Qt5Widgets)

      find_path(QSCINTILLA_INCLUDE_DIR
        NAMES Qsci/qsciglobal.h
        PATHS ${Qt5Widgets_INCLUDE_DIRS}
        PATH_SUFFIXES Qsci
      )
      find_library(QSCINTILLA_LIBRARY
        NAMES qscintilla2_qt5
        PATHS
          ${QT_LIBRARY_DIR}
          /usr/local/lib
          /usr/local/lib/qt5
          /usr/lib
      )

      if (QSCINTILLA_INCLUDE_DIR AND QSCINTILLA_LIBRARY AND Qt5Widgets_FOUND)
        add_executable(fteqccgui
          engine/qclib/qccguiqt.cpp
          engine/qclib/qccguistuff.c
          engine/qclib/comprout.c
          engine/qclib/hash.c
          engine/qclib/qcc_cmdlib.c
          engine/qclib/qcc_pr_comp.c
          engine/qclib/qcc_pr_lex.c
          engine/qclib/qccmain.c
          engine/qclib/decomp.c
        #  engine/qclib/packager.c
          engine/qclib/qcd_main.c
      )
      target_include_directories(fteqccgui PUBLIC ${Qt5Widgets_INCLUDE_DIRS} ${QSCINTILLA_INCLUDE_DIR})
      target_compile_definitions(fteqccgui
        PRIVATE
          ${FTE_LIB_DEFINES}
          ${FTE_REVISON}
          ${Qt5Widgets_COMPILE_DEFINITIONS})
      set_property(TARGET fteqccgui PROPERTY POSITION_INDEPENDENT_CODE TRUE)
      target_link_libraries(fteqccgui PRIVATE ${FTEQCC_LIBS} ${Qt5Widgets_LIBRARIES} ${QSCINTILLA_LIBRARY})
      list(APPEND INSTALLTARGS fteqccgui)
    else()
      message(WARNING "qscintilla/qt5widgets library not detected, no fteqccgui for you")
    endif()
  endif()
endif()

if(0)
  #software renderer plugin
  #not stable enough, and probably won't ever be
  add_library(sw MODULE
    plugins/plugin.c
    engine/sw/sw_backend.c
    engine/sw/sw_image.c
    engine/sw/sw_rast.c
    #engine/sw/sw_viddos.c
#   engine/sw/sw_vidwin.c
    engine/common/mathlib.c
#   engine/client/in_win.c

    engine/sw/sw.h
    engine/sw/sw_spans.h
  )
  target_compile_definitions(sw
    PRIVATE
      FTEPLUGIN
      ${FTE_LIB_DEFINES}
      SWQUAKE)
  target_link_libraries(sw ${SYS_LIBS})

  embed_plugin_meta(sw "Software Renderer" "Provides software rendering. Slow.")
endif()

#Quake Injector Alike plugin
if(FTE_PLUG_QI)
  add_library(plug_qi MODULE
    plugins/plugin.c
    plugins/qi/qi.c
    plugins/jabber/xml.c
  )
  target_compile_definitions(plug_qi PRIVATE FTEPLUGIN ${FTE_LIB_DEFINES})
  target_link_libraries(plug_qi ${SYS_LIBS})

  embed_plugin_meta(qi "Quaddicted Map Database" "Provides easy access to the quaddicted map database. Once installed you can use eg 'map qi_dopa:start' to begin playing dopa, or load it via the menus.")
endif()

if(FTE_PLUG_OPENSSL)
  #the openssl license is incompatible with the gpl, so while we have code to use it distributing the binaries built with it is not a (legal) option.
  #note that openssl 3.0.0 upwards are apache-2 licensed, which is gpl-3 compatible (though not gpl-2). debian has not caught up with that yet, however.
  #Crosscompile linux->win64: sudo ln -s ${pwd}/engine/libs-x86_64-w64-mingw32/openssl-openssl-3.0.1/ /usr/x86_64-w64-mingw32/OpenSSL
  if(OPENSSL_VERSION_MAJOR LESS 3)
    set(FTE_PRIVATE_USE_ONLY false CACHE BOOL "Ignore license violations.")
  endif()
  if(NOT OPENSSL_FOUND)
    message(WARNING "openssl library NOT available. you'll have to use some other library.")
  elseif(OPENSSL_VERSION_MAJOR LESS 3 AND NOT FTE_PRIVATE_USE_ONLY)
    message(WARNING "openssl v3 required for GPL compliance. Enable FTE_PRIVATE_USE_ONLY to compile openssl plugin.")
  else()
    if(OPENSSL_VERSION_MAJOR LESS 3)
      message(WARNING "openssl library version is not 3 or above. You may not distribute plugin binaries due to license conflict.")
    else()
      message(WARNING "Using openssl. Resulting plugin must be licensed as GPLv3.")
    endif()
    set(FTE_LIB_DEFINES ${FTE_LIB_DEFINES})
    if (WIN32)
      list(APPEND OPENSSL_LIBRARIES ws2_32)
    endif()

    add_library(plug_openssl MODULE
      plugins/plugin.c
      plugins/net_ssl_openssl.c
    )
    target_include_directories(plug_openssl PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_compile_definitions(plug_openssl
      PRIVATE
        FTEPLUGIN ${FTE_LIB_DEFINES})
    target_link_libraries(plug_openssl ${SYS_LIBS} ${OPENSSL_LIBRARIES})

    embed_plugin_meta(openssl "OpenSSL" "Provides OpenSSL support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar.")
  endif()
endif()

#set(FTE_PLUG_GNUTLS true CACHE BOOL "Compile GnuTLS Library.")
#if(FTE_PLUG_GNUTLS)
# find_package(GnuTLS)
# if(NOT GNUTLS_FOUND)
#   message(WARNING "gnutls library NOT available. you'll have to use some other library.")
# else()
#   set(FTE_LIB_DEFINES ${FTE_LIB_DEFINES})
#
#   add_library(plug_gnutls MODULE
#     plugins/plugin.c
#     engine/common/net_ssl_gnutls.c
#   )
#   target_compile_definitions(plug_gnutls PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})
#   target_link_libraries(plug_gnutls ${SYS_LIBS} ${GNUTLS_LIBRARIES})
#
#   embed_plugin_meta(gnutls "GnuTLS" "Provides GnuTLS support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar.")
# endif()
#endif()

#EzQuake Hud port plugin
add_library(plug_ezhud MODULE
  plugins/plugin.c
  plugins/ezhud/ezquakeisms.c
  plugins/ezhud/hud.c
  plugins/ezhud/hud_common.c
  plugins/ezhud/hud_editor.c)
target_compile_definitions(plug_ezhud
     PRIVATE FTEPLUGIN
     ${FTE_LIB_DEFINES})
target_link_libraries(plug_ezhud ${SYS_LIBS})
set_property(TARGET plug_ezhud PROPERTY EXCLUDE_FROM_ALL ${FTE_PLUG_EZHUD})
embed_plugin_meta(ezhud "EzHud Plugin" "Provides compat with ezquake's hud scripts.")

#NameMaker string generation plugin
if(FTE_PLUG_NAMEMAKER)
  add_library(plug_namemaker MODULE
    plugins/plugin.c
    plugins/namemaker/namemaker.c
  )
  target_compile_definitions(plug_namemaker
       PRIVATE ${FTE_LIB_DEFINES})
  target_link_libraries(plug_namemaker ${SYS_LIBS})

  embed_plugin_meta(namemaker "Name Maker Plugin" "Provides a lame UI for selecting arbitrary non-ascii glyphs as part of your nickname.")
endif()

#Terrain Generation plugin
set(FTE_PLUG_TERRAINGEN false CACHE BOOL "Compile sample terrain generation plugin.")
if(FTE_PLUG_TERRAINGEN)
  add_library(plug_terraingen MODULE
    plugins/plugin.c
    plugins/terrorgen/terragen.c
  )
  target_compile_definitions(plug_terraingen
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})
  target_link_libraries(plug_terraingen ${SYS_LIBS})

  embed_plugin_meta(terraingen "TerrainGen Plugin" "A lame example plugin for randomised terrain generation.")
endif()

#IRC client plugin
if(FTE_PLUG_IRC)
  add_library(plug_irc MODULE
    plugins/plugin.c
    plugins/irc/ircclient.c
  )
  target_compile_definitions(plug_irc
       PRIVATE FTEPLUGIN;${FTE_REVISON};${FTE_LIB_DEFINES})
  target_link_libraries(plug_irc ${SYS_LIBS})

  embed_plugin_meta(irc "IRC Plugin" "Allows you to chat on IRC without tabbing out.")
endif()

if(ZLIB_FOUND)
#mpq package format plugin (blizzard games)
if(FTE_PLUG_MPQ)
  add_library(plug_mpq MODULE
    plugins/plugin.c
    plugins/mpq/blast.c
    plugins/mpq/fs_mpq.c
  )
  target_compile_definitions(plug_mpq
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})
  target_link_libraries(plug_mpq ${SYS_LIBS} ${ZLIB_LIBRARIES})

  embed_plugin_meta(irc "MPQ Archive Plugin" "Adds support for reading .mpq files. Not very useful...")
endif()
endif()

#vpk package format plugin (halflife2)
if(FTE_PLUG_HL2)
  add_library(plug_hl2 MODULE
    plugins/plugin.c
    plugins/hl2/hl2.c
    plugins/hl2/fs_vpk.c
    plugins/hl2/img_vtf.c
    plugins/hl2/mod_hl2.c
    plugins/hl2/mod_vbsp.c
    plugins/hl2/mat_vmt.c
  )
  target_compile_definitions(plug_hl2
       PRIVATE FTEPLUGIN;MULTITHREAD;${FTE_LIB_DEFINES})
  target_link_libraries(plug_hl2 ${SYS_LIBS})

  embed_plugin_meta(hl2 "HalfLife2 Formats Plugin" "Adds support for reading various file formats used by HalfLife2. Requires mod support to be useful.")
endif()

#model formats plugin
if(FTE_PLUG_MODELS)
  add_library(plug_models MODULE
    plugins/plugin.c
    plugins/models/models.c
    plugins/models/gltf.c
    ${DRACO_FILES}
    engine/common/json.c
    plugins/models/exportiqm.c
  )
  target_compile_definitions(plug_models
       PRIVATE FTEPLUGIN;${DRACO_CFLAGS};${FTE_LIB_DEFINES})
  target_link_libraries(plug_models ${SYS_LIBS} ${DRACO_LIBRARY})

  embed_plugin_meta(models "Models Plugin" "Kinda redundant now that the engine has gltf2 loading.")
endif()

set(FTE_PLUG_X11SV false CACHE BOOL "Compile x11 server plugin.")
if(FTE_PLUG_X11SV)
  #x11 server plugin (note: for displaying other programs)
  #not stable enough, and probably won't ever be
  add_library(plug_x11sv MODULE
    plugins/plugin.c
    plugins/xsv/m_x.c
    plugins/xsv/x_reqs.c
    plugins/xsv/x_res.c
    engine/qclib/hash.c
  )
  target_compile_definitions(plug_x11sv
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})
  target_link_libraries(plug_x11sv ${SYS_LIBS})

  embed_plugin_meta(x11sv "X11 Server Plugin" "Provides a primitive X11 server in the form of a video decoder plugin.")
endif()

# TODO: Move to find_package
if(FTE_PLUG_FFMPEG)
  find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
  find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
  find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
  find_path(AVSWSCALE_INCLUDE_DIR libswscale/swscale.h)
  if(AVFORMAT_INCLUDE_DIR AND AVSWSCALE_INCLUDE_DIR)
    find_library(AVCODEC_LIBRARY avcodec)
    find_library(AVFORMAT_LIBRARY avformat)
    find_library(AVUTIL_LIBRARY avutil)
    find_library(AVSWSCALE_LIBRARY swscale)

    add_library(plug_ffmpeg MODULE
      plugins/plugin.c
      plugins/avplug/avaudio.c
      plugins/avplug/avdecode.c
      plugins/avplug/avencode.c
    )
    target_include_directories(plug_ffmpeg PUBLIC ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVSWSCALE_INCLUDE_DIR})
    target_link_libraries(plug_ffmpeg ${SYS_LIBS} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVSWSCALE_LIBRARY})
    target_compile_definitions(plug_ffmpeg
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})

    embed_plugin_meta(ffmpeg "FFMPEG Video Decoding Plugin" "Provides support for more audio formats, as well as video playback and better capture support.")
  else()
    message(WARNING "ffmpeg library NOT available. Quake shouldn't be playing fmv anyway.")
  endif()
endif()

if(FTE_PLUG_TIMIDITY)
  #timidity
  find_path(TIMIDITY_INCLUDE_DIR timidity/timidity.h)
  if(TIMIDITY_INCLUDE_DIR)
    find_library(TIMIDITY_LIBRARY timidity)

    add_library(plug_timidity MODULE
      plugins/plugin.c
      plugins/timidity.c
    )
    target_include_directories(plug_timidity PUBLIC ${TIMIDITY_INCLUDE_DIR})
    target_link_libraries(plug_timidity ${SYS_LIBS} ${TIMIDITY_LIBRARY})
    target_compile_definitions(plug_timidity
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES})

    embed_plugin_meta(timidity "Timidity Plugin" "Provides support for playback of midi files.")
  else()
    message(WARNING "timidity library NOT available. We'll just stick to fake-cd music for hexen2.")
  endif()
endif()

#openxr plugin
if(FTE_PLUG_OPENXR)
  # TODO: Use cmake_pkg_config instead
  find_package(PkgConfig)
  if (PKGCONFIG_FOUND)
    if (NOT CMAKE_CROSSCOMPILING) #its picking up the linux headers then complaining that they're missing in mingw. also almost entirely untested so no great loss.
      pkg_search_module(OPENXR openxr)
    endif()
    if (OPENXR_FOUND)
      add_library(plug_openxr MODULE
        plugins/plugin.c
        plugins/openxr.c
      )
      target_include_directories(plug_openxr PRIVATE ${OPENXR_INCLUDE_DIRS} )

      if (1)  #dynamically link
        target_compile_definitions(plug_openxr
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};XR_NO_PROTOTYPES)
        target_link_libraries(plug_openxr ${SYS_LIBS})
      else()  #statically link
        target_compile_definitions(plug_openxr
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES})
        target_link_libraries(plug_openxr ${SYS_LIBS} ${OPENXR_LIBRARIES})
      endif()

      embed_plugin_meta(openxr "OpenXR Plugin" "Provides support for Virtual Reality headsets and input devices.")
    else()
      message(WARNING "openxr library NOT available. Quake is already a reality anyway.")
    endif()
  endif()
endif()

##cef plugin
##libcef itself can be obtained from https://cef-builds.spotifycdn.com/index.html#linux64 (minimal builds, which still ends up with a 1,162,752,744 byte libcef.so - yes, actual size)
##(be sure to manually strip the binary of its debug info)
##to get this cmake stuff to recognise the headers etc:
##  cd $FTEQW-REPO && ln -s $FOO/cef_binary_$FOO+chromium-$FOO_linux64_minimal plugins/cef/cef_linux64
##(note that other systems use other subdir names)
# TODO(bruxisma): use fetchcontent and a find_package file 🙄
set(CEF_PATH CACHE PATH "Base location of libcef for target platform.")
if(FTE_PLUG_CEF)
  if(CEF_PATH MATCHES "")
    unset(CEF_PATH CACHE)
    if(WIN32)
      if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64")
        find_path (CEF_PATH include/cef_version.h plugins/cef/cef_windows64)
      elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86")
        find_path (CEF_PATH include/cef_version.h plugins/cef/cef_windows32)
      elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64")
        find_path (CEF_PATH include/cef_version.h plugins/cef/cef_windowsarm64)
      endif()
    elseif("${CMAKE_SYSTEM}" MATCHES "Linux")
      if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        find_path (CEF_PATH include/cef_version.h plugins/cef/cef_linux64)
      elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i686")
        find_path (CEF_PATH include/cef_version.h plugins/cef/cef_linux32)
      endif()
    endif()
  endif()
  #find_library(CEF_LIBRARIES cef ${CEF_PATH}/Release)
  if (CEF_PATH)
    ##statically link only for release builds. debug builds probably don't want to have to wait for ages for the debugger to finish loading debug info unless we're actually using this stuff.
    if(CMAKE_BUILD_TYPE MATCHES "Release")
      set(CEF_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/libcef.so")
    endif()
    add_library(plug_cef MODULE
      plugins/plugin.c
      plugins/cef/cef.c
    )
    target_include_directories(plug_cef PRIVATE ${CEF_PATH})
    set_target_properties(plug_cef PROPERTIES BUILD_RPATH_USE_ORIGIN true)
    if (CEF_LIBRARIES)
      target_compile_definitions(plug_cef
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};LIBCEF_STATIC)
      target_link_libraries(plug_cef ${SYS_LIBS} ${CEF_LIBRARIES} ${CMAKE_DL_LIBS})
    else()
      target_compile_definitions(plug_cef
       PRIVATE FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};LIBCEF_DYNAMIC)
      target_link_libraries(plug_cef ${SYS_LIBS} ${CMAKE_DL_LIBS})
    endif()

    if(NOT ${UNIX})
      add_custom_command(
        TARGET plug_cef PRE_LINK
        COMMAND cp ${CEF_PATH}/Release/* ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND cp ${CEF_PATH}/Resources/* ${CMAKE_CURRENT_BINARY_DIR}
        )
    else()
      if(CMAKE_BUILD_TYPE MATCHES "Release")
        #sigh, cef ain't stripped properly on linux.
        add_custom_command(
          TARGET plug_cef PRE_LINK
          COMMAND ln -f -s ${CEF_PATH}/Release/* ${CMAKE_CURRENT_BINARY_DIR}
          COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/libcef.so -o libcef.so
          COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/libEGL.so -o libEGL.so
          COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/libGLESv2.so -o libGLESv2.so
          COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/chrome-sandbox -o chrome-sandbox
          COMMAND ln -f -s ${CEF_PATH}/Resources/* ${CMAKE_CURRENT_BINARY_DIR}
          )
      else()
        add_custom_command(
          TARGET plug_cef PRE_LINK
          COMMAND ln -f -s ${CEF_PATH}/Release/* ${CMAKE_CURRENT_BINARY_DIR}
          COMMAND ln -f -s ${CEF_PATH}/Resources/* ${CMAKE_CURRENT_BINARY_DIR}
          )
      endif()
    endif()

    embed_plugin_meta(cef "libcef(Browser) Plugin" "This plugin provides support for an in-game web browser.")
  else()
    message(WARNING "libcef library NOT available. no web browser support on walls.")
  endif()
endif()

if(FTE_PLUG_XMPP)
  #XMPP/jabber client plugin
  add_library(plug_xmpp MODULE
    plugins/plugin.c
    plugins/jabber/jabberclient.c
    plugins/jabber/xml.c
    plugins/jabber/jingle.c
    plugins/jabber/sift.c
    engine/common/sha1.c
    engine/common/sha2.c
    plugins/emailnot/md5.c
  )
  target_compile_definitions(plug_xmpp
     PRIVATE
        FTEPLUGIN ${FTE_LIB_DEFINES})

  #for SRV lookups, so we actually get the right server from account names/etc.
  if(ANDROID) #libresolv issues.
  elseif(${WIN32}) #softlinks a dll
  else()
    target_link_libraries(plug_xmpp ${SYS_LIBS} resolv)
  endif()

  embed_plugin_meta(xmpp "XMPP Plugin" "XMPP/Jabber instant messenger plugin for chatting without tabbing out.")
endif()

install(TARGETS ${INSTALLTARGS}
  RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/${FTE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/${FTE_INSTALL_LIBDIR}"
)

install(FILES
  fteqw.desktop
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications/")


if(FTE_MENU_SYS)
  add_custom_target(menusys ALL
    VERBATIM
    DEPENDS fteqcc
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/quakec/menusys/"
    COMMAND fteqcc
      -srcfile "menu.src"
      -o "${CMAKE_CURRENT_BINARY_DIR}/menu.dat"
      -DREVISION="${SVNREVISION}"
      -DDATE="${FTE_DATE}"
      -DBRANCH="${FTE_BRANCH}"
    COMMAND /bin/echo -e "{\\n  package fte_menusys\\n  ver \"${SVNREVISION}\"\\n category Plugins\\n title \"Replacement Menus\"\\n  gamedir \"id1\"\\n  desc \"Modern menus to replace the ancient quake ones\"\\n}" | zip -j -q -9 -fz- "${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3" - "${CMAKE_CURRENT_BINARY_DIR}/menu.dat"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/menu.dat" "${CMAKE_CURRENT_BINARY_DIR}/menu.lno" "${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3"
    SOURCES
      quakec/menusys/menu.src
      quakec/menusys/fteextensions.qc
      quakec/menusys/menusys/mitems.qc
      quakec/menusys/menusys/mitems_common.qc
      quakec/menusys/menusys/mitem_frame.qc
      quakec/menusys/menusys/mitem_desktop.qc
      quakec/menusys/menusys/mitem_exmenu.qc
      quakec/menusys/menusys/mitem_edittext.qc
      quakec/menusys/menusys/mitem_tabs.qc
      quakec/menusys/menusys/mitem_colours.qc
      quakec/menusys/menusys/mitem_checkbox.qc
      quakec/menusys/menusys/mitem_slider.qc
      quakec/menusys/menusys/mitem_combo.qc
      quakec/menusys/menusys/mitem_bind.qc
      quakec/menusys/menusys/mitem_spinnymodel.qc
      quakec/menusys/menu/loadsave.qc
      quakec/menusys/menu/newgame.qc
      quakec/menusys/menu/options_basic.qc
      quakec/menusys/menu/options_effects.qc
      quakec/menusys/menu/options_keys.qc
      quakec/menusys/menu/options.qc
      quakec/menusys/menu/presets.qc
      quakec/menusys/menu/servers.qc
      quakec/menusys/menu/main.qc
      quakec/menusys/menu/mods.qc
      quakec/menusys/menu/cvars.qc
      quakec/menusys/menu/updates.qc
      quakec/menusys/menu/options_audio.qc
      quakec/menusys/menu/options_configs.qc
      quakec/menusys/menu/options_hud.qc
      quakec/menusys/menu/options_particles.qc
      quakec/menusys/menu/options_video.qc
      quakec/menusys/menu/quit.qc
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/games/quake/id1/")
endif()

set(FTE_CSADDON true CACHE BOOL "CS Addon.")
if(FTE_CSADDON)
  add_custom_target(csaddon ALL
    VERBATIM
    DEPENDS fteqcc
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/quakec/csaddon/src/"
    COMMAND fteqcc -srcfile "csaddon.src" -o "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat"
    COMMAND /bin/echo -e "{\\n  package fte_csaddon\\n  ver \"${SVNREVISION}\"\\n category Plugins\\n title \"${PLUGTITLE}\"\\n gamedir \"id1\"\\n  desc \"${PLUGDESC}\"\\n}" | zip -j -q -9 -fz- "${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3" - "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat" "${CMAKE_CURRENT_BINARY_DIR}/csaddon.lno" "${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3"
    SOURCES
      quakec/csaddon/src/csaddon.src

      quakec/csaddon/src/csplat.qc
      quakec/csaddon/src/csfixups.qc

      quakec/csaddon/src/editor_lights.qc
      quakec/csaddon/src/editor_terrain.qc
      quakec/csaddon/src/brush_selection.qc
      quakec/csaddon/src/brush_history.qc
      quakec/csaddon/src/brush_manip.qc
      quakec/csaddon/src/brush_draw.qc
      quakec/csaddon/src/brush_vertedit.qc
      quakec/csaddon/src/editor_brushes.qc
      quakec/csaddon/src/editor_ents.qc
      quakec/csaddon/src/textfield.qc
      quakec/csaddon/src/editor_particles.qc
      quakec/csaddon/src/menu.qc
      quakec/csaddon/src/cam.qc
      quakec/csaddon/src/csaddon.qc
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/games/quake/id1/")
endif()
