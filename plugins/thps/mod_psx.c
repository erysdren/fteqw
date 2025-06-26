/*
 * written by erysdren (it/its) in 2025
 *
 * based on GreaseMonkey's public domain PSX level viewer from 2017
 * https://gist.github.com/iamgreaser/2a67f7473d9c48a70946018b73fa1e40
 */
#include "../plugin.h"
#include "../engine/common/com_mesh.h"
#include "../engine/common/com_bih.h"

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

static plugthreadfuncs_t *threadfuncs = NULL;
static plugmodfuncs_t *modfuncs = NULL;

typedef struct psxheader {
	uint32_t magic;
	uint32_t ofs_meta;
	uint32_t num_objects;
} psxheader_t;

static void PSX_GenerateMaterials(void *ctx, void *data, size_t a, size_t b)
{

}

static qboolean QDECL Mod_LoadPSX(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai = NULL;
	int x, y, z;
	int i, j, k;
	psxheader_t *header;

	header = (psxheader_t *)buffer;

	// shouldn't happen anyway
	if (memcmp(&header->magic, "\x04\x00\x02\x00", 4) != 0)
	{
		Con_Printf("%s: unknown format\n", mod->name);
		return false;
	}

#if 0
	// generate geometry from map
	gai = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai));

	// is there a way to speed this up?
	for (z = 0; z < 8; z++)
	{
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				if (GROUND_TYPE((*city)[z][y][x].slope_type) == GROUND_TYPE_ROAD)
				{
					gai[0].numverts += 4;
					gai[0].numindexes += 6;
				}
			}
		}
	}

	// allocate arrays
	gai[0].ofs_skel_xyz = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_xyz) * gai[0].numverts);
	gai[0].ofs_st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_st_array) * gai[0].numverts);
	gai[0].ofs_rgbaf = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_rgbaf) * gai[0].numverts);
	gai[0].ofs_skel_norm = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_norm) * gai[0].numverts);
	gai[0].ofs_skel_svect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_svect) * gai[0].numverts);
	gai[0].ofs_skel_tvect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_skel_tvect) * gai[0].numverts);

	gai[0].ofs_indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofs_indexes) * gai[0].numindexes);

	// generate vertex positions and indices
	nquads = 0;
	for (z = 0; z < 8; z++)
	{
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 256; x++)
			{
				if (GROUND_TYPE((*city)[z][y][x].slope_type) == GROUND_TYPE_ROAD)
				{
					int dstverts[4] = {
						(nquads * 4) + 0,
						(nquads * 4) + 1,
						(nquads * 4) + 2,
						(nquads * 4) + 3
					};

					// setup positions
					gai[0].ofs_skel_xyz[dstverts[0]][0] = (x * 64) - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[0]][1] = (y * 64) - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[0]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[1]][0] = (x * 64) - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[1]][1] = (y * 64) + 64 - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[1]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[2]][0] = (x * 64) + 64 - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[2]][1] = (y * 64) + 64 - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[2]][2] = (z * 64);

					gai[0].ofs_skel_xyz[dstverts[3]][0] = (x * 64) + 64 - CITY_OFFSET_X;
					gai[0].ofs_skel_xyz[dstverts[3]][1] = (y * 64) - CITY_OFFSET_Y;
					gai[0].ofs_skel_xyz[dstverts[3]][2] = (z * 64);

					// bounds
					for (i = 0; i < 4; i++)
						AddPointToBounds(gai[0].ofs_skel_xyz[dstverts[i]], mod->mins, mod->maxs);

					// setup indices
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 0] = dstverts[0];
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 1] = dstverts[1];
					gai[0].ofs_indexes[(nquads * 2 + 0) * 3 + 2] = dstverts[2];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 0] = dstverts[0];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 1] = dstverts[2];
					gai[0].ofs_indexes[(nquads * 2 + 1) * 3 + 2] = dstverts[3];

					// setup texture coords
					gai[0].ofs_st_array[dstverts[0]][0] = 0;
					gai[0].ofs_st_array[dstverts[0]][1] = 0;

					gai[0].ofs_st_array[dstverts[1]][0] = 1;
					gai[0].ofs_st_array[dstverts[1]][1] = 0;

					gai[0].ofs_st_array[dstverts[2]][0] = 1;
					gai[0].ofs_st_array[dstverts[2]][1] = 1;

					gai[0].ofs_st_array[dstverts[3]][0] = 0;
					gai[0].ofs_st_array[dstverts[3]][1] = 1;

					nquads++;
				}
			}
		}
	}

	// generate normals and texture vectors
	modfuncs->AccumulateTextureVectors(gai[0].ofs_skel_xyz, gai[0].ofs_st_array, gai[0].ofs_skel_norm, gai[0].ofs_skel_svect, gai[0].ofs_skel_tvect, gai[0].ofs_indexes, gai[0].numindexes, true);
	modfuncs->NormaliseTextureVectors(gai[0].ofs_skel_norm, gai[0].ofs_skel_svect, gai[0].ofs_skel_tvect, gai[0].numverts, true);

	// allocate skin
	gai[0].numskins = 1;
	gai[0].ofsskins = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofsskins) * gai[0].numskins);
	gai[0].ofsskins[0].skinspeed = 0.1;
	gai[0].ofsskins[0].numframes = 1;
	gai[0].ofsskins[0].frame = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[0].ofsskins[0].frame));
	Q_snprintf(gai[0].ofsskins[0].name, sizeof(gai[0].ofsskins[0].name), "default");
	Q_snprintf(gai[0].ofsskins[0].frame[0].shadername, sizeof(gai[0].ofsskins[0].frame[0].shadername), "default");

	// setup other data
	Q_snprintf(gai[0].surfacename, sizeof(gai[0].surfacename), "default");
	gai[0].contents = FTECONTENTS_BODY;
	gai[0].geomset = ~0; //invalid set = always visible
	gai[0].geomid = 0;
	gai[0].mindist = 0;
	gai[0].maxdist = 0;
	gai[0].surfaceid = 0;
	gai[0].shares_verts = 0;
	gai[0].nextsurf = NULL;

	// finish setting up model
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;
	mod->numframes = 0;
#endif
	return true;
}

qboolean Plug_PSX_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	if (!modfuncs || !threadfuncs)
		return false;

	if (modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Tony Hawk's Pro Skater Level (.psx)", "\x04\x00\x02\x00", 4, Mod_LoadPSX);

	return true;
}
