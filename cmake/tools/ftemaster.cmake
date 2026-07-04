if(NOT FTE_TOOL_MASTER)
	return()
endif()

fte_add_tool(ftemaster
	SOURCES
		${FTE_ENGINE_SERVER_DIR}/sv_master.c
		${FTE_ENGINE_COMMON_DIR}/net_wins.c
		${FTE_ENGINE_COMMON_DIR}/net_ice.c
		${FTE_ENGINE_COMMON_DIR}/cvar.c
		${FTE_ENGINE_COMMON_DIR}/cmd.c
		${FTE_ENGINE_COMMON_DIR}/sha1.c
		${FTE_ENGINE_COMMON_DIR}/sha2.c
		${FTE_ENGINE_HTTP_DIR}/httpclient.c
		${FTE_ENGINE_COMMON_DIR}/log.c
		${FTE_ENGINE_COMMON_DIR}/fs.c
		${FTE_ENGINE_COMMON_DIR}/fs_stdio.c
		${FTE_ENGINE_COMMON_DIR}/common.c
		${FTE_ENGINE_COMMON_DIR}/translate.c
		${FTE_ENGINE_COMMON_DIR}/zone.c
		${FTE_ENGINE_QCLIB_DIR}/hash.c
		${FTE_ENGINE_COMMON_DIR}/net_ssl_gnutls.c
		$<$<BOOL:${WIN32}>:${FTE_ENGINE_COMMON_DIR}/fs_win32.c>
		$<$<BOOL:${WIN32}>:${FTE_ENGINE_SERVER_DIR}/sv_sys_win.c>
		$<$<BOOL:${WIN32}>:${FTE_ENGINE_COMMON_DIR}/sys_win_threads.c>
		$<$<BOOL:${WIN32}>:${FTE_ENGINE_COMMON_DIR}/net_ssl_winsspi.c>
		$<$<BOOL:${UNIX}>:${FTE_ENGINE_SERVER_DIR}/sv_sys_unix.c>
		$<$<BOOL:${UNIX}>:${FTE_ENGINE_COMMON_DIR}/sys_linux_threads.c>
)
target_include_directories(ftemaster PRIVATE ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
target_compile_definitions(ftemaster PRIVATE MASTERONLY)
target_link_libraries(ftemaster
	PRIVATE
		$<TARGET_NAME_IF_EXISTS:zlibstatic>
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		$<$<BOOL:${WIN32}>:ws2_32>
		$<$<BOOL:${WIN32}>:winmm>
)
