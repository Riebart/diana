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

//! Enumeration of the types of physics objects to allow for branching and basic polymorphics with structs.
enum PhysicsObjectType { PHYSOBJECT, PHYSOBJECT_SMART, BEAM_COMM, BEAM_SCAN, BEAM_SCANRESULT, BEAM_WEAP };

//! A physics object in the universe as well as all of its local variables. Let the compiler pack this one.
struct PhysicsObject
{
    //! Type of PhysicsObject
    PhysicsObjectType type;
    //! Unique ID as assigned by the universe
    uint64_t phys_id;
    //! Universe this object is assigned to.
    Universe* universe;
    //! Axis aligned bounding box for this object.
    struct AABB box;
    //! For collisions, this is in [0,1] and is how far into the interval we've already traversed.
    double t;
    //! Position in metres relative to universal origin
    struct Vector3 position,
        //! Velocity in metres/second
        velocity,
        //! Angular velocity in radians/second
        ang_velocity,
        //! Thrust in Newtons
        thrust,
        //! Normal vector indicating forward of its local basis
        forward,
        //! Normal vector indicating up of its local basis
        up,
        //! Normal vector indicating right of its local basis
        right;
    //! Mass in kilograms.
    double mass,
        //! Radius of bounding sphere in metres centres on its position
        radius,
        //! Health in some arbitrary hit points.
        health;
    //! Arbitrary C-string offering an object description of some sort
    char* obj_desc;
    //! A unique art ID for any art assets as assigned by an ArtCurator.
    uint32_t art_id;
    //! Whether or not this object should be considered an attractor by the Universe.
    //! Make sure to call Universe::update_attractor if you set this or change the mass/radius.
    bool emits_gravity;
};

//! @note The pack() pragmas here actually improve performance under Release MSVS2012 x64 by a very consistent 6%
#pragma pack(1)
//! A smart physics object which is a physics object that ties back to a ship or other object over a socket.
struct SmartPhysicsObject
{
    //! Physical object that forms teh base of the object
    struct PhysicsObject pobj;
    //!// OSim ID (UNUSED)
    //uint64_t osim_id,
    //    //! Parent physical ID (UNUSED)
    //    parent_phys_id,
    //    //! Query ID (UNUSED)
    //    query_id;
    //! Client FD to talk out of.
    int32_t client;
    //!// Is this client registered for vis data
    //bool vis_data,
    //    //! Is this client registered for vis meta data
    //    vis_meta_data,
    //    //! Does this exist in the world (UNUSED)
    //    exists;
};
#pragma pack()

#pragma pack(1)
//! A beam as it exists in the universe
struct Beam
{
    PhysicsObjectType type;
    uint64_t phys_id;
    Universe* universe;
    PhysicsObject* scan_target;
    struct Vector3 origin,
        direction,
        front_position,
        up,
        right;
    double cosines[2];
    double speed,
        area_factor,
        energy,
        distance_travelled,
        max_distance;
};
#pragma pack()

//! The structure that holds the effects on one object in a two-object physical collision
struct PhysCollisionEffect
{
    //! Direction the other object was moving, relative to this, at time of impact (normalized).
    struct Vector3 d;
    //! Position of impact on object's bounding sphere, relative to centre.
    struct Vector3 p;
    //! Velocity of the 'hit' object tangential to impact. This remains unchanged. t+n=v'
    struct Vector3 t;
    //! New velocity along the normal of impact imparted due to impact. t+n=v'
    struct Vector3 n;
};

//! The structure that holds the effects of a phys-phys collision
struct PhysCollisionResult
{
    //! Time in [0,1] alont dt of the impact
    double t;
    //! Energy involved: (m1+m2)dv^2
    double e;
    //! Effect on 'first' object
    struct PhysCollisionEffect pce1;
    //! effect on 'second' object
    struct PhysCollisionEffect pce2;
};

//! The structure that holds the effects of a beam-phys collision
struct BeamCollisionResult
{
    //! Time in [0,1] alont dt of the impact
    double t;
    //! Energy absorbed by the object given it's shadow, and the size of the wave front
    double e;
    //! Direction beam is travelling.
    struct Vector3 d;
    //! Collition point
    struct Vector3 p;
    //! Occlusion information
    void* occlusion;
};

void PhysicsObject_init(struct PhysicsObject* obj, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_desc);
void PhysicsObject_tick(struct PhysicsObject* obj, struct Vector3* g, double dt);

void PhysicsObject_collide(struct PhysCollisionResult* cr, struct PhysicsObject* obj1, struct PhysicsObject* obj2, double dt);
void PhysicsObject_collision(struct PhysicsObject* objt, struct PhysicsObject* othert, double energy, double dt, struct PhysCollisionEffect* args);
void PhysicsObject_resolve_damage(struct PhysicsObject* obj, double energy);
void PhysicsObject_resolve_phys_collision(struct PhysicsObject* obj, double energy, double dt, struct PhysCollisionEffect* pce);
void PhysicsObject_estimate_aabb(struct PhysicsObject* obj, struct AABB* b, double dt);

void SmartPhysicsObject_init(struct SmartPhysicsObject* obj, int32_t client, uint64_t osim_id, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_desc);

void Beam_init(struct Beam* beam, Universe* universe, struct Vector3* origin, struct Vector3* direction, struct Vector3* up, struct Vector3* right, double cosh, double cosv, double area_factor, double speed, double energy, PhysicsObjectType beam_type);
void Beam_init(struct Beam* beam, Universe* universe, struct Vector3* origin, struct Vector3* velocity, struct Vector3* up, double angle_h, double angle_v, double energy, PhysicsObjectType beam_type);
void Beam_collide(struct BeamCollisionResult* bcr, struct Beam* beam, struct PhysicsObject* obj, double dt);
void Beam_tick(struct Beam* beam, double dt);
struct Beam* Beam_make_return_beam(struct Beam* b, double energy, struct Vector3* origin, PhysicsObjectType type);

bool is_big_enough(double m, double r);

#endif
