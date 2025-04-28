/*
 * written by erysdren (it/its) in 2025
 *
 * based on the official format spec
 * https://archive.org/download/gta2-1999/Files/Formats%20Spec/gta2formats.zip/GTA2%20Style%20Format.doc
 */
#include "../plugin.h"
#include "sty.h"

typedef struct styheader {
	uint32_t magic;
	uint16_t version;
} styheader_t;

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

static plugfsfuncs_t *filefuncs = NULL;

sty_t *GTA2_LoadSty(const char *filename)
{
	sty_t *sty;
	vfsfile_t *file;
	styheader_t header;
	stypalettebase_t palettebase;
	uint16_t palette_index[16384];
	uint8_t palettes[1024][256][4];
	int i;

	if (!filefuncs)
		filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	if (!filefuncs)
		return NULL;

	file = filefuncs->OpenVFS(filename, "rb", FS_GAME);
	if (!file)
		return NULL;

	if (VFS_READ(file, &header, sizeof(header)) < sizeof(header))
		return NULL;
	if (memcmp(&header.magic, "GBST", sizeof(header.magic)) != 0)
	{
		Con_Printf("%s: not a valid style file\n", filename);
		return NULL;
	}
	if (header.version != 700)
	{
		Con_Printf("%s: version %d is invalid\n", filename, header.version);
		return NULL;
	}

	sty = plugfuncs->Malloc(sizeof(*sty));

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
			if (chunksize != sizeof(palettes))
			{
				Con_Printf("gta2: physical palette chunk size mismatch\n");
				return NULL;
			}

			VFS_READ(file, palettes, sizeof(palettes));
		}
		else if (memcmp(&chunktype, "PALB", sizeof(chunktype)) == 0)
		{
			// palette base
			if (chunksize != sizeof(palettebase))
			{
				Con_Printf("gta2: palette base chunk size mismatch\n");
				return NULL;
			}

			VFS_READ(file, &palettebase, sizeof(palettebase));
		}
		else if (memcmp(&chunktype, "TILE", sizeof(chunktype)) == 0)
		{
			// tile pixel data
			if (chunksize != STY_NUM_TILES * STY_TILE_WIDTH * STY_TILE_HEIGHT)
			{
				Con_Printf("gta2: tile chunk size mismatch\n");
				return NULL;
			}

			for (i = 0; i < STY_NUM_TILES; i++)
			{
				VFS_READ(file, sty->tiles[i], sizeof(sty->tiles[i]));
			}
		}
		else
		{
			// the rest are ignored (for now?)
			VFS_SEEK(file, VFS_TELL(file) + chunksize);
		}
	}

	// map virtual to physical palette indexes
	for (i = 0; i < STY_NUM_TILES; i++)
		memcpy(sty->palettes[i], palettes[palette_index[i + palettebase.tile]], sizeof(sty->palettes[i]));

	VFS_CLOSE(file);

	return sty;
}
