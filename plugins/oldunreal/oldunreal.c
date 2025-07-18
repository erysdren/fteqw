#include "../plugin.h"

// qboolean FSUPKG_Init(void);
// qboolean PSK_Init(void);

qboolean Plug_Init(void)
{
	char plugname[] = "oldunreal";
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// if (!FSUPKG_Init()) return false;
	// if (!PSK_Init()) return false;

	return true;
}
