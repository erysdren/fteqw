/*
 * written by erysdren (it/its) in 2025
 *
 * KV6 format information sourced from slab6.txt written by Ken Silverman
 */
#include "voxlap.h"

#pragma pack(push, 1)
typedef struct kv6_header {
	uint32_t magic; // "Kvxl"
	uint32_t size_x; // width
	uint32_t size_y; // depth
	uint32_t size_z; // height
	float pivot_x; // in whole voxel units
	float pivot_y; // in whole voxel units
	float pivot_z; // in whole voxel units
	uint32_t num_slabs; // number of voxel slabs
} kv6_header_t;

typedef struct kv6_slab {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t dummy;
	uint16_t height; // height of slab
	uint8_t cull; // lower 6 bits tell which faces are exposed
	uint8_t normal; // index into normal table
} kv6_slab_t;

typedef struct kv6_palette {
	uint32_t magic; // "SPal"
	uint8_t colors[256][3];
} kv6_palette_t;
#pragma pack(pop)

static qboolean QDECL Mod_LoadKV6(model_t *mod, void *buffer, size_t fsize)
{
	int i, j, k, x, y, z;
	kv6_header_t *header;
	kv6_slab_t *slabs;
	kv6_slab_t *slab;
	uint32_t *num_slabs_x;
	uint16_t *num_slabs_xy;
	uint8_t *verts;
	kv6_palette_t *palette;
	galiasinfo_t *gai;
	const int lut[6][4][3] = {
		{{0,0,0}, {0,1,0}, {0,1,1}, {0,0,1}},
		{{1,1,0}, {1,0,0}, {1,0,1}, {1,1,1}},
		{{1,0,0}, {0,0,0}, {0,0,1}, {1,0,1}},
		{{0,1,0}, {1,1,0}, {1,1,1}, {0,1,1}},
		{{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}},
		{{0,1,1}, {1,1,1}, {1,0,1}, {0,0,1}}
	};

	// check magic
	if (memcmp(buffer, "Kvxl", 4) != 0)
	{
		Con_Printf("%s is not a KV6 model\n", mod->name);
		return false;
	}

	// setup kv6 data pointers
	header = (kv6_header_t *)buffer;
	slab = slabs = (kv6_slab_t *)(header + 1);
	num_slabs_x = (uint32_t *)(slabs + header->num_slabs);
	num_slabs_xy = (uint16_t *)(num_slabs_x + header->size_x);
	palette = (kv6_palette_t *)(num_slabs_xy + (header->size_x * header->size_y));

	// no palette
	if ((uint8_t *)palette >= ((uint8_t *)buffer + fsize))
		palette = NULL;

	// invalid palette identifier
	if ((uint8_t *)palette < ((uint8_t *)buffer + fsize) && memcmp(&palette->magic, "SPal", 4) != 0)
		palette = NULL;

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// allocate gai
	gai = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai));

	// allocate a bitvector array for detecting shared vertex positions
	verts = plugfuncs->Malloc(header->size_x * header->size_y * header->size_z);
	if (!verts)
	{
		Con_Printf(CON_ERROR"%s: memory allocation failed\n", mod->name);
		return false;
	}

	// clear it just to be safe
	memset(verts, 0, header->size_x * header->size_y * header->size_z);

	// run down each slab to collect how many faces and vertices we'll need
	for (x = 0; x < header->size_x; x++)
	{
		for (y = 0; y < header->size_y; y++)
		{
			for (j = 0; j < num_slabs_xy[x * header->size_y + y]; j++)
			{
				uint8_t *v;

				z = slab->height;

				v = verts + (x * (header->size_y * header->size_z) + y * (header->size_z) + z);

				for (k = 0; k < 6; k++)
				{
					if (!(slab->cull & (1 << k)))
						continue;
				}

#define ADD(b) if (slab->cull & (1 << b)) { gai->numverts += 4; gai->numindexes += 6; }
				ADD(0);
				ADD(1);
				ADD(2);
				ADD(3);
				ADD(4);
				ADD(5);
#undef ADD

				slab++;
			}
		}
	}

	// clean up
	plugfuncs->Free(verts);

	// should this be a fatal error?
	if (gai->numverts >= UINT16_MAX)
		Con_Printf(CON_WARNING"Warning: %s has over 65k verts, and will probably not work on most FTEQW builds!\n", mod->name);

	// allocate arrays
	gai->ofs_skel_xyz = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_xyz) * gai->numverts);
	gai->ofs_st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_st_array) * gai->numverts);
	gai->ofs_skel_norm = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_norm) * gai->numverts);
	gai->ofs_skel_svect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_svect) * gai->numverts);
	gai->ofs_skel_tvect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_skel_tvect) * gai->numverts);

	gai->ofs_indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofs_indexes) * gai->numindexes);

	// generate normals and texture vectors
	modfuncs->AccumulateTextureVectors(gai->ofs_skel_xyz, gai->ofs_st_array, gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->ofs_indexes, gai->numindexes, true);
	modfuncs->NormaliseTextureVectors(gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->numverts, true);

	// setup dummy texture
	gai->numskins = 1;
	gai->ofsskins = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofsskins) * gai->numskins);
	gai->ofsskins->skinspeed = 0.1;
	gai->ofsskins->numframes = 1;
	gai->ofsskins->frame = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai->ofsskins->frame));
	Q_strlcpy(gai->ofsskins->name, "default", sizeof(gai->ofsskins->name));
	Q_strlcpy(gai->ofsskins->frame[0].shadername, "default", sizeof(gai->ofsskins->frame[0].shadername));

	// setup gai
	Q_strlcpy(gai->surfacename, "default", sizeof(gai->surfacename));
	gai->contents = FTECONTENTS_BODY;
	gai->geomset = ~0; //invalid set = always visible
	gai->geomid = 0;
	gai->mindist = 0;
	gai->maxdist = 0;
	gai->surfaceid = 0;
	gai->shares_verts = 0;

	// finish up
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;
	mod->numframes = 0;
	return true;
}

qboolean KV6_Init(void)
{
	modfuncs->RegisterModelFormatMagic("Voxlap model (.kv6)", "Kvxl", 4, Mod_LoadKV6);
	// modfuncs->RegisterModelFormatText("Voxlap model (.kv6)", ".kv6", Mod_LoadKV6);
	return true;
}
