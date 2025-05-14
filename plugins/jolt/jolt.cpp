
#include "quakedef.h"
#include "../plugin.h"
#include "../engine.h"

#include "pr_common.h"
#include "com_mesh.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#ifndef FTEENGINE
#define BZ_Malloc malloc
#define BZ_Free free
int VectorCompare(const vec3_t v1, const vec3_t v2)
{
	for (int i = 0; i < 3; i++)
		if (v1[i] != v2[i])
			return 0;
	return 1;
}
#endif

static rbeplugfuncs_t *rbefuncs = nullptr;
static cvar_t *physics_jolt_maxiterationsperframe = nullptr;

enum {
	LAYER_STATIC,
	LAYER_MOVING,
	NUM_LAYERS
};

typedef struct joltcontext {
	rigidbodyengine_t funcs;

	JPH::TempAllocatorMalloc temp_allocator;
	JPH::PhysicsSystem physics_system;
	JPH::JobSystemThreadPool job_system;

	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			if (inObject1 == LAYER_STATIC)
				return inObject2 == LAYER_MOVING;
			else
				return inObject1 == LAYER_MOVING;
		}
	} object_vs_object_layer_filter;

	class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual uint GetNumBroadPhaseLayers() const override
		{
			return NUM_LAYERS;
		}
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			return (JPH::BroadPhaseLayer)inLayer;
		}
	} broad_phase_layer_interface;

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			if (inLayer1 == LAYER_STATIC)
				return inLayer2 == (JPH::BroadPhaseLayer)LAYER_MOVING;
			else
				return inLayer1 == LAYER_MOVING;
		}
	} object_vs_broadphase_layer_filter;

	const unsigned int max_bodies = 1024;
	const unsigned int num_body_mutexes = 0;
	const unsigned int max_body_pairs = 1024;
	const unsigned int max_contact_constraints = 1024;

	const unsigned int max_physics_jobs = JPH::cMaxPhysicsJobs;
	const unsigned int max_physics_barriers = JPH::cMaxPhysicsBarriers;

	rbecommandqueue_t *cmdqueuehead;
	rbecommandqueue_t *cmdqueuetail;
} joltcontext_t;

static void QDECL World_Jolt_End(world_t *world)
{
	joltcontext_t *ctx = (joltcontext_t *)world->rbe;
	delete ctx;
	world->rbe = nullptr;
}

static void QDECL World_Jolt_RemoveJointFromEntity(world_t *world, wedict_t *ed)
{
	ed->rbe.joint_type = 0;
	ed->rbe.joint.joint = nullptr;
}

static void QDECL World_Jolt_RemoveFromEntity(world_t *world, wedict_t *ed)
{
	joltcontext_t *ctx = (joltcontext_t *)world->rbe;
	JPH::BodyInterface &body_interface = ctx->physics_system.GetBodyInterface();
	JPH::Body *body;
	JPH::Shape *geom;

	// not physics controlled
	if (!ed->rbe.physics)
		return;

	body = (JPH::Body *)ed->rbe.body.body;
	ed->rbe.body.body = nullptr;
	if (body)
	{
		body_interface.RemoveBody(body->GetID());
		body_interface.DestroyBody(body->GetID());
	}

	geom = (JPH::Shape *)ed->rbe.body.geom;
	ed->rbe.body.geom = nullptr;
	if (ed->rbe.body.geom)
	{
		delete geom;
	}

	// entity is not physics controlled, free any physics data
	rbefuncs->ReleaseCollisionMesh(ed);
	ed->rbe.massbuf = nullptr;
}

static void World_Jolt_RunCommand(world_t *world, rbecommandqueue_t *cmd)
{
	if (!cmd->edict->rbe.body.body)
		return;

	joltcontext_t *ctx = (joltcontext_t *)world->rbe;
	JPH::BodyInterface &body_interface = ctx->physics_system.GetBodyInterface();
	JPH::Body *body = (JPH::Body *)cmd->edict->rbe.body.body;
	const JPH::BodyID &body_id = body->GetID();

	switch (cmd->command)
	{
		case RBECMD_ENABLE:
		{
			body_interface.ActivateBody(body_id);
			break;
		}

		case RBECMD_DISABLE:
		{
			body_interface.DeactivateBody(body_id);
			break;
		}

		case RBECMD_FORCE:
		{
			JPH::RVec3 center = body_interface.GetCenterOfMassPosition(body_id);
			JPH::RVec3 v2 = JPH::RVec3(cmd->v2[0], cmd->v2[1], cmd->v2[2]);
			JPH::RVec3 pos = v2 - center;

			body_interface.AddImpulse(body_id, JPH::Vec3Arg(cmd->v1[0], cmd->v1[1], cmd->v1[2]), pos);
			break;
		}

		case RBECMD_TORQUE:
		{
			body_interface.AddTorque(body_id, JPH::Vec3Arg(cmd->v1[0], cmd->v1[1], cmd->v1[2]));
			break;
		}
	}
}

static void World_Jolt_BodyFromEntity(world_t *world, wedict_t *ed)
{
	JPH::Body *body = nullptr;
	JPH::Shape *geom = nullptr;
	vec3_t maxs;
	vec3_t mins;
	vec3_t center;
	vec3_t size;
	int modelindex = 0;
	int movetype = MOVETYPE_NONE;
	int solid = SOLID_NOT;
	int geomtype = GEOMTYPE_SOLID;
	vec_t scale = 1;
	vec_t mass = 1;
	model_t *model = nullptr;
	qboolean modified = qfalse;

	joltcontext_t *ctx = (joltcontext_t *)world->rbe;
	JPH::BodyInterface &body_interface = ctx->physics_system.GetBodyInterface();

	// get entity fields
	geomtype = ed->xv->geomtype;
	solid = ed->v->solid;
	movetype = ed->v->movetype;
	if (ed->xv->scale)
		scale = ed->xv->scale;

	// invalid geomtype, figure out what it should be based on solid type
	if (!geomtype)
	{
		switch (solid)
		{
			case SOLID_NOT:
				geomtype = GEOMTYPE_NONE;
				break;
			case SOLID_TRIGGER:
				geomtype = GEOMTYPE_NONE;
				break;
			case SOLID_BSP:
				geomtype = GEOMTYPE_TRIMESH;
				break;
			case SOLID_PHYSICS_TRIMESH:
				geomtype = GEOMTYPE_TRIMESH;
				break;
			case SOLID_PHYSICS_BOX:
				geomtype = GEOMTYPE_BOX;
				break;
			case SOLID_PHYSICS_SPHERE:
				geomtype = GEOMTYPE_SPHERE;
				break;
			case SOLID_PHYSICS_CAPSULE:
				geomtype = GEOMTYPE_CAPSULE;
				break;
			case SOLID_PHYSICS_CYLINDER:
				geomtype = GEOMTYPE_CYLINDER;
				break;
			default:
				geomtype = GEOMTYPE_BOX;
				break;
		}
	}

	// setup values based on geomtype
	switch (geomtype)
	{
		case GEOMTYPE_TRIMESH:
		{
			modelindex = ed->v->modelindex;
			model = world->Get_CModel(world, modelindex);
			if (!model || model->loadstate != MLS_LOADED)
			{
				model = nullptr;
				modelindex = 0;
			}
			if (model)
			{
				VectorScale(model->mins, scale, mins);
				VectorScale(model->maxs, scale, maxs);
				if (ed->xv->mass)
					mass = ed->xv->mass;
			}
			else
			{
				modelindex = 0;
				mass = 1.0f;
			}
			break;
		}

		case GEOMTYPE_BOX:
		case GEOMTYPE_SPHERE:
		case GEOMTYPE_CAPSULE:
		case GEOMTYPE_CAPSULE_X:
		case GEOMTYPE_CAPSULE_Y:
		case GEOMTYPE_CAPSULE_Z:
		case GEOMTYPE_CYLINDER:
		case GEOMTYPE_CYLINDER_X:
		case GEOMTYPE_CYLINDER_Y:
		case GEOMTYPE_CYLINDER_Z:
		{
			VectorCopy(ed->v->mins, mins);
			VectorCopy(ed->v->maxs, maxs);
			if (ed->xv->mass)
				mass = ed->xv->mass;
			break;
		}

		default:
		{
			if (ed->rbe.physics)
				World_Jolt_RemoveFromEntity(world, ed);
			return;
		}
	}

	// we don't allow point-size physics objects...
	VectorSubtract(maxs, mins, size);
	if (DotProduct(size, size) == 0)
	{
		if (ed->rbe.physics)
			World_Jolt_RemoveFromEntity(world, ed);
		return;
	}

	// check if we need to create or replace the geom
	if (!ed->rbe.physics || !VectorCompare(ed->rbe.mins, mins) || !VectorCompare(ed->rbe.maxs, maxs) || ed->rbe.modelindex != modelindex)
	{
		modified = qtrue;
		World_Jolt_RemoveFromEntity(world, ed);
		ed->rbe.physics = qtrue;
		VectorCopy(mins, ed->rbe.mins);
		VectorCopy(maxs, ed->rbe.maxs);
		ed->rbe.modelindex = modelindex;
		VectorAvg(mins, maxs, center);
		ed->rbe.movelimit = std::min(size[0], std::min(size[1], size[2]));
		ed->rbe.mass = mass;

		switch (geomtype)
		{
			case GEOMTYPE_TRIMESH:
			{
				if (!model)
				{
					Con_Printf("entity %i (classname %s) has no model\n", NUM_FOR_EDICT(world->progs, (edict_t *)ed), PR_GetString(world->progs, ed->v->classname));
					if (ed->rbe.physics)
						World_Jolt_RemoveFromEntity(world, ed);
					return;
				}

				if (!rbefuncs->GenerateCollisionMesh(world, model, ed, center))
				{
					if (ed->rbe.physics)
						World_Jolt_RemoveFromEntity(world, ed);
					return;
				}

				// TODO: trimesh
				break;
			}

			case GEOMTYPE_BOX:
			{
				geom = new JPH::BoxShape(JPH::Vec3Arg(size[0], size[1], size[2]) * 0.5);
				break;
			}

			case GEOMTYPE_SPHERE:
			{
				geom = new JPH::SphereShape(size[0] * 0.5f);
				break;
			}

			case GEOMTYPE_CAPSULE:
			case GEOMTYPE_CAPSULE_X:
			case GEOMTYPE_CAPSULE_Y:
			case GEOMTYPE_CAPSULE_Z:
			{
				int axisindex;
				vec_t radius, length;
				if (geomtype == GEOMTYPE_CAPSULE)
				{
					// the qc gives us 3 axis radius, the longest axis is the capsule axis
					axisindex = 0;
					if (size[axisindex] < size[1])
						axisindex = 1;
					if (size[axisindex] < size[2])
						axisindex = 2;
				}
				else
					axisindex = geomtype - GEOMTYPE_CAPSULE_X;

				if (axisindex == 0)
					radius = std::min(size[1], size[2]) * 0.5f;
				else if (axisindex == 1)
					radius = std::min(size[0], size[2]) * 0.5f;
				else
					radius = std::min(size[0], size[1]) * 0.5f;

				length = size[axisindex] - radius * 2;

				if (length <= 0)
				{
					radius -= (1 - length) * 0.5;
					length = 1;
				}

				// TODO: check if length should be divided by 2
				geom = new JPH::CapsuleShape(length, radius);
				break;
			}

			case GEOMTYPE_CYLINDER:
			case GEOMTYPE_CYLINDER_X:
			case GEOMTYPE_CYLINDER_Y:
			case GEOMTYPE_CYLINDER_Z:
			{
				int axisindex;
				vec_t radius, length;
				if (geomtype == GEOMTYPE_CYLINDER)
				{
					// the qc gives us 3 axis radius, the longest axis is the capsule axis
					axisindex = 0;
					if (size[axisindex] < size[1])
						axisindex = 1;
					if (size[axisindex] < size[2])
						axisindex = 2;
				}
				else
					axisindex = geomtype - GEOMTYPE_CYLINDER_X;

				if (axisindex == 0)
					radius = std::min(size[1], size[2]) * 0.5f;
				else if (axisindex == 1)
					radius = std::min(size[0], size[2]) * 0.5f;
				else
					radius = std::min(size[0], size[1]) * 0.5f;

				length = size[axisindex] - radius * 2;

				if (length <= 0)
				{
					radius -= (1 - length) * 0.5;
					length = 1;
				}

				// TODO: check if length should be divided by 2
				geom = new JPH::CylinderShape(length, radius);
				break;
			}

			default:
			{
				if (ed->rbe.physics)
					World_Jolt_RemoveFromEntity(world, ed);
				return;
			}
		}

		ed->rbe.body.geom = (void *)geom;
	}

	// non-moving objects need to be static objects (and thus need 0 mass)
	if (movetype != MOVETYPE_PHYSICS && movetype != MOVETYPE_WALK)
		mass = 0;

	// if the mass changes, we'll need to create a new body (but not the shape, so invalidate the current one)
	if (ed->rbe.mass != mass)
	{
		ed->rbe.mass = mass;

		body = (JPH::Body *)ed->rbe.body.body;
		if (body)
		{
			body_interface.RemoveBody(body->GetID());
			body_interface.DestroyBody(body->GetID());
		}

		ed->rbe.body.body = nullptr;
	}
}

static void QDECL World_Jolt_RunFrame(world_t *world, double frametime, double gravity)
{
	joltcontext_t *ctx = (joltcontext_t *)world->rbe;

	// no physics objects active
	if (!world->rbe_hasphysicsents)
		return;

	// copy physics properties from entities to physics engine
	for (unsigned int i = 0; i < world->num_edicts; i++)
	{
		wedict_t *ed = WEDICT_NUM_PB(world->progs, i);
		if (!ED_ISFREE(ed))
			World_Jolt_BodyFromEntity(world, ed);
	}

#if 0
	// oh, and it must be called after all bodies were created
	for (unsigned int i = 0; i < world->num_edicts; i++)
	{
		wedict_t *ed = WEDICT_NUM_PB(world->progs, i);
		if (!ED_ISFREE(ed))
			World_Bullet_Frame_JointFromEntity(world, ed);
	}
#endif

	// process command queue
	while (ctx->cmdqueuehead)
	{
		rbecommandqueue_t *cmd = ctx->cmdqueuehead;
		ctx->cmdqueuehead = cmd->next;
		if (!cmd->next)
			ctx->cmdqueuetail = nullptr;
		World_Jolt_RunCommand(world, cmd);
		BZ_Free(cmd);
	}

	// set gravity
	ctx->physics_system.SetGravity(JPH::Vec3Arg(0, 0, -gravity));

	// step the world
	ctx->physics_system.Update(frametime, physics_jolt_maxiterationsperframe->value, &ctx->temp_allocator, &ctx->job_system);
}

static void QDECL World_Jolt_PushCommand(world_t *world, rbecommandqueue_t *val)
{
	joltcontext_t *ctx = (joltcontext_t *)world->rbe;
	rbecommandqueue_t *cmd = (rbecommandqueue_t *)BZ_Malloc(sizeof(*cmd));
	world->rbe_hasphysicsents = qtrue;
	memcpy(cmd, val, sizeof(*cmd));
	cmd->next = nullptr;
	if (ctx->cmdqueuehead)
	{
		rbecommandqueue_t *oldtail = ctx->cmdqueuetail;
		oldtail->next = ctx->cmdqueuetail = cmd;
	}
	else
	{
		ctx->cmdqueuetail = ctx->cmdqueuehead = cmd;
	}
}

static void QDECL World_Jolt_Start(world_t *world)
{
	joltcontext_t *ctx;

	// already has one somehow
	if (world->rbe)
		return;

	ctx = new joltcontext_t();
	ctx->funcs.End = World_Jolt_End;
	ctx->funcs.RemoveJointFromEntity = World_Jolt_RemoveJointFromEntity;
	ctx->funcs.RemoveFromEntity = World_Jolt_RemoveFromEntity;
#if 0
	ctx->funcs.RagMatrixToBody = World_Jolt_RagMatrixToBody;
	ctx->funcs.RagCreateBody = World_Jolt_RagCreateBody;
	ctx->funcs.RagMatrixFromJoint = World_Jolt_RagMatrixFromJoint;
	ctx->funcs.RagMatrixFromBody = World_Jolt_RagMatrixFromBody;
	ctx->funcs.RagEnableJoint = World_Jolt_RagEnableJoint;
	ctx->funcs.RagCreateJoint = World_Jolt_RagCreateJoint;
	ctx->funcs.RagDestroyBody = World_Jolt_RagDestroyBody;
	ctx->funcs.RagDestroyJoint = World_Jolt_RagDestroyJoint;
#endif
	ctx->funcs.RunFrame = World_Jolt_RunFrame;
	ctx->funcs.PushCommand = World_Jolt_PushCommand;
#if 0
	ctx->funcs.Trace = World_Jolt_Trace;
#endif
	world->rbe = (rigidbodyengine_t *)ctx;

	// init jobs system
	// TODO: use FTE threads
	ctx->job_system.Init(ctx->max_physics_jobs, ctx->max_physics_barriers);

	// init physics system
	ctx->physics_system.Init(
		ctx->max_bodies,
		ctx->num_body_mutexes,
		ctx->max_body_pairs,
		ctx->max_contact_constraints,
		ctx->broad_phase_layer_interface,
		ctx->object_vs_broadphase_layer_filter,
		ctx->object_vs_object_layer_filter
	);
}

static void QDECL Plug_Jolt_Shutdown(void)
{
	if (rbefuncs)
		rbefuncs->UnregisterPhysicsEngine("Jolt");

	// do this globally rather than per-context
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}

extern "C" qboolean Plug_Init(void)
{
	// get funcs
	rbefuncs = (rbeplugfuncs_t *)plugfuncs->GetEngineInterface("RBE", sizeof(*rbefuncs));
	if (!rbefuncs || rbefuncs->version != RBEPLUGFUNCS_VERSION || rbefuncs->wedictsize != sizeof(wedict_t) || !rbefuncs->RegisterPhysicsEngine)
	{
		Con_Printf("Jolt: engine not compatible with physics plugins\n");
		return qfalse;
	}

	// register the shutdown func
	if (!plugfuncs->ExportFunction("Shutdown", (funcptr_t)Plug_Jolt_Shutdown))
	{
		Con_Printf("Jolt: plugfuncs->ExportFunction failed\n");
		return qfalse;
	}

	// register the engine
	if (!rbefuncs->RegisterPhysicsEngine("Jolt", World_Jolt_Start))
	{
		Con_Printf("Jolt: engine already has a physics plugin active.\n");
		return qfalse;
	}

	// get cvars
	physics_jolt_maxiterationsperframe = cvarfuncs->GetNVFDG("physics_jolt_maxiterationsperframe", "1", 0, "Maximum times to run the simulation per frame", "Jolt");

	// do this globally rather than per-context
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory;
	JPH::RegisterTypes();
	return qtrue;
}
