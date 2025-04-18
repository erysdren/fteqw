include_guard(GLOBAL)

function(embed_plugin_meta name title description)
if(0)
  set_target_properties(plug_${name}
    PROPERTIES
      OUTPUT_NAME "${name}"
      PREFIX "fteplug_")
  target_link_options(plug_${name} PRIVATE LINKER:--no-undefined)
  list(APPEND INSTALLTARGS plug_${name})
  # IXM: The below is not true
  # sadly we need to use a temp zip file, because otherwise zip insists on using
  # zip64 extensions which breaks zip -a (as well as any attempts to read any
  # files).
  file(GENERATE
    OUTPUT ${name}.plugin.cfg
    CONTENT [[
{
  package fteplug_$<TARGET_PROPERTY:NAME>
  ver "$<TARGET_PROPERTY:VERSION>"
  category Plugins
  title "$<TARGET_PROPERTY:PLUGIN_TITLE>"
  gamedir ""
  desc "$<TARGET_PROPERTY:PLUGIN_DESCRIPTION>"
}
]]
    TARGET plug_${name})
  add_custom_command(
    TARGET plug_${name} POST_BUILD
    COMMAND /bin/echo -e "{\\n  package fteplug_${name}\\n  ver \"${SVNREVISION}\"\\n category Plugins\\n title \"${PLUGTITLE}\"\\n gamedir \"\"\\n desc \"${PLUGDESC}\"\\n}" | zip -q -9 -fz- $<TARGET_FILE:plug_${PLUGNAME}>.zip -
    COMMAND cmake -E cat $<TARGET_FILE:$<TARGET_PROPERTY:NAME>>.zip >> "$<TARGET_FILE:$<TARGET_PROPERTY:NAME>>"
    COMMAND zip -A "$<TARGET_FILE:$<TARGET_PROPERTY:NAME>>"
    COMMAND cmake -E rm $<TARGET_FILE:$<TARGET_PROPERTY:NAME>>.zip
    VERBATIM)
  return(PROPAGATE INSTALLTARGS)
endif()
endfunction()

function (add_fte_plugin name)
  add_library(fte-plugin-${name} MODULE)
  add_library(FTE::Plugin::${name} ALIAS fte-plugin-name)
  target_compile_definitions(fte-plugin-name PRIVATE FTEPLUGIN)
endfunction()
