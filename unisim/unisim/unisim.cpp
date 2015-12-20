#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <signal.h>

#ifdef CPP11THREADS
#include <chrono>
#else
#include <unistd.h>
#endif

#include "universe.hpp"
#include "MIMOServer.hpp"

bool running = true;
Universe* u;
std::vector<struct PhysicsObject*> objs;
std::vector<struct Beam*> beams;

void sighandler(int32_t sig)
{
	fprintf(stderr, "Caught Ctrl+C, stopping.\n");
	running = false;
	u->stop_net();
	u->stop_sim();
}

int32_t compare(const void* aV, const void* bV)
{
	uint64_t a = *(uint64_t*)aV;
	uint64_t b = *(uint64_t*)bV;
	return ((a < b) ? -1 : ((a == b) ? 0 : 1));
}

void pool_rack()
{
	double ball_mass = 0.15;
	double cue_ball_mass = 0.26;
	double ball_radius = 0.056896;
	double cue_ball_radius = 1.1 * ball_radius;

	int num_rows = 5;

	// This loop produces a trangle of balls that points down the negative y axis.
	// That is, the 'head' ball is further negative than the 'back' of the rack.
	//
	// Mathematica code.
	// numRows = 5;
	// radius = 1;
	// balls = Reap[For[i = 0, i < numRows, i++,
	//    For[j = 0, j <= i, j++,
	//    x = (i - 2 j) radius;
	//    y = Sqrt[3]/2 (1 + 2 i) radius;
	//    Sow[Circle[{x, y}, radius]]
	//    ]
	//    ]][[2]][[1]];
	// Graphics[balls]

	struct PhysicsObject* obj;
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 0.0, 0.0, 0.0 };
	struct Vector3 velocity = { 0.0, -25, 0.0 };

	double C = 1;
	double y_scale = sqrt(3) / 2;
	double y_offset = 100;

	for (int i = 0; i < num_rows; i++)
	{
		for (int j = 0; j < i + 1; j++)
		{
			obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
			objs.push_back(obj);

			position.x = C * (i - 2 * j) * ball_radius;
			position.y = y_offset - C * y_scale * (1 + 2 * i) * ball_radius;

			PhysicsObject_init(obj, u, &position, &vector3_zero, &vector3_zero, &vector3_zero, ball_mass, ball_radius, NULL);
			u->add_object(obj);
		}
	}

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	position.x = 0.0;
	position.y = 10.0;
	position.z = 0.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, cue_ball_mass, cue_ball_radius, NULL);
	u->add_object(obj);
	objs.push_back(obj);
}

void simple_collision()
{
	struct PhysicsObject* obj;
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 0.0, 0.0, 0.0 };
	struct Vector3 velocity = { 0.0, 0.0, 0.0 };

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 0.0;
	position.y = 0.0;
	velocity.x = -5.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
	u->add_object(obj);

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = -20;
	position.y = 0.0;
	velocity.x = -1.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
	u->add_object(obj);
}

void fast_collision()
{
	struct PhysicsObject* obj;
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 0.0, 0.0, 0.0 };
	struct Vector3 velocity = { 0.0, 0.0, 0.0 };
	double mass = 1.0;

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 0.0;
	position.y = 0.0;
	velocity.x = -50000.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, 1, NULL);
	obj->health = 1e10;
	u->add_object(obj);

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = -20;
	position.y = 0.0;
	velocity.x = 0.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
	obj->health = 1e10;
	u->add_object(obj);
}

void beam_collision()
{
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 10.0, 0.0, 0.0 };
	struct Vector3 velocity = { -1.0, 0.0, 0.0 };
	struct Vector3 up = { 0.0, 0.0, 1.0 };
	struct Beam* beam;

	beam = (struct Beam*)malloc(sizeof(struct Beam));
	beams.push_back(beam);
	Beam_init(beam, u, &position, &velocity, &up, 1, 1, 1000, BEAM_WEAP);
	u->add_object((struct PhysicsObject*)beam);

	double mass = 1.0;
	struct PhysicsObject* obj;

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 0.0;
	position.y = 0.0;
	velocity.x = 0.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, 1, NULL);
	obj->health = 1e10;
	u->add_object(obj);

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = -20;
	position.y = 0.0;
	velocity.x = 0.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, 1, 1, NULL);
	obj->health = 1e10;
	u->add_object(obj);
}

void beam_multi_collision()
{
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 10.0, 0.0, 0.0 };
	struct Vector3 velocity = { -4000.0, 0.0, 0.0 };
	struct Vector3 up = { 0.0, 0.0, 1.0 };
	struct Beam* beam;

	beam = (struct Beam*)malloc(sizeof(struct Beam));
	beams.push_back(beam);
	Beam_init(beam, u, &position, &velocity, &up, 1, 1, 1000, BEAM_WEAP);
	u->add_object((struct PhysicsObject*)beam);

	double mass = 1.0;
	double radius = 1.0;
	struct PhysicsObject* obj;

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 0.0;
	position.y = 0.0;
	position.z = 0.0;
	velocity.x = 0.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
	obj->health = 1e10;
	u->add_object(obj);

	for (int i = 0; i < 5; i++)
	{
		obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
		objs.push_back(obj);
		position.x = -0.20 * i;
		position.y = 10 * i;
		position.z = 1.0 + 2 * i * radius;
		velocity.x = 0.0;
		PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
		obj->health = 1e10;
		u->add_object(obj);

		obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
		objs.push_back(obj);
		position.x = 0.20 * i;
		position.y = i * i;
		position.z = -1.0 - 2 * i * radius;
		velocity.x = 0.0;
		PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
		obj->health = 1e10;
		u->add_object(obj);
	}
}

void shifting()
{
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 10.0, 0.0, 0.0 };
	struct Vector3 velocity = { -4000.0, 0.0, 0.0 };

	int num_per_row = 100; // 2x+1 = actual number per row
	int num_rows = 200;

	double mass = 1.0;
	double radius = 1.0;
	double spacingX = 0.0;
	double spacingY = 0.0;
	struct PhysicsObject* obj;

	for (int i = 0; i < num_rows; i++)
	{
		obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
		objs.push_back(obj);
		position.x = 0;
		position.y = i * (2 * radius + spacingY);
		velocity.x = (2 * (i % 2) - 1) * 1.0;
		PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
		obj->health = 1e10;
		u->add_object(obj);

		for (int j = 1; j <= num_per_row; j++)
		{
			obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
			objs.push_back(obj);
			position.x = -1 * j * (2 * radius + spacingX);
			position.y = i * (2 * radius + spacingY);
			velocity.x = (2 * (i % 2) - 1) * 1.0;
			PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
			obj->health = 1e10;
			u->add_object(obj);

			obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
			objs.push_back(obj);
			position.x = j * (2 * radius + spacingX);
			position.y = i * (2 * radius + spacingY);
			velocity.x = (2 * (i % 2) - 1) * 1.0;
			PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
			obj->health = 1e10;
			u->add_object(obj);
		}
	}
}

void collision_exit()
{
	struct Vector3 vector3_zero = { 0.0, 0.0, 0.0 };
	struct Vector3 position = { 0.0, 0.0, 0.0 };
	struct Vector3 velocity = { 0.0, 0.0, 0.0 };

	double mass = 1.0;
	double radius = 1.0;
	struct PhysicsObject* obj;

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 0.0;
	position.y = 0.0;
	velocity.x = 1.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, 10 * radius, NULL);
	obj->health = 1e10;
	u->add_object(obj);

	obj = (struct PhysicsObject*)malloc(sizeof(struct PhysicsObject));
	objs.push_back(obj);
	position.x = 1.0;
	position.y = 0.0;
	velocity.x = 3.0;
	PhysicsObject_init(obj, u, &position, &velocity, &vector3_zero, &vector3_zero, mass, radius, NULL);
	obj->health = 1e10;
	u->add_object(obj);
}

void print_positions()
{
	for (size_t i = 0; i < objs.size(); i++)
	{
#if _WIN64 || __x86_64__
		fprintf(stderr, "PO%lu   %g   %g   %g\n", objs[i]->phys_id, objs[i]->position.x, objs[i]->position.y, objs[i]->position.z);
#else
		fprintf(stderr, "PO%llu   %g   %g   %g\n", objs[i]->phys_id, objs[i]->position.x, objs[i]->position.y, objs[i]->position.z);
#endif
	}

	for (size_t i = 0; i < beams.size(); i++)
	{
#if _WIN64 || __x86_64__
		fprintf(stderr, "BM%lu   %g   %g   %g\n", objs[i]->phys_id, beams[i]->front_position.x, beams[i]->front_position.y, beams[i]->front_position.z);
#else
		fprintf(stderr, "BM%llu   %g   %g   %g\n", objs[i]->phys_id, beams[i]->front_position.x, beams[i]->front_position.y, beams[i]->front_position.z);
#endif
	}
}

void check_packing()
{
	struct PhysicsObject p;
	printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
		(uint64_t)&p.type - (uint64_t)&p,
		(uint64_t)&p.phys_id - (uint64_t)&p,
		(uint64_t)&p.universe - (uint64_t)&p,
		(uint64_t)&p.box - (uint64_t)&p,
		(uint64_t)&p.t - (uint64_t)&p,
		(uint64_t)&p.position - (uint64_t)&p,
		(uint64_t)&p.velocity - (uint64_t)&p,
		(uint64_t)&p.ang_velocity - (uint64_t)&p,
		(uint64_t)&p.thrust - (uint64_t)&p,
		(uint64_t)&p.forward - (uint64_t)&p,
		(uint64_t)&p.up - (uint64_t)&p,
		(uint64_t)&p.right - (uint64_t)&p,
		(uint64_t)&p.mass - (uint64_t)&p,
		(uint64_t)&p.radius - (uint64_t)&p,
		(uint64_t)&p.health - (uint64_t)&p,
		(uint64_t)&p.obj_desc - (uint64_t)&p,
		(uint64_t)&p.art_id - (uint64_t)&p,
		(uint64_t)&p.emits_gravity - (uint64_t)&p);

	struct SmartPhysicsObject s;
	printf("%lu %lu\n",
		(uint64_t)&s.pobj - (uint64_t)&s,
		(uint64_t)&s.client - (uint64_t)&s);

	struct Beam b;
	printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
		(uint64_t)&b.type - (uint64_t)&b,
		(uint64_t)&b.phys_id - (uint64_t)&b,
		(uint64_t)&b.universe - (uint64_t)&b,
		(uint64_t)&b.scan_target - (uint64_t)&b,
		(uint64_t)&b.origin - (uint64_t)&b,
		(uint64_t)&b.direction - (uint64_t)&b,
		(uint64_t)&b.up - (uint64_t)&b,
		(uint64_t)&b.right - (uint64_t)&b,
		(uint64_t)&b.cosines - (uint64_t)&b,
		(uint64_t)&b.speed - (uint64_t)&b,
		(uint64_t)&b.area_factor - (uint64_t)&b,
		(uint64_t)&b.energy - (uint64_t)&b,
		(uint64_t)&b.distance_travelled - (uint64_t)&b,
		(uint64_t)&b.max_distance - (uint64_t)&b);
}

int main(int32_t argc, char** argv)
{
	signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	//u = new Universe(0.001, 0.05, 0.5, 5505, 3);
	u = new Universe(1e-6, 1e-6, 0.5, 5505, 4, 1.0, false);

	try
	{
		//pool_rack();
		//simple_collision();
		//fast_collision();
		shifting();
		//collision_exit();

		//beam_collision();
		//beam_multi_collision();

		//print_positions();

		u->start_net();
		u->start_sim();

		double frametimes[4];
		uint64_t last_ticks = u->get_ticks();
		uint64_t cur_ticks;

#ifdef CPP11THREADS
		std::chrono::seconds dura(1);
#endif

        fprintf(stderr, "Physics Framtime, Wall Framtime, Game Frametime, Vis Frametime, Total Sim Time, NTicks\n");
		while (running)
		{
			u->get_frametime(frametimes);
			cur_ticks = u->get_ticks();
#if _WIN64 || __x86_64__
			fprintf(stderr, "%g, %g, %g, %g, %g, %lu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#else
			fprintf(stderr, "%g, %g, %g, %g, %g, %llu\n", frametimes[0], frametimes[1], frametimes[2], frametimes[3], u->total_sim_time(), cur_ticks - last_ticks);
#endif
			if (objs.size() < 10)
			{
				print_positions();
			}
			last_ticks = cur_ticks;

#ifdef CPP11THREADS
			std::this_thread::sleep_for(dura);
#else
			usleep(1000000);
#endif
		}

		u->stop_net();
		u->stop_sim();
		delete u;

		return 0;
	}
	catch (char* e)
	{
		fprintf(stderr, "Caught error: %s\n", e);
		return 1;
	}
}
