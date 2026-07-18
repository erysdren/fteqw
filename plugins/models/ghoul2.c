/*
 * (c) erysdren 2026
 * license: GNU GPLv2
 */

#if !defined(GLQUAKE) && !defined(FTEENGINE)
#define GLQUAKE
#endif

#include "../plugin.h"
#include "com_mesh.h"

#ifdef SKELETALMODELS
#define GLMMODELS
#endif

#ifdef GLMMODELS

const uint8_t GHOUL2_GLM_IDENT[4] = { '2', 'L', 'G', 'M', };
const int GHOUL2_GLM_VERSION = 6;

#pragma pack(push, 1)

typedef struct glmHeader {
	uint8_t ident[4];
	int32_t version;
	char name[64];
	char animName[64];
	int32_t animIndex;
	int32_t numBones;
	int32_t numLODs;
	int32_t ofsLODs;
	int32_t numSurfaces;
	int32_t ofsSurfHierarchy;
	int32_t ofsEnd;
} glmHeader_t;

typedef struct glmSurfHierarchy {
	char name[64];
	uint32_t flags;
	char shader[64];
	int32_t shaderIndex;
	int32_t parentIndex;
	int32_t numChildren;
	int32_t childIndexes[1]; // numChildren
} glmSurfHierarchy_t;

typedef struct glmSurface {
	uint8_t ident[4];
	int32_t thisSurfaceIndex;
	int32_t ofsHeader;
	int32_t numVerts;
	int32_t ofsVerts;
	int32_t numTriangles;
	int32_t ofsTriangles;
	int32_t numBoneReferences;
	int32_t ofsBoneReferences;
	int32_t ofsEnd;
} glmSurface_t;

typedef struct glmLOD {
	int32_t ofsEnd;
	int32_t offsets[1]; // header->numSurfaces
} glmLOD_t;

typedef struct glmVertex {
	float normal[3];
	float vertCoords[3];
	uint32_t uiNmWeightsAndBoneIndexes;
	uint8_t BoneWeightings[4];
} glmVertex_t;

typedef struct glmVertexTexCoord {
	float texCoords[2];
} glmVertexTexCoord_t;

typedef struct glmTriangle {
	int32_t indexes[3];
} glmTriangle_t;

#pragma pack(pop)

static qboolean QDECL Mod_LoadGLMModel(model_t *mod, void *buffer, size_t fsize)
{
	glmHeader_t *header = (glmHeader_t *)buffer;

	// check ident
	if (strcmp(header->ident, GHOUL2_GLM_IDENT) != 0)
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return false;
	}

	// endian swap
	header->version = LittleLong(header->version);
	header->animIndex = LittleLong(header->animIndex);
	header->numBones = LittleLong(header->numBones);
	header->numLODs = LittleLong(header->numLODs);
	header->ofsLODs = LittleLong(header->ofsLODs);
	header->numSurfaces = LittleLong(header->numSurfaces);
	header->ofsSurfHierarchy = LittleLong(header->ofsSurfHierarchy);
	header->ofsEnd = LittleLong(header->ofsEnd);

	// check version
	if (header->version != GHOUL2_GLM_VERSION)
	{
		Con_Printf("%s: version not recognised (%d should be %d)\n", mod->name, header->version, GHOUL2_GLM_VERSION);
		return false;
	}

	// get first LOD surface list
	glmLOD_t *lod = (glmLOD_t *)((uint8_t *)header + LittleLong(header->ofsLODs));
	for (int32_t i = 0; i < header->numSurfaces; i++)
	{
		glmSurface_t *inSurface = (glmSurface_t *)((uint8_t *)lod + LittleLong(lod->offsets[i]) + sizeof(int32_t));
		glmVertex_t *inVerts = (glmVertex_t *)((uint8_t *)inSurface + LittleLong(inSurface->ofsVerts));
		glmVertexTexCoord_t *inCoords = (glmVertexTexCoord_t *)(inVerts + LittleLong(inSurface->numVerts));
		glmTriangle_t *inTris = (glmTriangle_t *)((uint8_t *)inSurface + LittleLong(inSurface->ofsTriangles));

		for (int32_t j = 0; j < LittleLong(inSurface->numVerts); j++)
		{
			glmVertex_t& inVert = inVerts[j];
			glmVertexTexCoord_t& inCoord = inCoords[j];
			outSurface.vertices().push_back(
				ArbitraryMeshVertex(
					Vertex3f( inVert.vertCoords[0], inVert.vertCoords[1], inVert.vertCoords[2] ),
					Normal3f( inVert.normal[0], inVert.normal[1], inVert.normal[2] ),
					TexCoord2f( inCoord.texCoords[0], inCoord.texCoords[1] )
				)
			);
		}

		for (int32_t j = 0; j < LittleLong(inSurface->numTriangles); j++)
		{
			glmTriangle_t& inTriangle = inTris[j];
			outSurface.indices().insert(inTriangle.indexes[0]);
			outSurface.indices().insert(inTriangle.indexes[1]);
			outSurface.indices().insert(inTriangle.indexes[2]);
		}

		glmSurfHierarchy_t *inSurfHierarchy = (glmSurfHierarchy_t *)((uint8_t *)header + LittleLong(header->ofsSurfHierarchy));
		for (int32_t j = 0; j < i; j++)
		{
			size_t size = (size_t)(&((glmSurfHierarchy_t *)0)->childIndexes[LittleLong(inSurfHierarchy->numChildren)]);
			inSurfHierarchy = (glmSurfHierarchy_t *)((uint8_t *)inSurfHierarchy + size);
		}
		outSurface.setShader( inSurfHierarchy->shader );
		globalOutputStream() << inSurfHierarchy->shader << '\n';

		// update aabb
		outSurface.updateAABB();
		model.updateAABB();
	}

	return true;
}

qboolean Plug_GLM_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (!modfuncs || modfuncs->version != MODPLUGFUNCS_VERSION)
		return false;

	modfuncs->RegisterModelFormatMagic("Ghoul 2 Model (glm)", "2LGM", 4, Mod_LoadGLMModel);
	return true;
}

#endif // GLMMODELS
