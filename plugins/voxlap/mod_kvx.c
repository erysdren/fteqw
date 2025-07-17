/*
 * written by erysdren (it/its) in 2025
 *
 * KVX format information sourced from slab6.txt written by Ken Silverman
 */
#include "voxlap.h"

#pragma pack(push, 1)
typedef struct kvx_header {
	int32_t len_mip_level; // byte size of mip level not including this value
	int32_t size_x; // width
	int32_t size_y; // depth
	int32_t size_z; // height
	int32_t pivot_x; // shifted up 8 bits
	int32_t pivot_y; // shifted up 8 bits
	int32_t pivot_z; // shifted up 8 bits
} kvx_header_t;

typedef struct kvx_slab {
	uint8_t top; // z coordinate of the top of the slab
	uint8_t height; // also the number of slabs in the color array
	uint8_t cull; // lower 6 bits tell which faces are exposed
	uint8_t colors[]; // [height] bytes in size, each value is a palette index
} kvx_slab_t;
#pragma pack(pop)

static qboolean QDECL Mod_LoadKVX(model_t *mod, void *buffer, size_t fsize)
{
	int i, j, k, x, y, z;
	kvx_header_t *kvx_header;
	int32_t *kvx_offsets_x;
	int16_t *kvx_offsets_xy;
	uint8_t *kvx_slabs_start;
	uint8_t *kvx_slabs_end;
	uint8_t *kvx_slabs_ptr;
	uint8_t *kvx_palette;
	galiasinfo_t *gai;

	// setup pointers to kvx data
	// we only care about the first mip level. there can be up to 5, but most
	// KVX files are stripped down to 1 anyway.
	kvx_header = (kvx_header_t *)buffer;
	kvx_offsets_x = (int32_t *)(kvx_header + 1);
	kvx_offsets_xy = (int16_t *)(kvx_offsets_x + (kvx_header->size_x + 1));
	kvx_slabs_ptr = kvx_slabs_start = (uint8_t *)(kvx_offsets_xy + (kvx_header->size_x * (kvx_header->size_y + 1)));
	kvx_slabs_end = kvx_slabs_start + kvx_header->len_mip_level - 24 - ((kvx_header->size_x + 1) * 4) - ((kvx_header->size_x * (kvx_header->size_y + 1)) * 2);
	kvx_palette = (uint8_t *)buffer + fsize - 768;

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// allocate gai
	gai = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai));

	// run down each slab to collect how many faces and vertices we'll need
	for (x = 0; x < kvx_header->size_x; x++)
	{
		for (y = 0; y < kvx_header->size_y; y++)
		{
			if (kvx_slabs_ptr >= kvx_slabs_end)
				return false;

			kvx_slab_t *slab = (kvx_slab_t *)kvx_slabs_ptr;

			int start_z = slab->top;

			z = 0;
			while (true)
			{
				if (z >= kvx_header->size_z)
					return false;

				slab = (kvx_slab_t *)kvx_slabs_ptr;

				Con_Printf("%dx%dx%d: top=%d height=%d cull=0x%02X\n", x, y, z, slab->top, slab->height, slab->cull);

				// probably reached next column
				if (slab->top < start_z)
					break;

				// skip slab header and color data
				kvx_slabs_ptr += 3 + slab->height;
				z += slab->height;
			}
		}
	}
escape:

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

qboolean KVX_Init(void)
{
	modfuncs->RegisterModelFormatText("Voxlap model (.kvx)", ".kvx", Mod_LoadKVX);
	return true;
}
