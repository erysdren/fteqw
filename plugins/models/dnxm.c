/*
 * (c) erysdren 2025
 * license: GNU GPLv2
 */

#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"

#ifdef FTEPLUGIN
#undef COM_ParseOut
#define COM_ParseOut(d,o,l) cmdfuncs->ParseToken(d,o,l,NULL)
#undef ZG_Malloc
#define ZG_Malloc(g,s) plugfuncs->GMalloc(g,s)
#undef Z_Free
#define Z_Free(p) plugfuncs->Free(p)
#undef Z_Malloc
#define Z_Malloc(s) plugfuncs->Free(s)
#undef Z_Realloc
#define Z_Realloc(p,s) plugfuncs->Realloc(p,s)
#undef Mod_AccumulateTextureVectors
#define Mod_AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms) modfuncs->AccumulateTextureVectors(vc,tc,nv,sv,tv,idx,numidx,calcnorms)
#undef Mod_NormaliseTextureVectors
#define Mod_NormaliseTextureVectors(n,s,t,v,calcnorms) modfuncs->NormaliseTextureVectors(n,s,t,v,calcnorms)
#endif

static plugmodfuncs_t *modfuncs;

struct dnxmheader {
	uint32_t container_magic;
	uint32_t type_magic;
	int16_t container_version;
	int16_t type_version;
	uint32_t file_size;
	uint32_t ofs_entries;
	uint32_t num_entries;
	uint32_t user1;
	uint32_t user2;
};

struct dnxmchunk {
	uint32_t type;
	uint32_t offset;
	uint32_t size;
	uint8_t version;
	uint8_t reserved[3];
	char name[32];
};

struct dnxmvert {
	uint8_t group;
	uint8_t pos[3];
	uint8_t normal[3];
	uint8_t mount;
};

struct dnxmst {
	int16_t s;
	int16_t t;
};

struct dnxmframeinfo {
	float scale[16][3];
	float translate[16][3];
	float bbox[2][16][3];
};

struct dnxmrfrm {
	struct dnxmframeinfo frameinfo;
	int32_t num_verts;
	int32_t num_tris;
};

struct dnxmtri {
	uint16_t verts[3];
	uint16_t edges[3];
	uint16_t flags;
	uint16_t reserved;
};

struct dnxmskin {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	char filename[64];
};

static struct dnxmchunk *DNXM_FindChunkByName(const char *type, const char *name, struct dnxmchunk *chunks, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
	{
		if (memcmp(&chunks[i].type, type, 4) == 0)
			if (strncmp(chunks[i].name, name, sizeof(chunks[i].name)) == 0)
				return &chunks[i];
	}

	return NULL;
}

static struct dnxmchunk *DNXM_FindChunk(const char *type, struct dnxmchunk *chunks, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
	{
		if (memcmp(&chunks[i].type, type, 4) == 0)
			return &chunks[i];
	}

	return NULL;
}

static qboolean QDECL Mod_LoadDNXMModel(model_t *mod, void *buffer, size_t fsize)
{
	struct dnxmheader *header;
	struct dnxmchunk *entries;
	struct dnxmchunk *chunk;
	struct dnxmrfrm *rfrmchunk;
	struct dnxmvert *verts;
	struct dnxmst *uvs;
	uint8_t *triskins;
	uint32_t num_tris;
	uint32_t num_skins;
	struct dnxmtri *tris;
	struct dnxmskin *skins;
	galiasinfo_t *gai;
	int i, j, k;

	// get header
	header = (struct dnxmheader *)buffer;

	// sanity check
	if (memcmp(&header->container_magic, "ASCF", 4) != 0 || memcmp(&header->type_magic, "DNXM", 4) != 0)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// sanity check
	if (header->container_version != 3 || header->type_version != 5)
	{
		Con_Printf("%s: version not recognised\n", mod->name);
		return false;
	}

	// sanity check
	if (header->file_size != fsize)
	{
		Con_Printf("%s: file size mismatch\n", mod->name);
	}

	// get entries
	entries = (struct dnxmchunk *)((qbyte *)buffer + header->ofs_entries);

	// get reference frame chunk
	chunk = DNXM_FindChunk("RFRM", entries, header->num_entries);
	if (!chunk)
	{
		Con_Printf("%s: no RFRM chunk\n", mod->name);
		return false;
	}

	// get reference frame data
	rfrmchunk = (struct dnxmrfrm *)((qbyte *)buffer + chunk->offset);
	verts = (struct dnxmvert *)(rfrmchunk + 1);
	uvs = (struct dnxmst *)(verts + rfrmchunk->num_verts);
	triskins = (uint8_t *)(uvs + (rfrmchunk->num_tris * 3));

	// get tris chunk
	chunk = DNXM_FindChunk("TRIS", entries, header->num_entries);
	if (!chunk)
	{
		Con_Printf("%s: no TRIS chunk\n", mod->name);
		return false;
	}

	// get tris chunk data
	num_tris = *(uint32_t *)((qbyte *)buffer + chunk->offset);
	tris = (struct dnxmtri *)((qbyte *)buffer + chunk->offset + sizeof(uint32_t));

	// sanity check
	if (num_tris != rfrmchunk->num_tris)
	{
		Con_Printf("%s: num_tris mismatch %d != %d\n", mod->name, num_tris, rfrmchunk->num_tris);
		return false;
	}

	// get skins chunk
	chunk = DNXM_FindChunk("SKIN", entries, header->num_entries);
	if (!chunk)
	{
		Con_Printf("%s: no SKIN chunk\n", mod->name);
		return false;
	}

	// get skin chunk data
	num_skins = *(uint32_t *)((qbyte *)buffer + chunk->offset);
	skins = (struct dnxmskin *)((qbyte *)buffer + chunk->offset + sizeof(uint32_t));

	// one gai for each texture
	gai = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t) * num_skins);

	// count up the number of triangles and vertices in each surface
	for (i = 0; i < num_tris; i++)
	{
		// FIXME: this is lazy as hell please stop it
		// duplicating the hell out of each vertex? why
		gai[triskins[i]].numverts += 3;
		gai[triskins[i]].numindexes += 3;
	}

	// bounds
	ClearBounds(mod->mins, mod->maxs);

	// read in data
	for (i = 0; i < num_skins; i++)
	{
		int nverts, ntris;

		// allocate arrays
		gai[i].ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_xyz) * gai[i].numverts);
		gai[i].ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_st_array) * gai[i].numverts);
		gai[i].ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_norm) * gai[i].numverts);
		gai[i].ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_svect) * gai[i].numverts);
		gai[i].ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_tvect) * gai[i].numverts);

		gai[i].ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_indexes) * gai[i].numindexes);

		// copy position, coord, and index data
		nverts = ntris = 0;
		for (j = 0; j < num_tris; j++)
		{
			if (triskins[j] == i)
			{
				int srcverts[3];
				int dstverts[3];

				// copy vertex data and indices
				for (k = 0; k < 3; k++)
				{
					srcverts[k] = tris[j].verts[k];
					dstverts[k] = nverts++;

					// positions
					gai[i].ofs_skel_xyz[dstverts[k]][0] = rfrmchunk->frameinfo.scale[verts[srcverts[k]].group][0] * (float)verts[srcverts[k]].pos[0] + rfrmchunk->frameinfo.translate[verts[srcverts[k]].group][0];
					gai[i].ofs_skel_xyz[dstverts[k]][1] = -(rfrmchunk->frameinfo.scale[verts[srcverts[k]].group][2] * (float)verts[srcverts[k]].pos[2] + rfrmchunk->frameinfo.translate[verts[srcverts[k]].group][2]);
					gai[i].ofs_skel_xyz[dstverts[k]][2] = rfrmchunk->frameinfo.scale[verts[srcverts[k]].group][1] * (float)verts[srcverts[k]].pos[1] + rfrmchunk->frameinfo.translate[verts[srcverts[k]].group][1];

					// texture coords
					gai[i].ofs_st_array[dstverts[k]][0] = (float)uvs[j * 3 + k].s / (float)skins[i].width;
					gai[i].ofs_st_array[dstverts[k]][1] = (float)uvs[j * 3 + k].t / (float)skins[i].height;

					// normals
					gai[i].ofs_skel_norm[dstverts[k]][0] = (float)(verts[srcverts[k]].normal[0] & 0x7F) / ((verts[srcverts[k]].normal[0] >> 7) ? -127.0f : 127.0f);
					gai[i].ofs_skel_norm[dstverts[k]][1] = (float)(verts[srcverts[k]].normal[1] & 0x7F) / ((verts[srcverts[k]].normal[1] >> 7) ? -127.0f : 127.0f);
					gai[i].ofs_skel_norm[dstverts[k]][2] = (float)(verts[srcverts[k]].normal[2] & 0x7F) / ((verts[srcverts[k]].normal[2] >> 7) ? -127.0f : 127.0f);

					// indices
					gai[i].ofs_indexes[ntris * 3 + k] = dstverts[k];

					AddPointToBounds(gai[i].ofs_skel_xyz[dstverts[k]], mod->mins, mod->maxs);
				}

				ntris++;
			}
		}

		// generate normals and texture vectors
		Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, false);
		Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, false);

		// setup skin
		gai[i].numskins = 1;
		gai[i].ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins));
		gai[i].ofsskins[0].skinspeed = 0.1;
		gai[i].ofsskins[0].numframes = 1;
		gai[i].ofsskins[0].frame = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins[0].frame));
		Q_strlcpy(gai[i].ofsskins[0].name, skins[i].filename, sizeof(gai[i].ofsskins[0].name));
		Q_strlcpy(gai[i].ofsskins[0].frame[0].shadername, skins[i].filename, sizeof(gai[i].ofsskins[0].frame[0].shadername));

		// setup misc info
		Q_strlcpy(gai[i].surfacename, skins[i].filename, sizeof(gai[i].surfacename));
		gai[i].contents = FTECONTENTS_BODY;
		gai[i].geomset = ~0; //invalid set = always visible
		gai[i].geomid = 0;
		gai[i].mindist = 0;
		gai[i].maxdist = 0;
		gai[i].surfaceid = i;
		gai[i].shares_verts = i;
		gai[i].nextsurf = &gai[i+1];
	}

	// reset the last pointer so it doesnt point to nothing
	gai[i-1].nextsurf = NULL;

	// finish up
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;

	return true;
}

qboolean Plug_DNXM_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Duke Nukem Extended Model (mdx)", "ASCFDNXM", 8, Mod_LoadDNXMModel);
	return true;
}
