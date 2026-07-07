if(NOT FTE_TOOL_HEIGHTMAPCONVERTER)
	return()
endif()

fte_add_tool(heightmapconverter
	SOURCES
		${FTE_TOOLS_ROOT_DIR}/heightmapconverter/heightmapconverter.c
		${inih_SOURCE_DIR}/ini.c
)
target_link_libraries(heightmapconverter PRIVATE $<TARGET_NAME_IF_EXISTS:Math::Math>)
target_include_directories(heightmapconverter PRIVATE ${inih_SOURCE_DIR} ${FTE_ENGINE_ROOT_DIR} ${FTE_ENGINE_CLIENT_DIR})
