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

// g++ has C++11 atomics with --std=c++11, and WIN32 does from VS2012+
#include <atomic>
#include <thread>
#include <mutex>

typedef std::mutex LOCK_T;
typedef std::thread THREAD_T;
typedef std::atomic<int64_t> ATOMIC_T;

#include "vector.hpp"
#include "physics.hpp"
#include "MIMOServer.hpp"
#include "messaging.hpp"
#include "bson.hpp"

//#include "scheduler.hpp"

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
        struct PhysCollisionEvent;
        
        friend void* sim(void* u);

        friend void Universe_hangup_objects(int32_t c, void* arg);
        friend void Universe_handle_message(int32_t socket, void* arg);
        friend void PhysicsObject_init(struct PhysicsObject* obj, Universe* universe, struct Vector3* position, struct Vector3* velocity, struct Vector3* ang_velocity, struct Vector3* thrust, double mass, double radius, char* obj_desc);

        friend void obj_tick(Universe* u, struct PhysicsObject* o, double dt);
        friend void* thread_check_collisions(void* argsV);
        friend void check_collision_loop(void* argsV);
        friend bool check_collision_single(Universe* u, struct PhysicsObject* obj1, struct PhysicsObject* obj2, double dt, struct Universe::PhysCollisionEvent& ev);

        friend void* vis_data_thread(void* argv);

    public:
        Universe(double min_frametime, double max_frametime, double min_vis_frametime, int32_t port, int32_t num_threads, double rate = 1.0, bool realtime = true);
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
        void get_frametime(double* out);

        //! Get the total number of ticks so far.
        uint64_t get_ticks();

        //! Get the total amount of time passed inside the simulation.
        double total_sim_time();

        //! Add an object to the universe. It will appear on the next physics tick.
        //! This function is used when the universe has never seen the object before.
        //! Objects have their phys_id property set, and are queued to be added
        //! at the end of the current physics tick.
        //! @param obj PhysicsObject to add to add. Can also be a recast Beam pointer.
        void add_object(struct PhysicsObject* obj);
        void add_object(struct Beam* beam);

        //! Queue an object for expiry in the next physics tick.
        void expire(int64_t phys_id);

        //! Expire all objects in the universe associated with the given client.
        void hangup_objects(int32_t c);

    private:
        int64_t get_id();
        void broadcast_vis_data();
        void tick(double dt);
        void sort_aabb(double dt, bool calc);
        void handle_message(int32_t socket);
        
        // Take care of expiring objects from the universe at the end of a physics tick.
        void handle_expired();
        // Take care of adding queued objects to the universe at the end of a physics tick.
        void handle_added();

        //! This is called to update either the attractors or radiators lists, and is supplied
        //! with the physics object in question, as well as the new and old values for the 
        //! conditional boolean as appropriate.
        void update_list(struct PhysicsObject* obj, std::vector<struct PhysicsObject*>* list, bool newval, bool oldval);
        
        void get_grav_pull(struct Vector3* g, struct PhysicsObject* obj);

        struct vis_client
        {
            int32_t socket;
            int64_t client_id;
            int64_t phys_id;

            bool operator <(const struct vis_client& rhs) const
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

            bool operator ==(const struct vis_client& rhs) const
            {
                return ((socket == rhs.socket) && (client_id == rhs.client_id) && (phys_id == rhs.phys_id));
            }
        };

        VisualDataMsg visdata_msg;

        // Register a socket to receive VisData messages.
        void register_vis_client(struct vis_client vc, bool enabled);
        std::vector<struct vis_client> vis_clients;

        MIMOServer* net;
        //libodb::Scheduler* sched;

        std::map<int64_t, struct SmartPhysicsObject*> smarties;
        std::vector<struct PhysicsObject*> attractors;
        std::vector<struct PhysicsObject*> radiators;
        std::vector<struct PhysicsObject*> phys_objects;
        std::vector<struct Beam*> beams;
        std::set<int64_t> expired;
        std::vector<struct PhysicsObject*> added;

        struct PhysCollisionEvent
        {
            struct PhysicsObject* obj1;
            size_t obj1_index;
            struct PhysicsObject* obj2;
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

            bool operator <(const struct scan_target& rhs) const
            {
                // Could also use std::tie
                return (beam_id == rhs.beam_id ? target_id < rhs.target_id : beam_id < rhs.beam_id);
            }

            bool operator ==(const struct scan_target& rhs) const
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
            struct Beam* origin_beam;
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
            Universe* u;
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

        struct phys_args* phys_worker_args;

        //! @todo Move these threaded operations onto the scheduler, let it handle the asynchronous stuff
        THREAD_T sim_thread;
        THREAD_T vis_thread;
        LOCK_T add_lock;
        LOCK_T expire_lock;
        LOCK_T phys_lock;
        LOCK_T vis_lock;
        LOCK_T query_lock;
        LOCK_T collision_lock;

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
