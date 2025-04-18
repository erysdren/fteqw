include(FindPackageHandleStandardArgs)

find_program(ccache_EXECUTABLE NAMES ccache)

block (SCOPE_FOR VARIABLES)
  if (NOT ccache_VERSION AND ccache_EXECUTABLE)
    execute_process(COMMAND "${ccache_EXECUTABLE}" --print-version
      OUTPUT_VARIABLE ccache_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ENCODING UTF-8)
    if (ccache_OUTPUT)
      set(ccache_VERSION "${ccache_OUTPUT}" CACHE STRING "ccache version")
      mark_as_advanced(ccache_VERSION)
    endif()
  endif()
endblock()

find_package_handle_standard_args(ccache
  REQUIRED_VARS ccache_EXECUTABLE
  VERSION_VAR ccache_VERSION
  HANDLE_VERSION_RANGE
  NAME_MISMATCHED)

if (ccache_FOUND AND NOT TARGET ccache::ccache)
  add_executable(ccache::ccache IMPORTED)
  set_target_properties(ccache::ccache
    PROPERTIES
      IMPORTED_LOCATION "${ccache_EXECUTABLE}"
      VERSION "${ccache_VERSION}")
endif()
