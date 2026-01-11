project(q3asm2)
if(NOT FTE_TOOL_ASM)
	return()
endif()
add_executable(q3asm2
	${PROJECT_SOURCE_DIR}/q3asm2.c
	${FTE_ENGINE_QCLIB_DIR}/hash.c
)
target_include_directories(q3asm2 PRIVATE ${FTE_ENGINE_QCLIB_DIR})
set_target_properties(q3asm2
	PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
		RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
)
