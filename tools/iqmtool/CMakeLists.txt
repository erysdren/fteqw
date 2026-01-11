project(iqmtool)
if(NOT FTE_TOOL_IQM)
	return()
endif()
add_executable(iqmtool ${PROJECT_SOURCE_DIR}/iqm.cpp)
set_target_properties(iqmtool
	PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
		RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
)
