/*
 * written by erysdren (it/its) in 2025
 */
#include "../plugin.h"

qboolean Plug_GBH_Init(void);
//qboolean Plug_STY_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "gta2";

	// set plugin name
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// initialize components
	if (!Plug_GBH_Init()) return false;
	//if (!Plug_STY_Init()) return false;

	return true;
}
