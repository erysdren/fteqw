if(NOT FTE_TOOLS)
	return()
endif()

function(fte_add_tool name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES")
	add_executable(${name} ${ARG_SOURCES})
	set_target_properties(${name}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
			RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game/bin
	)
endfunction()

# fteqtv
if(FTE_TOOL_QTV)
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
endif()

# imgtool
if(FTE_TOOL_IMG)
	fte_add_tool(imgtool
		SOURCES
			${FTE_TOOLS_ROOT_DIR}/imgtool/imgtool.c
			${FTE_ENGINE_CLIENT_DIR}/image.c
	)
	target_link_libraries(imgtool PRIVATE $<TARGET_NAME_IF_EXISTS:Math::Math>)
	target_include_directories(imgtool PRIVATE ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_GL_DIR})
	target_compile_definitions(imgtool PRIVATE IMGTOOL)
endif()

# iqmtool
if(FTE_TOOL_IQM)
	fte_add_tool(iqmtool
		SOURCES
			${FTE_TOOLS_ROOT_DIR}/iqmtool/iqm.cpp
	)
endif()

# q3asm2
if(FTE_TOOL_ASM)
	fte_add_tool(q3asm2
		SOURCES
			${FTE_TOOLS_ROOT_DIR}/q3asm2/q3asm2.c
			${FTE_ENGINE_QCLIB_DIR}/hash.c
	)
	target_include_directories(q3asm2 PRIVATE ${FTE_ENGINE_QCLIB_DIR})
endif()
