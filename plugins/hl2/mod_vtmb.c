#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"

static plugmodfuncs_t *modfuncs;
static plugfsfuncs_t *filefuncs;

struct vtmb_mdl_header {
	char magic[4];
	int32_t version;
	uint32_t checksum;
	char name[128];
	int32_t filesize;
	vec3_t scale;
	vec3_t eyeposition;
	vec3_t illumposition;
	vec3_t hullmin;
	vec3_t hullmax;
	vec3_t viewmin;
	vec3_t viewmax;
	uint32_t flags;
	uint32_t unknown[2];
	uint32_t num_bones;
	uint32_t ofs_bones;
	uint32_t num_bonecontrollers;
	uint32_t ofs_bonecontrollers;
	uint32_t num_hitboxsets;
	uint32_t ofs_hitboxsets;
	uint32_t num_localanims;
	uint32_t ofs_localamims;
	uint32_t num_sequences;
	uint32_t ofs_sequences;
	uint32_t num_localnodes;
	uint32_t ofs_localnodes;
	uint32_t ofs_localnodenames;
	uint32_t num_textures;
	uint32_t ofs_textures;
	uint32_t num_skinrefs;
	uint32_t num_skinfamilies;
	uint32_t ofs_skins;
	uint32_t num_bodies;
	uint32_t ofs_bodies;
	uint32_t num_localattachments;
	uint32_t ofs_localattachments;
	uint32_t num_flexdescs;
	uint32_t ofs_flexdescs;
	uint32_t num_flexcontrollers;
	uint32_t ofs_flexcontrollers;
	uint32_t num_flexrules;
	uint32_t ofs_flexrules;
	uint32_t num_ikchains;
	uint32_t ofs_ikchains;
	uint32_t num_mouths;
	uint32_t ofs_mouths;
	uint32_t num_localposeparameters;
	uint32_t ofs_localposeparameters;
	uint32_t num_localikautoplaylocks;
	uint32_t ofs_localikautoplaylocks;
	uint32_t ofs_animblockname;
	uint32_t num_flexcontrolleruis;
	uint32_t ofs_flexcontrolleruis;
	uint32_t num_includes;
	uint32_t ofs_includes;
	uint32_t ofs_surfaceprop;
	uint32_t ofs_bonetablename;
	uint32_t num_animblocks;
	uint32_t ofs_animblocks;
};

static qboolean QDECL Mod_LoadVTMBModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai;
	int i, j, k;
	struct vtmb_mdl_header *header = (struct vtmb_mdl_header *)buffer;
	char base[MAX_QPATH];
	qbyte *vtx = NULL;
	size_t vtxsize = 0;
	const char *vtxpostfixes[] = {
		".dx80.vtx",
		".dx7_2bone.vtx",
		".vtx"
	};

	// sanity check
	if (memcmp(header->magic, "IDST", 4) != 0 || header->version != 2531)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// load vtx
	for (i = 0; !vtx && i < countof(vtxpostfixes); i++)
	{
		modfuncs->StripExtension(mod->name, base, sizeof(base));
		Q_strncatz(base, vtxpostfixes[i], sizeof(base));
		vtx = filefuncs->LoadFile(base, &vtxsize);
	}

#if 0
	// skip preview image
	if (bufptr[3] > 2)
	{
		uint32_t w, h;
		w = *(uint32_t *)(bufptr + 4);
		h = *(uint32_t *)(bufptr + 8);
		bufptr += 12 + (w * h * 4);
	}
	else
	{
		bufptr += 4;
	}

	// get counts
	num_wads = *(uint32_t *)bufptr; bufptr += 4;
	num_entities = *(uint32_t *)bufptr; bufptr += 4;
	num_textures = *(uint32_t *)bufptr; bufptr += 4;

	// skip past wads
	for (i = 0; i < num_wads; i++)
	{
		char name[MAX_QPATH];
		strncpy(name, bufptr, sizeof(name));
		bufptr += strlen(name) + 1;
	}

	// allocate skins
	skins = ZG_Malloc(&mod->memgroup, sizeof(*skins) * num_textures);

	// read in skins
	// TODO: should add the video/ prefix for these, since the engine won't know to look there?
	for (i = 0; i < num_textures; i++)
	{
		char name[MAX_QPATH];
		strncpy(name, bufptr, sizeof(name));
		bufptr += strlen(name) + 1;

		skins[i].skinspeed = 0.1;
		skins[i].numframes = 1;
		skins[i].frame = ZG_Malloc(&mod->memgroup, sizeof(*skins[i].frame));
		Q_snprintfz(skins[i].name, sizeof(skins[i].name), "video/%s", name);
		Q_snprintfz(skins[i].frame[0].shadername, sizeof(skins[i].frame[0].shadername), "video/%s", name);
	}

	// allocate one gai for each texture
	gai = ZG_Malloc(&mod->memgroup, sizeof(*gai) * num_textures);

	// clear bounds
	ClearBounds(mod->mins, mod->maxs);

	// allocate entities text buffer that is hopefully big enough
	// TODO: dont be stupid, calculate this from the number of entities
	len_entities = num_entities * 256;
	entities = Z_Malloc(len_entities);
	entptr = entities;

	// read entities
	for (i = 0; i < num_entities; i++)
	{
		// read classname
		uint32_t num_properties, num_polygons;
		char classname[MAX_QPATH];
		strncpy(classname, bufptr, sizeof(classname));
		bufptr += strlen(classname) + 1;

		entptr += snprintf(entptr, len_entities - (entptr - entities), "{\"classname\"\"%s\"", classname);

		// read properties
		num_properties = *(uint32_t *)bufptr; bufptr += 4;
		for (j = 0; j < num_properties; j++)
		{
			char key[MAX_QPATH];
			char val[MAX_QPATH];
			strncpy(key, bufptr, sizeof(key));
			bufptr += strlen(key) + 1;
			strncpy(val, bufptr, sizeof(val));
			bufptr += strlen(val) + 1;

			entptr += snprintf(entptr, len_entities - (entptr - entities), "\"%s\"\"%s\"", key, val);
		}

		*entptr++ = '}';

		// read polygons
		num_polygons = *(uint32_t *)bufptr; bufptr += 4;
		if (num_polygons)
		{
			totalpolys += num_polygons;
			polygons = Z_Realloc(polygons, sizeof(*polygons) * totalpolys);
			for (j = 0; j < num_polygons; j++)
			{
				uint32_t texture;
				uint32_t num_vertices;

				polygons[totalpolys - num_polygons + j] = bufptr;

				texture = *(uint32_t *)bufptr; bufptr += 4;
				bufptr += 32; // skip plane

				// read vertices
				num_vertices = *(uint32_t *)bufptr; bufptr += 4;
				gai[texture].numverts += num_vertices;
				gai[texture].numindexes += 3 * (num_vertices - 2);
				bufptr += num_vertices * 40;
			}
		}
	}

	// load entities
	modfuncs->LoadEntities(mod, entities, entptr - entities);
	Z_Free(entities);

	// read in polygons
	for (i = 0; i < num_textures; i++)
	{
		int nverts, ntris;

		// allocate arrays
		gai[i].ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_xyz) * gai[i].numverts);
		gai[i].ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_st_array) * gai[i].numverts);
		gai[i].ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_norm) * gai[i].numverts);
		gai[i].ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_svect) * gai[i].numverts);
		gai[i].ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_skel_tvect) * gai[i].numverts);

		gai[i].ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofs_indexes) * gai[i].numindexes);

		// read in our vertex positions and uvs
		nverts = ntris = 0;
		for (j = 0; j < totalpolys; j++)
		{
			uint32_t texture, num_vertices;
			qbyte *ptr = polygons[j];
			texture = *(uint32_t *)ptr; ptr += 4;
			if (texture == i)
			{
				ptr += 32; // skip plane
				num_vertices = *(uint32_t *)ptr; ptr += 4;

				// make triangle fan
				for (k = 0; k < num_vertices - 2; k++)
				{
					gai[i].ofs_indexes[ntris * 3 + 0] = nverts;
					gai[i].ofs_indexes[ntris * 3 + 1] = nverts + k + 1;
					gai[i].ofs_indexes[ntris * 3 + 2] = nverts + k + 2;
					ntris++;
					totaltris++;
				}

				// make verts
				for (k = 0; k < num_vertices; k++)
				{
					double *v = (double *)ptr; ptr += 40;
					gai[i].ofs_skel_xyz[nverts][0] = v[0];
					gai[i].ofs_skel_xyz[nverts][1] = v[2];
					gai[i].ofs_skel_xyz[nverts][2] = v[1];
					gai[i].ofs_st_array[nverts][0] = v[3];
					gai[i].ofs_st_array[nverts][1] = v[4];
					AddPointToBounds(gai[i].ofs_skel_xyz[nverts], mod->mins, mod->maxs);
					nverts++;
				}
			}
		}

		// generate normals and texture vectors
		Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, true);
		Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, true);

		// setup skin
		gai[i].numskins = 1;
		gai[i].ofsskins = &skins[i];

		// setup misc info
		Q_strlcpy(gai[i].surfacename, skins[i].name, sizeof(gai[i].surfacename));
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

	// build BIH tree
	bihleaf = Z_Malloc(sizeof(*bihleaf) * totaltris);
	for (i = 0; i < num_textures; i++)
	{
		for (j = 0; j < gai[i].numindexes; j += 3)
		{
			bihleaf[totalleafs].type = BIH_TRIANGLE;
			bihleaf[totalleafs].data.contents = FTECONTENTS_BODY;
			bihleaf[totalleafs].data.tri.indexes = &gai[i].ofs_indexes[j];
			bihleaf[totalleafs].data.tri.xyz = gai[i].ofs_skel_xyz;
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 0]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 1]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			AddPointToBounds(gai[i].ofs_skel_xyz[gai[i].ofs_indexes[j + 2]], bihleaf[totalleafs].mins, bihleaf[totalleafs].maxs);
			totalleafs++;
		}
	}
	modfuncs->BIH_Build(mod, bihleaf, totaltris);
	Z_Free(bihleaf);

	// set funcs
	mod->funcs.FatPVS = CMF_FatPVS;
	mod->funcs.EdictInFatPVS = CMF_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = CMF_FindTouchedLeafs;
	mod->funcs.LightPointValues = CMF_LightPointValues;
	mod->funcs.ClusterForPoint = CMF_ClusterForPoint;
	mod->funcs.ClusterPVS = CMF_ClusterPVS;
#endif

	// finish up
	mod->meshinfo = gai;
	mod->type = mod_alias;
	mod->flags = 0;

	return true;
}

qboolean VTMB_MDL_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs || !modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Source model (VTMB)", "IDST\xe3\x09\0\0", 8, Mod_LoadVTMBModel);
	return true;
}
