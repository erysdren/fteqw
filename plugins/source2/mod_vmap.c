#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"
#include "com_bih.h"

// plugins continue to be silly
#ifdef FTEPLUGIN
#undef ZG_Malloc
#define ZG_Malloc(g,s) plugfuncs->GMalloc(g,s)
#undef Z_Free
#define Z_Free(p) plugfuncs->Free(p)
#undef Z_Malloc
#define Z_Malloc(s) plugfuncs->Malloc(s)
#undef Z_Realloc
#define Z_Realloc(p,s) plugfuncs->Realloc(p,s)
#undef Mod_AccumulateTextureVectors
#define Mod_AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms) modfuncs->AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms)
#undef Mod_NormaliseTextureVectors
#define Mod_NormaliseTextureVectors(n,s,t,v,calcnorms) modfuncs->NormaliseTextureVectors(n,s,t,v,calcnorms)
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

vec_t Length(const vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}
#endif

static plugmodfuncs_t *modfuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

static qboolean QDECL Mod_LoadVMAP(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai = NULL;

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// finish up
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	mod->flags = 0;

	return true;
}

qboolean VMAP_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION) // threadfuncs optional
		return false;

	return modfuncs->RegisterModelFormatMagic("Source 2 Level", "<!-- dmx encoding binary 9 format vmap 29 -->\x0a\x00", 47, Mod_LoadVMAP);
}
