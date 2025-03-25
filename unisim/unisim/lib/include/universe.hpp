#ifndef UNIVERSE_HPP
#define UNIVERSE_HPP

#include <stdint.h>
#include <stdlib.h>

//! @todo We should probably namespace all of this at some point.

// We get these from MIMOServer.hpp too
#include <map>
#include <vector>
#include <set>
#include <list>
#include <random>

// g++ has C++11 atomics with --std=c++11, and WIN32 does from VS2012+
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

typedef std::mutex LOCK_T;
typedef std::thread THREAD_T;
typedef std::atomic<int64_t> ATOMIC_T;

#include "vector.hpp"
#include "physics.hpp"
#include "MIMOServer.hpp"
#include "messaging.hpp"
#include "bson.hpp"

// #include "scheduler.hpp"

namespace Diana
{
//! Contains the code that handles physics simulation and communicating via smart objects
//!
//! The basic idea here is that the universe is filled with PhysicsObjects,
//! of which there are three types: PhysicsObjects, Beams, and SmartPhysicsObjects.
//! Because there are only three types, structs are used as opposed to classes
//! and inheritance, also because there could potentially be so many of these
//! objects. The three structs are in physics.hpp/cpp, and all share a first member
//! that is a PhysicsObjectType. For this reason, pointers to these objects
//! can be cast between types, and PhysicsObject is taken as the canonical 'base'
//! struct that others are cast away from after examining the object type.
//!
//! PhysicsObjects contain all of the information necessary to perform physical
//! calculations. Position, mass, velocity, thrust, and more are all stored.
//!
//! SmartPhysicsObjects are a PhysicsObject with additional information included
//! such as a socket FD and OSim ID that allow it to be tied to an object there.
//! SmartPhysicsObjects can interact with the universe in special ways, such as
//! spawning new objects (beams most commonly, but other objects as well), and
//! receiving special communications upon collision events.
//!
//! Beams are dumb (in that they do not correspond to an OSim object) like
//! PhysicsObjects, but have fundamentally different physical behaviour.
//!
//! The physics simulation is computationally intensive, and as such there are
//! ways of measuring whether it is fast enough to be 'real time' or not. The
//! simulation takes a min_frametime and max_frametime as well as a rate parameter.
//! The minimum frametime prevents the simulation from consuming all of the host
//! unnecessarily. The maximum frametime prevents the simulation from growing too
//! temporally coarse by capping the time per simulation step. The simulation is
//! allowed to organically simulate things in real-time with variable-sized time
//! steps (that is, however long the simulation takes to process a tick) as long
//! as its simulation ticks do not exceed the maximum frametime. When they do,
//! the perceived elapsed time is capped to the maximum frametime, and there will
//! appear to be slowing down of the progression of time within the game universe.
//! This is done to ensure that physics simulations are still accurate even under
//! heavy load. The rate parameter allows for arbitrary speedup or slowdown of
//! game time to be applied.
//!
//! Not all objects in the universe emit gravity. The cutoff is defined in physics.cpp
//! and the Universe keeps track of gravity emitters separately to improve performance.
//! If, for some reason, an object's mass or radius changes, it's gravitational
//! emission state should be updated in the universe.
//!
//! Each object gets an instance-unique 64-bit, monotonically increasing, integer
//! as its ID. This ID will never be re-used, and since it uniquely identifies that
//! object in the universe this ID is used as a reference to that object whenever
//! a reference needs to be made.
//!
//! The simulation can be run in a non-realtime manner by specifying realtime=false
//! on construction. This is useful for simulations that do not rely on real-time
//! constraints and are, perhaps, not interactive.
class Universe
{
public:
#ifdef INSTRUMENT_PHYSICS
#define HRN std::chrono::high_resolution_clock::now()
#define HRN_T(v) std::chrono::time_point<std::chrono::high_resolution_clock> v();
#define HRN_DT std::chrono::nanoseconds
#define HRN_COUNT(t) t.count()
#else
#define HRN 0
#define HRN_T(v) uint64_t v = 0;
#define HRN_DT uint64_t
#define HRN_COUNT(t) t
#endif

    struct CollisionMetrics
    {
        size_t
        primary_aabb_tests,
        secondary_aabb_tests,
        sphere_tests,
        simultaneous_collisions,
        collision_rounds,
        collisions;
        HRN_DT aabb_test_ns,
               sphere_test_ns,
               total_ns;

        CollisionMetrics();
        void add(struct CollisionMetrics other);
        void _fprintf(FILE *fd);
    };

    struct TickMetrics
    {
        struct CollisionMetrics collision_metrics, multicollision_metrics;
        uint64_t num_objects;
        HRN_DT sort_aabb_ns,
               object_tick_ns,
               beam_tick_ns,
               thread_join_wait_ns,
               collision_resolution_ns,
               object_lifecycle_ns,
               total_ns;

        TickMetrics();
        void _fprintf(FILE *fd);
    };

private:
    struct PhysCollisionEvent;

    friend void *sim(void *u);

    friend void Universe_hangup_objects(int32_t c, void *arg);
    friend void Universe_handle_message(int32_t socket, void *arg);

    friend void PhysicsObject_init(struct PhysicsObject *obj, Universe *universe, struct Vector3 *position, struct Vector3 *velocity, struct Vector3 *ang_velocity, struct Vector3 *thrust, double mass, double radius, char *obj_desc, struct Spectrum *spectrum);
    friend void Beam_init(struct Beam *beam, Universe *universe, struct Vector3 *origin, struct Vector3 *direction, struct Vector3 *up, struct Vector3 *right, double cosh, double cosv, double area_factor, double speed, double energy, PhysicsObjectType type, char *comm_msg, char *data, struct Spectrum *spectrum);

    friend void obj_tick(Universe *u, struct PhysicsObject *o, double dt);
    friend void *thread_check_collisions(void *argsV);
    friend struct Universe::CollisionMetrics check_collision_loop(void *argsV);
    friend bool check_collision_single(Universe *u, struct PhysicsObject *obj1, struct PhysicsObject *obj2, double dt, struct Universe::PhysCollisionEvent &ev);

    friend void *vis_data_thread(void *argv);

public:
    struct Parameters
    {
        Parameters() : verbose_logging(true),
            min_physics_frametime(0.002),
            max_physics_frametime(0.002),
            permit_spin_sleep(false),
            min_vis_frametime(0.1),
            network_port(5505),
            num_worker_threads(1),
            simulation_rate(1.0),
            no_realtime_physics(false),
            gravitational_constant(6.67384e-11),
            speed_of_light(299792458l),
            collision_energy_cutoff(1e-9),
            id_rand_max(1),
            max_simultaneous_collision_rounds(100),
            gravity_magnitude_cutoff(0.01),
            beam_energy_cutoff(1e-10),
            radiation_energy_cutoff(1.5e4),
            spectrum_slush_range(0.01),
            health_damage_threshold(0.1),
            health_mass_scale(1e6),
            visual_acuity(4e-7) {}

        // Print verbose information, such as collision rounds per tick, and when a
        // collision happens, and which objects were involved.
        bool verbose_logging;

        // Minimum unscaled simulated time that is allowed to pass in a single tick.
        // If the previous phsyics tick took less wall-clock time than this, the time
        // delta for the next tick is increased up to this amount.
        // Setting this value too low will result in increased CPU use on the physics
        // server to simulate at a time step that is finer than necessary.
        // Setting this value too high may result in too coarse of a physics time step.
        double min_physics_frametime;

        // Maximum unscaled simulated time that is allowed to pass in a single tick.
        // If the previous physics tick took more wall-clock time than this, the time
        // delta for the next tick is reduced to this amount.
        // If physics ticks routinely take more wall clock time than this amount, the
        // clamping of simulated time will result in a perceived slowdown of the
        // simulation to slower than realtime.
        // Setting this value too high may result in a physics simulation that is too
        // coarse. Setting this value too low may result in unnecessary apparent slowdown
        // of the game to achieve a time step that may be finer than necessary.
        double max_physics_frametime;

        // Permit spin sleeping to slow realtime simulations down to real time.
        bool permit_spin_sleep;

        // The minimum wall-clock time that will pass between subsequent rounds of
        // vis-data transmission.
        // Setting this value too low will result in vis data transmission beginning to
        // interfere with the simulation's ability to effectively compute physics ticks
        // at a sufficient rate.
        // It is recommended that clients perform local interpolation to provide smooth
        // apparent motion in between the vis data update frames. First-order interpolation
        // should be considered minimal, with second-order interpolation preferable.
        double min_vis_frametime;

        // TCP port to listen for connections on.
        int32_t network_port;

        // Number of threads to use for service work tasks (physics, vis-data transmission,
        // etc...).
        // Note that this does not control the threads used for TCP clients, as every client
        // receives it's own thread independent of this setting.
        int32_t num_worker_threads;

        // When paired with no_realtime_physics=true, this controls a simulation rate relative to
        // realtime. Values <1.0 result in simulations that are slower than real time, and
        // values >1.0 result in simulations faster than realtime.
        double simulation_rate;

        // When set, the simulation will sleep as appropriate to try to match the amount of
        // simulated time to the amount of elapsed wall-clock time.
        // When using this option=false, careful and informed selection of minimum and maximum
        // physics tick time limits should be used to ensure a smooth and sufficiently
        // fine simulation without forcing overly fine simulation at the expense of a
        // slowdown.
        bool no_realtime_physics;

        // Universal gravitational constant.
        double gravitational_constant;

        // The speed of light in this universe. The defualt speed for beams, and the speed at
        // which relativistic effects do/would take effect.
        double speed_of_light;

        // Collisions (physical or beam) that result in a transfer of energy below this amount
        // are ignored. This helps to ensure that spurious collisions (stiction) are gracefully
        // handled.
        double collision_energy_cutoff;

        // Maximum value used when selecting the random increment for the next physics ID generated
        // by the USim. Setting this to 1 will result in sequential IDs assigned to objects.
        // For live servers, it is recommended to set this in the range of around 10-million (the
        // exact value should be randomly chosen in that range, so as to provide an amount of
        // entropy for hiding the actual IDs of objects from players).
        int64_t id_rand_max = 1;

        // Maximum number of rounds of collision simulation in which simultaneous collisions are
        // considered. This value is only going to come into play with precisely placed objects
        // like done in code. True simultaneous collisions are astronomically unlikely to occur
        // in a real simulation scenario.
        double max_simultaneous_collision_rounds;

        // Objects that would produce a gravitational acceleration below this amount at their
        // bounding radius are not considered attractors in the universe. This is used to
        // optimize the selection of objects that are considered attractors for practical
        // purposes, to reduce the O(N^2) nature of gravitational calculations.
        double gravity_magnitude_cutoff;

        // On initialization, a beam has a maximum distance that is calculated from it's spread
        // values, energy, and this cutoff. The maximum distance, D, is the amount of distance
        // travelled, such that the wavefront at D distance from the source has less than this
        // amount of energy (Joules) per square metre of wavefront area.
        // Raising this value will expire beams sooner, and this may improve performance if
        // large number of beams are in use.
        // This value is derived from commodity wireless transceivers that operate at -70dBmW.
        double beam_energy_cutoff;

        // This value is used to calculate the safe distance of a radiation source, that is the
        // distance from the source at which other objects begin to incur radiation collision
        // events (and damage). Raising this value will reduce the damage caused by radiation
        // sources. The unts of this value is Watts per square metre.
        // This value is derived from the black-body radiative power of steel at it's melting
        // point. Steel absorbing this amount of radiative power.
        // See: http://nssdc.gsfc.nasa.gov/planetary/factsheet/mercuryfact.html
        // See: https://en.wikipedia.org/wiki/Mercury_(planet)#Surface_conditions_and_exosphere
        // To compare, Sol outputs 61.7MW/m^2 at it's surface.
        double radiation_energy_cutoff;

        // Spectrum power levels are randomly adjusted by this proportion upon receipt by the
        // universe to provide a statistical guarantee that two objects don't have identical
        // signature spectra. If teh power level is below this amount, then it is set to a
        // random value between 0 and this value, inclusive.
        double spectrum_slush_range;

        // During a collision, non-smart objects make take damage equal to one point of health
        // per Joule of collision energy that exceeds this proportion of the object's current
        // health.
        double health_damage_threshold;

        // Non-smart physics objects are assigned a number of hit points that is their mass
        // multiplied by this value.
        double health_mass_scale;

        // The cutoff (in (radius/distance)^2) used to determine whether a given object is sent
        // as a piece of visual data. The square is to prevent unnecessary square roots being
        // performed frequently. The physical units of this tan(radians).
        // This assumes an acuity of approximately 60cm at 1km, which roughly corresponds to a
        // typical human
        double visual_acuity;
    };

    Universe(struct Parameters _params);
    ~Universe();

    void start_net();
    void stop_net();
    void start_sim();
    void pause_sim();
    void pause_visdata();
    void stop_sim();

    //! The parameter should have space for four doubles:
    //! The time spent actually calculating physics.
    //! The wall clock time spent on the last tick, including physics and sleeping
    //! The time elapsed in game on the last physics tick.
    //! The wall clock duration of the last visdata blast.
    void get_frametime(double *out);

    //! Get the total number of ticks so far.
    uint64_t get_ticks();

    struct TickMetrics get_tick_metrics();

    //! Get the total amount of time passed inside the simulation.
    double total_sim_time();

    //! Add an object to the universe. It will appear on the next physics tick.
    //! This function is used when the universe has never seen the object before.
    //! Objects have their phys_id property set, and are queued to be added
    //! at the end of the current physics tick.
    //! @param obj PhysicsObject to add to add. Can also be a recast Beam pointer.
    void add_object(struct PhysicsObject *obj);
    void add_object(struct Beam *beam);

    //! Queue an object for expiry in the next physics tick.
    void expire(int64_t phys_id);

    //! Expire all objects in the universe associated with the given client.
    void hangup_objects(int32_t c);

private:
    int64_t get_id();
    void broadcast_vis_data();
    struct Universe::TickMetrics tick(double dt);
    void sort_aabb(double dt, bool calc);
    void handle_message(int32_t socket);

    // Take care of expiring objects from the universe at the end of a physics tick.
    void handle_expired();
    // Take care of adding queued objects to the universe at the end of a physics tick.
    void handle_added();

    //! Generate a random number according to the universe's spectrum perturbation rules.
    double gen_rand(std::normal_distribution<double> dist);

    //! This is called to update either the attractors or radiators lists, and is supplied
    //! with the physics object in question, as well as the new and old values for the
    //! conditional boolean as appropriate.
    void update_list(struct PhysicsObject *obj, std::vector<struct PhysicsObject *> *list, bool newval, bool oldval);

    void get_grav_pull(struct Vector3 *g, struct PhysicsObject *obj);

    double time();

    struct Parameters params;

    struct TickMetrics tick_metrics;

    // Random generation engine used for generating random increments for the IDs.
    std::default_random_engine re;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    struct vis_client
    {
        int32_t socket;
        int64_t client_id;
        int64_t phys_id;

        bool operator<(const struct vis_client &rhs) const
        {
            // Could also use std::tie
            if (socket == rhs.socket)
            {
                if (client_id == rhs.client_id)
                {
                    return phys_id < rhs.phys_id;
                }
                else
                {
                    return client_id < rhs.client_id;
                }
            }
            else
            {
                return socket < rhs.socket;
            }
        }

        bool operator==(const struct vis_client &rhs) const
        {
            return ((socket == rhs.socket) && (client_id == rhs.client_id) && (phys_id == rhs.phys_id));
        }
    };

    VisualDataMsg visdata_msg;

    // Register a socket to receive VisData messages.
    void register_vis_client(struct vis_client vc, bool enabled);
    std::vector<struct vis_client> vis_clients;

    MIMOServer *net;
    // libodb::Scheduler* sched;

    std::map<int64_t, struct SmartPhysicsObject *> smarties;
    std::vector<struct PhysicsObject *> attractors;
    std::vector<struct PhysicsObject *> radiators;
    std::vector<struct PhysicsObject *> phys_objects;
    std::vector<struct Beam *> beams;
    std::set<int64_t> expired;
    std::vector<struct PhysicsObject *> added;

    struct PhysCollisionEvent
    {
        struct PhysicsObject *obj1;
        size_t obj1_index;
        struct PhysicsObject *obj2;
        size_t obj2_index;
        struct PhysCollisionResult pcr;
    };
    // Vector of all collisions encountered in the current tick
    std::vector<struct PhysCollisionEvent> collisions;

    // Represents the pair of IDs that uniquely identifies a beam/object collision event.
    // This is used as the index object for SCAN queries sent to the OSIM.
    struct scan_target
    {
        int64_t beam_id;
        int64_t target_id;

        bool operator<(const struct scan_target &rhs) const
        {
            // Could also use std::tie
            return (beam_id == rhs.beam_id ? target_id < rhs.target_id : beam_id < rhs.beam_id);
        }

        bool operator==(const struct scan_target &rhs) const
        {
            return ((beam_id == rhs.beam_id) && (target_id == rhs.target_id));
        }
    };

    // The information required to build the response beam when the response comes back
    // from the OSIM. It requires the energy of the impact, position of impact, and the
    // original beam to recreate dispersion and orientation properties.
    struct scan_origin
    {
        //! @todo Why won't this let me use a struct, it complains about undefined type.
        struct Beam *origin_beam;
        double energy;
        struct Vector3 hit_position;
    };

    //! Keeps track of the queries from SCAN beam collisions that are in progress.
    //! When a scan beam collides with a smart objects, certain information can
    //! be reported, but that requires a query to the OSim. These queries are
    //! sent over the network, with a unique ID, and the query ID is logged in
    //! this structure. When the query result message comes back, the original
    //! collision information is retrieved, the SCANRESULT beam is built and
    //! added to the universe.
    std::map<struct scan_target, struct scan_origin> queries;

    //! Structure holding the arguments for the threaded checking of collisions
    struct phys_args
    {
        //! Universe to check
        Universe *u;
        //! Position in the sorted list to start at, 0-based
        size_t offset;
        //! Amount to move along the sorted list after processing.
        size_t stride;
        //! Time tick to use for real collision testing.
        double dt;
        //! Whether to test all objects against the original offset object, used in multipass-collision testing.
        bool test_all;
        //! Whether or not this worker has finished its work
        volatile bool done;
    };

    struct phys_args *phys_worker_args;

    //! @todo Move these threaded operations onto the scheduler, let it handle the asynchronous stuff
    THREAD_T sim_thread;
    THREAD_T vis_thread;
    LOCK_T add_lock;
    LOCK_T expire_lock;
    LOCK_T phys_lock;
    LOCK_T vis_lock;
    LOCK_T query_lock;
    LOCK_T collision_lock;
    LOCK_T rand_lock;

    //! Rate (1.0 = real time) at which to simulate the world. Useful for speeding up orbital mechanics.
    double rate;
    //! Total time elapsed in the game world
    double total_time;
    //! Last persitent environmental effect time. Total simulation time that the last event was
    //! triggered for the last environment effect (radiation, etc...)
    double last_effect_time;
    //! The minimum time to spend on a physics frame, this can be used to keep CPU usage down or to smooth out ticks.
    double min_frametime;
    //! The maximum allowed time to elapse in game per tick. This prevents physics ticks from getting too coarse.
    double max_frametime;
    //! The time spent calculating the physics on the last physics tick.
    double phys_frametime;
    //! The time elapsed calcualting physics and sleeping (if that happened) on the last physics tick.
    double wall_frametime;
    //! The time elapsed in the game world on the last physics tick.
    double game_frametime;
    //! The minimum time to spend between VISDATA blasts.
    double min_vis_frametime;
    //! The time spent sending out the last VISDATA blast.
    double vis_frametime;
    //! Total number of physics ticks so far.
    volatile uint64_t num_ticks;

    int32_t num_threads;
    // Because these are used to control the running-state of threads, we need them to update
    // across caching with some reliability.
    volatile bool paused;
    volatile bool visdata_paused;
    volatile bool running;
    bool realtime;

    ATOMIC_T total_objs;
};
}

#endif
