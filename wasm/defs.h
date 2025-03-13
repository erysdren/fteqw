
#ifndef _DEFS_H_
#define _DEFS_H_
#ifdef __cplusplus
extern "C" {
#endif

/*
 * standard includes
 */

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/*
 * wasm macros
 */

#define WASM_EXPORT(n) __attribute((export_name(#n), used, visibility("default"))) n
#define WASM_IMPORT(n) __attribute((import_module("env"), import_name(#n))) n

/*
 * builtin funcitons provided by the compiler (hopefully!)
 */

#define rint(f) __builtin_rint(f)
#define floor(f) __builtin_floor(f)
#define ceil(f) __builtin_ceil(f)
#define fabs(f) __builtin_fabs(f)
#define memcpy(dst, src, n) __builtin_memcpy(dst, src, n)
#define memset(dst, val, n) __builtin_memset(dst, val, n)

/*
 * vector math
 */

typedef float vec3_t[3];

static inline void VectorCopy(vec3_t dst, vec3_t src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

static inline void VectorAdd(vec3_t dst, vec3_t a, vec3_t b)
{
	dst[0] = a[0] + b[0];
	dst[1] = a[1] + b[1];
	dst[2] = a[2] + b[2];
}

static inline void VectorSub(vec3_t dst, vec3_t a, vec3_t b)
{
	dst[0] = a[0] - b[0];
	dst[1] = a[1] - b[1];
	dst[2] = a[2] - b[2];
}

static inline float VectorDot(vec3_t a, vec3_t b)
{
	return a[0] * b[0] + a[1] * b[1] + b[2] * b[2];
}

static inline float VectorLength(vec3_t v)
{
	return VectorDot(v, v);
}

static inline void VectorCross(vec3_t dst, vec3_t a, vec3_t b)
{
	vec3_t c;
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
	VectorCopy(dst, c);
}

/*
 * random helper macros
 */

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

/*
 * very naive and probably slow string library
 */

static inline size_t strlen(const char *s)
{
	size_t n = 0;
	while (s && *s++)
		n++;
	return n;
}

static inline size_t strnlen(const char *s, size_t maxlen)
{
	size_t n = 0;
	while (s && n < maxlen && *s++)
		n++;
	return n;
}

static inline int strcmp(const char *s1, const char *s2)
{
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (*p1 && (*p1 == *p2))
	{
		++p1;
		++p2;
	}

	return (*p1 > *p2) - (*p2 > *p1);
}

static inline int strncmp(const char *s1, const char *s2, size_t n)
{
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (n && *p1 && (*p1 == *p2))
	{
		++p1;
		++p2;
		--n;
	}

	if (n == 0)
		return 0;

	return (*p1 > *p2) - (*p2 > *p1);
}


/*
 * progs imports
 */

typedef const char *string_t;
typedef struct entvars *entity_t;
typedef void (*func_t)(void);

typedef struct entvars {
	float modelindex;
	vec3_t absmin;
	vec3_t absmax;
	float ltime;
	float lastruntime;
	float movetype;
	float solid;
	vec3_t origin;
	vec3_t oldorigin;
	vec3_t velocity;
	vec3_t angles;
	vec3_t avelocity;
	string_t classname;
	string_t model;
	float frame;
	float skin;
	float effects;
	vec3_t mins;
	vec3_t maxs;
	vec3_t size;
	func_t touch;
	func_t use;
	func_t think;
	func_t blocked;
	float nextthink;
	entity_t groundentity;
	float health;
	float frags;
	float weapon;
	string_t weaponmodel;
	float weaponframe;
	float currentammo;
	float ammo_shells;
	float ammo_nails;
	float ammo_rockets;
	float ammo_cells;
	float items;
	float takedamage;
	entity_t chain;
	float deadflag;
	vec3_t view_ofs;
	float button0;
	float button1;
	float button2;
	float impulse;
	float fixangle;
	vec3_t v_angle;
	string_t netname;
	entity_t enemy;
	float flags;
	float colormap;
	float team;
	float max_health;
	float teleport_time;
	float armortype;
	float armorvalue;
	float waterlevel;
	float watertype;
	float ideal_yaw;
	float yaw_speed;
	entity_t aiment;
	entity_t goalentity;
	float spawnflags;
	string_t target;
	string_t targetname;
	float dmg_take;
	float dmg_save;
	entity_t dmg_inflictor;
	entity_t owner;
	vec3_t movedir;
	string_t message;
	float sounds;
	string_t noise;
	string_t noise1;
	string_t noise2;
	string_t noise3;
} entvars_t;

/*
 * constants
 */

/* edict.flags */
enum {
	FL_FLY = 1,
	FL_SWIM = 2,
	FL_CLIENT = 8, /* set for all client edicts */
	FL_INWATER = 16, /* for enter / leave water splash */
	FL_MONSTER = 32,
	FL_GODMODE = 64, /* player cheat */
	FL_NOTARGET = 128, /* player cheat */
	FL_ITEM = 256, /* extra wide size for bonus items */
	FL_ONGROUND = 512, /* standing on something */
	FL_PARTIALGROUND = 1024, /* not all corners are valid */
	FL_WATERJUMP = 2048, /* player jumping out of water */
	FL_JUMPRELEASED = 4096 /* for jump debouncing */
};

/* edict.movetype values */
enum {
	MOVETYPE_NONE = 0, /* never moves */
	MOVETYPE_WALK = 3, /* players only */
	MOVETYPE_STEP = 4, /* discrete, not real time unless fall */
	MOVETYPE_FLY = 5,
	MOVETYPE_TOSS = 6, /* gravity */
	MOVETYPE_PUSH = 7, /* no clip to world, push and crush */
	MOVETYPE_NOCLIP = 8,
	MOVETYPE_FLYMISSILE = 9, /* fly with extra size against monsters */
	MOVETYPE_BOUNCE = 10,
	MOVETYPE_BOUNCEMISSILE = 11 /* bounce with extra size */
};

/* edict.solid values */
enum {
	SOLID_NOT = 0, /* no interaction with other objects */
	SOLID_TRIGGER = 1, /* touch on edge, but not blocking */
	SOLID_BBOX = 2, /* touch on edge, block */
	SOLID_SLIDEBOX = 3, /* touch on edge, but not an onground */
	SOLID_BSP = 4 /* bsp clip, touch on edge, block */
};

/* takedamage values */
enum {
	DAMAGE_NO = 0,
	DAMAGE_YES = 1,
	DAMAGE_AIM = 2
};

/* point content values */
enum {
	CONTENT_EMPTY = -1,
	CONTENT_SOLID = -2,
	CONTENT_WATER = -3,
	CONTENT_SLIME = -4,
	CONTENT_LAVA = -5,
	CONTENT_SKY = -6
};

/*
 * progs exports
 */

/* main.c */
extern entity_t self;
extern entity_t other;
extern entity_t world;
extern float time;
extern float frametime;
extern entity_t newmis;
extern float force_retouch;
extern string_t mapname;
extern float deathmatch;
extern float coop;
extern float teamplay;
extern float serverflags;
extern float dimension_send;
extern float physics_mode;
extern float total_secrets;
extern float total_monsters;
extern float found_secrets;
extern float killed_monsters;
extern vec3_t v_forward;
extern vec3_t v_up;
extern vec3_t v_right;
extern float trace_allsolid;
extern float trace_startsolid;
extern float trace_fraction;
extern vec3_t trace_endpos;
extern vec3_t trace_plane_normal;
extern float trace_plane_dist;
extern entity_t trace_ent;
extern float trace_inopen;
extern float trace_inwater;
extern float trace_endcontentsf;
extern int trace_endcontentsi;
extern float trace_surfaceflagsf;
extern int trace_surfaceflagsi;
extern float cycle_wrapped;
extern entity_t msg_entity;
extern float dimension_default;
extern vec3_t global_gravitydir;
extern string_t startspot;

void *WASM_EXPORT(GetGlobal)(const char *name);

/* client.c */
void WASM_EXPORT(PlayerPreThink)(void);
void WASM_EXPORT(PlayerPostThink)(void);
void WASM_EXPORT(ClientKill)(void);
void WASM_EXPORT(ClientConnect)(void);
void WASM_EXPORT(PutClientInServer)(void);
void WASM_EXPORT(ClientDisconnect)(void);

/* world.c */
void WASM_EXPORT(StartFrame)(void);

#ifdef __cplusplus
}
#endif
#endif /* _DEFS_H_ */
