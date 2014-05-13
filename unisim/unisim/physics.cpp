#include "physics.hpp"

#include <stdlib.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

/// Area of a sphere is 4 pi r^2
/// Multiplying two angles to give a solid angle area results in sa=2 pi^2 square radians for a full sphere
/// Converting that to are square length units means multiplying by sa*((2/pi) r^2)=c where sa=Solid Angle, c=cutoff
#define BEAM_SOLID_ANGLE_FACTOR (2 / M_PI)
#define GRAVITY_CUTOFF 0.01
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef struct Beam B;
typedef PhysicsObjectType POT;
typedef struct PhysicsObject PO;
typedef struct SmartPhysicsObject SPO;
typedef struct Vector3 V3;

/// @todo implement grids, this ties into physics, but the properties need to be added here.

bool is_big_enough(double m, double r)
{
    return ((6.67384e-11 * m / r) >= GRAVITY_CUTOFF);
}

void PhysicsObject_init(PO* obj, Universe* universe, V3* position, V3* velocity, V3* ang_velocity, V3* thrust, double mass, double radius, char* obj_desc)
{
    obj->type = PHYSOBJECT;
    obj->phys_id = 0;
    obj->art_id = -1;

    obj->universe = universe;
    obj->position = *position;
    obj->velocity = *velocity;
    obj->ang_velocity = *ang_velocity;
    obj->thrust = *thrust;
    obj->mass = mass;
    obj->radius = radius;
    obj->obj_desc = obj_desc;

    obj->health = mass * 1000000;
    obj->emits_gravity = is_big_enough(mass, radius);
}

void PhysicsObject_tick(PO* obj, V3* g, double dt)
{
    /// @todo Relativistic mass

    /// @todo Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
    obj->position.x += obj->velocity.x * dt + 0.5 * dt * dt * g->x;
    obj->position.y += obj->velocity.y * dt + 0.5 * dt * dt * g->y;
    obj->position.z += obj->velocity.z * dt + 0.5 * dt * dt * g->z;

    obj->velocity.x += dt * g->x;
    obj->velocity.y += dt * g->y;
    obj->velocity.z += dt * g->z;

    V3 a = { dt * obj->ang_velocity.x, dt * obj->ang_velocity.y, dt * obj->ang_velocity.z };

    Vector3_apply_ypr(&obj->forward, &obj->up, &obj->right, &a);
}

void PhysicsObject_collide(struct PhysCollisionResult* cr, PO* obj1, PO* obj2, double dt)
{
    // Currently we treat dt and forces as small enough that they can be neglected,
    // and assume that they won't change teh trajectory appreciably over the course
    // of dt
    //
    // What we do is get the line segments that each object will trace outside
    // if they followed their current velocity for dt seconds.
    //
    // We then find the minimum distance between those two line segments, and
    // compare that with the sum of the radii (the minimum distance the objects
    // need to be away from each other in order to not collide). Becuse sqrt()
    // is so expensive, we compare the square of everything. Any lengths are
    // actually the squares of the length, which means that we need to compare
    // against the square of the sum of the radii of the objects.
    //
    // Because the parameters for the parameterizations of the line segments
    // will be in [0,1], we'll need to scale the velocity by dt. If we
    // get around to handling force application we need to scale the acceleration
    // vector by 0.5*dt^2, and then apply a parameter of t^2 to that term.
    //
    // Parameters for force-less trajectories are found for object 1 and 2 (t)
    // thanks to some differentiation of parameterizations of the trajectories
    // using a parameter for time. Note that this will find the global minimum
    // and the parameters that come back likely won't be in the [0,1] range.
    //
    // Normally if the parameter is outside of the range [0,1], you should check
    // the endpoints for the minimum. If the parameter is in [0,1], then you
    // can skip endpoing checking, yay! In this case though, we know that if
    // there is an inflection point in [0,1], t will point at it. If there isn't,
    // then the distance function is otherwise monotonic (it is quadratic in t)
    // and we can just clip t to [0,1] and get the minimum.
    //
    //      (v2-v1).(o1-o2)
    // t = -----------------
    //      (v2-v1).(v2-v1)
    //
    // So, here we go!

    V3 v1 = obj1->velocity;
    Vector3_scale(&v1, dt);

    V3 v2 = obj2->velocity;
    Vector3_scale(&v2, dt);

    V3 vd;
    Vector3_subtract(&vd, &v2, &v1);

    if (Vector3_almost_zero(&vd))
    {
        cr->t = -1.0;
        return;
    }

    V3 o1 = obj1->position;
    V3 o2 = obj2->position;

    V3 od;
    Vector3_subtract(&od, &v1, &v2);

    double t = Vector3_dot(&vd, &od) / Vector3_length2(&vd);
    t = MIN(1, MAX(0, t));
    cr->t = t;

    Vector3_fmad(&o1, t, &v1);
    Vector3_fmad(&o2, t, &v2);

    double r = (obj1->radius + obj2->radius);
    r *= r;

    if (Vector3_distance2(&o1, &o2) <= r)
    {
        /// @todo Relativistic kinetic energy
        /// @todo Angular velocity, which reqires location of impact and shape and stuff

        // collision normal: unit vector from points from obj1 to obj2
        V3 n;
        Vector3_subtract(&n, &obj2->position, &obj1->position);
        Vector3_normalize(&n);

        // Collision tangential velocities. These parts don't change in the collision
        // Take the velocity, then subtract off the part that is along the normal
        // (OPTIONAL?!) Normalize the result, and scale by the dot of original velocity with this.
        cr->pce1.t = obj1->velocity;
        Vector3_fmad(&cr->pce1.t, -1 * Vector3_dot(&n, &obj1->velocity), &n);
        //Vector3_normalize(&cr->pce1.t);
        //Vector3_scale(&cr->pce1.t, Vector3_dot(&obj1->velocity, &cr->pce1.t));

        cr->pce2.t = obj2->velocity;
        Vector3_fmad(&cr->pce2.t, -1 * Vector3_dot(&n, &obj2->velocity), &n);
        //Vector3_normalize(&cr->pce2.t);
        //Vector3_scale(&cr->pce2.t, Vector3_dot(&obj2->velocity, &cr->pce2.t));

        // Now we need the amount of energy transferred in each direction along the normal

        // This is the amount of velocity given up by each object.
        // It is the velocity of obj2 relative to obj1, or rather obj2's velocity in obj1's
        // inertial reference frame.
        V3 v;
        Vector3_subtract(&v, &obj2->velocity, &obj1->velocity);
        // The amount of obj2's velocity that is along the normal relative to obj1. Since the normal
        // points from obj1 to obj2, this is negative if there is energy transfer from 2 to 1, and positive
        // if there is energy transfer from 1 to 2.
        //double vdn = Vector3_dot(&v, &n);

        // vn1 and vn2 are the velcoities that the objects are giving up
        // along the normal.
        //
        // Since the normal points from 1 to 2, if obj1 is moving away from 2,
        // then it contiributes nothing to 2, and vice versa.
        Vector3_init(&cr->pce1.n, 0.0, 0.0, 0.0);
        Vector3_init(&cr->pce2.n, 0.0, 0.0, 0.0);

        double vdn;

        // We make an exception here. If the object receiving object has zero mass,
        // it doesn't get any energy.

        // Energy given to obj1.
        if (obj1->mass > 0)
        {
            // This dot is positive if obj1 is moving towards obj2
            vdn = Vector3_dot(&obj1->velocity, &n);
            if (vdn > 0)
            {
                cr->pce1.n = n;
                Vector3_scale(&cr->pce1.n, vdn);
            }
            cr->e1 = Vector3_length2(&cr->pce1.n) * obj1->mass * 0.5;

            // Add the other object's mass-ratio-scaled velocity to our normal
            // contribution.
            Vector3_fmad(&cr->pce1.n, obj2->mass / obj1->mass, &cr->pce2.n);

            // Negate this one because of how the normal points from obj1 to obj2.
            Vector3_scale(&cr->pce1.n, -1.0);
        }

        if (obj2->mass > 0)
        {
            // This dot is negative if obj2 is moving towards obj1.
            vdn = Vector3_dot(&obj1->velocity, &n);
            if (vdn < 0)
            {
                cr->pce2.n = n;
                Vector3_scale(&cr->pce2.n, vdn);
            }
            cr->e2 = Vector3_length2(&cr->pce2.n) * obj2->mass * 0.5;

            // Add the other object's mass-ratio-scaled velocity to our normal
            // contribution.
            Vector3_fmad(&cr->pce2.n, -1 * obj1->mass / obj2->mass, &cr->pce1.n);

            // Don't need to negate here.
        }

        cr->pce1.d = v;
        Vector3_normalize(&cr->pce1.d);
        cr->pce2.d = cr->pce1.d;
        Vector3_scale(&cr->pce2.d, -1);

        cr->pce1.p = o2;
        Vector3_fmad(&cr->pce1.p, -1, &o1);
        Vector3_normalize(&cr->pce1.p);
        cr->pce2.p = cr->pce1.p;
        Vector3_scale(&cr->pce1.p, obj1->radius);
        Vector3_scale(&cr->pce2.p, -1 * obj2->radius);
    }
    else
    {
        cr->t = -1.0;
        return;
    }
}

void PhysicsObject_resolve_damage(PO* obj, double energy)
{
    // We'll take damage if we absorb an impact whose energy is above ten percent
    // of our current health, and only by the energy that is above that threshold

    if (obj->health < 0)
    {
        return;
    }

    double t = 0.1 * obj->health;

    if (energy > t)
    {
        obj->health -= (energy - t);
    }

    if (obj->health <= 0)
    {
        printf("%lu has been destroyed\n", obj->phys_id);
        obj->universe->expire(obj->phys_id);
    }
}

void PhysicsObject_resolve_phys_collision(PO* obj, double energy, struct PhysCollisionEffect* pce)
{
    Vector3_add(&obj->velocity, &obj->velocity, &pce->n);
}

/// @todo fix variable and argument names. They aren't types.
/// @param args This describes the information about the thing that hit obj, that is other.
void PhysicsObject_collision(PO* objt, PO* othert, double energy, struct PhysCollisionEffect* effect)
{
    switch(othert->type)
    {
    case PHYSOBJECT:
    case PHYSOBJECT_SMART:
        {
            PO* obj = (PO*)objt;
            PhysicsObject_resolve_phys_collision(obj, energy, effect);
            PhysicsObject_resolve_damage(obj, energy);
            break;
        }
    case BEAM_WEAP:
        {
            PO* obj = (PO*)objt;
            PhysicsObject_resolve_damage(obj, energy);
            break;
        }
    case BEAM_SCAN:
        {
            PO* obj = (PO*)objt;
            Beam* other = (Beam*)othert;
            Beam* res = Beam_make_return_beam(other, energy, &effect->p, BEAM_SCANRESULT);
            res->type = BEAM_SCANRESULT;
            res->scan_target = obj;
            obj->universe->add_object((PO*)res);
            break;
        }
    default:
        {
            break;
        }
    }
}

void SmartPhysicsObject_init(SPO* obj, int32_t client, uint64_t osim_id, Universe* universe, V3* position, V3* velocity, V3* ang_velocity, V3* thrust, double mass, double radius, char* obj_desc)
{
    PhysicsObject_init(&obj->pobj, universe, position, velocity, ang_velocity, thrust, mass, radius, obj_desc);
    obj->pobj.type = PHYSOBJECT_SMART;

    obj->client = client;
    obj->osim_id = osim_id;
    obj->vis_data = false;
    obj->vis_meta_data = false;
    obj->exists = true;
    obj->parent_phys_id = 0;
}

void Beam_init(B* beam, Universe* universe, V3* origin, V3* direction, V3* up, V3* right, double cosh, double cosv, double area_factor, double speed, double energy, PhysicsObjectType type)
{
    beam->universe = universe;
    beam->origin = *origin;
    beam->direction = *direction;
    beam->front_position = *origin;
    beam->up = *up;
    beam->right = *right;
    beam->speed = speed;
    beam->cosines[0] = cosh;
    beam->cosines[1] = cosv;
    beam->area_factor = area_factor;
    beam->energy = energy;
    beam->type = type;

    beam->distance_travelled = 0;
    beam->max_distance = sqrt(energy / (area_factor * 1e-10));
}

void Beam_init(B* beam, Universe* universe, V3* origin, V3* velocity, V3* up, double angle_h, double angle_v, double energy, PhysicsObjectType beam_type)
{
    V3 direction = *velocity;
    Vector3_normalize(&direction);
    V3 up2 = *up;
    Vector3_normalize(&up2);

    V3 right = { 0, 0, 0 };
    Vector3_cross(&right, &direction, &up2);

    double speed = Vector3_length(velocity);
    double area_factor = BEAM_SOLID_ANGLE_FACTOR * (angle_h * angle_v);
    double cosh = cos(angle_h / 2);
    double cosv = cos(angle_v / 2);

    Beam_init(beam, universe, origin, &direction, &up2, &right, cosh, cosv, area_factor, speed, energy, beam_type);
}

void Beam_collide(struct BeamCollisionResult* bcr, B* b, PO* obj, double dt)
{
    /// @todo Take radius into account

    // Move the object position to a point32_t relative to the beam's origin.
    // Then scale the velocity by dt, and add it to the position to get the
    // start, end, and difference vectors.

    //struct BeamCollisionResult* bcr = (struct BeamCollisionResult*)malloc(sizeof(struct BeamCollisionResult));
    bcr->d = b->direction;

    // Relative position of object to beam origin.
    V3 p = obj->position;
    Vector3_subtract(&p, &p, &b->origin);

    // Object position delta in this tick.
    V3 dp = obj->velocity;
    Vector3_scale(&dp, dt);

    // The object's position at the end of the physics tick.
    V3 p2;
    Vector3_add(&p2, &p, &dp);

    // Get the components of the position at the start and end of the physics tick
    // that lie in the plane defined by the up and right vectors of the beam
    // respectively.

    V3 proj_s[2];
    Vector3_project_down(&proj_s[0], &p, &b->up);
    Vector3_project_down(&proj_s[1], &p, &b->right);

    V3 proj_e[2];
    Vector3_project_down(&proj_e[0], &p2, &b->up);
    Vector3_project_down(&proj_e[1], &p2, &b->right);

    // Now dot with the beam direction to get the angles of the rays make. Do it for
    // the original position, and for the position plus the position delta. Projecting
    // down the up vector gets us the horizontal angle, and down teh right vector
    // gets us the vertical angle.
    //
    // Note that cosine decreses from 1 to -1 as the angle goes from 0 to 180.
    // We are inside, if we are > than the cosine of our beam. Note that since cosine
    // is symmetric about 0, the beam's cosines are actually from the direction to the
    // edge of the volume, and we can just test this half-angle.

    double current[2] = { Vector3_dot(&proj_s[0], &b->direction), Vector3_dot(&proj_s[1], &b->direction) };
    double future[2] =  { Vector3_dot(&proj_e[0], &b->direction), Vector3_dot(&proj_e[1], &b->direction) };
    double delta[2] = { future[0] - current[0], future[1] - current[1] };

    bool current_b[2] = { (current[0] >= b->cosines[0]), (current[1] >= b->cosines[1]) };
    bool future_b[2] =  { (future[0] >= b->cosines[0]), (future[1] >= b->cosines[1]) };

    double entering = -0.1;
    double leaving = 1.1;

    // This assumes that out position delta is small enough that we can consider things linear
    // Lots of linear approximation going on.
    for (int32_t i = 0 ; i < 2 ; i++)
    {
        if (!current_b[i])
        {
            // If we're out, and coming in
            if (future_b[i])
            {
                entering = MAX(entering, ((b->cosines[i] - current[i]) / delta[i]));
            }
            // If we're out and staying out
            else
            {
                bcr->t = -1.0;
                return;
            }
        }
        else
        {
            // If we're in, and going out
            if (!future_b[i])
            {
                leaving = MIN(entering, ((current[i] - b->cosines[i]) / delta[i]));
            }
            // If we're in and staying in
            else
            {
                // Do nothing, this is here for clarity.
            }
        }
    }

    // Bail if we are somehow leaving before we are entering.
    // This can happen if we are entering some planes and leaving others,
    // and are leaving the plane before we enter the other plane, since
    // we need to be inside both planes before 
    if (entering > leaving)
    {
        bcr->t = -1.0;
        return;
    }

    // NOTE: I'm not sure if this hunch is right, but I think it is a
    // close enough approximation. The magic of linear situations might actually
    // make me right...
    //
    // If there are still candidate values of t, then just take the middle of
    // the interval as the 'collision' time that maximizes the energy transfer.
    // This only might work for spheres...
    //
    // Either way, it is probably good enough.

    // Time that maximizes the energy transfer.
    // If we are only entering a plane, this is 1.0.
    // If we are only leaving a plane, this is the time of leaving.
    // If we are entering and leaving, this is the middle?
    // Entering
    if ((entering >= 0.0) && (leaving > 1.0))
    {
        bcr->t = 1.0;
    }
    else if ((leaving <= 1.0) && (entering < 0.0))
    {
        bcr->t = leaving;
    }
    else
    {
        bcr->t = (entering + leaving) / 2.0;
    }

    bcr->p = p;
    Vector3_fmad(&bcr->p, bcr->t, &dp);
    double collision_dist = abs(Vector3_dot(&bcr->p, &b->direction));

    // If we want collisions for as long as the bounding sphere is intersecting
    // the front, use:
    //
    // if ((collision_dist + obj.radius) >= b.distance_travelled) and ((collision_dist - obj.radius) <= (b.distance_travelled + b.speed * dt)):
    if ((collision_dist >= b->distance_travelled) &&
        (collision_dist <= (b->distance_travelled + b->speed * dt)))
    {
        double wavefront_area = b->area_factor * collision_dist * collision_dist;
        double object_surface = M_PI * obj->radius * obj->radius;

        double energy_factor = object_surface / wavefront_area;
        energy_factor = MIN(1.0, energy_factor);
        bcr->e = b->energy * energy_factor;
    }
    else
    {
        bcr->t = -1.0;
        return;
    }
}

void Beam_tick(B* beam, double dt)
{
    beam->distance_travelled += beam->speed * dt;
    Vector3_fmad(&beam->front_position, dt * beam->speed, &beam->direction);

    if (beam->distance_travelled > beam->max_distance)
    {
        beam->universe->expire(beam->phys_id);
    }
}

B* Beam_make_return_beam(B* beam, double energy, V3* origin, PhysicsObjectType type)
{
    V3 d;
    Vector3_subtract(&d, &beam->origin, origin);
    Vector3_normalize(&d);

    V3 up = { -1 * d.y, d.x, 0.0 };
    Vector3_normalize(&up);
    V3 right;
    Vector3_cross(&right, &d, &up);

    B* ret = (B*)malloc(sizeof(B));
    Beam_init(ret, beam->universe, origin, &d, &up, &right, beam->cosines[0], beam->cosines[1], beam->area_factor, beam->speed, energy, type);
    return ret;
}
