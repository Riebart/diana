#include "universe.hpp"
#include "messaging.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#include <algorithm>

#include "utility.hpp"

// Memory allocation practices in the universe simulation
//
// An instance of the Universe class takes almost responsibility for the compelte
// management of the lifecycle of memory for physics objects in it's purview.
// The only exception for the compelteness of control is when objects are added
// directly by Universe::add_object(), in which case the allocation of memory
// must be done outside of the universe, as add_object() assumes that memory is
// already allocated.
//
// Memory allocation occurs in the following flows:
//
// - Constructor for worker-thread arguments
//   > A runtime-sized array is allocated to hold return worker information, this
//     array is deallocated in the destructor.
//
// - PhysicalPropertiesMsg received on the network containing a specified obj_type
//   > A replacement obj_type string is allocated, and if allocation succeeds the
//     existing array is freed, and the pointer set to the new array (which had the
//     message's obj_type array copied in).
//
// - SpawnMsg message received on the network.
//   > The new PhysicsObject (or SmartPhysicsObject) is allocated, filled with the
//     parameters from the message object (which must be fully specified except for
//     the server_id, which must be unspecified). The obj_type char* is also allocated
//     and copied from message, since the memory allocated for that string in the
//     message will be freed when the message is destroyed (in ~BSONMessage()).
//
// - BeamMsg message received on the network.
//   > Similar to a SpawnMsg, Beams are allocated and filled from the message, which
//     must he FULLY specified, including server_id which will point to the parent
//     object for relative position/velocity composition. As with physical objects,
//     the comm_msg char* string is reallocated and copied, although despite
//     the message beam_type being a four-character string, it is an enumerated value
//     in the Beam object and so no copies are necessary. Note that spawned beams
//     cannot have anything in the data or scan_target pointers, so those are left
//     NULL in this event.
//
// - Scan beam colliding with smart physics object.
//   > Similar to physics.cpp::PhysicsObject_collision(), when a scan beam collides with
//     an SPO, a shallow clone of the origin beam is made, as well as a clone of the
//     object it collided with (by physics.cpp::PhysicsObject_clone()), which is set as
//     the scan_target of the origin beam. The data and comm_msg of the origin beam copy
//     are set to NULL, since they are not required, only the geometric properties of the
//     beam are needed. This event also triggers a ScanQueryMsg to the OSIM, the response
//     to which triggers the next event.
//
// - ScanResponseMsg received on the network.
//   > A response beam is constructed from the origin beam stored in the queries map(),
//     the scan_target is set to that of the origin beam, the comm_msg is set to NULL,
//     the data field of the network message is set to a newly allocated string and the
//     contents of the message's data field copied in. The origin beam is freed, and the
//     query erased from the map.
//
// Allocations are performed in auxiliary code, including physics and vector code:
//
// - physics.cpp::PhysicsObject_clone()
//   > Performs a deep-clone of a physics object or beam, including all non-NUL pointers
//     to referenced data. For physics objects, this includes the obj_type string, and
//     for beams, this includes the comm_msg, data, and a deep clone of the scan_target
//     via recursion.
// - physics.cpp::Beam_make_return_beam()
//   > Constructs a return beam, allocating a new beam object with return parameters,
//     however does not perform deep-cloning of the comm_msg, data, or scan_target members.
// - physics.cpp::PhysicsObject_collision()
//   > If a beam collides with a (non-smart) physics object, a return beam is made on
//     the spot and sent back as it does not require a query/response pair with the OSIM.
//
// - vector.cpp::Vector3_alloc(), vector.cpp::Vector3_clone()
//   > Allocates a new Vector3 struct.
// - vector.cpp::Vector3_easy_look_at(), vector.cpp::Vector3_easy_look_at2()
//   > Calls vector.cpp:Vector3_alloc()

#include <chrono>

#define ABSOLUTE_MIN_FRAMETIME 1e-7
#define SPIN_SLEEP_MAX_US 2000

#define LOCK(l) l.lock()
#define UNLOCK(l) l.unlock()
#define THREAD_CREATE(t, f, a) t = std::thread(f, a)
#define THREAD_JOIN(t) \
    if (t.joinable())  \
    t.join()

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CLAMP(m, v, M) MIN((M), MAX((v), (m)))

void spin_sleep_for(std::chrono::microseconds sleep_duration);

namespace Diana
{
    const int MIN_OBJECTS_PER_THREAD = 500;

    typedef struct Beam B;
    typedef PhysicsObjectType POT;
    typedef struct PhysicsObject PO;
    typedef struct SmartPhysicsObject SPO;
    typedef struct Vector3 V3;

    Universe::CollisionMetrics::CollisionMetrics() : primary_aabb_tests(0),
                                                     secondary_aabb_tests(0),
                                                     sphere_tests(0),
                                                     simultaneous_collisions(0),
                                                     collision_rounds(0),
                                                     collisions(0),
                                                     aabb_test_ns(0),
                                                     sphere_test_ns(0),
                                                     total_ns(0) {}

    void Universe::CollisionMetrics::add(struct Universe::CollisionMetrics other)
    {
        this->primary_aabb_tests += other.primary_aabb_tests;
        this->secondary_aabb_tests += other.secondary_aabb_tests;
        this->sphere_tests += other.sphere_tests;
        this->simultaneous_collisions += other.simultaneous_collisions;
        this->collision_rounds += other.collision_rounds;
        this->collisions += other.collisions;
        this->total_ns += other.total_ns;
        this->aabb_test_ns += other.aabb_test_ns;
        this->sphere_test_ns += other.sphere_test_ns;
    }

    void Universe::CollisionMetrics::_fprintf(FILE *fd)
    {
        fprintf(fd, "CollisionMetrics %u %u %u %u %u %u %lu %lu %lu\n",
                primary_aabb_tests, secondary_aabb_tests, sphere_tests,
                simultaneous_collisions, collision_rounds, collisions,
                HRN_COUNT(aabb_test_ns), HRN_COUNT(sphere_test_ns), HRN_COUNT(total_ns));
    }

    Universe::TickMetrics::TickMetrics() : collision_metrics(),
                                           multicollision_metrics(),
                                           num_objects(0),
                                           sort_aabb_ns(0),
                                           object_tick_ns(0),
                                           beam_tick_ns(0),
                                           thread_join_wait_ns(0),
                                           collision_resolution_ns(0),
                                           object_lifecycle_ns(0) {}

    void Universe::TickMetrics::_fprintf(FILE *fd)
    {
        collision_metrics._fprintf(fd);
        multicollision_metrics._fprintf(fd);
        fprintf(fd, "TickMetrics %lu %lu %lu %lu %lu %lu %lu %lu\n",
                num_objects,
                HRN_COUNT(sort_aabb_ns), HRN_COUNT(object_tick_ns), HRN_COUNT(beam_tick_ns), HRN_COUNT(thread_join_wait_ns),
                HRN_COUNT(collision_resolution_ns), HRN_COUNT(object_lifecycle_ns), HRN_COUNT(total_ns));
    }

    void gravity(double G, V3 *out, PO *big, PO *small)
    {
        double m = G * big->mass * small->mass / Vector3_distance2(&big->position, &small->position);
        Vector3_ray(out, &small->position, &big->position);
        Vector3_scale(out, m);
    }

    void *sim(void *uV)
    {
        fprintf(stderr, "Universe (%p) physics sim thread PID: %u\n", uV, get_this_thread_pid());
        Universe *u = (Universe *)uV;

        // dt is the amount of time that will pass in the game world during the next tick.
        double dt = u->min_frametime;

        // Cutoff to determine whether we're close enough to the end of the tick to justify sleeping.
        double dt_cutoff = 0.001 * u->min_frametime;

        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        std::chrono::duration<double> elapsed;

        while (u->running)
        {
            // If we're paused, then just sleep. We're not picky on how long we sleep for.
            // Sleep for max_frametime time so that we're responsive to unpausing, but not
            // waking up too often.
            if (u->paused)
            {
                std::this_thread::sleep_for(std::chrono::microseconds((int32_t)(1000000 * u->max_frametime)));
                continue;
            }

            start = std::chrono::high_resolution_clock::now();
            u->tick_metrics = u->tick(u->rate * dt);
            end = std::chrono::high_resolution_clock::now();

            elapsed = end - start;
            double e = elapsed.count();
            u->phys_frametime = e;

            // If we're simulating in real time, make sure that we aren't going too fast
            // If the physics took less time than the minimum we're allowing, sleep for
            // at least the remainder. The precision is nanoseconds, because that's the smallest
            // interval we can represent with std::chrono.
            while (u->realtime && ((u->min_frametime - e) > dt_cutoff))
            {
                // C++11 sleep_for is guaranteed to sleep for AT LEAST as long as requested.
                //
                // In practice, a 1ms min frame time actually causes the average
                // frame tiem to be about 2ms (Tested on Windows 8 and Ubuntu in
                // a VBox VM).
                int32_t sleep_duration_us = (int32_t)(1000000 * (u->min_frametime - e));
                std::chrono::microseconds sleep_duration = std::chrono::microseconds(sleep_duration_us);

                if (u->params.permit_spin_sleep && (sleep_duration_us < SPIN_SLEEP_MAX_US))
                {
                    spin_sleep_for(sleep_duration);
                }
                else
                {
                    std::this_thread::sleep_for(sleep_duration);
                }

                end = std::chrono::high_resolution_clock::now();
                elapsed = end - start;
                e = elapsed.count();
            }

            // Regardless of what we did, we need to clamp the time-delta for the next tick to be
            // in the acceptable range.
            // - If the elapsed time is too small, bring it up to the minimum
            //   > If realtime is off, this will result in a faster-than-realtime simulation
            //   > If realtime is on, this should never happen as we should be sleeping to get e close to max_frametime
            // - If the elapsed time is too large, bring it down to the max.
            //   > Regardless of realtime setting, his will result in a slower-than-realtime simulation.
            dt = CLAMP(u->min_frametime, e, u->max_frametime);

            // The wall frametime is how long it took to actually do the physics plus
            // any sleeping to get us up to the min_frametime
            u->wall_frametime = e;
            // The time elapsed in the game world on the next physics tick.
            u->game_frametime = dt;
            // Total time elapsed in the game world at the end of the next tick.
            u->total_time += u->rate * dt;
            u->num_ticks++;
        }

        return NULL;
    }

    void *vis_data_thread(void *uV)
    {
        fprintf(stderr, "Universe (%p) visdata sim thread PID: %u\n", uV, get_this_thread_pid());
        Universe *u = (Universe *)uV;

        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        std::chrono::duration<double> elapsed;

        while (u->running)
        {
            // If we're paused, then just sleep. We're not picky on how long we sleep for.
            // Sleep for max_frametime time so that we're responsive to unpausing, but not
            // waking up too often.
            if (u->visdata_paused)
            {
                std::this_thread::sleep_for(std::chrono::microseconds((int32_t)(1000000 * (u->min_vis_frametime))));
                continue;
            }

            start = std::chrono::high_resolution_clock::now();
            u->broadcast_vis_data();
            end = std::chrono::high_resolution_clock::now();

            elapsed = end - start;
            double e = elapsed.count();

            // Make sure to pace ourselves, and not broadcast faster than the minimum interval.
            if (e < u->min_vis_frametime)
            {
                int32_t sleep_duration_us = (int32_t)(1000000 * (u->min_vis_frametime - e));
                std::chrono::microseconds sleep_duration = std::chrono::microseconds(sleep_duration_us);

                if (u->params.permit_spin_sleep && (sleep_duration_us < SPIN_SLEEP_MAX_US))
                {
                    spin_sleep_for(sleep_duration);
                }
                else
                {
                    std::this_thread::sleep_for(sleep_duration);
                }

                end = std::chrono::high_resolution_clock::now();
                elapsed = end - start;
                e = elapsed.count();
            }
        }

        return NULL;
    }

    void Universe_hangup_objects(int32_t c, void *arg)
    {
        Universe *u = (Universe *)arg;
        u->hangup_objects(c);
    }

    void Universe_handle_message(int32_t c, void *arg)
    {
        Universe *u = (Universe *)arg;
        u->handle_message(c);
    }

    Universe::Universe(struct Parameters _params)
    {
        this->params = _params;
        this->rate = _params.simulation_rate;
        this->realtime = _params.realtime_physics;
        this->min_frametime = _params.min_physics_frametime;
        this->max_frametime = _params.max_physics_frametime;
        this->min_vis_frametime = _params.min_vis_frametime;

        if (realtime && (min_frametime < ABSOLUTE_MIN_FRAMETIME))
        {
            fprintf(stderr, "WARNING: min_framtime set below absolute minimum. Raising it to %g s.\n", ABSOLUTE_MIN_FRAMETIME);
            min_frametime = ABSOLUTE_MIN_FRAMETIME;
            max_frametime = MAX(max_frametime, ABSOLUTE_MIN_FRAMETIME);
        }

        this->num_threads = _params.num_worker_threads;
        // sched = new libodb::Scheduler(this->num_threads - 1);

        phys_worker_args = (struct phys_args *)malloc(this->num_threads * sizeof(struct phys_args));
        for (int i = 0; i < this->num_threads; i++)
        {
            phys_worker_args[i].u = this;
            phys_worker_args[i].offset = i;
            phys_worker_args[i].stride = this->num_threads;
            phys_worker_args[i].dt = 0.0;
            phys_worker_args[i].done = true;
        }

        total_time = 0.0;
        last_effect_time = 0.0;
        phys_frametime = 0.0;
        wall_frametime = 0.0;
        game_frametime = 0.0;
        vis_frametime = 0.0;

        total_time = 0.0;
        num_ticks = 0;
        total_objs = 1;

        paused = true;
        visdata_paused = true;
        running = false;

        net = new MIMOServer(Universe_handle_message, this, Universe_hangup_objects, this, _params.network_port);
    }

    Universe::~Universe()
    {
        for (int i = 0; i < num_threads; i++)
        {
            phys_worker_args[i].done = false;
        }

        // sched->block_until_done();
        free(phys_worker_args);

        stop_net();
        stop_sim();
        // delete sched;
    }

    void Universe::start_net()
    {
        net->start();
    }

    void Universe::stop_net()
    {
        net->stop();
    }

    void Universe::start_sim()
    {
        if (!running || (running && paused))
        {
            running = true;
            paused = false;
            visdata_paused = false;
            THREAD_CREATE(sim_thread, sim, (void *)this);
            THREAD_CREATE(vis_thread, vis_data_thread, (void *)this);
        }
    }

    void Universe::pause_sim()
    {
        paused = true;
    }

    void Universe::pause_visdata()
    {
        visdata_paused = true;
    }

    void Universe::stop_sim()
    {
        running = false;
        THREAD_JOIN(sim_thread);
        THREAD_JOIN(vis_thread);
    }

    void Universe::get_frametime(double *out)
    {
        out[0] = phys_frametime;
        out[1] = wall_frametime;
        out[2] = game_frametime;
        out[3] = vis_frametime;
    }

    uint64_t Universe::get_ticks()
    {
        return num_ticks;
    }

    struct Universe::TickMetrics Universe::get_tick_metrics()
    {
        return tick_metrics;
    }

    double Universe::total_sim_time()
    {
        return total_time;
    }

    int64_t Universe::get_id()
    {
        int64_t offset;
        if (params.id_rand_max > 1)
        {

            std::uniform_int_distribution<int64_t> dist(1, params.id_rand_max);
            offset = dist(re);
        }
        else
        {
            offset = 1;
        }

        int64_t r = total_objs.fetch_add(offset);
        return r;
    }

    void Universe::add_object(PO *obj)
    {
        obj->phys_id = get_id();

        LOCK(add_lock);
        added.push_back(obj);
        UNLOCK(add_lock);
    }

    void Universe::add_object(B *beam)
    {
        //! @todo Are there issues with casting between Beams and POs?
        // Before #pragma pack()-ing the beam struct, the phys_id was being rearranged into a padding portion.
        add_object((PO *)beam);
    }

    void Universe::expire(int64_t phys_id)
    {
        LOCK(expire_lock);
        expired.insert(phys_id);
        UNLOCK(expire_lock);
    }

    //! @todo Do not expire smarty objects on disconnect.
    void Universe::hangup_objects(int32_t c)
    {
        LOCK(expire_lock);
        LOCK(add_lock);
        //! @todo Add a second std::map to change out this linear search.
        std::map<int64_t, struct SmartPhysicsObject *>::iterator it;

        for (it = smarties.begin(); it != smarties.end(); ++it)
        {
            if ((it->second != NULL) && (it->second->socket == c))
            {
                expired.insert(it->first);
            }
        }
        UNLOCK(expire_lock);
        UNLOCK(add_lock);
    }

    void Universe::register_vis_client(struct vis_client vc, bool enabled)
    {
        //! @todo Remove the linear searches here
        LOCK(vis_lock);

        // Try to find the one we're looking for.
        std::vector<struct vis_client>::iterator it = std::lower_bound(vis_clients.begin(), vis_clients.end(), vc);
        bool found = (((vis_clients.size() == 0) || (it == vis_clients.end())) ? false : (vc == *it));

        if (enabled && !found)
        {
            // If we find it, push it onto the back and re-sort the list.
            vis_clients.push_back(vc);
            std::sort(vis_clients.begin(), vis_clients.end());
        }
        else if (!enabled && found)
        {
            it = vis_clients.erase(it);
        }
        UNLOCK(vis_lock);
    }

    void Universe::broadcast_vis_data()
    {
        //! @todo Should we acquire a physics lock here? Maybe, maybe not?
        if (vis_clients.size() == 0)
        {
            return;
        }

        struct vis_client vc;
        PO *o;
        PO *ro;
        std::chrono::time_point<std::chrono::high_resolution_clock> t0;
        std::chrono::duration<double> dt;
        int64_t client_nbytes = 0;

        LOCK(vis_lock);
        for (std::vector<struct vis_client>::iterator it = vis_clients.begin(); it != vis_clients.end();)
        {
            vc = *it;

            // We're going to specify all of the options, so just set them all to specced.
            visdata_msg.spec_all(true);
            visdata_msg.client_id = vc.client_id;

            //! @todo Unlocked access to the smarties map.
            ro = (vc.phys_id == -1 ? NULL : (PO *)smarties[vc.phys_id]);
            bool disconnect = false;

            if (this->params.verbose_logging)
            {
                t0 = std::chrono::high_resolution_clock::now();
            }

            //! @todo unlocked access to phsy_objects array.
            for (size_t i = 0; i < phys_objects.size(); i++)
            {
                o = phys_objects[i];
                visdata_msg.server_id = vc.phys_id;
                visdata_msg.radius = o->radius;

                // Don't forget to unset the sign bits, negative IDs would be weird.
                if (params.id_rand_max == 1)
                {
                    visdata_msg.phys_id = o->phys_id;
                }
                else
                {
                    visdata_msg.phys_id = ((o->phys_id ^ (int64_t)o)) & 0x7FFFFFFFFFFFFFFF;
                }

                visdata_msg.position = o->position;

                if (ro != NULL)
                {
                    Vector3_subtract(&visdata_msg.position, &visdata_msg.position, &(ro->position));
                }

                // Apply the visual acuity cutoff
                if ((4 * visdata_msg.radius * visdata_msg.radius / Vector3_length2(&visdata_msg.position)) < params.visual_acuity)
                {
                    continue;
                }

                visdata_msg.orientation.w = o->forward.x;
                visdata_msg.orientation.x = o->forward.y;
                visdata_msg.orientation.y = o->up.x;
                visdata_msg.orientation.z = o->up.y;
                int64_t nbytes = visdata_msg.send(vc.socket);
                client_nbytes += nbytes;

                if (nbytes < 0)
                {
                    it = vis_clients.erase(it);
                    printf("Client %d erased due to failed network send", vc.socket);
                    disconnect = true;
                    break;
                }
            }

            if (this->params.verbose_logging && (client_nbytes > 0))
            {
                dt = std::chrono::high_resolution_clock::now() - t0;
                fprintf(stderr,
                        "%g seconds to send %lu bytes of visdata to client %d\n",
                        dt.count(), client_nbytes, vc.socket);
            }

            if (!disconnect)
            {
                // Send an empty message with an unspecced server ID, only specifying client ID
                // This is an end-of-frame message, indicating the end of round of messages.
                visdata_msg.spec_all(false);
                visdata_msg.specced[0] = true;
                visdata_msg.server_id = -1;
                visdata_msg.specced[1] = true;
                int64_t nbytes = visdata_msg.send(vc.socket);
                if (nbytes < 0)
                {
                    it = vis_clients.erase(it);
                    printf("Client %d erased due to failed network send", vc.socket);
                    disconnect = true;
                }
                else
                {
                    it++;
                }
            }
        }
        UNLOCK(vis_lock);
    }

    double Universe::gen_rand(std::normal_distribution<double> dist)
    {
        double ret = 0.0;
        LOCK(rand_lock);
        ret = dist(re);
        UNLOCK(rand_lock);
        return ret;
    }

    void Universe::handle_message(int32_t socket)
    {
        //! @todo Support optional arguments with sensible defaults.
        BSONMessage *msg_base = BSONMessage::ReadMessage(socket);

        // For objects spawned as 'children' from a parent, they should have the parent's phys_id in the server_id
        // field. That is used to find the parent, and adjust the position/velocity/whatever accordingly.
        // This only applies, right now, to smart parents.
        LOCK(add_lock);
        LOCK(expire_lock);
        std::map<int64_t, struct SmartPhysicsObject *>::iterator it = smarties.find(msg_base->server_id);
        UNLOCK(add_lock);
        UNLOCK(expire_lock);

        // If this smarty is non-NULL, then this points to the PARENT
        SPO *smarty = (it != smarties.end() ? it->second : NULL);

        if (params.verbose_logging)
        {
            printf("Received message of type %d from client %d\n", msg_base->msg_type, socket);
        }

        switch (msg_base->msg_type)
        {
        case BSONMessage::MessageType::VisualDataEnable:
        {
            VisualDataEnableMsg *msg = (VisualDataEnableMsg *)msg_base;
            if (msg->specced[2])
            {
                printf("Client %d %s for VisData\n", socket, (msg->enabled ? "REGISTERED" : "UNREGISTERED"));

                struct vis_client vc;
                vc.socket = socket;
                vc.phys_id = (msg->specced[0] ? msg->server_id : -1);
                vc.client_id = (msg->specced[1] ? msg->client_id : -1);
                register_vis_client(vc, msg->enabled);
            }
            break;
        }
        case BSONMessage::PhysicalProperties:
        {
            PhysicalPropertiesMsg *msg = (PhysicalPropertiesMsg *)msg_base;
            // We currently only support PhysProps messages for smarties. Why would you be able to adjust
            // the properties of another object?
            if (smarty != NULL)
            {
                // Object type
                if (msg->specced[2])
                {
                    size_t new_type_len = strlen(msg->obj_type) + 1;
                    char *new_type = (char *)malloc(sizeof(char) * new_type_len);
                    if (new_type != NULL)
                    {
                        free(smarty->pobj.obj_type);
#ifdef _WIN32
                        strncpy_s(new_type, new_type_len, msg->obj_type, new_type_len);
#else
                        strncpy(new_type, msg->obj_type, new_type_len);
#endif
                        smarty->pobj.obj_type = new_type;
                    }
                }

                // If the mass or radius changes, we need to update attractors if the values changes
                if ((msg->specced[3] && !Vector3_almost_zeroS(smarty->pobj.mass - msg->mass)) ||
                    (msg->specced[17] && !Vector3_almost_zeroS(smarty->pobj.radius - msg->radius)))
                {
                    // The mass and/or radius changed, so we need to recalculate whether or not
                    // it can be a gravity source now. Probably not, but who knows.
                    bool newval = is_big_enough(smarty->pobj.mass, smarty->pobj.radius, params.gravity_magnitude_cutoff);

                    // We don't want to grab the lock unnecessarily
                    if (newval != smarty->pobj.emits_gravity)
                    {
                        LOCK(add_lock);
                        LOCK(expire_lock);
                        update_list(&smarty->pobj, &attractors, newval, smarty->pobj.emits_gravity);
                        UNLOCK(add_lock);
                        UNLOCK(expire_lock);
                        smarty->pobj.emits_gravity = newval;
                    }
                }

#define ASSIGN_VAL(i, var, absolute)                                       \
    if (msg->specced[i])                                                   \
    {                                                                      \
        smarty->pobj.var = (absolute ? 0.0 : smarty->pobj.var) + msg->var; \
    };
#define ASSIGN_V3(i, var, absolute) ASSIGN_VAL(i, var.x, absolute) ASSIGN_VAL(i + 1, var.y, absolute) ASSIGN_VAL(i + 2, var.z, absolute);
                ASSIGN_VAL(3, mass, true);
                ASSIGN_V3(4, position, false);
                ASSIGN_V3(7, velocity, false);
                if (msg->specced[10] && msg->specced[11] && msg->specced[12] && msg->specced[13])
                {
                    PhysicsObject_from_orientation(&smarty->pobj, &msg->orientation);
                }
                ASSIGN_V3(14, thrust, true);
                ASSIGN_VAL(17, radius, true);
#undef ASSIGN_V3
#undef ASSIGN_VAL

                // Ensure that the spectrum was specified in the message
                if (msg->all_specced(msg->num_el - 3))
                {
                    struct Spectrum *spectrum = Spectrum_clone(msg->spectrum);
                    spectrum = Spectrum_perturb(spectrum, params.spectrum_slush_range,
                                                [this]()
                                                { return this->gen_rand(std::normal_distribution<double>(1.0, params.spectrum_slush_range)); });
                    spectrum = Spectrum_combine(smarty->pobj.spectrum, spectrum);
                    smarty->pobj.spectrum = spectrum;

                    radiates_strong_enough(spectrum, params.radiation_energy_cutoff);
                    bool newval = (spectrum->safe_distance_sq > (smarty->pobj.radius * smarty->pobj.radius));

                    if (newval != smarty->pobj.dangerous_radiation)
                    {
                        LOCK(add_lock);
                        LOCK(expire_lock);
                        update_list(&smarty->pobj, &radiators, newval, smarty->pobj.dangerous_radiation);
                        UNLOCK(add_lock);
                        UNLOCK(expire_lock);
                        smarty->pobj.dangerous_radiation = newval;
                    }
                }
            }
            break;
        }
        case BSONMessage::Beam:
        {
            BeamMsg *msg = (BeamMsg *)msg_base;
            // We need all of the properties, except message, if it's not a COMM beam,
            // If it's a COMM beam, we need them all.
            if (((strcmp(msg->beam_type, "COMM") == 0) && !msg->all_specced()) ||
                ((strcmp(msg->beam_type, "COMM") != 0) && !msg->all_specced(0, msg->num_el - 1, msg->num_el - 4)))
            {
                break;
            }

            B *b = (B *)malloc(sizeof(B));
            if (b == NULL)
            {
                throw std::runtime_error("OOM");
            }

            char *comm_msg = NULL;
            PhysicsObjectType btype;
            if (strcmp(msg->beam_type, "COMM") == 0)
            {
                btype = BEAM_COMM;
                size_t len = strlen(msg->comm_msg) + 1;
                comm_msg = (char *)malloc(sizeof(char) * len);
                if (comm_msg != NULL)
                {
                    memcpy(comm_msg, msg->comm_msg, len);
                    b->comm_msg = comm_msg;
                }
                else
                {
                    throw std::runtime_error("Universe::UnableToAllocateCommMsg");
                }
            }
            if (strcmp(msg->beam_type, "WEAP") == 0)
            {
                btype = BEAM_WEAP;
            }
            if (strcmp(msg->beam_type, "SCAN") == 0)
            {
                btype = BEAM_SCAN;
            }
            if (strcmp(msg->beam_type, "SCRE") == 0)
            {
                btype = BEAM_SCANRESULT;
            }

            //! @todo This should also apply for non-smarties.
            if (smarty != NULL)
            {
                Vector3_add(&msg->origin, &smarty->pobj.position);
                //! @todo Relativistic velocity composition
                Vector3_add(&msg->velocity, &smarty->pobj.velocity);
            }

            //! @todo Couple the energy of the beam to the power of the spectrum somehow.

            // Note that the spectrum is a non-optional component, so we can assume it exists
            // as enforcement of that condition occurs earlier with the all_specced() call.
            Beam_init(b, this, &msg->origin, &msg->velocity, &msg->up,
                      msg->spread_h, msg->spread_v, msg->energy, btype, comm_msg, NULL, msg->spectrum);

            // Now that the beam has a cloned version of the spectrum, perturb it.
            b->spectrum = Spectrum_perturb(b->spectrum, params.spectrum_slush_range,
                                           [this]()
                                           { return this->gen_rand(std::normal_distribution<double>(1.0, params.spectrum_slush_range)); });

            add_object(b);
            break;
        }
        case BSONMessage::MessageType::Spawn:
        {
            SpawnMsg *msg = (SpawnMsg *)msg_base;
            // We don't need, or rather, shouldn't have, a server ID on a spawn, but we should
            // have everything else. Similarly, spectrum specification is optional.
            if (!msg->all_specced(1, msg->num_el - 4))
            {
                break;
            }

            PO *obj;
            // If this object is smart, add it to the smarties, and such.
            if (msg->is_smart)
            {
                SPO *sobj = (SPO *)malloc(sizeof(struct SmartPhysicsObject));
                if (sobj == NULL)
                {
                    throw std::runtime_error("OOM");
                }
                obj = &sobj->pobj;
                sobj->client_id = (msg->specced[1] ? msg->client_id : -1);
                sobj->socket = socket;
            }
            else
            {
                obj = (PO *)malloc(sizeof(struct PhysicsObject));
                if (obj == NULL)
                {
                    throw std::runtime_error("OOM");
                }
            }

            size_t len = strlen(msg->obj_type) + 1;
            char *obj_type = (char *)malloc(len);
            if (obj_type == NULL)
            {
                throw std::runtime_error("OOM you twat");
            }
            memcpy(obj_type, msg->obj_type, len);

            struct Spectrum *spectrum = NULL;
            // If the message specifies a spectrum, use it
            if (msg->all_specced(msg->num_el - 3))
            {
                spectrum = msg->spectrum;
            }

            PhysicsObject_init(obj, this, &msg->position, &msg->velocity,
                               const_cast<struct Vector3 *>(&vector3d_zero), &msg->thrust,
                               msg->mass, msg->radius, obj_type, spectrum);

            // Now that the object contains a cloned version of the spectrum, perturb it.
            if (obj->spectrum != NULL)
            {
                obj->spectrum = Spectrum_perturb(obj->spectrum, params.spectrum_slush_range,
                                                 [this]()
                                                 { return this->gen_rand(std::normal_distribution<double>(1.0, params.spectrum_slush_range)); });
            }

            // Note that adding the object to the attractors and radiators lists is handled
            // at the end of the physics tick when the added vector is emptied.

            // If the object creating the object is a smarty, it's position and velocity
            // are relative.
            if (smarty != NULL)
            {
                Vector3_add(&obj->position, &smarty->pobj.position);
                Vector3_add(&obj->velocity, &smarty->pobj.velocity);
            }

            // We need to assign type BEFORE adding to the universe, because the adding to smarties{} is async from this.
            //! @todo Should smartie-adding be synchronous here? Grab the lock, add, release?
            if (msg->is_smart)
            {
                obj->type = PhysicsObjectType::PHYSOBJECT_SMART;
                obj->health = -1;
            }

            add_object(obj);

            if (msg->is_smart)
            {
                HelloMsg hm;
                hm.client_id = msg->client_id;
                hm.server_id = obj->phys_id;
                hm.spec_all();
                hm.send(socket);
            }
            break;
        }
        case BSONMessage::MessageType::ScanQuery:
        {
            ScanQueryMsg *msg = (ScanQueryMsg *)msg_base;
            // The only time we should receive this is when a ship is querying for a passive
            // scan. We expect the query to have no parameters, as it simply begins an event.

            // A passive scan result contains the signature and direction.
            //
            // The signature's power levels are scaled based on the distance (the area of the
            // wave front, total power amortized across the area, times the area of the object
            // intersection with that).

            if (smarty != NULL)
            {
                // The message to send back.
                ScanResultMsg srm;

                // Spec the IDs
                srm.specced[0] = true;
                srm.server_id = msg->server_id;
                srm.specced[1] = true;
                srm.client_id = msg->client_id;

                // Spec the position (which will be a unit vector in the direction of the source
                srm.specced[4] = true;
                srm.specced[5] = true;
                srm.specced[6] = true;

                // Spec the obj_spectrum component, which will be the same spectrum as the object
                // with the power levels scaled inverse-quadratically by distance.
                srm.specced[22] = true;
                srm.specced[23] = true;
                srm.specced[24] = true;

                PO *other;
                V3 dp;
                double distance_sq;
                double power_scale;

                //! @todo THIS IS BAD! Locking the physics lock synchronously for networking? Oh no.
                //! @todo Use read/write locks, since most of the time, we only want read locks.
                LOCK(phys_lock);
                //! @todo Move all objects with spectra to their own vector
                for (std::vector<PO *>::iterator it = phys_objects.begin(); it != phys_objects.end(); it++)
                {
                    other = *it;
                    if (other->spectrum == NULL)
                    {
                        continue;
                    }

                    Vector3_subtract(&dp, &other->position, &smarty->pobj.position);
                    distance_sq = Vector3_length2(&dp);

                    // If we're looking at our own radiation signature, then we need to do
                    // things a little different. THis will stand out as having a position that
                    // is (0,0,0), so we can leave the power spectrum alone.
                    if (Vector3_almost_zeroS(distance_sq))
                    {
                        power_scale = 1.0;
                    }
                    else
                    {
                        // You can't absorb more power than it's outputting...
                        power_scale = MIN(1.0, smarty->pobj.radius * smarty->pobj.radius / (4 * distance_sq));
                    }

                    if ((power_scale * other->spectrum->total_power) < params.collision_energy_cutoff)
                    {
                        continue;
                    }

                    srm.obj_spectrum = Spectrum_clone(other->spectrum);
                    struct SpectrumComponent *components = &(srm.obj_spectrum->components);
                    for (size_t i = 0; i < srm.obj_spectrum->n; i++)
                    {
                        components[i].power *= power_scale;
                    }

                    // Make the position a unit direction vector if it isn't (0,0,0)
                    if (!Vector3_almost_zeroS(distance_sq))
                    {
                        Vector3_scale(&dp, 1.0 / sqrt(distance_sq));
                    }

                    srm.position = dp;
                    srm.send(smarty->socket);
                    free(srm.obj_spectrum);
                }
                UNLOCK(phys_lock);
                srm.obj_spectrum = NULL;
            }

            break;
        }
        case BSONMessage::MessageType::ScanResponse:
        {
            ScanResponseMsg *msg = (ScanResponseMsg *)msg_base;
            if (!msg->all_specced())
            {
                break;
            }

            struct Universe::scan_target st = {msg->scan_id, msg->server_id};

            // We SHOULD be able to find the target in the map, and something is very
            // wrong if we can't.
            LOCK(query_lock);
            const std::map<Universe::scan_target, Universe::scan_origin>::iterator it = queries.find(st);
            if ((it != queries.end()) && (it->first == st))
            {
                struct scan_origin so = it->second;
                Beam *return_beam = Beam_make_return_beam(so.origin_beam, so.energy, &so.hit_position, PhysicsObjectType::BEAM_SCANRESULT);
                return_beam->scan_target = so.origin_beam->scan_target;
                size_t len = strlen(msg->data) + 1;
                char *data_copy = (char *)malloc(len);
                if (data_copy == NULL)
                {
                    throw std::runtime_error("OOM");
                }
                memcpy(data_copy, msg->data, len);
                //! @todo The value of the ->data member should be NULL here, verify that.
                return_beam->data = data_copy;

                // Free any cloned bits and pieces as necessary.
                // Don't free the scan target, that's also linked to the return beam.
                // But we should nullify the pointer. Also note that the origin beam was a shallow
                // clone, and so the comm_msg and data fields are NULL.
                so.origin_beam->scan_target = NULL;
                free(so.origin_beam);

                add_object(return_beam);
                //! @todo Potentially large leak here, queries that are never responded to won't get freed.
                queries.erase(it);
            }
            else
            {
                //! @todo Bad things? This should be handled gracefully
                // In the future, we'll get here if we've expired the query before we get a response
                // from the OSIM.
                throw std::runtime_error("Universe::ExpiredQuery");
            }
            UNLOCK(query_lock);
            break;
        }
        //! @todo Use Hello messages to tell a smart to send it's response messages back to the socket
        // the Hello came from.
        case BSONMessage::MessageType::Hello:
        {
            HelloMsg *msg = (HelloMsg *)msg_base;
            if (!msg->specced[0] && msg->specced[1])
            {
                HelloMsg hm;
                hm.server_id = get_id();
                hm.client_id = msg->client_id;
                hm.spec_all();
                hm.send(socket);
            }
            break;
        }
        //! @todo Use Goodbye messages to expire smarty objects. Only expire them on a goodbye, not on
        // socket disconnect.
        case BSONMessage::MessageType::Goodbye:
        {
            GoodbyeMsg *msg = (GoodbyeMsg *)msg_base;
            // Make sure it is speccing the server ID.
            //! @todo Secure this so only associated clients can Goodbye smarties?
            if (msg->specced[0])
            {
                LOCK(add_lock);
                LOCK(expire_lock);
                std::map<int64_t, struct SmartPhysicsObject *>::iterator it = smarties.find(msg_base->server_id);
                UNLOCK(add_lock);
                UNLOCK(expire_lock);
                if ((it != smarties.end()) && (it->first == msg->server_id))
                {
                    expire(msg->server_id);
                }
            }
            break;
        }
        default:
            throw std::runtime_error("Universe::UnrecognizedMessageType");
        }

        delete msg_base;
    }

    void Universe::get_grav_pull(V3 *g, PO *obj)
    {
        V3 cg;
        for (size_t i = 0; i < attractors.size(); i++)
        {
            // An object can't attract itself.
            if (attractors[i]->phys_id == obj->phys_id)
            {
                continue;
            }

            gravity(params.gravitational_constant, &cg, attractors[i], obj);
            Vector3_add(g, &cg);
        }
    }

    bool check_collision_single(Universe *u, struct PhysicsObject *obj1, struct PhysicsObject *obj2, double dt, struct Universe::PhysCollisionEvent &ev)
    {
        struct PhysCollisionResult phys_result;

        // Return from a phys-phys collision is
        //    [t, [e1, e2], [obj1  collision data], [obj2 collision data]]
        // t is in [0,1] and indicates when in the interval the collision happened
        // energy is obvious.
        // d1 is the direction obj2 was travelling relative to obj1 whe they collided
        // p1 is a vector from obj1's position to the collision sport on its bounding ball.
        // Note that dt here is the remainder of the interval
        PhysicsObject_collide(&phys_result, obj1, obj2, dt);

        // We need to make sure that this is in the 'future' of both objects, so we should test it against the
        // 't' component of both.
        // Recall that the object's 't' component is in real-time seconds, so compare with the real-time seconds
        // that the collision happened at. Ensure that the time of the collision is within the time
        // remaining in the tick of each of the objects.
        if ((phys_result.t >= 0) &&
            (phys_result.t * dt <= (dt - obj1->t)) &&
            (phys_result.t * dt <= (dt - obj2->t)))
        {
            // By comparing the energy to a cutoff, this will help prevent
            // spurious collision notifications due to physics time step
            // increments and temporary object intersection.

            // Total energy involved. Both objects 'absorb' the same amount
            // of energy from an 'impact effect' perspective, and that is in
            // the 'e' field of the collision result.

            if ((phys_result.e < -u->params.collision_energy_cutoff) ||
                (phys_result.e > u->params.collision_energy_cutoff))
            {
                ev.obj1 = obj1;
                ev.obj2 = obj2;
                ev.pcr = phys_result;
                return true;
            }
        }

        return false;
    }

    struct Universe::CollisionMetrics check_collision_loop(void *argsV)
    {
        struct Universe::phys_args *args = (struct Universe::phys_args *)argsV;
        Universe *u = args->u;

        struct Universe::CollisionMetrics metrics;
        HRN_T t0 = HRN;
        HRN_T aabb0, sphere0;

        struct AABB *a;
        struct AABB *b;

        size_t end = args->offset + args->stride;
        end = MIN(end, u->phys_objects.size() - 1);

        for (size_t i = args->offset; i < end; i++)
        {
            // In order for a full intersection to be possible, there has to be intersection
            // of the AABBs in all three dimensions.
            //
            // AABBs sorted by their lowest coordinate in the X dimensions.
            // We can test for potential intersections, first pruning by intersection along
            // the X axis.
            //
            // First test to see if the X projections intersect. If they do, then test the others.

            a = &u->phys_objects[i]->box;
            for (size_t j = (args->test_all ? 0 : i + 1); j < u->phys_objects.size(); j++)
            {
                aabb0 = HRN;

                // Don't collide objects with themselves.
                if (i == j)
                {
                    continue;
                }

                b = &u->phys_objects[j]->box;

                double d;
                d = a->u.x - b->l.x;

                // This is to text if they intersect/touch in X
                // It's simpler here, because we have a guarantee (thanks to sorting)
                // about the relative positions of the lower endpoints of the intervals so
                // we don't need to worry about those here.
                metrics.primary_aabb_tests++;
                if (Vector3_almost_zeroS(d) || (d > 0))
                {
                    // Now do a full test on the Y axis.
                    metrics.secondary_aabb_tests++;
                    if (!Vector3_intersect_interval(a->l.y, a->u.y, b->l.y, b->u.y))
                    {
                        continue;
                    }

                    // And then a full test on Z
                    metrics.secondary_aabb_tests++;
                    if (!Vector3_intersect_interval(a->l.z, a->u.z, b->l.z, b->u.z))
                    {
                        continue;
                    }

                    metrics.aabb_test_ns += HRN - aabb0;

                    // If we succeeded on both, count this as a potential collision for
                    // honest-to-goodness testing and hand it off to bounding-ball testing
                    // followed by collision effect calculation.
                    struct Universe::PhysCollisionEvent ev;
                    metrics.sphere_tests++;
                    sphere0 = HRN;
                    if (check_collision_single(u, u->phys_objects[i], u->phys_objects[j], args->dt, ev))
                    {
                        ev.obj1_index = i;
                        ev.obj2_index = j;
                        LOCK(u->collision_lock);
                        u->collisions.push_back(ev);
                        UNLOCK(u->collision_lock);
                        metrics.collisions++;
                    }
                    metrics.sphere_test_ns += HRN - sphere0;
                }
                else
                {
                    metrics.aabb_test_ns += HRN - aabb0;

                    // If we fail that X comparison, we know that we will fail for every object
                    // 'after', so we can bail on the loop.
                    break;
                }
            }
        }

        metrics.total_ns = HRN - t0;
        return metrics;
    }

    void *thread_check_collisions(void *argsV)
    {
        struct Universe::phys_args *args = (struct Universe::phys_args *)argsV;

        // Spin until we get the signal to start going.
        // The universe will flip this after the next workload is ready to go.
        while (args->done)
        {
        }

        check_collision_loop(argsV);

        // Indicate that we're done, and spin until we get the signal to stop.
        // The universe will flip this after the next workload is ready to go.
        args->done = true;
        while (args->done)
        {
        }

        return NULL;
    }

    void Universe::sort_aabb(double dt, bool calc)
    {
        // Employs gnome sort to sort the lists, computing the bounding boxes along the way
        // http://en.wikipedia.org/wiki/Gnome_sort

        struct PhysicsObject *box_swap;
        size_t max_so_far = 0;

        if (calc)
        {
            PhysicsObject_estimate_aabb(phys_objects[0], &phys_objects[0]->box, dt);
        }

        for (size_t i = 1; i < phys_objects.size();)
        {
            if (i > max_so_far)
            {
                // We only need to compute the bounding boxes on the first pass.
                if (calc)
                {
                    PhysicsObject_estimate_aabb(phys_objects[i], &phys_objects[i]->box, dt);
                }

                max_so_far = i;
            }

            //! @todo Could the AABB comparisons be costly?

            // Compare to the previous one if we're not at the first one.
            int c = Vector3_compare_aabbX(&phys_objects[i]->box, &phys_objects[i - 1]->box);

            if (c < 0)
            {
                box_swap = phys_objects[i];
                phys_objects[i] = phys_objects[i - 1];
                phys_objects[i - 1] = box_swap;

                // And then step back if we're not at the start.
                i -= (i > 1);
            }
            else
            {
                // Jump back to before we started going backwards swapping.
                i = max_so_far + 1;
            }
        }
    }

    //! @todo Convert this to a private member function.
    void obj_tick(Universe *u, struct PhysicsObject *o, double dt)
    {
        V3 g = {0.0, 0.0, 0.0};
        struct BeamCollisionResult beam_result;

        // Needed to resolve the physical effects of the collision.
        struct PhysCollisionResult phys_result;

        struct Beam *b;

        for (size_t bi = 0; bi < u->beams.size(); bi++)
        {
            b = u->beams[bi];
            Beam_collide(&beam_result, b, o, dt);

            if (beam_result.t >= 0.0)
            {
                phys_result.pce1.d = beam_result.d;
                phys_result.pce1.p = beam_result.p;

                if (u->params.verbose_logging)
                {
#if __x86_64__
                    fprintf(stderr, "Beam Collision: %lu -> %lu (%.15g J)\n", b->phys_id, o->phys_id, beam_result.e);
#else
                    fprintf(stderr, "Beam Collision: %llu -> %llu (%.15g J)\n", b->phys_id, o->phys_id, beam_result.e);
#endif
                }

                PhysicsObject_collision(o, (PO *)b, beam_result.e, beam_result.t * dt, &phys_result.pce1, u->params.health_damage_threshold);

                //! @todo Smarty beam collision messages
                //! @todo Beam collision messages
                if (o->type == PHYSOBJECT_SMART)
                {
                    struct SmartPhysicsObject *s = (SPO *)o;
                    CollisionMsg cm;
                    cm.client_id = s->client_id;
                    cm.server_id = o->phys_id;
                    cm.direction = beam_result.d;
                    cm.position = beam_result.p;
                    Vector3_subtract(&cm.position, &o->position);
                    cm.energy = beam_result.e;
                    cm.comm_msg = NULL;
                    // It makes sense that we know about the power levels of the beam here,
                    // since in theory the ship would know about the duration of the beam,
                    // even if we're not simulating that.
                    cm.spectrum = Spectrum_clone(b->spectrum);
                    cm.spec_all();

                    // Note that there is no comm message (yet, maybe), so that's unspecced.
                    cm.specced[cm.num_el - 4] = false;

                    switch (b->type)
                    {
                    case BEAM_COMM:
                        cm.specced[cm.num_el - 4] = true;
                        cm.set_colltype((char *)"COMM");
                        cm.comm_msg = b->comm_msg;
                        cm.send(s->socket);
                        break;
                    case BEAM_SCAN:
                    {
                        cm.set_colltype((char *)"SCAN");
                        cm.send(s->socket);

                        ScanQueryMsg sqm;
                        sqm.client_id = s->client_id;
                        sqm.server_id = o->phys_id;
                        sqm.scan_id = b->phys_id;
                        sqm.energy = cm.energy;
                        sqm.direction = cm.direction;
                        sqm.spectrum = Spectrum_clone(b->spectrum);
                        sqm.spec_all();

                        // Note that we need to ensure that the queries map is ready for a response before sending
                        // the query, otherwise we're in one hell of a race condition.

                        // Ignore multiple hits of the same beam/object pair.
                        // Could, in theory, use a multimap for queries instead, but really, multiple hits are spurious.
                        struct Universe::scan_target st = {b->phys_id, o->phys_id};
                        LOCK(u->query_lock);
                        const std::map<struct Universe::scan_target, struct Universe::scan_origin>::iterator it = u->queries.find(st);
                        if ((it == u->queries.end()) || !(st == (it->first)))
                        {
                            // Add the query to the universe so that it can send the response beam
                            // when the osim responds.
                            // Beam_make_return_beam(b, energy, &effect->p, BEAM_SCANRESULT);
                            // Clone the beam so that if the beam expires before the return beam is sent, we don't access the freed space.
                            B *b_copy = (B *)malloc(sizeof(B));
                            PO *o_copy = PhysicsObject_clone(o);
                            if ((b_copy == NULL) || (o_copy == NULL))
                            {
                                throw std::runtime_error("OOM::Universe::BeamHitObjClone");
                            }

                            *b_copy = *b;
                            b_copy->data = NULL;
                            b_copy->comm_msg = NULL;
                            b_copy->scan_target = o_copy;
                            b_copy->spectrum = Spectrum_clone(b->spectrum);
                            u->queries[st] = {b_copy, cm.energy, beam_result.p};
                        }
                        UNLOCK(u->query_lock);

                        sqm.send(s->socket);
                        break;
                    }
                    case BEAM_SCANRESULT:
                    {
                        //! @todo This feels forced, ugh... Enum?
                        cm.set_colltype((char *)"SCRE");
                        cm.send(s->socket);

                        //! @todo Should we consider only sending this message if the phys_id of the object matches that of the originator?
                        // We'd need to add that information to the beaming chain.
                        ScanResultMsg srm;
                        srm.client_id = s->client_id;
                        srm.server_id = s->pobj.phys_id;
                        srm.position = b->scan_target->position;
                        Vector3_subtract(&srm.position, &o->position);
                        srm.velocity = b->scan_target->velocity;
                        Vector3_subtract(&srm.velocity, &o->velocity);
                        srm.thrust = b->scan_target->thrust;
                        srm.mass = b->scan_target->mass;
                        srm.radius = b->scan_target->radius;
                        srm.orientation = {b->scan_target->forward.x, b->scan_target->forward.y, b->scan_target->up.x, b->scan_target->up.y};
                        srm.obj_type = b->scan_target->obj_type;
                        srm.beam_spectrum = Spectrum_clone(b->spectrum);
                        srm.data = b->data;
                        srm.spec_all();

                        if (b->data == NULL)
                        {
                            srm.specced[srm.num_el - 7] = false;
                        }

                        // If the spectrum for the object we hit is NULL, then we should unmark
                        // the spectrum parts of the message.
                        if (b->scan_target->spectrum == NULL)
                        {
                            srm.specced[srm.num_el - 3] = false;
                            srm.specced[srm.num_el - 2] = false;
                            srm.specced[srm.num_el - 1] = false;
                            srm.obj_spectrum = NULL;
                        }
                        else
                        {
                            srm.obj_spectrum = Spectrum_clone(b->scan_target->spectrum);
                        }

                        srm.send(s->socket);

                        // The message object will go out-of-scope, but is currently pointing to the beam's
                        // target's object type, which we don't want to free in the message's constructor.
                        // Re-point it to NULL here, so we don't try to free it in the next line.
                        srm.obj_type = NULL;
                        srm.data = NULL;

                        break;
                    }
                    case BEAM_WEAP:
                        cm.set_colltype((char *)"WEAP");
                        cm.send(s->socket);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        //! @todo Multithread this.
        V3 pd;
        PO *other;
        double distance_sq;

        if ((u->total_time - u->last_effect_time) >= 1.0)
        {
            for (size_t i = 0; i < u->radiators.size(); i++)
            {
                other = u->radiators[i];
                // Don't collide an object with it's own radiation
                if (other->phys_id == o->phys_id)
                {
                    continue;
                }

                Vector3_subtract(&pd, &other->position, &o->position);
                distance_sq = Vector3_length2(&pd);
                if (distance_sq < other->spectrum->safe_distance_sq)
                {
                    double energy = o->radius * o->radius * other->spectrum->total_power / (4 * distance_sq);

                    if (o->type == PHYSOBJECT_SMART)
                    {
                        struct SmartPhysicsObject *s = (SPO *)o;
                        CollisionMsg cm;
                        cm.client_id = s->client_id;
                        cm.server_id = o->phys_id;
                        Vector3_scale(&pd, 1.0 / Vector3_length(&pd));
                        cm.direction = pd;
                        Vector3_scale(&pd, o->radius);
                        cm.position = pd;
                        cm.energy = energy;
                        cm.comm_msg = NULL;
                        cm.spectrum = Spectrum_clone(other->spectrum);
                        //! @todo Ok, this four-character string is starting to feel forced.
                        cm.set_colltype((char *)"RADN");
                        cm.spec_all();

                        // Note that there is no comm message, so that's unspecced.
                        cm.specced[cm.num_el - 4] = false;
                        cm.send(s->socket);
                    }
                    else if (o->type == PHYSOBJECT)
                    {
                        PhysicsObject_resolve_damage(o, energy, u->params.health_damage_threshold);
                    }
                }
            }
        }

        u->get_grav_pull(&g, o);
        PhysicsObject_tick(o, &g, dt);
    }

    // Append a value to the end of a list, assuming there's enough room, if it's not already in the list
    // Return whether or not the value was added to the list (true if it was added, false if it was already
    // there.)
    bool unique_append(size_t *list, size_t max_index, size_t val)
    {
        for (size_t i = 0; i < max_index; i++)
        {
            if (val == list[i])
            {
                return false;
            }
        }

        list[max_index] = val;
        return true;
    }

    void Universe::handle_expired()
    {
        if (expired.size() > 0)
        {
            LOCK(expire_lock);
            //! @todo Promote this into a function that'll make this a LOT simpler here.
            //! Use something like PhysicsObject_free() and Beam_free()

            // Handle expiry queue
            // First sort the expiry queue, which is just a vector of phys_ids
            // Then we can binary search our way as we iterate over the list of phys IDs.
            // That might have a big constant though
            //! @todo Examine the runtime behaviour here, and maybe optimize out some of the linear searches.
            struct PhysicsObject *po;
            struct Beam *b;

            // See if any are a beam.
            for (size_t i = 0; i < beams.size(); i++)
            {
                b = beams[i];
                for (std::set<int64_t>::iterator it = expired.begin(); it != expired.end(); it++)
                {
                    if (*it == b->phys_id)
                    {
                        free(b->data);
                        free(b->comm_msg);

                        // Remember, we cloned the object we're interested in into the scan_target
                        // to prevent a use-after-free, so free that here.
                        if (b->scan_target != NULL)
                        {
                            free(b->scan_target->obj_type);
                            free(b->scan_target);
                        }

                        free(b->spectrum);
                        free(b);
                        beams.erase(beams.begin() + i);
                        it = expired.erase(it);
                        i--;
                        break;
                    }
                }
            }

            // See if any are a physical object.
            for (size_t i = 0; i < phys_objects.size(); i++)
            {
                po = phys_objects[i];
                for (std::set<int64_t>::iterator it = expired.begin(); it != expired.end(); it++)
                {
                    if (*it == po->phys_id)
                    {
                        free(po->obj_type);

                        if (po->type == PHYSOBJECT_SMART)
                        {
                            smarties.erase(*it);
                        }

                        if (phys_objects[i]->emits_gravity)
                        {
                            // This is nicer to read than the iterator in the for loop method.
                            for (size_t k = 0; k < attractors.size(); k++)
                            {
                                if (attractors[k]->phys_id == *it)
                                {
                                    attractors.erase(attractors.begin() + k);
                                    break;
                                }
                            }
                        }

                        if (phys_objects[i]->dangerous_radiation)
                        {
                            // This is nicer to read than the iterator in the for loop method.
                            for (size_t k = 0; k < radiators.size(); k++)
                            {
                                if (radiators[k]->phys_id == *it)
                                {
                                    radiators.erase(radiators.begin() + k);
                                    break;
                                }
                            }
                        }

                        free(po->spectrum);
                        free(po);
                        phys_objects.erase(phys_objects.begin() + i);
                        it = expired.erase(it);
                        i--;
                        break;
                    }
                }
            }

            if (expired.size() != 0)
            {
                for (std::set<int64_t>::iterator it = expired.begin(); it != expired.end(); it++)
                {
#if __x86_64__
                    printf("%ld\n", *it);
#else
                    printf("%lld\n", *it);
#endif
                }
                throw std::runtime_error("WAT");
            }
            UNLOCK(expire_lock);
        }
    }

    void Universe::handle_added()
    {
        if (added.size() > 0)
        {
            LOCK(add_lock);
            // Handle added queue
            for (size_t i = 0; i < added.size(); i++)
            {
                switch (added[i]->type)
                {
                case PHYSOBJECT:
                {
                    phys_objects.push_back(added[i]);
                    if (added[i]->emits_gravity)
                    {
                        attractors.push_back(added[i]);
                    }
                    if (added[i]->dangerous_radiation)
                    {
                        radiators.push_back(added[i]);
                    }
                    break;
                }
                case PHYSOBJECT_SMART:
                {
                    SPO *obj = (SPO *)added[i];
                    //! @todo Check to make sure this smarty adding is right.
                    smarties[obj->pobj.phys_id] = obj;
                    phys_objects.push_back(added[i]);
                    if (added[i]->emits_gravity)
                    {
                        attractors.push_back(added[i]);
                    }
                    if (added[i]->dangerous_radiation)
                    {
                        radiators.push_back(added[i]);
                    }
                    break;
                }
                case BEAM_COMM:
                case BEAM_SCAN:
                case BEAM_SCANRESULT:
                case BEAM_WEAP:
                {
                    beams.push_back((B *)added[i]);
                    break;
                }
                default:
                    break;
                }
            }
            added.clear();
            UNLOCK(add_lock);
        }
    }

    struct Universe::TickMetrics Universe::tick(double dt)
    {
        struct Universe::TickMetrics metrics;
        HRN_T t00 = HRN;

        LOCK(phys_lock);
        HRN_T t0;

        // Only the visdata thread conflicts with this...
        // Are we OK with it getting data that is in the middle of being updated to?
        // This can be done single-threaded for now, and we'll thread it later.

        //! @todo Think about something proper. Temporally ordered collisions, for one,
        //! and something smarter than an N choose 2 approach, such as Axis Aligned Bounding Boxes,
        //! or Oriented Bounding Boxes. The former with gnome-sort turns it into O(NlogN+N) complexity
        //! per pass, roughly, but for an almost sorted list, the sort is O(N) basically.
        //! http://en.wikipedia.org/wiki/Hyperplane_separation_theorem
        //! http://www.gamasutra.com/view/feature/3190/advanced_collision_detection_.php

        //! @todo Multi-level collisoin detecitons in the tick.
        //! @todo Have some concept of collision destruction criteria here.
        //! @todo Temporally ordered collision resolution. (See multi-level collision detection)

        metrics.num_objects = phys_objects.size();
        if (phys_objects.size() > 1)
        {
            t0 = HRN;
            sort_aabb(dt, true);
            metrics.sort_aabb_ns = HRN - t0;

            // How many worker threads, in addition to this main thread.
            // In single-threaded operation, this will result in `n == 0`
            int32_t n = (int32_t)(phys_objects.size() / MIN_OBJECTS_PER_THREAD);
            n = CLAMP(0, n, num_threads - 1);

            // If we're using more than 1 thread, try to split them evenly.
            // Note that this clamps (rounds) down in absolute value, so we'll have extra slack
            // that we'll need to pick up at the end.
            int32_t d = (int32_t)(phys_objects.size() / (n + 1));

            // Reserve one spot for this main thread, all others get primed for work.
            for (int i = 0; i < n; i++)
            {
                phys_worker_args[i].offset = i * d;
                phys_worker_args[i].stride = d;
                phys_worker_args[i].dt = dt;
                phys_worker_args[i].done = false;
                phys_worker_args[i].test_all = false;
                // sched->add_work(thread_check_collisions, &phys_worker_args[i], NULL, libodb::Scheduler::NONE);
                metrics.collision_metrics.add(check_collision_loop(&phys_worker_args[i]));
            }

            // Save one work unit for this thread, so it isn't just sitting doing nothing useful.
            phys_worker_args[n].offset = n * d;
            // Also note that it has to pick up slack objcts not accounted for by the other threads
            // There's at most num_threads-1 such slack, so it isn't going to affect computation time a lot.
            phys_worker_args[n].stride = phys_objects.size() - (n * d);
            phys_worker_args[n].dt = dt;
            phys_worker_args[n].done = false;
            phys_worker_args[n].test_all = false;
            metrics.collision_metrics.add(check_collision_loop(&phys_worker_args[n]));

            // Wait for the workers to report that they are done.
            // This depends on the volatility of their 'done' variable. We don't need
            // atomicity, but we need it to break cache.
            t0 = HRN;
            for (int i = 0; i < n; i++)
            {
                while (!phys_worker_args[i].done)
                {
                }
            }
            metrics.thread_join_wait_ns = HRN - t0;

            // Set this once, we'll use it in all loops in the following.
            phys_worker_args[0].dt = dt;
            phys_worker_args[0].stride = 1;
            phys_worker_args[0].done = false;
            phys_worker_args[0].test_all = true;

            // At this point all collisions should be in the collision list.
            // Sort it by time of collisions (everything should be >= 0), and then resolve them one by one
            // Each collision resolution should have the following steps:
            // - The effects are applied to the two objects, including updating how 'far' into the tick
            //    the collisions have taken the objects ths far.
            // - Any collisions involving either of the two involved objects are discarded from the vector
            // - The two objects are re-collided with all other objects
            //   > Test the sizes of the vector before and after the re-collide, any collisions resulting
            //     from that operation will require a re-sort of the list. Don't re-sort if there's no new events.

            // WHether or not to re-sort the vector after handling a collision, this will be determined by whether
            // a collision resolution results in new collisions being added.
            bool re_sort = true;

            // For any smart objects, we'll need this, so pre-set some values.
            CollisionMsg cm;
            cm.set_colltype((char *)"PHYS");
            cm.comm_msg = NULL;
            cm.spec_all();

            // Since this is a physical collision, there's no spectrum attached.
            cm.spectrum = NULL;
            cm.specced[cm.num_el - 1] = false;
            cm.specced[cm.num_el - 2] = false;
            cm.specced[cm.num_el - 3] = false;

            // The value in params.max_simultaneous_collision_rounds defines the number of rounds that we'll consider
            // multiple collision at the same instant.
            // After this cutoff, only one collision per instant is considered, and the rest discarded for the sake
            // of interactivity. After enough rounds, the eventual effects of the extra energy distribution will be
            // negligible.

            // Number of rounds of collisions we've gone through, for fun.
            uint32_t n_rounds = 0;

            t0 = HRN;
            while (collisions.size() != 0)
            {
                n_rounds++;
                // If we need to, re-sort collisions by time in the interval.
                if (re_sort && (collisions.size() > 1))
                {
                    std::sort(collisions.begin(), collisions.end(),
                              [](struct PhysCollisionEvent &a, struct PhysCollisionEvent &b)
                              { return a.pcr.t < b.pcr.t; });
                }

                // Simultaneous collision handling is not obvious, but ends up working out to be not too complex.
                //
                // We need to ensure conservation of momentum and energy in all of the simultenaous collisions that
                // involve a specific object, the object can't end up with more energy or momentum as a result of
                // the combined effects of multiple collisions than was present in the original system (of all
                // objects of interest). This actually turns out to come for 'free' in some sense as a consequence
                // of the conservation of momentuem/energy in the two-body case.
                //
                // Because the elastic collision of two objects can be reduced to a one-dimensional problem, they
                // are easy to solve, and the solution conserves momentum and energy in that two-object system.
                // This can be extended to address N-body collisions, but considering all necessary two-body
                // collisions, each of which conserves energy/momentum, and applying all of their effects in
                // summation only gets us part way. That is, each collisions results in a delta-v for each object,
                // and so applying the sum of all delta-v (to both objects) in all collisions involving an object
                // will result in a conserved system still MOMENTUM conserved system, but not energy.
                //
                // To regain energy conservation, it is important to realize that, in a collision, an object
                // exchanges more energy with it's neighbours than it possessed in the original system. This results
                // in a final system that contains more energy than the original system possessed. To reconcile this
                // it is possible to calculate the energy in the final system, and original system, which will be
                // related by E'=kE, with k>=1. Conservation can be regained by scaling all resulting velocities
                // by 1/sqrt(k).
                //
                // IMPORTANT NOTE: This approximation is physically incorrect, since it will return conservation of
                // energy, but destroy conservation of momentum. This is a fine approximation, since it results in
                // a system that has a slightly too 'spread-out' (in some sense) set of resulting velocities. But
                // at least the magnitudes, directions, and energiers are approximately, and qualitatively, right.
                //
                // The only tricky part is ensuring that we're keeping the dt stepping of each object correct.

                // Now resolve the collisions on the objects, setting the time-delta to be the time from the last-moved
                // time of the object we're moving, to the time that this collision happened
                // Note that we're setting the object's ticked time to an actual amount of interval simulated time, not a
                // proportion of the interval.
                //
                // The proportional time-interval value, 't' in the physics collicion result, is the proportion of the
                // original total interval. We want to pass the time elapsed since the last notable event to the
                // collision resolution code. This elapsed time is the proportion times dt, since we do the consideration
                // earlier that ensures that any collisions that we're considering are happening in the future, and
                // within the remainder of the time interval, of ALL objects involved.

                // Keep track of the number of simultaneous collisions, we'll need to use this information for discarding
                // invalidated future collisions
                size_t n_simultaneous = 0;

                // A list of all objects involved in the collisions
                size_t *objs = (size_t *)malloc(sizeof(size_t) * 2 * collisions.size());
                if (objs == NULL)
                {
                    throw std::runtime_error("Universe::Tick::UnableToAllocateObjectList");
                }

                // Number of distinct objects we've encountered so far.
                size_t n_objs = 0;
                double energy0 = 0.0;

                while ((n_simultaneous < collisions.size()) &&
                       (Vector3_almost_zeroS(collisions[0].pcr.t - collisions[n_simultaneous].pcr.t)))
                {
                    // For each collision that happens at the same time as the first, apply it to the objects involved
                    // Retrieve the earliest collision details.
                    struct PhysCollisionEvent collision_event = collisions[n_simultaneous];
                    struct PhysicsObject *obj1 = collision_event.obj1;
                    struct PhysicsObject *obj2 = collision_event.obj2;
                    struct PhysCollisionResult phys_result = collision_event.pcr;

                    if ((n_rounds == 1) && params.verbose_logging)
                    {
#if __x86_64__
                        fprintf(stderr, "Collision: %lu <-> %lu (%.15g J)\n", obj1->phys_id, obj2->phys_id, phys_result.e);
#else
                        fprintf(stderr, "Collision: %llu <-> %llu (%.15g J)\n", obj1->phys_id, obj2->phys_id, phys_result.e);
#endif
                    }

                    // Note that when applying the collision, we need to make sure that each object is observing the
                    // correct time-delta to have elapsed since their last physics event. This is why we take the
                    // collision results 't' parameter portion of the total tick time (t*dt), and subtract off the
                    // object's alread-ticked time. This gives us the delta since the last time that object had it's
                    // velocity accounted for.

                    // Have we already seen the object in question? (Does it exist in the objs array) Should be true
                    // if this is a new object not in the array.
                    bool never_seen;

                    // While we're at it, keep track of all distinct objects that we've collided by adding/counting them
                    // in the array, in case we haven't seen them already. Additionally, take this time to calculate the
                    // kinetic energy of all objects before any collisions are taken into consideration, store it in
                    // energy0.
                    never_seen = unique_append(objs, n_objs, collision_event.obj1_index);
                    n_objs += never_seen;
                    energy0 += (never_seen ? obj1->mass * Vector3_length2(&obj1->velocity) : 0.0);
                    PhysicsObject_collision(obj1, obj2, phys_result.e, never_seen * phys_result.t * dt, &phys_result.pce1, params.health_damage_threshold);
                    never_seen = unique_append(objs, n_objs, collision_event.obj2_index);
                    n_objs += never_seen;
                    energy0 += (never_seen ? obj2->mass * Vector3_length2(&obj2->velocity) : 0.0);
                    PhysicsObject_collision(obj2, obj1, phys_result.e, never_seen * phys_result.t * dt, &phys_result.pce2, params.health_damage_threshold);

                    //! @todo This messaging should probably be done asynchronously, out of the physics code,
                    //! but I don't think, in general, this will cause much of a problem for a 60hz
                    //! physics refrecsh rate.

                    // Alert the smarties that there was a collision, if either re smart.
                    // Only physical collisions are generated here, beams are elsewhere.
                    cm.energy = phys_result.e;

                    if (obj1->type == PHYSOBJECT_SMART)
                    {
                        SPO *s = (SPO *)obj1;
                        cm.client_id = s->socket;
                        cm.server_id = obj1->phys_id;
                        cm.direction = phys_result.pce1.d;
                        cm.position = phys_result.pce1.p;
                        cm.send(s->socket);
                    }

                    if (obj2->type == PHYSOBJECT_SMART)
                    {
                        SPO *s = (SPO *)obj2;
                        cm.client_id = s->socket;
                        cm.server_id = obj2->phys_id;
                        cm.direction = phys_result.pce2.d;
                        cm.position = phys_result.pce2.p;
                        cm.send(s->socket);
                    }

                    n_simultaneous++;

                    // To prevent hanging in the situation where there's a constant feedback of collisions,
                    // limit the number of rounds we'll support.
                    if (n_rounds > params.max_simultaneous_collision_rounds)
                    {
                        break;
                    }
                }

                // The scaling factor for the post-collision velocities to restore energy conservation, if there is
                // only one collision, then the two-body solution conserves energy.
                double k = 1.0;
                if (n_simultaneous > 1)
                {
                    // Post-collision system energy
                    double energy1 = 0.0;
                    for (size_t i = 0; i < n_objs; i++)
                    {
                        energy1 += phys_objects[objs[i]]->mass * Vector3_length2(&phys_objects[objs[i]]->velocity);
                    }

                    // If there's more than one collision, then this factor is the ratio of original to final
                    // energy.
                    k = sqrt(energy0 / energy1);
                }

                // Note that these collisions have invalidated the correctness of future collisions, so we need to discard
                // all future collisions that involve any object that we've already considered, as well as the first
                // n_simultaneous events, because those have been applied to the world.
                // @todo There's a bunch of O(N) and other expensive events happening here...
                // Only both if there's collisions that didn't happen 'now'.
                for (size_t i = 0; i < n_objs; i++)
                {
                    Vector3_scale(&phys_objects[objs[i]]->velocity, k);
                    if (collisions.size() > n_simultaneous)
                    {
                        for (std::vector<struct PhysCollisionEvent>::iterator it = collisions.begin() + n_simultaneous; it != collisions.end();)
                        {
                            if ((objs[i] == (*it).obj1_index) || (objs[i] == (*it).obj1_index))
                            {
                                it = collisions.erase(it);
                            }
                            else
                            {
                                it++;
                            }
                        }
                    }
                }
                // Note that n_simultaneous >= 1, so this is well defined. This could be handled in the above loop, but
                // in theory this is a little bit less inefficient.
                collisions.erase(collisions.begin(), collisions.begin() + n_simultaneous);

                // Re-collide the affected objects by calling back to check_collision_loop() with appropriate args.
                // Remaining time left in the tick is the total time minus what has been accounted for so far.
                // Note that obj1 and obj2 should have identical values here, that should be assert()-able
                //
                // Note that, really, EVERY physics object should be ticked to this point. All further comparisons
                // can assume that other physics objects are 'valid' up until the point of the collision we just
                // resolved. It's important to note, as well, that we have to consider the whole tick event for every
                // object, and we will have to consider the 't' parameter of objects, to make sure we're still within
                // the interval of the objects of interest. This consideration is done in check_collision_single().

                // Keep track of pre-recollide size, and if it changes, we'll know we have to re-sort on the next loop.
                size_t pre_size = collisions.size();
                for (size_t i = 0; i < n_objs; i++)
                {
                    struct PhysicsObject *o = phys_objects[objs[i]];
                    PhysicsObject_estimate_aabb(o, &o->box, phys_worker_args[0].dt);
                    phys_worker_args[0].offset = objs[i];
                    metrics.multicollision_metrics.add(check_collision_loop(&phys_worker_args[0]));
                }
                // If our vector isn't the same size as before we re-collided, then we'll have to re-sort.
                re_sort = (collisions.size() != pre_size);

                //! @todo This is inefficient, realloc()?
                free(objs);
                objs = NULL;
                metrics.collision_metrics.simultaneous_collisions += n_simultaneous;
            }

            if ((n_rounds > 1) && params.verbose_logging)
            {
                printf("Collision set required %u rounds\n", n_rounds);
            }

            metrics.collision_metrics.collision_rounds += n_rounds;
        }
        metrics.collision_resolution_ns = HRN - t0;

        t0 = HRN;
        // Now tick along each object, handling any gravity and radiation while we're at it
        for (size_t i = 0; i < phys_objects.size(); i++)
        {
            obj_tick(this, phys_objects[i], dt);
        }
        metrics.object_tick_ns = HRN - t0;

        // Now tick along each beam while we're here because we won't be
        // needing to reference them afer this.
        t0 = HRN;
        for (size_t bi = 0; bi < beams.size(); bi++)
        {
            Beam_tick(beams[bi], dt);
        }
        metrics.beam_tick_ns = HRN - t0;

        t0 = HRN;
        handle_expired();
        handle_added();
        metrics.object_lifecycle_ns = HRN - t0;

        // Now update the periodic timer that tells us when to do periodic effects.
        if ((total_time - last_effect_time) >= 1.0)
        {
            last_effect_time = total_time;
        }

        metrics.total_ns = HRN - t00;
        // Unlock everything
        UNLOCK(phys_lock);

        return metrics;
    }

    void Universe::update_list(struct PhysicsObject *obj, std::vector<struct PhysicsObject *> *list, bool newval, bool oldval)
    {
        // If it's a current candidate, and the old value indicates it wasn't before, then
        // we can safely just add it.
        if (newval && !oldval)
        {
            list->push_back(obj);
        }
        // If we're an existing member, and the new value indicates we should no longer be,
        // then find it and remove it.
        else if (!newval && oldval)
        {
            // Then linear searches here shouldn't be problematic, because
            for (size_t i = 0; i < list->size(); i++)
            {
                if (list->at(i)->phys_id == obj->phys_id)
                {
                    list->erase(list->begin() + i);
                    break;
                }
            }
        }
    }
}

// TODO This is subject to some wild variation in actual sleep time,
// occasionally up and and including an extra order of magnitude.
//
// This needs to be more precise to be able to work with write time divs
// smaller than 1ms.
//
// This can, in theory, be used to provide the ability to spin_sleep for
// frametimes smaller than about 2ms, and maintain realtime processing.
// Important for precise realtime activities.
void spin_sleep_for(std::chrono::microseconds dura)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start;
    auto elapsed = end - start;
    while (elapsed < dura)
    {
        end = std::chrono::high_resolution_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }
}
