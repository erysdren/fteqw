project(q3asm2)
if(NOT FTE_TOOL_ASM)
	return()
endif()
add_executable(q3asm2
	${PROJECT_SOURCE_DIR}/q3asm2.c
	${FTE_ENGINE_QCLIB_DIR}/hash.c
)
target_include_directories(q3asm2 PRIVATE ${FTE_ENGINE_QCLIB_DIR})
