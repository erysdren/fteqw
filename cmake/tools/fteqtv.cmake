if(NOT FTE_TOOL_QTV)
	return()
endif()

fte_add_tool(fteqtv
	SOURCES
		${FTE_TOOLS_ROOT_DIR}/fteqtv/libqtvc/glibc_sucks.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/libqtvc/msvc_sucks.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/netchan.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/parse.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/msg.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/qw.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/source.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/bsp.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/rcon.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/mdfour.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/crc.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/control.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/forward.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/pmove.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/menu.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/httpsv.c
		${FTE_TOOLS_ROOT_DIR}/fteqtv/relay.c
		${FTE_ENGINE_COMMON_DIR}/sha1.c
		${FTE_ENGINE_COMMON_DIR}/md5.c
)
target_link_libraries(fteqtv PRIVATE ZLIB::ZLIB $<TARGET_NAME_IF_EXISTS:Math::Math>)
target_include_directories(fteqtv PRIVATE ${FTE_ENGINE_CLIENT_DIR})
