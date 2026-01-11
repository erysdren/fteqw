if(NOT FTE_TOOL_ASM)
	return()
endif()

fte_add_tool(q3asm2
	SOURCES
		${FTE_TOOLS_ROOT_DIR}/q3asm2/q3asm2.c
		${FTE_ENGINE_QCLIB_DIR}/hash.c
)
target_include_directories(q3asm2 PRIVATE ${FTE_ENGINE_QCLIB_DIR})
