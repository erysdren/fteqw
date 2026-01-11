if(NOT FTE_TOOL_IQM)
	return()
endif()

fte_add_tool(iqmtool
	SOURCES
		${FTE_TOOLS_ROOT_DIR}/iqmtool/iqm.cpp
)
