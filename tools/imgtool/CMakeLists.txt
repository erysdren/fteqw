project(imgtool)
if(NOT FTE_TOOL_IMG)
	return()
endif()
add_executable(imgtool
	${PROJECT_SOURCE_DIR}/imgtool.c
	${FTE_ENGINE_CLIENT_DIR}/image.c
)
target_link_libraries(imgtool PRIVATE $<TARGET_NAME_IF_EXISTS:Math::Math>)
target_include_directories(imgtool PRIVATE ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_GL_DIR})
target_compile_definitions(imgtool PRIVATE IMGTOOL)
