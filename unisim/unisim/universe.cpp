#include "universe.hpp"

#include <stdio.h>

#ifdef CPP11THREADS
#include <chrono>
#define LOCK(l) l.lock()
#define UNLOCK(l) l.unlock()
#else
#include <sys/timeb.h>
#include <unistd.h>
#define LOCK(l) pthread_rwlock_wrlock(&l)
#define UNLOCK(l) pthread_rwlock_unlock(&l)
#endif

#define COLLISION_ENERGY_CUTOFF 1e-9
#define ABSOLUTE_MIN_FRAMETIME 0.001
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef struct Beam B;
typedef PhysicsObjectType POT;
typedef struct PhysicsObject PO;
typedef struct SmartPhysicsObject SPO;
typedef struct Vector3 V3;

void gravity(V3* out, PO* big, PO* small)
{
    double m = 6.67384e-11 * big->mass * small->mass / Vector3_distance2(&big->position, &small->position);
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
Universe::Universe(double min_frametime, double max_frametime, double min_vis_frametime, int32_t port, int32_t num_threads, double rate, bool realtime)
{
    this->rate = rate;

    if (realtime && (min_frametime < ABSOLUTE_MIN_FRAMETIME))
    {
        fprintf(stderr, "WARNING: min_framtime set below absolute minimum. Raising it to %g s.\n", ABSOLUTE_MIN_FRAMETIME);
        min_frametime = ABSOLUTE_MIN_FRAMETIME;
        max_frametime = MAX(max_frametime, ABSOLUTE_MIN_FRAMETIME);
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

    total_time = 0.0;
    num_ticks = 0;
    total_objs = 1;

    net = new MIMOServer(Universe_handle_message, this, Universe_hangup_objects, this, port);
}

Universe::~Universe()
{
    stop_net();
    stop_sim();
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
#ifdef CPP11THREADS
        sim_thread = std::thread(sim, this);
#else
        pthread_create(&sim_thread, NULL, sim, (void*)this);
#endif
    }
}

void Universe::pause_sim()
{
    paused = true;
}

void Universe::stop_sim()
{
    running = false;

#ifdef CPP11THREADS
    if (sim_thread.joinable())
    {
        sim_thread.join();
    }
#else
    pthread_join(sim_thread, NULL);
#endif
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

uint64_t Universe::get_id()
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

void Universe::expire(uint64_t phys_id)
{
    LOCK(expire_lock);
    expired.push_back(phys_id);
    UNLOCK(expire_lock);
}

void Universe::hangup_objects(int32_t c)
{
    LOCK(expire_lock);
    std::map<uint64_t, struct SmartPhysicsObject*>::iterator it;

    for (it = smarties.begin() ; it != smarties.end() ; ++it)
    {
        if (it->second->client == c)
        {
            expired.push_back(it->first);
        }
    }
    UNLOCK(expire_lock);
}

void Universe::handle_message(int32_t c)
{
    throw "Universe::handle_message";
}

void Universe::get_grav_pull(V3* g, PO* obj)
{
    V3 cg;
    for (uint32_t i = 0 ; i < attractors.size() ; i++)
    {
        gravity(&cg, attractors[i], obj);
        Vector3_add(g, &cg);
    }
}


void Universe::tick(double dt)
{
    LOCK(phys_lock);

    // Only the visdata thread conflicts with this...
    // Are we OK with it getting data that is in the middle of being updated to?
    // This can be done single-threaded for now, and we'll thread it later.

    /// @todo Allocate the result on the stack here and pass in a pointer.
    /// @todo Multi-level collisoin detecitons in the tick.
    /// @todo Have some concept of collision destruction criteria here.
    struct PhysCollisionResult phys_result;
    struct BeamCollisionResult beam_result;
    for (uint32_t i = 0 ; i < phys_objects.size() ; i++)
    {
        for (uint32_t j = i + 1 ; j < phys_objects.size() ; j++)
        {
            // Return from a phys-phys collision is
            //    [t, [e1, e2], [obj1  collision data], [obj2 collision data]]
            // t is in [0,1] and indicates when in the interval the collision happened
            // energy is obvious.
            // d1 is the direction obj2 was travelling relative to obj1 whe they collided
            // p1 is a vector from obj1's position to the collision sport on its bounding ball.
            PhysicsObject_collide(&phys_result, phys_objects[i], phys_objects[j], dt);

            if (phys_result.t >= 0.0)
            {
                // By comparing the energy to a cutoff, this will help prevent
                // spurious collision notifications due to physics time step
                // increments and temporary object intersection.

                // Total energy involved. Both objects 'absorb' the samea amount
                // of energy from an 'impact effect' perspective.
                double e = phys_result.e1 + phys_result.e2;

                if ((e < -COLLISION_ENERGY_CUTOFF) || 
                    (e > COLLISION_ENERGY_CUTOFF))
                {
                    /// @todo Messaging in the tick is going to be back for performance.
#if _WIN64 || __x86_64__
                    fprintf(stderr, "Collision: %lu <-> %lu (%.15g J)\n", phys_objects[i]->phys_id, phys_objects[j]->phys_id, e);
#else
                    fprintf(stderr, "Collision: %llu <-> %llu (%.15g J)\n", phys_objects[i]->phys_id, phys_objects[j]->phys_id, e);
#endif
                    PhysicsObject_collision(phys_objects[i], phys_objects[j], e, &phys_result.pce1);
                    PhysicsObject_collision(phys_objects[j], phys_objects[i], e, &phys_result.pce2);

                    if (phys_objects[i]->type == PHYSOBJECT_SMART)
                    {
                        //SPO* s = (SPO*)phys_objects[i];
                        /// @todo Smart phys collision messages
                    }

                    if (phys_objects[j]->type == PHYSOBJECT_SMART)
                    {
                        //SPO* s = (SPO*)phys_objects[j];
                        /// @todo Smart phys collision (other) messages
                    }
                }
            }
        }

        for (uint32_t bi = 0 ; bi < beams.size() ; bi++)
        {
            Beam_collide(&beam_result, beams[bi], phys_objects[i], dt);

            if (beam_result.t >= 0.0)
            {
                phys_result.pce1.d = beam_result.d;
                phys_result.pce1.p = beam_result.p;

#if _WIN64 || __x86_64__
                fprintf(stderr, "Beam Collision: %lu -> %lu (%.15g J)\n", beams[bi]->phys_id, phys_objects[i]->phys_id, beam_result.e);
#else
                fprintf(stderr, "Beam Collision: %llu -> %llu (%.15g J)\n", beams[bi]->phys_id, phys_objects[i]->phys_id, beam_result.e);
#endif
                PhysicsObject_collision(phys_objects[i], (PO*)beams[bi], beam_result.e, &phys_result.pce1);
                /// @todo Smarty beam collision messages
                /// @todo Beam collision messages
            }
        }
    }

    // Now tick along each object
    V3 g;
    for (uint32_t i = 0 ; i < phys_objects.size() ; i++)
    {
        g.x = 0.0;
        g.y = 0.0;
        g.z = 0.0;

        get_grav_pull(&g, phys_objects[i]);
        PhysicsObject_tick(phys_objects[i], &g, dt);
    }

    // Now tick along each beam while we're here because we won't be
    // needing to reference them afer this.
    for (uint32_t bi = 0 ; bi < beams.size() ; bi++)
    {
        Beam_tick(beams[bi], dt);
    }

    LOCK(expire_lock);
    // Handle expiry queue
    // First sort the expiry queue, which is just a vector of phys_ids
    // Then we can binary search our way as we iterate over the list of phys IDs.
    // That might have a big constant though
    /// @todo Examine the runtime behaviour here, and maybe optimize out some of the linear searches.
    bool backtrack = false;
    for (uint32_t i = 0 ; i < phys_objects.size() ; i++)
    {
        if (backtrack)
        {
            i--;
            backtrack = false;
        }

        for (uint32_t j = 0 ; j < expired.size() ; j++)
        {
            if (expired[j] == phys_objects[i]->phys_id)
            {
                // If it's a beam...
                if (phys_objects[i]->type >= BEAM_COMM)
                {
                    for (uint32_t k = 0 ; k < beams.size() ; k++)
                    {
                        if (beams[k]->phys_id == expired[j])
                        {
                            beams.erase(beams.begin()+k);
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
                        for (uint32_t k = 0 ; k < attractors.size() ; k++)
                        {
                            if (attractors[k]->phys_id == expired[j])
                            {
                                attractors.erase(attractors.begin()+k);
                                break;
                            }
                        }
                    }

                    phys_objects.erase(phys_objects.begin()+i);
                    backtrack = true;
                }
                continue;
            }
        }
    }
    expired.clear();
    UNLOCK(expire_lock);

    LOCK(add_lock);
    // Handle added queue
    for (uint32_t i = 0 ; i < added.size() ; i++)
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
                break;
            }
        case PHYSOBJECT_SMART:
            {
                SPO* obj = (SPO*)added[i];
                /// @todo Check to make sure this smarty adding is right.
                smarties[obj->pobj.phys_id] = obj;
                phys_objects.push_back(added[i]);
                if (added[i]->emits_gravity)
                {
                    attractors.push_back(added[i]);
                }
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
            for (uint32_t i = 0 ; i < attractors.size() ; i++)
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
        for (uint32_t i = 0 ; i < attractors.size() ; i++)
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
