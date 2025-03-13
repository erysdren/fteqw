
#include "defs.h"

void WASM_EXPORT(PlayerPreThink)(void)
{

}

void WASM_EXPORT(PlayerPostThink)(void)
{

}

void WASM_EXPORT(ClientKill)(void)
{

}

void WASM_EXPORT(ClientConnect)(void)
{

}

void WASM_EXPORT(PutClientInServer)(void)
{
	self->classname = "player";
	self->health = 100;
	self->takedamage = DAMAGE_AIM;
	self->solid = SOLID_SLIDEBOX;
	self->movetype = MOVETYPE_WALK;
	self->max_health = 100;
	self->flags = FL_CLIENT;
	self->effects = 0;
}

void WASM_EXPORT(ClientDisconnect)(void)
{

}
