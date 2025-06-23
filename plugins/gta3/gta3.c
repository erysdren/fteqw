#include "../plugin.h"

qboolean Plug_FSGTA3_Init(void);
//qboolean Plug_DFF_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "gta3";

	// set plugin name
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// initialize components
	if (!Plug_FSGTA3_Init()) return false;
	//if (!Plug_DFF_Init()) return false;

	return true;
}
