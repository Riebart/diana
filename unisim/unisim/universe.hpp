#ifndef UNIVERSE_HPP
#define UNIVERSE_HPP

#include <stdint.h>

// We get these from MIMOServer.hpp too
#include <map>
#include <vector>

/// @todo Should we be using typedefs instead of #defines here?

#ifdef CPP11THREADS
// GCC doesn't have C++11 atomics, but WIN32 does from VS2012+
#include <atomic>
#include <thread>
#include <mutex>

#define LOCK_T std::mutex
#define THREAD_T std::thread
#define ATOMIC_T std::atomic_uint64_t
#else
#include <pthread.h>
#define LOCK_T pthread_rwlock_t
#define THREAD_T pthread_t
#define ATOMIC_T uint64_t
#endif

#include "vector.hpp"
#include "physics.hpp"
#include "MIMOServer.hpp"


/// Contains the code that handles physics simulation and communicating via smart objects
///
/// The basic idea here is that the universe is filled with PhysicsObjects,
/// of which there are three types: PhysicsObjects, Beams, and SmartPhysicsObjects.
/// Because there are only three types, structs are used as opposed to classes
/// and inheritance, also because there could potentially be so many of these
/// objects. The three structs are in physics.hpp/cpp, and all share a first member
/// that is a PhysicsObjectType. For this reason, pointers to these objects
/// can be cast between types, and PhysicsObject is taken as the canonical 'base'
/// struct that others are cast away from after examining the object type.
///
/// PhysicsObjects contain all of the information necessary to perform physical
/// calculations. Position, mass, velocity, thrust, and more are all stored.
///
/// SmartPhysicsObjects are a PhysicsObject with additional information included
/// such as a socket FD and OSim ID that allow it to be tied to an object there.
/// SmartPhysicsObjects can interact with the universe in special ways, such as 
/// spawning new objects (beams most commonly, but other objects as well), and
/// receiving special communications upon collision events. 
///
/// Beams are dumb (in that they do not correspond to an OSim object) like
/// PhysicsObjects, but have fundamentally different physical behaviour. 
///
/// The physics simulation is computationally intensive, and as such there are
/// ways of measuring whether it is fast enough to be 'real time' or not. The 
/// simulation takes a min_frametime and max_frametime as well as a rate parameter.
/// The minimum frametime prevents the simulation from consuming all of the host
/// unnecessarily. The maximum frametime prevents the simulation from growing too
/// temporally coarse by capping the time per simulation step. The simulation is
/// allowed to organically simulate things in real-time with variable-sized time
/// steps (that is, however long the simulation takes to process a tick) as long
/// as its simulation ticks do not exceed the maximum frametime. When they do,
/// the perceived elapsed time is capped to the maximum frametime, and there will
/// appear to be slowing down of the progression of time within the game universe.
/// This is done to ensure that physics simulations are still accurate even under
/// heavy load. The rate parameter allows for arbitrary speedup or slowdown of 
/// game time to be applied.
///
/// Not all objects in the universe emit gravity. The cutoff is defined in physics.cpp
/// and the Universe keeps track of gravity emitters separately to improve performance.
/// If, for some reason, an object's mass or radius changes, it's gravitational
/// emission state should be updated in the universe.
///
/// Each object gets an instance-unique 64-bit, monotonically increasing, integer
/// as its ID. This ID will never be re-used, and since it uniquely identifies that
/// object in the universe this ID is used as a reference to that object whenever
/// a reference needs to be made.
///
/// The simulation can be run in a non-realtime manner by specifying realtime=false
/// on construction. This is useful for simulations that do not rely on real-time
/// constraints and are, perhaps, not interactive.
class Universe
{
    friend void* sim(void* u);

    friend void Universe_hangup_objects(int32_t c, void* arg);
    friend void Universe_handle_message(int32_t c, void* arg);
    friend void PhysicsObject_init(struct PhysicsObject* obj, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_desc);

public:
    Universe(double min_frametime, double max_frametime, double min_vis_frametime, int32_t port, int32_t num_threads, double rate = 1.0, bool realtime = true);
    ~Universe();
    void start_net();
    void stop_net();
    void start_sim();
    void pause_sim();
    void stop_sim();

    /// The parameter should have space for four doubles:
    /// The time spent actually calculating physics.
    /// The wall clock time spent on the last tick, including physics and sleeping
    /// The time elapsed in game on the last physics tick.
    /// The wall clock duration of the last visdata blast.
    void get_frametime(double* out);

    /// Get the total number of ticks so far.
    uint64_t get_ticks();

    /// Get the total amount of time passed inside the simulation.
    double total_sim_time();

    /// Add an object to the universe. It will appear on the next physics tick.
    /// This function is used when the universe has never seen the object before.
    /// Objects have their phys_id property set, and are queued to be added
    /// at the end of the current physics tick.
    /// @param obj PhysicsObject to add to add. Can also be a recast Beam pointer.
    void add_object(struct PhysicsObject* obj);
    void add_object(struct Beam* beam);

    /// Queue an object for expiry in the next physics tick.
    void expire(uint64_t phys_id);

    /// Expire all objects in the universe associated with the given client.
    void hangup_objects(int32_t c);

    /// Update whether or not an object emits gravity.
    void update_attractor(struct PhysicsObject* obj, bool calculate);

    void register_for_vis_data(uint64_t phys_id, bool enable);

private:
    uint64_t get_id();
    void broadcast_vis_data();
    void tick(double dt);
    void handle_message(int32_t c);
    void get_grav_pull(struct Vector3* g, struct PhysicsObject* obj);

    MIMOServer* net;
    //ArtCurator* curator;

    std::map<uint64_t, struct SmartPhysicsObject*> smarties;
    std::vector<struct PhysicsObject*> attractors;
    std::vector<struct PhysicsObject*> phys_objects;
    std::vector<struct Beam*> beams;
    std::vector<uint64_t> expired;
    std::vector<struct PhysicsObject*> added;
    /// Keeps track of the queries from SCAN beam collisions that are in progress.
    /// When a scan beam collides with a smart objects, certain information can
    /// be reported, but that requires a query to the OSim. These queries are
    /// sent over the network, with a unique ID, and the query ID is logged in
    /// this structure. When the query result message comes back, the original
    /// collision information is retrieved, the SCANRESULT beam is built and
    /// added to the universe.
    std::map<uint64_t, uint64_t> queries;
    std::vector<int32_t> vis_clients;

    THREAD_T sim_thread;
    THREAD_T vis_thread;
    LOCK_T add_lock;
    LOCK_T expire_lock;
    LOCK_T phys_lock;
    LOCK_T vis_lock;

    /// Rate (1.0 = real time) at which to simulate the world. Useful for speeding up orbital mechanics.
    double rate;
    /// Total time elapsed in the game world
    double total_time;
    /// The minimum time to spend on a physics frame, this can be used to keep CPU usage down or to smooth out ticks.
    double min_frametime;
    /// The maximum allowed time to elapse in game per tick. This prevents physics ticks from getting too coarse.
    double max_frametime;
    /// The time spent calculating the physics on the last physics tick.
    double phys_frametime;
    /// The time elapsed calcualting physics and sleeping (if that happened) on the last physics tick.
    double wall_frametime;
    /// The time elapsed in the game world on the last physics tick.
    double game_frametime;
    /// The minimum time to spend between VISDATA blasts.
    double min_vis_frametime;
    /// The time spent sending out the last VISDATA blast.
    double vis_frametime;
    /// Total number of physics ticks so far.
    uint64_t num_ticks;

    int32_t num_threads;
    bool paused;
    bool running;
    bool realtime;

    ATOMIC_T total_objs;
};

#endif
