#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <stdint.h>
#include "vector.hpp"
#include "universe.hpp"

class Universe;

// Rigid body physics references
// + http://www.olhovsky.com/2011/05/physics-engine/
// + http://www.cord.edu/dept/physics/simulations/Two%20Spheres%20Colliding%20in%20Two%20Dimensions.html
// + http://archive.ncsa.illinois.edu/Classes/MATH198/townsend/math.html
// + http://chrishecker.com/Rigid_body_dynamics
//
// - http://stackoverflow.com/questions/345838/ball-to-ball-collision-detection-and-handling
// - http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=4&t=1452
// - http://idav.ucdavis.edu/~okreylos/ResDev/Balls/MainPage.html
// - http://introcs.cs.princeton.edu/java/assignments/collisions.html
// - http://www.ics.uci.edu/~wayne/Gas/
// - http://www.myphysicslab.com/collision.html

enum PhysicsObjectType { PHYSOBJECT, PHYSOBJECT_SMART, BEAM_COMM, BEAM_SCAN, BEAM_SCANRESULT, BEAM_WEAP };

#pragma pack(1)
struct PhysicsObject
{
	PhysicsObjectType type;
	uint64_t phys_id;
	struct Vector3 position,
		velocity,
		ang_velocity,
		thrust,
		forward,
		up,
		right;
	double mass,
		radius,
		health;
	Universe* universe;
	char* obj_type;
	int art_id;
	bool emits_gravity;
};
#pragma pack()

#pragma pack(1)
struct SmartPhysicsObject
{
	struct PhysicsObject pobj;
	uint64_t osim_id,
		parent_phys_id,
		query_id;
	int client;
	bool vis_data,
		vis_meta_data,
		exists;
};
#pragma pack()

#pragma pack(1)
struct Beam
{
	PhysicsObjectType type;
	uint64_t phys_id;
	PhysicsObject* scan_target;
	Universe* universe;
	struct Vector3 origin,
		direction,
		front_position,
		up,
		right;
	double cosines[2];
	double speed,
		//cosh,
		//cosv,
		area_factor,
		energy,
		distance_travelled,
		max_distance;
};
#pragma pack()

struct PhysCollisionEffect
{
	// Direction of impact
	struct Vector3 d;
	// Position of impact on object's bounding sphere, relative to centre
	struct Vector3 p;
	// Velocity tangential to impact, remains unchanged
	struct Vector3 t;
	// Velocity along normal of impact
	struct Vector3 n;
};

struct PhysCollisionResult
{
	// Effect on 'first' object
	struct PhysCollisionEffect pce1;
	// effect on 'second' object
	struct PhysCollisionEffect pce2;
	// Time in [0,1] alont dt of the impact
	double t;
	// Energy given up by object 1 to object 2
	double e1;
	// Energy given up by object 2 to object 1
	double e2;
};

struct BeamCollisionResult
{
	// Time in [0,1] alont dt of the impact
	double t;
	// Energy absorbed by the object given it's shadow, and the size of the wave front
	double e;
	// Direction beam is travelling.
	struct Vector3 d;
	// Collition point
	struct Vector3 p;
	// Occlusion information
	void* occlusion;
};

void PhysicsObject_init(struct PhysicsObject* obj, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_type);
void PhysicsObject_tick(struct PhysicsObject* obj, struct Vector3* g, double dt);

void PhysicsObject_collide(struct PhysCollisionResult* cr, struct PhysicsObject* obj1, struct PhysicsObject* obj2, double dt);
void PhysicsObject_collision(struct PhysicsObject* objt, struct PhysicsObject* othert, double energy, struct PhysCollisionEffect* args);
void PhysicsObject_resolve_damage(struct PhysicsObject* obj, double energy);
void PhysicsObject_resolve_phys_collision(struct PhysicsObject* obj, double energy, struct PhysCollisionEffect* pce);

void SmartPhysicsObject_init(struct SmartPhysicsObject* obj, int client, uint64_t osim_id, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_type);
//void SmartPhysicsObject_collision(struct PhysicsObject* objt, struct PhysicsObject* othert, double energy, struct PhysCollisionEffect* args);
//void SmartPhysicsObject_handle(struct SmartPhysicsObject* obj, void* msg);

void Beam_init(struct Beam* beam, Universe* universe, struct Vector3* origin, struct Vector3* direction, struct Vector3* up, struct Vector3* right, double cosh, double cosv, double area_factor, double speed, double energy, PhysicsObjectType beam_type);
void Beam_init(struct Beam* beam, Universe* universe, struct Vector3* origin, struct Vector3* velocity, struct Vector3* up, double angle_h, double angle_v, double energy, PhysicsObjectType beam_type);
void Beam_collide(struct BeamCollisionResult* bcr, struct Beam* beam, struct PhysicsObject* obj, double dt);
void Beam_tick(struct Beam* beam, double dt);
struct Beam* Beam_make_return_beam(struct Beam* b, double energy, struct Vector3* origin, PhysicsObjectType type);

#endif
