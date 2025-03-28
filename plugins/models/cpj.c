/*
 * (c) erysdren 2025
 * license: GNU GPLv2
 */

#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"

#ifdef SKELETALMODELS
#define CPJMODELS
#endif

#ifdef CPJMODELS

#ifdef FTEPLUGIN
#undef COM_ParseOut
#define COM_ParseOut(d,o,l) cmdfuncs->ParseToken(d,o,l,NULL)
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

float RadiusFromBounds (const vec3_t mins, const vec3_t maxs)
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

static plugmodfuncs_t *modfuncs;
static plugfsfuncs_t *filefuncs;

struct cpjfileheader {
	uint32_t riff_magic;
	uint32_t len_file;
	uint32_t file_magic;
};

struct cpjchunkheader {
	uint32_t magic;
	uint32_t len_chunk;
	uint32_t version;
	uint32_t timestamp;
	uint32_t ofs_name;
};

struct cpjmacsection {
	uint32_t ofs_name;
	uint32_t num_commands;
	uint32_t first_command;
};

struct cpjmac {
	uint32_t num_sections;
	uint32_t ofs_sections;
	uint32_t num_commands;
	uint32_t ofs_commands;
};

enum {
	CPJ_GEOVF_LODLOCK = 1 // vertex is locked during LOD processing
};

struct cpjgeovert {
	uint8_t flags; // CPJ_GEOVF_ vertex flags
	uint8_t compression_group_index; // group index for vertex frame compression
	uint16_t reserved; // reserved for future use, must be zero
	uint16_t num_edge_links; // number of edges linked to this vertex
	uint16_t num_tri_links; // number of triangles linked to this vertex
	uint32_t first_edge_link; // first edge index in object link array
	uint32_t first_tri_link; // first triangle index in object link array
	vec3_t position; // reference position of vertex
};

struct cpjgeoedge {
	uint16_t head_vertex; // vertex list index of edge's head vertex
	uint16_t tail_vertex; // vertex list index of edge's tail vertex
	uint16_t inverted_edge; // edge list index of inverted mirror edge
	uint16_t num_tri_links; // number of triangles linked to this edge
	uint32_t first_tri_link; // first triangle index in object link array
};

struct cpjgeotri {
	uint16_t edge_ring[3]; // edge list indices used by triangle, whose tail vertices are V0, V1, and V2, in order
	uint16_t reserved; // reserved for future use, must be zero
};

struct cpjgeomount {
	uint32_t ofs_name; // offset of mount point name string in data block
	uint32_t tri_index; // triangle index of mount base
	vec3_t tri_barys; // barycentric coordinates of mount origin
	vec3_t base_scale; // base transform scaling
	vec4_t base_rotate; // base transform rotation quaternion
	vec3_t base_translate; // base transform translation
};

struct cpjgeo {
	// vertices (array of cpjgeovert)
	uint32_t num_vertices; // number of vertices
	uint32_t ofs_vertices; // offset of vertices in data block
	// edges (array of cpjgeoedge)
	uint32_t num_edges; // number of edges
	uint32_t ofs_edges; // offset of edges in data block
	// triangles (array of cpjgeotri)
	uint32_t num_tris; // number of triangles
	uint32_t ofs_tris; // offset of triangles in data block
	// mount points (array of cpjgeomount)
	uint32_t num_mounts; // number of mounts
	uint32_t ofs_mounts; // offset of mounts in data block
	// object links (array of uint16_t)
	uint32_t num_obj_links; // number of object links
	uint32_t ofs_obj_links; // number of object links in data block
};

struct cpjsrftex {
	uint32_t ofs_name; // offset of texture name string in data block
	uint32_t ofs_refname; // offset of optional reference name in block
};

enum {
	CPJ_SRFTF_INACTIVE = 1 << 0, // triangle is not active
	CPJ_SRFTF_HIDDEN = 1 << 1, // present but invisible
	CPJ_SRFTF_VNIGNORE = 1 << 2, // ignored in vertex normal calculations
	CPJ_SRFTF_TRANSPARENT = 1 << 3, // transparent rendering is enabled
	CPJ_SRFTF_SPECULAR = 1 << 4, // triangle is specular mapped
	CPJ_SRFTF_UNLIT = 1 << 5, // not affected by dynamic lighting
	CPJ_SRFTF_TWOSIDED = 1 << 6, // visible from both sides
	CPJ_SRFTF_MASKING = 1 << 7, // color key masking is active
	CPJ_SRFTF_MODULATED = 1 << 8, // modulated rendering is enabled
	CPJ_SRFTF_ENVMAP = 1 << 9, // environment mapped
	CPJ_SRFTF_NONCOLLIDE = 1 << 10, // shouldn't collide with world ray traces
	CPJ_SRFTF_TEXBLEND = 1 << 11, // triangle uses alpha blending
	CPJ_SRFTF_ZLATER = 1 << 12, // unknown
	CPJ_SRFTF_RESERVED = 1 << 16 // unknown
};

struct cpjsrftri {
	uint16_t uv_index[3]; // UV texture coordinate indices used
	uint8_t tex_index; // surface texture index
	uint8_t reserved; // reserved for future use, must be zero
	uint32_t flags; // CPJ_SRFTF_ triangle flags
	uint8_t smooth_group; // light smoothing group
	uint8_t alpha_level; // transparent/modulated alpha level
	uint8_t glaze_tex_index; // second-pass glaze texture index if used
	uint8_t glaze_func; // second-pass glaze function
};

struct cpjsrfuv {
	float u; // texture U coordinate
	float v; // texture V coordinate
};

struct cpjsrf {
	// textures (array of cpjsrftex)
	uint32_t num_textures; // number of textures
	uint32_t ofs_textures; // offset of textures in data block
	// triangles (array of cpjsrftri)
	uint32_t num_tris; // number of triangles
	uint32_t ofs_tris; // offset of triangles in data block
	// UV texture coordinates (array of cpjsrfuv)
	uint32_t num_uv; // number of UV texture coordinates
	uint32_t ofs_uv; // offset of UV texture coordinates in data block
};

static struct cpjchunkheader *CPJ_FindChunk(const char *magic, const char *buffer, size_t fsize)
{
	char *ptr, *end;

	ptr = (char *)buffer;
	end = ptr + fsize;

	while (ptr < end)
	{
		struct cpjchunkheader *h = (struct cpjchunkheader *)ptr;

		if (memcmp(&h->magic, magic, 4) == 0)
			return h;

		ptr += h->len_chunk + (h->len_chunk % 2) + (sizeof(uint32_t) * 2);
	}

	return NULL;
}

static struct cpjchunkheader *CPJ_FindChunkByName(const char *magic, const char *name, const char *buffer, size_t fsize)
{
	char *ptr, *end;

	ptr = (char *)buffer;
	end = ptr + fsize;

	while (ptr < end)
	{
		struct cpjchunkheader *h = (struct cpjchunkheader *)ptr;

		if (memcmp(&h->magic, magic, 4) == 0)
			if (strcmp(ptr + h->ofs_name, name) == 0)
				return h;

		ptr += h->len_chunk + (h->len_chunk % 2) + (sizeof(uint32_t) * 2);
	}

	return NULL;
}

static galiasinfo_t *Mod_ParseCpjModel(model_t *mod, const char *buffer, size_t fsize)
{
	const char *chunkstart = NULL;
	size_t chunksize = 0;
	struct cpjchunkheader *ch = NULL;
	struct cpjfileheader *h = NULL;
	struct cpjmac *mac = NULL;
	struct cpjmacsection *macsections = NULL;
	struct cpjmacsection *autoexec = NULL;
	char *macdatablock = NULL;
	struct cpjgeo *geo = NULL;
	struct cpjgeovert *geoverts = NULL;
	struct cpjgeotri *geotris = NULL;
	struct cpjgeoedge *geoedges = NULL;
	char *geodatablock = NULL;
	struct cpjsrf *srf = NULL;
	struct cpjsrftex *srftex = NULL;
	struct cpjsrftri *srftris = NULL;
	struct cpjsrfuv *srfuv = NULL;
	char *srfdatablock = NULL;
	uint32_t *maccommands = NULL;
	galiasinfo_t *gai = NULL;
	int i = 0, j = 0, k = 0;

	qboolean internalgeo = false;
	qboolean internalsrf = false;
	qboolean internalskl = false;
	qboolean internalfrm = false;
	qboolean internalseq = false;

	char geoname[256];
	char srfname[256];
	char sklname[256];

	int srfindex = 0;

	h = (struct cpjfileheader *)buffer;

	if (memcmp(&h->riff_magic, "RIFF", 4) != 0 || memcmp(&h->file_magic, "CPJB", 4) != 0)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return NULL;
	}

	chunkstart = buffer + sizeof(struct cpjfileheader);
	chunksize = h->len_file - sizeof(uint32_t);

	// find MAC chunk to get an actor definition
	ch = CPJ_FindChunkByName("MACB", "default", chunkstart, chunksize);
	if (!ch)
	{
		ch = CPJ_FindChunk("MACB", chunkstart, chunksize);

		if (!ch)
		{
			Con_Printf("%s: no MAC chunk\n", mod->name);
			return NULL;
		}
	}

	// sanity check
	if (ch->version != 1)
	{
		Con_Printf("%s: MAC version not recognized (%d, should be %d)\n", mod->name, ch->version, 1);
		return NULL;
	}

	// find autoexec section
	mac = (struct cpjmac *)(ch + 1);
	macdatablock = (char *)(mac + 1);
	macsections = (struct cpjmacsection *)&macdatablock[mac->ofs_sections];
	maccommands = (uint32_t *)&macdatablock[mac->ofs_commands];
	for (i = 0; i < mac->num_sections; i++)
	{
		if (strcmp(&macdatablock[macsections[i].ofs_name], "autoexec") == 0)
		{
			autoexec = &macsections[i];
			break;
		}
	}

	if (!autoexec)
	{
		Con_Printf("%s: no MAC \"autoexec\" section\n", mod->name);
		return NULL;
	}

	// now parse MAC commands
	for (i = autoexec->first_command; i < autoexec->first_command + autoexec->num_commands; i++)
	{
		char tok[1024];
		char *command = &macdatablock[maccommands[i]];

		command = COM_ParseOut(command, tok, sizeof(tok));

		// UNDONE:
		// SetBoundsMin x y z
		// SetBoundsMax x y z
		if (strncmp(tok, "SetGeometry", 11) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			if (!strchr(tok, '\\'))
			{
				Q_strlcpy(geoname, tok, sizeof(geoname));
				internalgeo = true;
			}
			else
			{
				Con_Printf("%s: external geometry chunks not yet supported, cannot load\n", mod->name);
				return NULL;
			}
		}
		else if (strncmp(tok, "SetSurface", 10) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			srfindex = atoi(tok);
			command = COM_ParseOut(command, tok, sizeof(tok));
			if (!strchr(tok, '\\'))
			{
				Q_strlcpy(srfname, tok, sizeof(srfname));
				internalsrf = true;
			}
			else
			{
				Con_Printf("%s: external surface chunks not yet supported, cannot load\n", mod->name);
				return NULL;
			}
		}
		else if (strncmp(tok, "SetLodData", 10) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			Con_Printf("%s: lod chunks not yet supported (refusing to load \"%s\")\n", mod->name, tok);
		}
		else if (strncmp(tok, "SetSkeleton", 11) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			if (!strchr(tok, '\\'))
			{
				Q_strlcpy(sklname, tok, sizeof(sklname));
				internalskl = true;
			}
			else
			{
				Con_Printf("%s: external skeleton chunks not yet supported (refusing to load \"%s\")\n", mod->name, tok);
			}
		}
		else if (strncmp(tok, "AddFrames", 9) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			if (strcmp(tok, "NULL") == 0)
			{
				internalfrm = true;
			}
			else
			{
				Con_Printf("%s: external frame chunks not yet supported (refusing to load \"%s\")\n", mod->name, tok);
			}
		}
		else if (strncmp(tok, "AddSequences", 12) == 0)
		{
			command = COM_ParseOut(command, tok, sizeof(tok));
			if (strcmp(tok, "NULL") == 0)
			{
				internalseq = true;
			}
			else
			{
				Con_Printf("%s: external sequence chunks not yet supported (refusing to load \"%s\")\n", mod->name, tok);
			}
		}
	}

	// sanity check
	if (!internalgeo)
	{
		Con_Printf("%s: no geometry chunk found, cannot continue\n", mod->name);
		return NULL;
	}

	// sanity check
	if (!internalsrf)
	{
		Con_Printf("%s: no surface chunk found, cannot continue\n", mod->name);
		return NULL;
	}

	// get geometry chunk
	ch = CPJ_FindChunkByName("GEOB", geoname, chunkstart, chunksize);
	if (!ch)
	{
		Con_Printf("%s: failed to find internal geometry chunk \"%s\"\n", mod->name, geoname);
		return NULL;
	}

	// sanity check
	if (ch->version != 1)
	{
		Con_Printf("%s: GEO version not recognized (%d, should be %d)\n", mod->name, ch->version, 1);
		return NULL;
	}

	// get geometry chunk
	geo = (struct cpjgeo *)(ch + 1);
	geodatablock = (char *)(geo + 1);
	geoverts = (struct cpjgeovert *)&geodatablock[geo->ofs_vertices];
	geotris = (struct cpjgeotri *)&geodatablock[geo->ofs_tris];
	geoedges = (struct cpjgeoedge *)&geodatablock[geo->ofs_edges];

	// get surface chunk
	ch = CPJ_FindChunkByName("SRFB", srfname, chunkstart, chunksize);
	if (!ch)
	{
		Con_Printf("%s: failed to find internal surface chunk \"%s\"\n", mod->name, geoname);
		return NULL;
	}

	// sanity check
	if (ch->version != 1)
	{
		Con_Printf("%s: SRF version not recognized (%d, should be %d)\n", mod->name, ch->version, 1);
		return NULL;
	}

	// get surface chunk
	srf = (struct cpjsrf *)(ch + 1);
	srfdatablock = (char *)(srf + 1);
	srftex = (struct cpjsrftex *)&srfdatablock[srf->ofs_textures];
	srftris = (struct cpjsrftri *)&srfdatablock[srf->ofs_tris];
	srfuv = (struct cpjsrfuv *)&srfdatablock[srf->ofs_uv];

	// one gai for each texture
	gai = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t) * srf->num_textures);

	// count up the number of triangles and vertices in each surface
	for (i = 0; i < srf->num_tris; i++)
	{
		// FIXME: this is lazy as hell please stop it
		// duplicating the hell out of each vertex? why
		gai[srftris[i].tex_index].numverts += 3;
		gai[srftris[i].tex_index].numindexes += 3;
	}

	// bounds
	ClearBounds(mod->mins, mod->maxs);

	// read in data
	for (i = 0; i < srf->num_textures; i++)
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
		for (j = 0; j < srf->num_tris; j++)
		{
			if (srftris[j].tex_index == i)
			{
				int srcverts[3];
				int dstverts[3];

				// copy vertex data
				for (k = 0; k < 3; k++)
				{
					srcverts[k] = geoedges[geotris[j].edge_ring[k]].tail_vertex;
					dstverts[k] = nverts++;

					gai[i].ofs_skel_xyz[dstverts[k]][0] = geoverts[srcverts[k]].position[0];
					gai[i].ofs_skel_xyz[dstverts[k]][1] = -geoverts[srcverts[k]].position[2];
					gai[i].ofs_skel_xyz[dstverts[k]][2] = geoverts[srcverts[k]].position[1];

					gai[i].ofs_st_array[dstverts[k]][0] = srfuv[srftris[j].uv_index[k]].u;
					gai[i].ofs_st_array[dstverts[k]][1] = srfuv[srftris[j].uv_index[k]].v;

					AddPointToBounds(gai[i].ofs_skel_xyz[dstverts[k]], mod->mins, mod->maxs);
				}

				// copy indices
				for (k = 0; k < 3; k++)
					gai[i].ofs_indexes[ntris * 3 + k] = dstverts[k];
				ntris++;
			}
		}

		// generate normals and texture vectors
		Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, true);
		Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, true);

		// setup skin
		gai[i].numskins = 1;
		gai[i].ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins));
		gai[i].ofsskins[0].skinspeed = 0.1;
		gai[i].ofsskins[0].numframes = 1;
		gai[i].ofsskins[0].frame = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins[0].frame));
		Q_strlcpy(gai[i].ofsskins[0].name, &srfdatablock[srftex[i].ofs_name], sizeof(gai[i].ofsskins[0].name));
		Q_strlcpy(gai[i].ofsskins[0].frame[0].shadername, &srfdatablock[srftex[i].ofs_name], sizeof(gai[i].ofsskins[0].frame[0].shadername));

		// setup misc info
		Q_strlcpy(gai[i].surfacename, &srfdatablock[srftex[i].ofs_name], sizeof(gai[i].surfacename));
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

#if 0
	// allocate gai
	gai = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t));

	gai->numverts = geo->num_vertices;
	gai->numindexes = geo->num_tris * 3;

	// allocate arrays
	gai->ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_xyz) * gai->numverts);
	gai->ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_st_array) * gai->numverts);
	gai->ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_norm) * gai->numverts);
	gai->ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_svect) * gai->numverts);
	gai->ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_skel_tvect) * gai->numverts);

	gai->ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofs_indexes) * gai->numindexes);

	// copy vertex positions
	for (i = 0; i < gai->numverts; i++)
	{
		gai->ofs_skel_xyz[i][0] = geoverts[i].position[0];
		gai->ofs_skel_xyz[i][1] = -geoverts[i].position[2];
		gai->ofs_skel_xyz[i][2] = geoverts[i].position[1];
	}

	// copy triangles
	for (i = 0; i < geo->num_tris; i++)
		for (j = 0; j < 3; j++)
			gai->ofs_indexes[i * 3 + j] = geoedges[geotris[i].edge_ring[j]].tail_vertex;

	// copy uv data
	for (i = 0; i < srf->num_tris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			int vert = geoedges[geotris[i].edge_ring[j]].tail_vertex;
			gai->ofs_st_array[vert][0] = srfuv[srftris[i].uv_index[j]].u;
			gai->ofs_st_array[vert][1] = srfuv[srftris[i].uv_index[j]].v;
		}
	}

	// generate normals and texture vectors
	Mod_AccumulateTextureVectors(gai->ofs_skel_xyz, gai->ofs_st_array, gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->ofs_indexes, gai->numindexes, true);
	Mod_NormaliseTextureVectors(gai->ofs_skel_norm, gai->ofs_skel_svect, gai->ofs_skel_tvect, gai->numverts, true);

	// allocate skins
	gai->numskins = srf->num_textures;
	gai->ofsskins = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofsskins) * gai->numskins);

	// copy skin info
	for (i = 0; i < gai->numskins; i++)
	{
		gai->ofsskins[i].skinspeed = 0.1;
		gai->ofsskins[i].numframes = 1;
		gai->ofsskins[i].frame = ZG_Malloc(&mod->memgroup, sizeof(*gai->ofsskins[i].frame));

		Q_strlcpy(gai->ofsskins[i].name, &srfdatablock[srftex[i].ofs_name], sizeof(gai->ofsskins[i].name));
		Q_strlcpy(gai->ofsskins[i].frame[0].shadername, &srfdatablock[srftex[i].ofs_name], sizeof(gai->ofsskins[i].frame[0].shadername));
	}

	// setup info
	Q_strlcpy(gai->surfacename, srfname, sizeof(gai->surfacename));
	gai->contents = FTECONTENTS_BODY;
	gai->geomset = ~0; //invalid set = always visible
	gai->geomid = 0;
	gai->mindist = 0;
	gai->maxdist = 0;
	gai->surfaceid = 0;
	gai->shares_verts = 0;
#endif

	return gai;
}

static qboolean QDECL Mod_LoadCpjModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *root;

	root = Mod_ParseCpjModel(mod, buffer, fsize);
	if (!root)
		return false;

	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

	mod->meshinfo = root;
	mod->type = mod_alias;
	mod->flags = 0;
	mod->numframes = root->numanimations;

	return true;
}

qboolean Plug_CPJ_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));

	if (!modfuncs || !filefuncs)
		return false;

	if (modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Cannibal Project (cpj)", "RIFF", 4, Mod_LoadCpjModel);
	return true;
}

#endif // CPJMODELS
