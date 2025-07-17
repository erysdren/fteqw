/*
 * written by erysdren (it/its) in 2025
 */
#include "voxlap.h"

plugmodfuncs_t *modfuncs = NULL;
cvar_t *mod_voxlap_scale = NULL;

// plugins are silly huh
#ifdef FTEPLUGIN
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = FLT_MAX;
	maxs[0] = maxs[1] = maxs[2] = -FLT_MAX;
}
#endif

qboolean KVX_Init(void);
qboolean KV6_Init(void);

qboolean Plug_Init(void)
{
	qboolean somethingisokay = false;

	// set plugin name
	char plugname[] = "voxlap";
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	// get modfuncs
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	// register cvars
	mod_voxlap_scale = cvarfuncs->GetNVFDG("mod_voxlap_scale", "4", CVAR_RENDERERLATCH, "Number of cubic Quake units per Voxel", "Voxlap");
	if (!mod_voxlap_scale)
		return false;

	// init components
	// if (!KVX_Init()) Con_Printf(CON_ERROR"%s: KVX support unavailable\n", plugname); else somethingisokay = true;
	if (!KV6_Init()) Con_Printf(CON_ERROR"%s: KV6 support unavailable\n", plugname); else somethingisokay = true;

	// return status
	return somethingisokay;
}
