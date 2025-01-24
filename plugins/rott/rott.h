#ifndef _ROTT_H_
#define _ROTT_H_

#define NUM_MAPS (100)
#define NUM_PLANES (3)
#define MAP_WIDTH (128)
#define MAP_HEIGHT (128)

#define NUM_AREATILES (47)
#define FIRST_AREATILE (107)
#define LAST_AREATILE (FIRST_AREATILE + NUM_AREATILES)

static const uint32_t rtl_version = 0x0101;
static const uint32_t rxl_version = 0x0200;
static const uint32_t rle_tag_registered = 0x4344;
static const uint32_t rle_tag_shareware = 0x4d4b;

// decompressed plane
typedef uint16_t rottplanes_t[NUM_PLANES][MAP_HEIGHT][MAP_WIDTH];

// disk structure of a rott map
typedef struct drottmap {
	uint32_t used;
	uint32_t crc;
	uint32_t tag;
	uint32_t flags;
	uint32_t ofs_planes[NUM_PLANES];
	uint32_t len_planes[NUM_PLANES];
	char name[24];
} drottmap_t;

// memory structure of a rott map for the vfs:
// same as the disk structure, except the plane data immediately follows the
// definition (with the offsets adjusted accordingly). the "used" flag is also
// replaced with the bogus magic value "ROTT" for the model loader to see.
typedef struct mrottmap {
	char magic[4];
	uint32_t crc;
	uint32_t tag;
	uint32_t flags;
	uint32_t ofs_planes[NUM_PLANES];
	uint32_t len_planes[NUM_PLANES];
	char name[24];
} mrottmap_t;

#endif // _ROTT_H_
