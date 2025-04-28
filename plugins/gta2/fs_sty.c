/*
 * written by erysdren (it/its) in 2025
 *
 * based on the official format spec
 * https://archive.org/download/gta2-1999/Files/Formats%20Spec/gta2formats.zip/GTA2%20Style%20Format.doc
 */
#include "../plugin.h"
#include "../../engine/common/fs.h"

static plugfsfuncs_t *filefuncs = NULL;
static plugthreadfuncs_t *threadfuncs = NULL;
#define Sys_CreateMutex() (threadfuncs?threadfuncs->CreateMutex():NULL)
#define Sys_LockMutex(m) (threadfuncs?threadfuncs->LockMutex(m):true)
#define Sys_UnlockMutex if(threadfuncs)threadfuncs->UnlockMutex
#define Sys_DestroyMutex if(threadfuncs)threadfuncs->DestroyMutex

// GTA2 Style Format.doc
#define NUM_TILES (992)
#define NUM_PALETTES (1024)
#define TILE_WIDTH (64)
#define TILE_HEIGHT (64)

typedef struct styheader {
	uint32_t magic;
	uint16_t version;
} styheader_t;

typedef struct stytile {
	fsbucket_t bucket;
	char name[MAX_QPATH];
	uint8_t pixels[TILE_HEIGHT][TILE_WIDTH];
	int palette;
} stytile_t;

typedef struct stypalette {
	uint8_t colors[256][4];
} stypalette_t;

typedef struct stypalettebase {
	uint16_t tile;
	uint16_t sprite;
	uint16_t car_remap;
	uint16_t ped_remap;
	uint16_t code_obj_remap;
	uint16_t map_obj_remap;
	uint16_t user_remap;
	uint16_t font_remap;
} stypalettebase_t;

typedef struct sty {
	searchpathfuncs_t pub;
	char desc[MAX_OSPATH];
	size_t handlepos;
	vfsfile_t *handle;
	stytile_t tiles[NUM_TILES];
	stypalette_t palettes[NUM_PALETTES];
	stypalettebase_t palettebase;
	void *mutex;
	int references;
} sty_t;

#define FAKE_TILE_LEN (4 + (256 * 4) + (64 * 64)) // sizeof(magic), sizeof(palette), sizeof(pixels)

static void QDECL FSSTY_GetPathDetails(searchpathfuncs_t *handle, char *out, size_t outlen)
{
	sty_t *pak = (sty_t *)handle;
	*out = 0;
	if (pak->references != 1)
		Q_snprintfz(out, outlen, "(%i)", pak->references - 1);
}

static void QDECL FSSTY_ClosePath(searchpathfuncs_t *handle)
{
	qboolean stillopen;
	sty_t *pak = (sty_t *)handle;

	if (!Sys_LockMutex(pak->mutex))
		return; //ohnoes
	stillopen = --pak->references > 0;
	Sys_UnlockMutex(pak->mutex);
	if (stillopen)
		return; //not free yet

	VFS_CLOSE(pak->handle);

	Sys_DestroyMutex(pak->mutex);

	plugfuncs->Free(pak);
}

static void QDECL FSSTY_BuildHash(searchpathfuncs_t *handle, int depth, void (QDECL *AddFileHash)(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle))
{
	sty_t *pak = (sty_t *)handle;
	int i;
	for (i = 0; i < NUM_TILES; i++)
		AddFileHash(depth, pak->tiles[i].name, &pak->tiles[i].bucket, &pak->tiles[i]);
}

static unsigned int QDECL FSSTY_FindFile(searchpathfuncs_t *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	stytile_t *tile = (stytile_t *)hashedresult;
	sty_t *pak = (sty_t *)handle;
	int i;

	if (tile)
	{
		//is this a pointer to a tile in this pak?
		if (tile < pak->tiles || tile > pak->tiles + NUM_TILES)
			return FF_NOTFOUND;
	}
	else
	{
		for (i = 0; i < NUM_TILES; i++)
		{
			if (strcasecmp(filename, pak->tiles[i].name) == 0)
			{
				tile = &pak->tiles[i];
				break;
			}
		}
	}

	if (tile)
	{
		if (loc)
		{
			loc->fhandle = tile;
			*loc->rawname = 0;
			loc->offset = (qofs_t)-1;
			loc->len = FAKE_TILE_LEN;
		}

		return FF_FOUND;
	}

	return FF_NOTFOUND;
}

typedef struct vfsstytile {
	vfsfile_t funcs;
	sty_t *parent;
	qofs_t pos;
	int tile;
} vfsstytile_t;

static int QDECL VFSSTY_ReadBytes(struct vfsfile_s *vfs, void *buffer, int bytestoread)
{
	// this is where the "magic" happens
	vfsstytile_t *vfsp = (vfsstytile_t *)vfs;
	int read = 0;
	uint8_t *out = (uint8_t *)buffer;

	if (bytestoread + vfsp->pos > FAKE_TILE_LEN)
		bytestoread = FAKE_TILE_LEN - vfsp->pos;

	while (bytestoread > 0)
	{
		if (vfsp->pos < 4)
		{
			// sizeof(magic)
			const char magic[4] = "TILE";
			int len = bytestoread > 4 - vfsp->pos ? 4 - vfsp->pos : bytestoread;
			memcpy(out, magic + vfsp->pos, len);
			bytestoread -= len;
			read += len;
			out += len;
			vfsp->pos += len;
		}
		else if (vfsp->pos < (4 + sizeof(stypalette_t)))
		{
			// sizeof(magic) + sizeof(palette)
			stytile_t *tile = &vfsp->parent->tiles[vfsp->tile];
			uint8_t *palette = (uint8_t *)vfsp->parent->palettes[tile->palette].colors;
			int pos = vfsp->pos - 4;
			int len = bytestoread > sizeof(stypalette_t) - pos ? sizeof(stypalette_t) - pos : bytestoread;
			memcpy(out, palette + pos, len);
			bytestoread -= len;
			read += len;
			out += len;
			vfsp->pos += len;
		}
		else
		{
			// sizeof(magic) + sizeof(palette) + sizeof(pixels)
			stytile_t *tile = &vfsp->parent->tiles[vfsp->tile];
			uint8_t *pixels = (uint8_t *)tile->pixels;
			int pos = vfsp->pos - (4 + sizeof(stypalette_t));
			int len = bytestoread > sizeof(tile->pixels) - pos ? sizeof(tile->pixels) - pos : bytestoread;
			memcpy(out, pixels + pos, len);
			bytestoread -= len;
			read += len;
			out += len;
			vfsp->pos += len;
		}
	}

	return read;
}

static int QDECL VFSSTY_WriteBytes(struct vfsfile_s *vfs, const void *buffer, int bytestoread)
{
	plugfuncs->Error("Cannot write to sty files\n");
	return 0;
}

static qofs_t QDECL VFSSTY_Tell(struct vfsfile_s *vfs)
{
	vfsstytile_t *vfsp = (vfsstytile_t *)vfs;
	return vfsp->pos;
}

static qofs_t QDECL VFSSTY_GetLen(struct vfsfile_s *vfs)
{
	return FAKE_TILE_LEN;
}

static qboolean QDECL VFSSTY_Close(vfsfile_t *vfs)
{
	vfsstytile_t *vfsp = (vfsstytile_t *)vfs;
	FSSTY_ClosePath(&vfsp->parent->pub); //tell the parent that we don't need it open any more (reference counts)
	plugfuncs->Free(vfsp); //free ourselves.
	return true;
}

static qboolean QDECL VFSSTY_Seek(struct vfsfile_s *vfs, qofs_t pos)
{
	vfsstytile_t *vfsp = (vfsstytile_t *)vfs;
	if (pos > FAKE_TILE_LEN)
		return false;
	vfsp->pos = pos;
	return true;
}

static vfsfile_t *QDECL FSSTY_OpenVFS(searchpathfuncs_t *handle, flocation_t *loc, const char *mode)
{
	sty_t *pak = (sty_t *)handle;
	vfsstytile_t *vfs;
	stytile_t *tile = (stytile_t *)loc->fhandle;

	if (strcmp(mode, "rb") != 0)
		return NULL;

	vfs = plugfuncs->Malloc(sizeof(vfsstytile_t));

	vfs->parent = pak;
	if (!Sys_LockMutex(vfs->parent->mutex))
	{
		plugfuncs->Free(vfs);
		return NULL;
	}

	vfs->parent->references++;
	Sys_UnlockMutex(vfs->parent->mutex);

	vfs->pos = 0;

#ifdef _DEBUG
	Q_strlcpy(vfs->funcs.dbgname, tile->name, sizeof(vfs->funcs.dbgname));
#endif
	vfs->funcs.Close = VFSSTY_Close;
	vfs->funcs.GetLen = VFSSTY_GetLen;
	vfs->funcs.ReadBytes = VFSSTY_ReadBytes;
	vfs->funcs.Seek = VFSSTY_Seek;
	vfs->funcs.Tell = VFSSTY_Tell;
	vfs->funcs.WriteBytes = VFSSTY_WriteBytes; //not supported

	return (vfsfile_t *)vfs;
}

static void QDECL FSSTY_ReadFile(searchpathfuncs_t *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	f = FSSTY_OpenVFS(handle, loc, "rb");
	if (!f)
		return;
	VFS_READ(f, buffer, loc->len);
	VFS_CLOSE(f);
}

static int QDECL FSSTY_EnumerateFiles(searchpathfuncs_t *handle, const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t *spath), void *parm)
{
	sty_t *pak = (sty_t *)handle;
	int i;

	for (i = 0; i < NUM_TILES; i++)
	{
		if (filefuncs->WildCmp(match, pak->tiles[i].name))
		{
			if (!func(pak->tiles[i].name, FAKE_TILE_LEN, 0, parm, handle))
				return false;
		}
	}

	return true;
}

static int QDECL FSSTY_GeneratePureCRC(searchpathfuncs_t *handle, const int *seed)
{
	sty_t *pak = (sty_t *)handle;
	return filefuncs->BlockChecksum(pak->tiles, sizeof(*pak->tiles) * NUM_TILES);
}

static searchpathfuncs_t *QDECL FSSTY_LoadArchive(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix)
{
	sty_t *pak;
	styheader_t header;
	uint16_t palette_index[16384];
	int i;

	// sanity checks
	if (!file) return NULL;
	if (prefix && *prefix) return NULL;

	// read header
	VFS_READ(file, &header, sizeof(header));
	if (memcmp(&header.magic, "GBST", sizeof(header.magic)) != 0)
		return NULL;
	if (header.version != 600)
		return NULL;

	// allocate pak
	pak = plugfuncs->Malloc(sizeof(*pak));

	// read various chunk types
	while (1)
	{
		uint32_t chunktype;
		uint32_t chunksize;

		if (VFS_READ(file, &chunktype, sizeof(chunktype)) < sizeof(chunktype))
			break;
		if (VFS_READ(file, &chunksize, sizeof(chunksize)) < sizeof(chunksize))
			break;

		if (memcmp(&chunktype, "PALX", sizeof(chunktype)) == 0)
		{
			// palette index
			if (chunksize != sizeof(palette_index))
			{
				Con_Printf("gta2: palette index chunk size mismatch\n");
				return NULL;
			}

			VFS_READ(file, palette_index, sizeof(palette_index));
		}
		else if (memcmp(&chunktype, "PPAL", sizeof(chunktype)) == 0)
		{
			// physical palettes
			if (chunksize != sizeof(pak->palettes))
			{
				Con_Printf("gta2: physical palette chunk size mismatch\n");
				return NULL;
			}

			VFS_READ(file, pak->palettes, sizeof(pak->palettes));
		}
		else if (memcmp(&chunktype, "PALB", sizeof(chunktype)) == 0)
		{
			// palette base
			if (chunksize != sizeof(pak->palettebase))
			{
				Con_Printf("gta2: palette base chunk size mismatch\n");
				return NULL;
			}

			VFS_READ(file, &pak->palettebase, sizeof(pak->palettebase));
		}
		else if (memcmp(&chunktype, "TILE", sizeof(chunktype)) == 0)
		{
			// tile pixel data
			if (chunksize != NUM_TILES * 64 * 64)
			{
				Con_Printf("gta2: tile chunk size mismatch\n");
				return NULL;
			}

			for (i = 0; i < NUM_TILES; i++)
			{
				VFS_READ(file, pak->tiles[i].pixels, sizeof(pak->tiles[i].pixels));
				Q_snprintfz(pak->tiles[i].name, sizeof(pak->tiles[i].name), "%s_tile%03d", filename, i);
				Con_Printf("%s\n", pak->tiles[i].name);
			}
		}
		else
		{
			// the rest are ignored (for now?)
			VFS_SEEK(file, VFS_TELL(file) + chunksize);
		}
	}

	// map virtual to physical palette indexes
	for (i = 0; i < NUM_TILES; i++)
		pak->tiles[i].palette = palette_index[i + pak->palettebase.tile];

	// setup info
	pak->references++;
	pak->handle = file;
	pak->handlepos = 0;
	Q_strlcpy(pak->desc, desc, sizeof(pak->desc));
	VFS_SEEK(pak->handle, pak->handlepos);
	pak->mutex = Sys_CreateMutex();

	// setup funcs
	pak->pub.fsver = FSVER;
	pak->pub.GetPathDetails = FSSTY_GetPathDetails;
	pak->pub.ClosePath = FSSTY_ClosePath;
	pak->pub.BuildHash = FSSTY_BuildHash;
	pak->pub.FindFile = FSSTY_FindFile;
	pak->pub.ReadFile = FSSTY_ReadFile;
	pak->pub.EnumerateFiles = FSSTY_EnumerateFiles;
	pak->pub.GeneratePureCRC = FSSTY_GeneratePureCRC;
	pak->pub.OpenVFS = FSSTY_OpenVFS;

	return (searchpathfuncs_t *)pak;
}

qboolean Plug_STY_Init(void)
{
	// get funcs
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs) // threadfuncs optional
		return false;

	// export our fs function
	if (!plugfuncs->ExportFunction("FS_RegisterArchiveType_sty", FSSTY_LoadArchive))
	{
		Con_Printf("engine doesn't support filesystem plugins\n");
		return false;
	}

	return true;
}
