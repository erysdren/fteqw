include(FindPackageHandleStandardArgs)

find_library(Wayland_Client_LIBRARY NAMES wayland-client libwayland-client)
find_library(Wayland_EGL_LIBRRAY NAMES wayland-egl)

find_path(Wayland_Client_INCLUDE_DIR NAMES wayland-client.h)
find_path(Wayland_EGL_INCLUDE_DIR NAMES wayland-egl.h)

block (SCOPE_FOR VARIABLES PROPAGATE Wayland_Client_FOUND)
  set(Wayland_Client_FIND_QUIETLY YES)
  find_package_handle_standard_args(Wayland_Client
    REQUIRED_VARS
      Wayland_Client_INCLUDE_DIR
      Wayland_Client_LIBRARY)
endblock()

block (SCOPE_FOR VARIABLES PROPAGATE Wayland_EGL_FOUND)
  set(Wayland_EGL_FIND_QUIETLY YES)
  find_package_handle_standard_args(Wayland_EGL
    REQUIRED_VARS
      Wayland_EGL_INCLUDE_DIR
      Wayland_EGL_LIBRARY)
endblock()

find_package_handle_standard_args(Wayland
  REQUIRED_VARS Wayland_Client_LIBRARY
  HANDLE_COMPONENTS
  NAME_MISMATCHED)

if (Wayland_Client_FOUND AND NOT TARGET Wayland::Client)
  add_library(Wayland::Client UNKNOWN IMPORTED)
  set_property(TARGET Wayland::Client
    PROPERTY
      IMPORTED_LOCATION "${Wayland_Client_LIBRARY}")
  target_include_directories(Wayland::Client
    INTERFACE
      "${Wayland_Client_INCLUDE_DIR}")
endif()

if (Wayland_EGL_FOUND AND NOT TARGET Wayland::EGL)
  add_library(Wayland::EGL UNKNOWN IMPORTED)
  set_property(TARGET Wayland::EGL
    PROPERTY
      IMPORTED_LOCATION "${Wayland_EGL_LIBRARY}")
  target_include_directories(Wayland::EGL
    INTERFACE
      "${Wayland_EGL_INCLUDE_DIR}")
endif()
