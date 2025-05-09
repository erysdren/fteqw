/*
 * author: erysdren
 * written: may 2025
 * purpose: Vampire: The Masquerade - Bloodlines MDL file loader
 * license: GNU GPLv2
 *
 * i know that this is a lot of duplicated work from mod_hl2.c, but i thought
 * it'd be easier to do this rather than hacking the Source MDL loader to
 * support this confusing mess.
 */

#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"

static plugmodfuncs_t *modfuncs;
static plugfsfuncs_t *filefuncs;

#pragma pack(push, 1)
typedef struct vtmb_vtx_header {
	uint32_t version;
	uint32_t vertcachesize;
	uint16_t maxbonesperstrip;
	uint16_t maxbonespertri;
	uint32_t maxbonespervert;
	uint32_t checksum;
	uint32_t num_lods;
	uint32_t ofs_materialreplacements;
	uint32_t num_bodies;
	uint32_t ofs_bodies;
} vtmb_vtx_header_t;

typedef struct vtmb_mdl_header {
	char magic[4];
	uint32_t version;
	uint32_t checksum;
	char name[128];
	uint32_t filesize;
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
	uint32_t num_anims;
	uint32_t ofs_anims;
	uint32_t num_sequences;
	uint32_t ofs_sequences;
	uint32_t sequences_indexed;
	uint32_t num_sequencegroups;
	uint32_t ofs_sequencegroups;
	uint32_t num_textures;
	uint32_t ofs_textures;
	uint32_t num_texpaths;
	uint32_t ofs_texpaths;
	uint32_t num_skinrefs;
	uint32_t num_skinfamilies;
	uint32_t ofs_skinrefs;
	uint32_t num_bodies;
	uint32_t ofs_bodies;
} vtmb_mdl_header_t;

typedef struct vtmb_mdl_texture {
	uint32_t ofs_name;
	uint32_t flags;
	float width;
	float height;
	float unknown;
} vtmb_mdl_texture_t;

typedef struct vtmb_mdl_body {
	uint32_t ofs_name;
	uint32_t num_models;
	uint32_t base;
	uint32_t ofs_models;
} vtmb_mdl_body_t;

typedef struct vtmb_mdl_model {
	char name[128];
	uint32_t type;
	float bounding_radius;
	uint32_t num_meshes;
	uint32_t ofs_meshes;
	uint32_t num_vertices;
	uint32_t ofs_vertices;
	uint32_t ofs_tangents;
	uint32_t filetype;
	vec3_t unknown1[2];
	uint32_t unknown2[5];
	uint32_t num_attachments;
	uint32_t ofs_attachments;
	uint32_t num_eyeballs;
	uint32_t ofs_eyeballs;
} vtmb_mdl_model_t;

typedef struct vtmb_mdl_mesh {
	uint32_t material;
	uint32_t ofs_model;
	uint32_t num_vertices;
	uint32_t ofs_vertices;
	uint32_t num_flexes;
	uint32_t ofs_flexes;
	uint32_t materialtype;
	uint32_t materialparam;
	uint32_t id;
	vec3_t center;
	uint32_t unknown[3];
} vtmb_mdl_mesh_t;

typedef struct vtmb_mdl_vertex {
	uint8_t weights[3];
	uint16_t bones[3];
	vec3_t position;
	vec3_t normal;
	vec2_t st;
} vtmb_mdl_vertex_t;

typedef struct vtmb_vtx_body {
	uint32_t num_models;
	uint32_t ofs_models;
} vtmb_vtx_body_t;

typedef struct vtmb_vtx_model {
	uint32_t num_lods;
	uint32_t ofs_lods;
} vtmb_vtx_model_t;

typedef struct vtmb_vtx_lod {
	uint32_t num_meshes;
	uint32_t ofs_meshes;
	float dist;
} vtmb_vtx_lod_t;

typedef struct vtmb_vtx_mesh {
	uint32_t num_stripgroups;
	uint32_t ofs_stripgroups;
	uint8_t flags;
} vtmb_vtx_mesh_t;

typedef struct vtmb_vtx_stripgroup {
	uint32_t num_vertices;
	uint32_t ofs_vertices;
	uint32_t num_indices;
	uint32_t ofs_indices;
	uint32_t num_strips;
	uint32_t ofs_strips;
	uint8_t flags;
	uint32_t num_topologyindices;
	uint32_t ofs_topologyindices;
} vtmb_vtx_stripgroup_t;

typedef struct vtmb_vtx_strip {
	uint32_t num_indices;
	uint32_t ofs_indices;
	uint32_t num_vertices;
	uint32_t ofs_vertices;
	uint16_t num_bones;
	uint8_t flags;
	uint32_t num_bonestatechanges;
	uint32_t ofs_bonestatechanges;
} vtmb_vtx_strip_t;

typedef struct vtmb_vtx_vertex {
	uint8_t boneweights[3];
	uint8_t num_bones;
	uint16_t vertex_id;
	uint8_t bone_ids[3];
} vtmb_vtx_vertex_t;
#pragma pack(pop)

#define LittleVec3(v) \
	v[0] = LittleFloat(v[0]); \
	v[1] = LittleFloat(v[1]); \
	v[2] = LittleFloat(v[2]);

static qboolean QDECL Mod_LoadVTMBModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *gai;
	int i, j, k, v;
	vtmb_mdl_header_t *mdl = (vtmb_mdl_header_t *)buffer;
	vtmb_vtx_header_t *vtx = NULL;
	vtmb_mdl_body_t *bodies = NULL;
	vtmb_vtx_body_t *vtxbodies = NULL;
	vtmb_mdl_texture_t *textures = NULL;
	size_t totalmeshes = 0;
	size_t currentmesh = 0;
	char vtxname[MAX_QPATH];
	size_t vtxsize = 0;
	const char *vtxpostfixes[] = {
		".dx80.vtx",
		".dx7_2bone.vtx",
		".vtx"
	};

	// sanity check
	if (memcmp(mdl->magic, "IDST", 4) != 0 || LittleLong(mdl->version) != 2531)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// sanity check
	if (LittleLong(mdl->filesize) != fsize)
	{
		Con_Printf("%s: file size mismatch\n", mod->name);
		return false;
	}

	// load vtx
	for (i = 0; !vtx && i < countof(vtxpostfixes); i++)
	{
		modfuncs->StripExtension(mod->name, vtxname, sizeof(vtxname));
		Q_strncatz(vtxname, vtxpostfixes[i], sizeof(vtxname));
		vtx = (vtmb_vtx_header_t *)filefuncs->LoadFile(vtxname, &vtxsize);
	}

	// sanity check
	if (!vtx)
	{
		Con_Printf("%s: failed to load vtx\n", mod->name);
		return false;
	}

	// sanity check
	if (LittleLong(vtx->version) != 107)
	{
		Con_Printf("%s: format not recognised\n", vtxname);
		plugfuncs->Free(vtx);
		return false;
	}

	// sanity check
	if (LittleLong(mdl->checksum) != LittleLong(vtx->checksum))
	{
		Con_Printf("%s: mdl and vtx checksum mismatch\n", mod->name);
		plugfuncs->Free(vtx);
		return false;
	}

	// sanity check
	if (LittleLong(mdl->num_bodies) != LittleLong(vtx->num_bodies))
	{
		Con_Printf("%s: mdl and vtx num_bodies mismatch\n", mod->name);
		plugfuncs->Free(vtx);
		return false;
	}

	// endian swap mdl header
	mdl->version = LittleLong(mdl->version);
	mdl->checksum = LittleLong(mdl->checksum);
	mdl->filesize = LittleLong(mdl->filesize);
	LittleVec3(mdl->scale);
	LittleVec3(mdl->eyeposition);
	LittleVec3(mdl->illumposition);
	LittleVec3(mdl->hullmin);
	LittleVec3(mdl->hullmax);
	LittleVec3(mdl->viewmin);
	LittleVec3(mdl->viewmax);
	mdl->flags = LittleLong(mdl->flags);
	mdl->unknown[0] = LittleLong(mdl->unknown[0]);
	mdl->unknown[1] = LittleLong(mdl->unknown[1]);
	mdl->num_bones = LittleLong(mdl->num_bones);
	mdl->ofs_bones = LittleLong(mdl->ofs_bones);
	mdl->num_bonecontrollers = LittleLong(mdl->num_bonecontrollers);
	mdl->ofs_bonecontrollers = LittleLong(mdl->ofs_bonecontrollers);
	mdl->num_hitboxsets = LittleLong(mdl->num_hitboxsets);
	mdl->ofs_hitboxsets = LittleLong(mdl->ofs_hitboxsets);
	mdl->num_anims = LittleLong(mdl->num_anims);
	mdl->ofs_anims = LittleLong(mdl->ofs_anims);
	mdl->num_sequences = LittleLong(mdl->num_sequences);
	mdl->ofs_sequences = LittleLong(mdl->ofs_sequences);
	mdl->sequences_indexed = LittleLong(mdl->sequences_indexed);
	mdl->num_sequencegroups = LittleLong(mdl->num_sequencegroups);
	mdl->ofs_sequencegroups = LittleLong(mdl->ofs_sequencegroups);
	mdl->num_textures = LittleLong(mdl->num_textures);
	mdl->ofs_textures = LittleLong(mdl->ofs_textures);
	mdl->num_texpaths = LittleLong(mdl->num_texpaths);
	mdl->ofs_texpaths = LittleLong(mdl->ofs_texpaths);
	mdl->num_skinrefs = LittleLong(mdl->num_skinrefs);
	mdl->num_skinfamilies = LittleLong(mdl->num_skinfamilies);
	mdl->ofs_skinrefs = LittleLong(mdl->ofs_skinrefs);
	mdl->num_bodies = LittleLong(mdl->num_bodies);
	mdl->ofs_bodies = LittleLong(mdl->ofs_bodies);

	// endian swap vtx header
	vtx->version = LittleLong(vtx->version);
	vtx->vertcachesize = LittleLong(vtx->vertcachesize);
	vtx->maxbonesperstrip = LittleShort(vtx->maxbonesperstrip);
	vtx->maxbonespertri = LittleShort(vtx->maxbonespertri);
	vtx->maxbonespervert = LittleLong(vtx->maxbonespervert);
	vtx->checksum = LittleLong(vtx->checksum);
	vtx->num_lods = LittleLong(vtx->num_lods);
	vtx->ofs_materialreplacements = LittleLong(vtx->ofs_materialreplacements);
	vtx->num_bodies = LittleLong(vtx->num_bodies);
	vtx->ofs_bodies = LittleLong(vtx->ofs_bodies);

	// get pointers
	// TODO: endian
	textures = (vtmb_mdl_texture_t *)((qbyte *)mdl + mdl->ofs_textures);
	bodies = (vtmb_mdl_body_t *)((qbyte *)mdl + mdl->ofs_bodies);
	vtxbodies = (vtmb_vtx_body_t *)((qbyte *)vtx + vtx->ofs_bodies);

	// parse vtx bodies to count up meshes
	for (i = 0; i < vtx->num_bodies; i++)
	{
		// parse models
		vtmb_vtx_model_t *vtxmodels = (vtmb_vtx_model_t *)((qbyte *)&vtxbodies[i] + vtxbodies[i].ofs_models);
		for (j = 0; j < vtxbodies[i].num_models; j++)
		{
			// TODO: only focusing on on the first LOD for now
			vtmb_vtx_lod_t *vtxlods = (vtmb_vtx_lod_t *)((qbyte *)&vtxmodels[j] + vtxmodels[j].ofs_lods);
			totalmeshes += vtxlods[0].num_meshes;
		}
	}

	// sanity check
	if (!totalmeshes)
	{
		Con_Printf("%s: no meshes in model\n", mod->name);
		plugfuncs->Free(vtx);
		return false;
	}

	// allocate surfaces
	gai = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai) * totalmeshes);

	// parse vtx bodies
	for (i = 0; i < vtx->num_bodies; i++)
	{
		// parse models
		vtmb_vtx_model_t *vtxmodels = (vtmb_vtx_model_t *)((qbyte *)&vtxbodies[i] + vtxbodies[i].ofs_models);
		for (j = 0; j < vtxbodies[i].num_models; j++)
		{
			// TODO: only focusing on on the first LOD for now
			vtmb_vtx_lod_t *vtxlods = (vtmb_vtx_lod_t *)((qbyte *)&vtxmodels[j] + vtxmodels[j].ofs_lods);
			vtmb_vtx_mesh_t *vtxmeshes = (vtmb_vtx_mesh_t *)((qbyte *)&vtxlods[0] + vtxlods[0].ofs_meshes);

#if 0
			for (k = 0; k < models[j].num_meshes; k++)
			{
				vtmb_mdl_vertex_t *vertices = (vtmb_mdl_vertex_t *)((qbyte *)&meshes[k] + meshes[k].ofs_vertices);
				gai[currentmesh].numverts = meshes[k].num_vertices;
				gai[currentmesh].numindexes = meshes[k].num_vertices * 3;

				// allocate arrays
				gai[currentmesh].ofs_skel_xyz = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_skel_xyz) * gai[currentmesh].numverts);
				gai[currentmesh].ofs_st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_st_array) * gai[currentmesh].numverts);
				gai[currentmesh].ofs_skel_norm = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_skel_norm) * gai[currentmesh].numverts);
				gai[currentmesh].ofs_skel_svect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_skel_svect) * gai[currentmesh].numverts);
				gai[currentmesh].ofs_skel_tvect = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_skel_tvect) * gai[currentmesh].numverts);

				gai[currentmesh].ofs_indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofs_indexes) * gai[currentmesh].numindexes);

				// read in vertices
				for (v = 0; v < gai[currentmesh].numverts; v++)
				{
					gai[currentmesh].ofs_skel_xyz[v][0] = vertices[v].position[0];
					gai[currentmesh].ofs_skel_xyz[v][1] = vertices[v].position[1];
					gai[currentmesh].ofs_skel_xyz[v][2] = vertices[v].position[2];
					gai[currentmesh].ofs_skel_norm[v][0] = vertices[v].normal[0];
					gai[currentmesh].ofs_skel_norm[v][1] = vertices[v].normal[1];
					gai[currentmesh].ofs_skel_norm[v][2] = vertices[v].normal[2];
					gai[currentmesh].ofs_st_array[v][0] = vertices[v].st[0];
					gai[currentmesh].ofs_st_array[v][1] = vertices[v].st[1];
				}

				// generate normals and texture vectors
				modfuncs->AccumulateTextureVectors(gai[currentmesh].ofs_skel_xyz, gai[currentmesh].ofs_st_array, gai[currentmesh].ofs_skel_norm, gai[currentmesh].ofs_skel_svect, gai[currentmesh].ofs_skel_tvect, gai[currentmesh].ofs_indexes, gai[currentmesh].numindexes, false);
				modfuncs->NormaliseTextureVectors(gai[currentmesh].ofs_skel_norm, gai[currentmesh].ofs_skel_svect, gai[currentmesh].ofs_skel_tvect, gai[currentmesh].numverts, false);

				// setup phony skin
				gai[currentmesh].numskins = 1;
				gai[currentmesh].ofsskins = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofsskins) * gai[currentmesh].numskins);
				gai[currentmesh].ofsskins[0].skinspeed = 0.1;
				gai[currentmesh].ofsskins[0].numframes = 1;
				gai[currentmesh].ofsskins[0].frame = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gai[currentmesh].ofsskins[0].frame));
				Q_snprintfz(gai[currentmesh].ofsskins[0].name, sizeof(gai[currentmesh].ofsskins[0].name), "default");
				Q_snprintfz(gai[currentmesh].ofsskins[0].frame[0].shadername, sizeof(gai[currentmesh].ofsskins[0].frame[0].shadername), "default");

				// finish setting up
				Q_snprintfz(gai[currentmesh].surfacename, sizeof(gai[currentmesh].surfacename), "default");
				gai[currentmesh].contents = FTECONTENTS_BODY;
				gai[currentmesh].geomset = ~0; //invalid set = always visible
				gai[currentmesh].geomid = 0;
				gai[currentmesh].mindist = 0;
				gai[currentmesh].maxdist = 0;
				gai[currentmesh].surfaceid = currentmesh;
				gai[currentmesh].shares_verts = currentmesh;
				gai[currentmesh].nextsurf = &gai[currentmesh+1];

				currentmesh++;
			}
#endif
		}
	}

	// reset the last pointer so it doesnt point to nothing
	gai[totalmeshes-1].nextsurf = NULL;

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

	// clean up
	plugfuncs->Free(vtx);

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

	modfuncs->RegisterModelFormatMagic("Source model (VTMB)", "IDST\xe3\x09\x00\x00", 8, Mod_LoadVTMBModel);
	return true;
}
