
#include "defs.h"

/* globals */
entity_t self;
entity_t other;
entity_t world;
float time;
float frametime;
entity_t newmis;
float force_retouch;
string_t mapname;
float deathmatch;
float coop;
float teamplay;
float serverflags;
float dimension_send;
float physics_mode;
float total_secrets;
float total_monsters;
float found_secrets;
float killed_monsters;
vec3_t v_forward;
vec3_t v_up;
vec3_t v_right;
float trace_allsolid;
float trace_startsolid;
float trace_fraction;
vec3_t trace_endpos;
vec3_t trace_plane_normal;
float trace_plane_dist;
entity_t trace_ent;
float trace_inopen;
float trace_inwater;
float trace_endcontentsf;
int trace_endcontentsi;
float trace_surfaceflagsf;
int trace_surfaceflagsi;
float cycle_wrapped;
entity_t msg_entity;
float dimension_default;
vec3_t global_gravitydir;
string_t startspot;

/* globaldefs */
#define GLOBALDEF(n) {#n, &n}
static struct globaldef {
	const char *name;
	void *ptr;
} globaldefs[] = {
	GLOBALDEF(self),
	GLOBALDEF(other),
	GLOBALDEF(world),
	GLOBALDEF(time),
	GLOBALDEF(frametime),
	GLOBALDEF(newmis),
	GLOBALDEF(force_retouch),
	GLOBALDEF(mapname),
	GLOBALDEF(deathmatch),
	GLOBALDEF(coop),
	GLOBALDEF(teamplay),
	GLOBALDEF(serverflags),
	GLOBALDEF(dimension_send),
	GLOBALDEF(physics_mode),
	GLOBALDEF(total_secrets),
	GLOBALDEF(total_monsters),
	GLOBALDEF(found_secrets),
	GLOBALDEF(killed_monsters),
	GLOBALDEF(v_forward),
	GLOBALDEF(v_up),
	GLOBALDEF(v_right),
	GLOBALDEF(trace_allsolid),
	GLOBALDEF(trace_startsolid),
	GLOBALDEF(trace_fraction),
	GLOBALDEF(trace_endpos),
	GLOBALDEF(trace_plane_normal),
	GLOBALDEF(trace_plane_dist),
	GLOBALDEF(trace_ent),
	GLOBALDEF(trace_inopen),
	GLOBALDEF(trace_inwater),
	GLOBALDEF(trace_endcontentsf),
	GLOBALDEF(trace_endcontentsi),
	GLOBALDEF(trace_surfaceflagsf),
	GLOBALDEF(trace_surfaceflagsi),
	GLOBALDEF(cycle_wrapped),
	GLOBALDEF(msg_entity),
	GLOBALDEF(dimension_default),
	GLOBALDEF(global_gravitydir),
	GLOBALDEF(startspot)
};
#undef GLOBALDEF

/* required by the engine to get the pointer to a global so it can be modified */
void *WASM_EXPORT(GetGlobal)(const char *name)
{
	for (int i = 0; i < ASIZE(globaldefs); i++)
		if (strncmp(name, globaldefs[i].name, strlen(globaldefs[i].name)) == 0)
			return globaldefs[i].ptr;
	return NULL;
}
