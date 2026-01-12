if(NOT FTE_TOOL_HTTP)
	return()
endif()

fte_add_tool(httpserver
	SOURCES
		${FTE_ENGINE_HTTP_DIR}/httpserver.c
		${FTE_ENGINE_HTTP_DIR}/iwebiface.c
		${FTE_ENGINE_COMMON_DIR}/fs_stdio.c
		${FTE_ENGINE_HTTP_DIR}/ftpserver.c
)
target_include_directories(httpserver PRIVATE ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_HTTP_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR})
target_compile_definitions(httpserver PRIVATE WEBSERVER WEBSVONLY)
