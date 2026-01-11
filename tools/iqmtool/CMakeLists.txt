project(iqmtool)
if(NOT FTE_TOOL_IQM)
	return()
endif()
fte_add_tool(iqmtool
	SOURCES
		${PROJECT_SOURCE_DIR}/iqm.cpp
)
