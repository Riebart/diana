#include "universe.hpp"
#include "messaging.hpp"

#include <stdio.h>
#include <algorithm>

#ifdef CPP11THREADS
#include <chrono>
#define LOCK(l) l.lock()
#define UNLOCK(l) l.unlock()
#define THREAD_CREATE(t, f, a) t = std::thread(f, a)
#define THREAD_JOIN(t) if (t.joinable()) t.join()
#else
#include <sys/timeb.h>
#include <unistd.h>
#define LOCK(l) pthread_rwlock_wrlock(&l)
#define UNLOCK(l) pthread_rwlock_unlock(&l)
#define THREAD_CREATE(t, f, a) pthread_create(&t, NULL, &f, a)
#define THREAD_JOIN(t) pthread_join(t, NULL)
#endif

#define GRAVITATIONAL_CONSTANT  6.67384e-11
#define COLLISION_ENERGY_CUTOFF 1e-9
#define ABSOLUTE_MIN_FRAMETIME 1e-7
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

const int MIN_OBJECTS_PER_THREAD = 500;

typedef struct Beam B;
typedef PhysicsObjectType POT;
typedef struct PhysicsObject PO;
typedef struct SmartPhysicsObject SPO;
typedef struct Vector3 V3;

void gravity(V3* out, PO* big, PO* small)
{
    double m = GRAVITATIONAL_CONSTANT * big->mass * small->mass / Vector3_distance2(&big->position, &small->position);
    Vector3_ray(out, &small->position, &big->position);
    Vector3_scale(out, m);
}

void* sim(void* uV)
{
    Universe* u = (Universe*)uV;

    // dt is the amount of time that will pass in the game world during the next tick.
    double dt = u->min_frametime;

#ifdef CPP11THREADS
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<double> elapsed;
#else
    struct timeb start, end;
#endif

    while (u->running)
    {
        // If we're paused, then just sleep. We're not picky on how long we sleep for.
        // Sleep for max_frametime time so that we're responsive to unpausing, but not
        // waking up too often.
        if (u->paused)
        {
#ifdef CPP11THREADS
            std::this_thread::sleep_for(std::chrono::milliseconds((int32_t)(1000 * u->max_frametime)));
#else
            usleep((uint32_t)(1000000 * u->max_frametime));
#endif
            continue;
        }

#ifdef CPP11THREADS
        start = std::chrono::high_resolution_clock::now();
        u->tick(u->rate * dt);
        end = std::chrono::high_resolution_clock::now();

        elapsed = end - start;
        double e = elapsed.count();
#else
        ftime(&start);
        u->tick(u->rate * dt);
        ftime(&end);
        double e = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
#endif
        u->phys_frametime = e;

        // If we're simulating in real time, make sure that we aren't going too fast
        // If the physics took less time than the minimum we're allowing, sleep for
        // at least the remainder. The precision is nanoseconds, because that's the smallest
        // interval we can represent with std::chrono.
        if (u->realtime && ((u->min_frametime - e) > 1e-9))
        {
            // C++11 sleep_for is guaranteed to sleep for AT LEAST as long as requested.
            // As opposed to Python's sleep which may wake up early.
            //
            // In practice, a 1ms min frame time actually causes the average
            // frame tiem to be about 2ms (Tested on Windows 8 and Ubuntu in
            // a VBox VM).
#ifdef CPP11THREADS
            std::this_thread::sleep_for(std::chrono::microseconds((int32_t)(1000000 * (u->min_frametime - e))));
            end = std::chrono::high_resolution_clock::now();
            elapsed = end - start;
            e = elapsed.count();
#else
            usleep((uint32_t)(1000000 * (u->min_frametime - e)));
            ftime(&end);
            e = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
#endif

            // If the tick lasted less than the max, let it pass by in 'real' time.
            // Otherwise, clamp it down which is where we get the slowdown effect.
            dt = MIN(u->max_frametime, e);
        }
        // If we're not simulating then just make sure that the time step next time
        // is at least the min_frametime.
        else if (!u->realtime)
        {
            dt = MIN(u->max_frametime, MAX(e, u->min_frametime));
        }

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

void* vis_data_thread(void* argV)
{
    Universe* u = (Universe*)argV;

#ifdef CPP11THREADS
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<double> elapsed;
#else
    struct timeb start, end;
#endif
    
    while (u->running)
    {
        // If we're paused, then just sleep. We're not picky on how long we sleep for.
        // Sleep for max_frametime time so that we're responsive to unpausing, but not
        // waking up too often.
        if (u->visdata_paused)
        {
#ifdef CPP11THREADS
            std::this_thread::sleep_for(std::chrono::milliseconds((int32_t)(1000 * u->max_frametime)));
#else
            usleep((uint32_t)(1000000 * u->max_frametime));
#endif
            continue;
        }

#ifdef CPP11THREADS
        start = std::chrono::high_resolution_clock::now();
        u->broadcast_vis_data();
        end = std::chrono::high_resolution_clock::now();

        elapsed = end - start;
        double e = elapsed.count();
#else
        ftime(&start);
        u->broadcast_vis_data();
        ftime(&end);
        double e = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
#endif
        
        // Make sure to pace ourselves, and not broadcast faster than the minimum interval.
        if (e < u->min_vis_frametime)
        {
#ifdef CPP11THREADS
            std::this_thread::sleep_for(std::chrono::microseconds((int32_t)(1000000 * (u->min_vis_frametime - e))));
            end = std::chrono::high_resolution_clock::now();
            elapsed = end - start;
            e = elapsed.count();
#else
            usleep((uint32_t)(1000000 * (u->min_vis_frametime - e)));
            ftime(&end);
            e = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);
#endif
        }
    }

    return NULL;
}

void Universe_hangup_objects(int32_t c, void* arg)
{
    Universe* u = (Universe*)arg;
    u->hangup_objects(c);
}

void Universe_handle_message(int32_t c, void* arg)
{
    Universe* u = (Universe*)arg;
    u->handle_message(c);
}

/// @todo Support minimum frametimes in the micro and nanosecond ranges without sleeping, maybe via a real_time boolean flag.
/// @in min_frametime Minimum time in the simulation between physics ticks, regardles of the real wall clock time of a physics tick.
///    If this is set to a value lower than the wall-clock time of a physics tick, then the simulation thread will fully utilize the available CPU resources.
///    To achieve a sense of throttling, set this to a value above the typical wall-clock time to cause the simulation thread to sleep between ticks.
/// @in max_frametime Maximum time allowed to pass by in the simulation in a single tick.
///    If this is less than the wall clock time, it will result in a perceived slowdown of the simulation.
/// @in min_vis_frametime The minimum time between visualization updates, reduces load on the physics server.
/// @in port TCP port to listen on when start_net() is called.
/// @in num_threads Number of threads to use for collision detection, no more than 4 is recommended (there's no gains at that point)
/// @in rate Multiplier used to scale the time-tick in the simulation. Most useful in conjunction with realtime.
/// @in realtime If set to true, then the simulation attempts to pass in real time, with physics ticks sleeping to match wall-clock time if necessary.
/// The constructor to initialize a universe for physics simulation.
Universe::Universe(double min_frametime, double max_frametime, double min_vis_frametime, int32_t port, int32_t num_threads, double rate, bool realtime)
{
    this->rate = rate;

    if (realtime && (min_frametime < ABSOLUTE_MIN_FRAMETIME))
    {
        fprintf(stderr, "WARNING: min_framtime set below absolute minimum. Raising it to %g s.\n", ABSOLUTE_MIN_FRAMETIME);
        min_frametime = ABSOLUTE_MIN_FRAMETIME;
        max_frametime = MAX(max_frametime, ABSOLUTE_MIN_FRAMETIME);
    }

    this->num_threads = num_threads;
    sched = new libodb::Scheduler(this->num_threads - 1);

    phys_worker_args = (struct phys_args*)malloc(this->num_threads * sizeof(struct phys_args));
    for (int i = 0; i < this->num_threads; i++)
    {
        phys_worker_args[i].u = this;
        phys_worker_args[i].offset = i;
        phys_worker_args[i].stride = this->num_threads;
        phys_worker_args[i].dt = 0.0;
        phys_worker_args[i].done = true;
    }

    this->min_frametime = min_frametime;
    this->max_frametime = max_frametime;
    this->min_vis_frametime = min_vis_frametime;
    this->realtime = realtime;

    total_time = 0.0;
    phys_frametime = 0.0;
    wall_frametime = 0.0;
    game_frametime = 0.0;
    vis_frametime = 0.0;

    num_boxes_updated = 0;

    total_time = 0.0;
    num_ticks = 0;
    total_objs = 1;

    paused = true;
    visdata_paused = true;
    running = false;

    net = new MIMOServer(Universe_handle_message, this, Universe_hangup_objects, this, port);
}

Universe::~Universe()
{
    for (int i = 0; i < num_threads; i++)
    {
        phys_worker_args[i].done = false;
    }

    sched->block_until_done();
    free(phys_worker_args);

    stop_net();
    stop_sim();
    delete sched;
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
        THREAD_CREATE(sim_thread, sim, (void*)this);
        THREAD_CREATE(vis_thread, vis_data_thread, (void*)this);
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

void Universe::get_frametime(double* out)
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

double Universe::total_sim_time()
{
    return total_time;
}

int64_t Universe::get_id()
{
#ifdef CPP11THREADS
    uint64_t r = total_objs.fetch_add(1);
#else
    uint64_t r = __sync_fetch_and_add(&total_objs, 1);
#endif
    return r;
}

void Universe::add_object(PO* obj)
{
    obj->phys_id = get_id();

    LOCK(add_lock);
    added.push_back(obj);
    UNLOCK(add_lock);
}

void Universe::add_object(B* beam)
{
    add_object((PO*)beam);
}

void Universe::expire(int64_t phys_id)
{
    LOCK(expire_lock);
    expired.push_back(phys_id);
    UNLOCK(expire_lock);
}

void Universe::hangup_objects(int32_t c)
{
    LOCK(expire_lock);
    std::map<int64_t, struct SmartPhysicsObject*>::iterator it;

    for (it = smarties.begin(); it != smarties.end(); ++it)
    {
        if (it->second->client == c)
        {
            expired.push_back(it->first);
        }
    }
    UNLOCK(expire_lock);
}

void Universe::register_vis_client(struct vis_client vc, bool enabled)
{
    //! @todo Remove the linear searches here
    LOCK(vis_lock);
    
    // Try to find the one we're looking for.
    std::vector<struct vis_client>::iterator it = std::lower_bound(vis_clients.begin(), vis_clients.end(), vc,
        [](struct vis_client a, struct vis_client b) { return (memcmp(&a, &b, sizeof(struct vis_client)) < 0); });

    bool found = (vis_clients.size() == 0 ? false : (memcmp(&vc, &*it, sizeof(struct vis_client)) == 0));
    
    if (enabled && !found)
    {
        // If we find it, push it onto the back and re-sort the list.
        vis_clients.push_back(vc);
        std::sort(vis_clients.begin(), vis_clients.end(),
            [](struct vis_client a, struct vis_client b) { return (memcmp(&a, &b, sizeof(struct vis_client)) < 0); });
    }
    else if (!enabled && found)
    {
        vis_clients.erase(it);
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
    PO* o;
    PO* ro;
    
    LOCK(vis_lock);
    for (std::vector<struct vis_client>::iterator it = vis_clients.begin(); it != vis_clients.end();)
    {
        vc = *it;
        visdata_msg.client_id = vc.client_id;
        ro = (vc.phys_id == -1 ? NULL : (PO*)smarties[vc.phys_id]);
        bool disconnect = false;
        
        for (size_t i = 0; i < phys_objects.size(); i++)
        {
            o = phys_objects[i];
            visdata_msg.server_id = o->phys_id;
            visdata_msg.radius = o->radius;
            visdata_msg.phys_id = o->phys_id;
            visdata_msg.position = o->position;
            
            if (ro != NULL)
            {
                Vector3_subtract(&visdata_msg.position, &visdata_msg.position, &(ro->position));
            }
            
            visdata_msg.orientation.w = o->forward.x;
            visdata_msg.orientation.x = o->forward.y;
            visdata_msg.orientation.y = o->up.x;
            visdata_msg.orientation.z = o->up.y;
            int64_t nbytes = visdata_msg.send(vc.socket);
            
            if (nbytes < 0)
            {
                vis_clients.erase(it);
                printf("Client %d erased due to failed network send");
                disconnect = true;
                break;
            }
        }

        if (!disconnect)
        {
            it++;
        }
    }
    UNLOCK(vis_lock);
}

void Universe::handle_message(int32_t c)
{
    BSONMessage* msg_base = BSONMessage::ReadMessage(c);

    printf("Received message of type %d from client %d\n", msg_base->msg_type, c);

    switch (msg_base->msg_type)
    {
    case BSONMessage::MessageType::VisualDataEnable:
    {
        VisualDataEnableMsg* msg = (VisualDataEnableMsg*)msg_base;
        printf("Client %d %s for VisData\n", c, (msg->enabled ? "REGISTERED" : "UNREGISTERED"));
        struct vis_client vc;
        vc.socket = c;
        vc.client_id = msg->client_id;
        vc.phys_id = msg->server_id;
        register_vis_client(vc, msg->enabled);
        break;
    }
    case BSONMessage::MessageType::Spawn:
    {
        SpawnMsg* msg = (SpawnMsg*)msg_base;
        break;
    }
    case BSONMessage::MessageType::ScanResponse:
    {
        ScanResponseMsg* msg = (ScanResponseMsg*)msg_base;
        break;
    }
    case BSONMessage::MessageType::Hello:
    {
        HelloMsg* msg = (HelloMsg*)msg_base;
        break;
    }
    default:
        throw "Universe::UnrecognizedMessageType";
    }

    delete msg_base;
}

void Universe::get_grav_pull(V3* g, PO* obj)
{
    V3 cg;
    for (size_t i = 0; i < attractors.size(); i++)
    {
        gravity(&cg, attractors[i], obj);
        Vector3_add(g, &cg);
    }
}

void check_collision_single(Universe* u, struct PhysicsObject* obj1, struct PhysicsObject* obj2, double dt)
{
    struct PhysCollisionResult phys_result;

    // Return from a phys-phys collision is
    //    [t, [e1, e2], [obj1  collision data], [obj2 collision data]]
    // t is in [0,1] and indicates when in the interval the collision happened
    // energy is obvious.
    // d1 is the direction obj2 was travelling relative to obj1 whe they collided
    // p1 is a vector from obj1's position to the collision sport on its bounding ball.
    PhysicsObject_collide(&phys_result, obj1, obj2, dt);

    if (phys_result.t >= 0.0)
    {
        // By comparing the energy to a cutoff, this will help prevent
        // spurious collision notifications due to physics time step
        // increments and temporary object intersection.

        // Total energy involved. Both objects 'absorb' the same amount
        // of energy from an 'impact effect' perspective, and that is in
        // the 'e' field of the collision result.

        if ((phys_result.e < -COLLISION_ENERGY_CUTOFF) ||
            (phys_result.e > COLLISION_ENERGY_CUTOFF))
        {
            //! @todo Messaging in the tick is going to be bad for performance.
#if _WIN64 || __x86_64__
            fprintf(stderr, "Collision: %lu <-> %lu (%.15g J)\n", obj1->phys_id, obj2->phys_id, phys_result.e);
#else
            fprintf(stderr, "Collision: %llu <-> %llu (%.15g J)\n", obj1->phys_id, obj2->phys_id, phys_result.e);
#endif
            PhysicsObject_collision(obj1, obj2, phys_result.e, phys_result.t * dt, &phys_result.pce1);
            PhysicsObject_collision(obj2, obj1, phys_result.e, phys_result.t * dt, &phys_result.pce2);

            //! @todo This should probably be done asynchronously, out of the physics code,
            //! But I don't think, in general, this will cause much of a problem for a 60hz
            //! physics refresh rate.

            if (obj1->type == PHYSOBJECT_SMART)
            {
                //SPO* s = (SPO*)obj1;
                //! @todo Smart phys collision messages
            }

            if (obj2->type == PHYSOBJECT_SMART)
            {
                //SPO* s = (SPO*)obj2;
                //! @todo Smart phys collision (other) messages
            }
        }
    }
}

//! Get the next collision, as ordered by time
void Universe::get_next_collision(double dt, struct PhysCollisionResult* phys_result)
{
}

void check_collision_loop(void* argsV)
{
    struct Universe::phys_args* args = (struct Universe::phys_args*)argsV;
    Universe* u = args->u;

    struct AABB* a;
    struct AABB* b;

    size_t end = args->offset + args->stride;
    end = MIN(end, u->sorted.size() - 1);

    for (size_t i = args->offset; i < end; i++)
        //for (size_t i = args->offset; i < u->sorted.size() - 1; i += args->stride)
    {
        // In order for a full intersection to be possible, there has to be intersection
        // of the AABBs in all three dimensions. The sorted list contains the objects'
        //
        // AABBs sorted by their lowest coordinate in the X dimensions.
        // We can test for potential intersections, first pruning by intersection along
        // the X axis.
        //
        // First test to see if the X projections intersect. If they do, then test the others.

        a = &u->phys_objects[u->sorted[i]]->box;
        for (size_t j = i + 1; j < u->sorted.size(); j++)
        {
            b = &u->phys_objects[u->sorted[j]]->box;

            double d;
            d = a->u.x - b->l.x;

            // This is to text if they intersect/touch in X
            // It's simpler here, because we have a guarantee (thanks to sorting)
            // about the relative positions of the lower endpoints of the intervals so
            // we don't need to worry about those here.
            if (Vector3_almost_zeroS(d) || (d > 0))
            {
                // Now do a full test on the Y axis.
                if (!Vector3_intersect_interval(a->l.y, a->u.y, b->l.y, b->u.y))
                {
                    continue;
                }

                // And then a full test on Z
                if (!Vector3_intersect_interval(a->l.z, a->u.z, b->l.z, b->u.z))
                {
                    continue;
                }

                // If we succeeded on both, count this as a potential collision for
                // honest-to-goodness testing and hand it off to bounding-ball testing
                // followed by collision effect calculation.
                check_collision_single(u, u->phys_objects[u->sorted[i]], u->phys_objects[u->sorted[j]], args->dt);
            }
            else
            {
                // If we fail that X comparison, we know that we will fail for every object
                // 'after', so we can bail on the loop.
                break;
            }
        }
    }
}

void* thread_check_collisions(void* argsV)
{
    struct Universe::phys_args* args = (struct Universe::phys_args*)argsV;

    // Spin until we get the signal to start going.
    // The universe will flip this after the next workload is ready to go.
    while (args->done){}

    check_collision_loop(argsV);

    // Indicate that we're done, and spin until we get the signal to stop.
    // The universe will flip this after the next workload is ready to go.
    args->done = true;
    while (args->done){}

    return NULL;
}

void Universe::sort_aabb(double dt, bool calc)
{
    // Employs gnome sort to sort the lists, computing the bounding boxes along the way
    // http://en.wikipedia.org/wiki/Gnome_sort

    size_t box_swap;
    size_t max_so_far = 0;

    if (calc)
    {
        PhysicsObject_estimate_aabb(phys_objects[sorted[0]], &phys_objects[sorted[0]]->box, dt);
    }

    for (size_t i = 1; i < phys_objects.size();)
    {
        if (i > max_so_far)
        {
            // We only need to compute the bounding boxes on the first pass.
            if (calc)
            {
                PhysicsObject_estimate_aabb(phys_objects[sorted[i]], &phys_objects[sorted[i]]->box, dt);
            }

            max_so_far = i;
        }

        //! @todo Could the AABB comparisons be costly?

        // Compare to the previous one if we're not at the first one.
        int c = Vector3_compare_aabbX(&phys_objects[sorted[i]]->box, &phys_objects[sorted[i - 1]]->box);

        if (c < 0)
        {
            box_swap = sorted[i];
            sorted[i] = sorted[i - 1];
            sorted[i - 1] = box_swap;

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

void obj_tick(Universe* u, struct PhysicsObject* o, double dt)
{
    V3 g = { 0.0, 0.0, 0.0 };
    struct BeamCollisionResult beam_result;
    struct PhysCollisionResult phys_result;

    struct Beam* b;

    for (size_t bi = 0; bi < u->beams.size(); bi++)
    {
        b = u->beams[bi];
        Beam_collide(&beam_result, b, o, dt);

        if (beam_result.t >= 0.0)
        {
            phys_result.pce1.d = beam_result.d;
            phys_result.pce1.p = beam_result.p;

#if _WIN64 || __x86_64__
            fprintf(stderr, "Beam Collision: %lu -> %lu (%.15g J)\n", b->phys_id, o->phys_id, beam_result.e);
#else
            fprintf(stderr, "Beam Collision: %llu -> %llu (%.15g J)\n", b->phys_id, o->phys_id, beam_result.e);
#endif
            PhysicsObject_collision(o, (PO*)b, beam_result.e, beam_result.t * dt, &phys_result.pce1);

            //! @todo Smarty beam collision messages
            //! @todo Beam collision messages
        }
    }

    u->get_grav_pull(&g, o);
    PhysicsObject_tick(o, &g, dt);
}

void Universe::tick(double dt)
{
    LOCK(phys_lock);

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
    //! @todoTemporally ordered collision resolution. (See multi-level collision detection)

    if (phys_objects.size() > 1)
    {
        sort_aabb(dt, true);

        int32_t n = (int32_t)(phys_objects.size() / MIN_OBJECTS_PER_THREAD);
        n = MAX(0, MIN(num_threads - 1, n));

        // If we're using more than 1 thread, try to split them evenly.
        // Note that this clamps (rounds) down in absolute value, so we'll have extra slack
        // that we'll need to pick up at the end.
        int32_t d = (int32_t)(phys_objects.size() / (n + 1));

        for (int i = 0; i < n; i++)
        {
            phys_worker_args[i].offset = i * d;
            phys_worker_args[i].stride = d;
            phys_worker_args[i].dt = dt;
            sched->add_work(thread_check_collisions, &phys_worker_args[i], NULL, libodb::Scheduler::NONE);
            phys_worker_args[i].done = false;
        }

        // Save one work unit for this thread, so it isn't just sitting doing nothing useful.
        phys_worker_args[n].offset = n * d;
        // Also note that it has to pick up slack objcts not accounted for by the other threads
        // There's at most num_threads-1 such slack, so it isn't going to affect computation time a lot.
        phys_worker_args[n].stride = phys_objects.size() - (n * d);
        phys_worker_args[n].dt = dt;
        check_collision_loop(&phys_worker_args[n]);

        // Wait for the workers to report that they are done.
        // This depends on the volatility of their 'done' variable. We don't need
        // atomicity, but we need it to break cache.
        for (int i = 0; i < n; i++)
        {
            while (!phys_worker_args[i].done) {}
        }
    }

    // Now tick along each object
    for (size_t i = 0; i < phys_objects.size(); i++)
    {
        obj_tick(this, phys_objects[i], dt);
    }

    // Now tick along each beam while we're here because we won't be
    // needing to reference them afer this.
    for (size_t bi = 0; bi < beams.size(); bi++)
    {
        Beam_tick(beams[bi], dt);
    }

    if (expired.size() > 0)
    {
        LOCK(expire_lock);
        // Handle expiry queue
        // First sort the expiry queue, which is just a vector of phys_ids
        // Then we can binary search our way as we iterate over the list of phys IDs.
        // That might have a big constant though
        //! @todo Examine the runtime behaviour here, and maybe optimize out some of the linear searches.
        bool backtrack = false;
        for (size_t i = 0; i < phys_objects.size(); i++)
        {
            if (backtrack)
            {
                i--;
                backtrack = false;
            }

            for (size_t j = 0; j < expired.size(); j++)
            {
                if (expired[j] == phys_objects[i]->phys_id)
                {
                    // If it's a beam...
                    if (phys_objects[i]->type >= BEAM_COMM)
                    {
                        for (size_t k = 0; k < beams.size(); k++)
                        {
                            if (beams[k]->phys_id == expired[j])
                            {
                                if (beams[k]->scan_target != NULL)
                                {
                                    // Remember, we cloned the object we're interested in into the scan_target
                                    // to prevent a use-after-free, so free that here.
                                    free(beams[k]->scan_target);
                                }
                                beams.erase(beams.begin() + k);
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (phys_objects[i]->type == PHYSOBJECT_SMART)
                        {
                            smarties.erase(expired[j]);
                        }

                        if (phys_objects[i]->emits_gravity)
                        {
                            // This is nicer to read than the iterator in the for loop method.
                            for (size_t k = 0; k < attractors.size(); k++)
                            {
                                if (attractors[k]->phys_id == expired[j])
                                {
                                    attractors.erase(attractors.begin() + k);
                                    break;
                                }
                            }
                        }
                    }
                    free(phys_objects[i]);
                    phys_objects.erase(phys_objects.begin() + i);
                    backtrack = true;
                    continue;
                }
            }
        }

        sorted.resize(phys_objects.size());
        for (size_t i = 1; i < phys_objects.size(); i++)
        {
            sorted[i - 1] = i;
        }

        expired.clear();
        UNLOCK(expire_lock);
    }

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

                // Allocate and push back a bounding box, as well as index
                // members to each of the sorting vectors.
                sorted.push_back(phys_objects.size() - 1);
                break;
            }
            case PHYSOBJECT_SMART:
            {
                SPO* obj = (SPO*)added[i];
                //! @todo Check to make sure this smarty adding is right.
                smarties[obj->pobj.phys_id] = obj;
                phys_objects.push_back(added[i]);
                if (added[i]->emits_gravity)
                {
                    attractors.push_back(added[i]);
                }

                // Allocate and push back a bounding box, as well as index
                // members to each of the sorting vectors.
                sorted.push_back(phys_objects.size() - 1);
                break;
            }
            case BEAM_COMM:
            case BEAM_SCAN:
            case BEAM_SCANRESULT:
            case BEAM_WEAP:
            {
                beams.push_back((B*)added[i]);
                break;
            }
            default:
                break;
            }
        }
        added.clear();
        UNLOCK(add_lock);
    }

    // Unlock everything
    UNLOCK(phys_lock);
}

void Universe::update_attractor(struct PhysicsObject* obj, bool calculate)
{
    // This requires a linear search of the attractors, but whatever.
    if (calculate)
    {
        // If 
        bool newval = is_big_enough(obj->mass, obj->radius);

        // If we're a new attractor, assume that obj->emits_gravity hasn't been changed out of band
        // And just push it onto the list
        if (newval && !obj->emits_gravity)
        {
            attractors.push_back(obj);
        }
        else if (!newval && obj->emits_gravity)
        {
            for (size_t i = 0; i < attractors.size(); i++)
            {
                if (attractors[i]->phys_id == obj->phys_id)
                {
                    attractors.erase(attractors.begin() + i);
                    break;
                }
            }
        }
    }
    else
    {
        // Here we have no idea what oldval was, so we need to iterate every time.
        bool in = false;
        for (size_t i = 0; i < attractors.size(); i++)
        {
            if (attractors[i]->phys_id == obj->phys_id)
            {
                if (!obj->emits_gravity)
                {
                    attractors.erase(attractors.begin() + i);
                }
                in = true;
                break;
            }

            // If we didn't find it, and we want it in there, push it back.
            if (!in && obj->emits_gravity)
            {
                attractors.push_back(obj);
            }
        }
    }
}
