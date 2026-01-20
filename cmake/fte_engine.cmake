if(NOT FTE_ENGINE)
	return()
endif()

set(FTE_ENGINE_COMMON_SOURCES
	${FTE_ENGINE_GL_DIR}/gl_alias.c
	${FTE_ENGINE_GL_DIR}/gl_hlmdl.c
	${FTE_ENGINE_GL_DIR}/gl_heightmap.c
	${FTE_ENGINE_GL_DIR}/gl_model.c
	${FTE_ENGINE_GL_DIR}/glmod_doom.c

	${FTE_ENGINE_CLIENT_DIR}/m_download.c
	${FTE_ENGINE_CLIENT_DIR}/r_d3.c
	${FTE_ENGINE_CLIENT_DIR}/net_master.c

	${FTE_ENGINE_HTTP_DIR}/httpclient.c
	${FTE_ENGINE_HTTP_DIR}/iwebiface.c

	${FTE_ENGINE_COMMON_DIR}/com_bih.c
	${FTE_ENGINE_COMMON_DIR}/com_mesh.c
	${FTE_ENGINE_COMMON_DIR}/common.c
	${FTE_ENGINE_COMMON_DIR}/json.c
	${FTE_ENGINE_COMMON_DIR}/cvar.c
	${FTE_ENGINE_COMMON_DIR}/cmd.c
	${FTE_ENGINE_COMMON_DIR}/crc.c
	${FTE_ENGINE_COMMON_DIR}/net_ssl_gnutls.c
	${FTE_ENGINE_COMMON_DIR}/fs.c
	${FTE_ENGINE_COMMON_DIR}/fs_stdio.c
	${FTE_ENGINE_COMMON_DIR}/fs_pak.c
	${FTE_ENGINE_COMMON_DIR}/fs_zip.c
	${FTE_ENGINE_COMMON_DIR}/fs_dzip.c
	${FTE_ENGINE_COMMON_DIR}/fs_xz.c
	${FTE_ENGINE_COMMON_DIR}/mathlib.c
	${FTE_ENGINE_COMMON_DIR}/huff.c
	${FTE_ENGINE_COMMON_DIR}/md4.c
	${FTE_ENGINE_COMMON_DIR}/md5.c
	${FTE_ENGINE_COMMON_DIR}/sha1.c
	${FTE_ENGINE_COMMON_DIR}/sha2.c
	${FTE_ENGINE_COMMON_DIR}/log.c
	${FTE_ENGINE_COMMON_DIR}/net_chan.c
	${FTE_ENGINE_COMMON_DIR}/net_wins.c
	${FTE_ENGINE_COMMON_DIR}/net_ice.c
	${FTE_ENGINE_COMMON_DIR}/zone.c
	${FTE_ENGINE_COMMON_DIR}/qvm.c
	${FTE_ENGINE_COMMON_DIR}/gl_q2bsp.c
	${FTE_ENGINE_COMMON_DIR}/pmove.c
	${FTE_ENGINE_COMMON_DIR}/pmovetst.c
	${FTE_ENGINE_COMMON_DIR}/translate.c
	${FTE_ENGINE_COMMON_DIR}/plugin.c
	${FTE_ENGINE_COMMON_DIR}/q1bsp.c
	${FTE_ENGINE_COMMON_DIR}/q2pmove.c

	${FTE_ENGINE_SERVER_DIR}/world.c
	${FTE_ENGINE_SERVER_DIR}/sv_phys.c
	${FTE_ENGINE_SERVER_DIR}/sv_move.c
)

set(FTE_ENGINE_SERVER_SOURCES
	${FTE_ENGINE_SERVER_DIR}/pr_cmds.c
	${FTE_ENGINE_SERVER_DIR}/pr_q1qvm.c
	${FTE_ENGINE_SERVER_DIR}/pr_lua.c
	${FTE_ENGINE_SERVER_DIR}/sv_master.c
	${FTE_ENGINE_SERVER_DIR}/sv_init.c
	${FTE_ENGINE_SERVER_DIR}/sv_main.c
	${FTE_ENGINE_SERVER_DIR}/sv_nchan.c
	${FTE_ENGINE_SERVER_DIR}/sv_ents.c
	${FTE_ENGINE_SERVER_DIR}/sv_send.c
	${FTE_ENGINE_SERVER_DIR}/sv_user.c
	${FTE_ENGINE_SERVER_DIR}/sv_sql.c
	${FTE_ENGINE_SERVER_DIR}/sv_mvd.c
	${FTE_ENGINE_SERVER_DIR}/sv_ccmds.c
	${FTE_ENGINE_SERVER_DIR}/sv_cluster.c
	${FTE_ENGINE_SERVER_DIR}/sv_rankin.c
	${FTE_ENGINE_SERVER_DIR}/sv_chat.c
	${FTE_ENGINE_SERVER_DIR}/sv_demo.c
	${FTE_ENGINE_SERVER_DIR}/net_preparse.c
	${FTE_ENGINE_SERVER_DIR}/savegame.c
	${FTE_ENGINE_SERVER_DIR}/svq2_ents.c
	${FTE_ENGINE_SERVER_DIR}/svq2_game.c

	${FTE_ENGINE_HTTP_DIR}/webgen.c
	${FTE_ENGINE_HTTP_DIR}/ftpserver.c
	${FTE_ENGINE_HTTP_DIR}/httpserver.c
)

set(FTE_ENGINE_PROGS_SOURCES
	${FTE_ENGINE_COMMON_DIR}/pr_bgcmd.c

	${FTE_ENGINE_CLIENT_DIR}/pr_skelobj.c

	${FTE_ENGINE_QCLIB_DIR}/comprout.c
	${FTE_ENGINE_QCLIB_DIR}/hash.c
	${FTE_ENGINE_QCLIB_DIR}/qcc_cmdlib.c
	${FTE_ENGINE_QCLIB_DIR}/qccmain.c
	${FTE_ENGINE_QCLIB_DIR}/qcc_pr_comp.c
	${FTE_ENGINE_QCLIB_DIR}/qcc_pr_lex.c
	${FTE_ENGINE_QCLIB_DIR}/qcd_main.c
	${FTE_ENGINE_QCLIB_DIR}/initlib.c
	${FTE_ENGINE_QCLIB_DIR}/pr_edict.c
	${FTE_ENGINE_QCLIB_DIR}/pr_exec.c
	${FTE_ENGINE_QCLIB_DIR}/pr_multi.c
	${FTE_ENGINE_QCLIB_DIR}/pr_x86.c
	${FTE_ENGINE_QCLIB_DIR}/qcdecomp.c
)

set(FTE_ENGINE_SERVER_ONLY_SOURCES
	$<$<BOOL:${WIN32}>:${FTE_ENGINE_SERVER_DIR}/sv_sys_win.c>
	$<$<BOOL:${WIN32}>:${FTE_ENGINE_COMMON_DIR}/sys_win_threads.c>
	$<$<BOOL:${WIN32}>:${FTE_ENGINE_COMMON_DIR}/net_ssl_winsspi.c>
	$<$<BOOL:${UNIX}>:${FTE_ENGINE_SERVER_DIR}/sv_sys_unix.c>
	$<$<BOOL:${UNIX}>:${FTE_ENGINE_COMMON_DIR}/sys_linux_threads.c>
)

if(FTE_ENGINE_BOTH)

endif()

if(FTE_ENGINE_SERVER)
	add_executable(fteqw-sv
		${FTE_ENGINE_COMMON_SOURCES}
		${FTE_ENGINE_SERVER_SOURCES}
		${FTE_ENGINE_PROGS_SOURCES}
		${FTE_ENGINE_SERVER_ONLY_SOURCES}
	)
	target_compile_options(fteqw-sv PRIVATE ${FTE_COMMON_OPTIONS})
	target_compile_definitions(fteqw-sv PRIVATE SERVERONLY ${FTE_COMMON_DEFINITIONS})
	target_include_directories(fteqw-sv PRIVATE ${FTE_ENGINE_ROOT_DIR} ${FTE_ENGINE_COMMON_DIR} ${FTE_ENGINE_CLIENT_DIR} ${FTE_ENGINE_QCLIB_DIR} ${FTE_ENGINE_GL_DIR})
	target_link_libraries(fteqw-sv PRIVATE ZLIB::ZLIB $<TARGET_NAME_IF_EXISTS:Math::Math>)
	set_target_properties(fteqw-sv
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game
			RUNTIME_OUTPUT_DIRECTORY ${FTE_ROOT_DIR}/game
			SUFFIX ${FTE_EXECUTABLE_SUFFIX}
	)
endif()

if(FTE_ENGINE_CLIENT)

endif()
