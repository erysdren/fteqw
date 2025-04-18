include_guard(GLOBAL)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git" AND NOT DEFINED FTE_REVISON)
  execute_process(COMMAND
    git describe --always --long --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE FTE_REVISON_GIT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(COMMAND
    git log -1 --format=%cs
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE FTE_DATE
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(COMMAND
    git branch --show-current
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE FTE_BRANCH
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(COMMAND
    git rev-parse --is-shallow-repository
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE FTE_GIT_IS_SHALLOW
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(FTE_GIT_IS_SHALLOW)
    message(STATUS "shallow clone prevents calculation of revision number.")
    set(SVNREVISION "git-${FTE_REVISON_GIT}") #if its a shallow clone then we can't count commits properly so don't know what revision we actually are.
  else()
    execute_process(COMMAND
      git rev-list HEAD --count
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE SVNREVISION
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    math(EXPR SVNREVISION "${SVNREVISION} + 29") #not all svn commits managed to appear im the git repo, so we have a small bias to keep things consistent.
    if (FTE_BRANCH STREQUAL "master" OR FTE_BRANCH STREQUAL "")
      set(SVNREVISION "${SVNREVISION}-git-${FTE_REVISON_GIT}")
    else()
      set(SVNREVISION "${FTE_BRANCH}-${SVNREVISION}-git-${FTE_REVISON_GIT}") #weird branches get a different form of revision, to reduce confusion.
    endif()
  endif()
  message(STATUS "FTE GIT ${FTE_BRANCH} Revision ${SVNREVISION}, ${FTE_DATE}")
  set(FTE_REVISON SVNREVISION=${SVNREVISION} SVNDATE=${FTE_DATE} FTE_BRANCH=${FTE_BRANCH})
endif()


