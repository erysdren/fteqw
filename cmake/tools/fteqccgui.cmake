if(NOT FTE_TOOL_QCCGUI)
	return()
endif()

fte_add_tool(fteqccgui
	SOURCES
		${FTE_ENGINE_QCLIB_DIR}/qccgui.c
		${FTE_ENGINE_QCLIB_DIR}/qccguistuff.c
		${FTE_ENGINE_QCLIB_DIR}/packager.c
		${FTE_ENGINE_QCLIB_DIR}/decomp.c
		${FTE_ENGINE_QCLIB_DIR}/comprout.c
		${FTE_ENGINE_QCLIB_DIR}/hash.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_cmdlib.c
		${FTE_ENGINE_QCLIB_DIR}/qccmain.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_pr_comp.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_pr_lex.c
		${FTE_ENGINE_QCLIB_DIR}/qcd_main.c
		$<$<BOOL:${WIN32}>:${FTE_ENGINE_QCLIB_DIR}/fteqcc.rc>
)
target_include_directories(fteqccgui PRIVATE ${FTE_ENGINE_QCLIB_DIR})
target_link_libraries(fteqccgui
	PRIVATE
		$<TARGET_NAME_IF_EXISTS:zlibstatic>
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		$<$<BOOL:${WIN32}>:shlwapi>
		$<$<BOOL:${WIN32}>:comctl32>
)
