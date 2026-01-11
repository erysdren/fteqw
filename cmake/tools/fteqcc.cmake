if(NOT FTE_TOOL_QCC)
	return()
endif()

fte_add_tool(fteqcc
	SOURCES
		${FTE_ENGINE_QCLIB_DIR}/qcctui.c
		${FTE_ENGINE_QCLIB_DIR}/packager.c
		${FTE_ENGINE_QCLIB_DIR}/decomp.c
		${FTE_ENGINE_QCLIB_DIR}/comprout.c
		${FTE_ENGINE_QCLIB_DIR}/hash.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_cmdlib.c
		${FTE_ENGINE_QCLIB_DIR}/qccmain.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_pr_comp.c
		${FTE_ENGINE_QCLIB_DIR}/qcc_pr_lex.c
		${FTE_ENGINE_QCLIB_DIR}/qcd_main.c
)
target_include_directories(fteqcc PRIVATE ${FTE_ENGINE_QCLIB_DIR})
target_link_libraries(fteqcc PRIVATE ZLIB::ZLIB $<TARGET_NAME_IF_EXISTS:Math::Math>)
