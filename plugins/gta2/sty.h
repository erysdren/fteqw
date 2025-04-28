/*
 * written by erysdren (it/its) in 2025
 *
 * based on the official format spec
 * https://archive.org/download/gta2-1999/Files/Formats%20Spec/gta2formats.zip/GTA2%20Style%20Format.doc
 */
#ifndef _STY_H_
#define _STY_H_

#define STY_NUM_TILES (992)
#define STY_TILE_WIDTH (64)
#define STY_TILE_HEIGHT (64)

typedef struct sty {
	uint8_t tiles[STY_NUM_TILES][STY_TILE_HEIGHT][STY_TILE_WIDTH];
	uint8_t palettes[STY_NUM_TILES][256][4];
} sty_t;

sty_t *GTA2_LoadSty(const char *filename);

#endif // _STY_H_
