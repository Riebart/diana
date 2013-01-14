#!/usr/bin/env python

import sys
import socket
from vector import Vector3
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer
from message import Message
from message import HelloMsg, GoodbyeMsg, PhysicalPropertiesMsg, VisualPropertiesMsg, CollisionMsg, ScanResultMsg
from message import VisualDataEnableMsg, VisualMetaDataEnableMsg, ScanQueryMsg
from message import BeamMsg

# ### TODO ### implement grids, this ties into physics, but the properties
#  need to be added here.
class PhysicsObject:
    @staticmethod
    def is_big_enough(mass, min_distance):
        if mass == None or min_distance == None:
            return 0

        f = 6.67384e-11 * mass / min_distance

        # ### PARAMETER ### GRAVITY CUTOFF
        if f >= 0.01:
            return 1
        else:
            return 0

    def __init__(self, universe,
                    position = None,
                    velocity = None,
                    orientation = None,
                    mass = None,
                    radius = None,
                    thrust = None,
                    object_type = "Unknown"):
        self.universe = universe
        self.phys_id = universe.get_id()
        self.position = Vector3(position) if position else None
        self.velocity = Vector3(velocity) if velocity else None
        self.orientation = Vector3(orientation) if orientation else None
        self.mass = mass
        self.radius = radius
        self.thrust = Vector3(thrust) if thrust else None
        self.object_type = object_type
        self.art_id = None
        self.health = self.mass * 1000000 if self.mass != None else None

        if self.mass and self.radius:
            self.emits_gravity = PhysicsObject.is_big_enough(self.mass, self.radius)
        else:
            self.emits_gravity = 0

    def handle(self, msg):
        pass

    def tick(self, acc, dt):
        # ### TODO ### Relativistic mass

        # Verlet integration: http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
        self.position.x += self.velocity.x * dt + 0.5 * dt * dt * acc.x
        self.position.y += self.velocity.y * dt + 0.5 * dt * dt * acc.y
        self.position.z += self.velocity.z * dt + 0.5 * dt * dt * acc.z

        self.velocity.x += dt * acc.x
        self.velocity.y += dt * acc.y
        self.velocity.z += dt * acc.z

    @staticmethod
    def collide(obj1, obj2, dt):
        # Currently we treat dt and forces as small enough that they can be neglected,
        # and assume that they won't change teh trajectory appreciably over the course
        # of dt

        # What we do is get the line segments that each object will trace outside
        # if they followed their current velocity for dt seconds.

        # We then find the minimum distance between those two line segments, and
        # compare that with the sum of the radii (the minimum distance the objects
        # need to be away from each other in order to not collide). Becuse sqrt()
        # is so expensive, we compare the square of everything. Any lengths are
        # actually the squares of the length, which means that we need to compare
        # against the square of the sum of the radii of the objects.

        # Because the parameters for the parameterizations of the line segments
        # will be in [0,1], we'll need to scale the velocity by dt. If we
        # get around to handling force application we need to scale the acceleration
        # vector by 0.5*dt^2, and then apply a parameter of t^2 to that term.

        # Parameters for force-less trajectories are found for object 1 and 2 (t)
        # thanks to some differentiation of parameterizations of the trajectories
        # using a parameter for time. Note that this will find the global minimum
        # and the parameters that come back likely won't be in the [0,1] range.

        # Normally if the parameter is outside of the range [0,1], you should check
        # the endpoints for the minimum. If the parameter is in [0,1], then you
        # can skip endpoing checking, yay! In this case though, we know that if
        # there is an inflection point in [0,1], t will point at it. If there isn't,
        # then the distance function is otherwise monotonic (it is quadratic in t)
        # and we can just clip t to [0,1] and get the minimum.

        #      (v2-v1).(o1-o2)
        # t = -----------------
        #      (v2-v1).(v2-v1)

        # So, here we go!

        v1 = obj1.velocity.clone()
        v1.scale(dt)

        v2 = obj2.velocity.clone()
        v2.scale(dt)

        vd = v2 - v1

        if vd.almost_zero():
            return -1

        o1 = obj1.position.clone()
        o2 = obj2.position.clone()

        t = vd.dot(o1 - o2) / vd.length2()

        t = min(1, max(0, t))

        o1.add(v1, t)
        o2.add(v2, t)

        r = (obj1.radius + obj2.radius)
        r *= r

        if o1.dist2(o2) <= r:
            # ### TODO ### Relativistic kinetic energy
            # ### TODO ### Angular velocity, which reqires location.

            # collision normal
            n = obj2.position - obj1.position
            n.normalize()

            # Collision tangential velocities. These parts don't change in the collision
            t1 = Vector3.combine([[1, obj1.velocity], [- obj1.velocity.dot(n), n]])
            t1.normalize()
            t1.scale(obj1.velocity.dot(t1))

            t2 = Vector3.combine([[1, obj2.velocity], [- obj2.velocity.dot(n), n]])
            t2.normalize()
            t2.scale(obj2.velocity.dot(t2))

            # Now we need the amount of energy transferred in each direction along the normal
            if obj1.mass == 0:
                n1 = None
                e1 = 0
            else:
                n1 = n.clone()
                e1 = obj2.velocity.dot(n)
                n1.scale(e1 * obj2.mass / obj1.mass)
                e1 = e1 * e1 * 0.5 * obj2.mass

            if obj2.mass == 0:
                vn2 = None
                vn2l = 0
            else:
                n2 = n.clone()
                e2 = obj1.velocity.dot(n)
                n2.scale(e2 * obj1.mass / obj2.mass)
                e2 = e2 * e2 * 0.5 * obj1.mass

            v = obj2.velocity - obj1.velocity
            d1 = v.unit()
            d2 = d1.clone()
            d2.scale(-1)

            p1 = o2 - o1
            p1.normalize()
            p2 = p1.clone()
            p1.scale(obj1.radius)
            p2.scale(-1 * obj2.radius)

            return [t, [e1, e2], [d1, p1, t1, n1], [d2, p2, t2, n2]]
        else:
            return -1

    def resolve_damage(self, energy):
        # We'll take damage if we absorb an impact whose energy is above ten percent
        # of our current health, and only by the energy that is above that threshold

        if self.health == None:
            return

        t = 0.1 * self.health
        if energy > t:
            self.health -= (energy - t)
        
        if self.health <= 0:
            self.universe.expired.append(self)

    def resolve_phys_collision(self, energy, args):
        self.velocity = args[2] + args[3]
        print "Setting velocity of", self.phys_id, "to", self.velocity

    def collision(self, obj, energy, args):
        if isinstance(obj, PhysicsObject):
            self.resolve_phys_collision(energy, args)
            self.resolve_damage(energy)
        elif isinstance(obj, Beam):
            if obj.beam_type == "WEAP":
                self.resolve_damage(energy)
            elif obj.beam_type == "SCAN":
                result_beam = obj.make_return_beam(energy, p)
                result_beam.beam_type = "SCANRESULT"
                result_beam.scan_target = self
                self.universe.add_beam(result_beam)

class SmartPhysicsObject(PhysicsObject):
    def __init__(self, universe, client, osim_id,
                    position = None,
                    velocity = None,
                    orientation = None,
                    mass = None,
                    radius = None,
                    thrust = None,
                    object_type = "Unknown"):
        PhysicsObject.__init__(self, universe, position, velocity, orientation, mass, radius, thrust, object_type)
        self.client = client
        self.osim_id = osim_id
        self.vis_data = 0
        self.vis_meta_data = 0
        self.exists = 0

    def handle(self, msg):
        if isinstance(msg, HelloMsg):
            if msg.endpoint_id == None:
                return

            self.sim_id = msg.endpoint_id
            HelloMsg.send(self.client, self.phys_id, self.osim_id, self.phys_id)

        # If we don't have a sim_id by this point, we can't accept any of the
        # following in good conscience...
        if self.sim_id == None:
            return

        if isinstance(msg, GoodbyeMsg):
            # ### TODO ### In any real language, we'll need to figure out who
            # still is keeping track of this object if the universe isn't...
            self.universe.expired.append(self)
            
        elif isinstance(msg, PhysicalPropertiesMsg):
            if msg.object_type != None:
                self.object_type = msg.object_type

            if msg.mass != None:
                self.mass = msg.mass

            if msg.position != None:
                self.position = Vector3(msg.position)

            if msg.velocity != None:
                self.velocity = Vector3(msg.velocity)

            if msg.orientation != None:
                self.orientation = Vector3(msg.orientation)

            if msg.thrust != None:
                self.thrust = Vector3(msg.thrust)

            if msg.radius != None:
                self.radius = msg.radius

            if self.exists == 1 and self.mass != None and self.radius != None:
                new_emits = PhysicsObject.is_big_enough(self.mass, self.radius)
                diff = self.emits_gravity - new_emits
                if diff != 0:
                    self.universe.update_attractor(self)

            if (self.exists == 0 and self.object_type != None and self.mass != None and
                self.radius != None and self.position != None and 
                self.velocity != None and self.orientation != None and self.thrust != None):
                self.universe.add_object(self)
                self.exists = 1

        elif isinstance(msg, VisualPropertiesMsg):
            if self.art_id == None:
                self.art_id = self.universe.curator.register_art(msg.mesh, msg.texture)
            else:
                self.universe.curator.update_art(self.art_id, msg.mesh, msg.texture)
                self.universe.curator.attach_art_asset(self.art_id, self.phys_id)

        elif isinstance(msg, VisualDataEnableMsg):
            changed = msg.enabled - self.vis_data
            self.vis_data = msg.enabled

            if changed != 0:
                self.universe.register_for_vis_data(self, self.vis_data)

        elif isinstance(msg, VisualMetaDataEnableMsg):
            changed = msg.enabled - self.vis_meta_data
            self.vis_meta_data = msg.enabled

            if changed != 0:
                self.universe.curator.register_client(self, self.vis_meta_data)

        elif isinstance(msg, BeamMsg):
            msg.origin[0] += self.position.x
            msg.origin[1] += self.position.y
            msg.origin[2] += self.position.z

            # ### TODO ### Relativistic velocity composition?
            msg.velocity[0] += self.velocity.x
            msg.velocity[0] += self.velocity.y
            msg.velocity[0] += self.velocity.z
            
            beam = Beam.build(msg, self.universe)

            if msg.beam_type == "COMM":
                beam.message = msg.msg

            self.universe.add_beam(beam)

    # I was in a collision with the other object, with a certain amount of energy,
    # the other object came from d and hit me at p
    def collision(self, obj, energy, args):
        d = args[0]
        p = args[1]
        if isinstance(obj, PhysicsObject):
            CollisionMsg.send(self.client, self.phys_id, self.osim_id, [p.x, p.y, p.z, d.x, d.y, d.x, energy, "PHYS"])
            self.resolve_phys_collision(energy, args)
        elif isinstance(obj, Beam):
            if obj.beam_type == "SCAN":
                print "IN SMARTY", self.object_type
                query_id = self.universe.queries.add([obj, energy, self])
                ScanQueryMsg.send(self.client, self.phys_id, self.osim_id, [ query_id, energy, d.x, d.y, d.z ])
            elif obj.beam_type == "SCANRESULT":
                ScanResultMsg.send(self.client, self.phys_id, self.osim_id, PhysicalPropertiesMsg.make_from_object(obj.scan_target, self.position, self.velocity))
            elif obj.beam_type == "COMM":
                CollisionMsg.send(self.client, self.phys_id, self.osim_id, [p.x, p.y, p.z, d.x, d.y, d.x, energy, obj.beam_type] + obj.message)
            else:
                CollisionMsg.send(self.client, self.phys_id, self.osim_id, [p.x, p.y, p.z, d.x, d.y, d.x, energy, obj.beam_type])

class Beam:
    solid_angle_factor = 2 / pi
    
    def __init__(self, universe,
                    origin, direction,
                    up, right, cosines, area_factor, velocity,
                    energy, beam_type):
        self.universe = universe
        self.origin = origin if isinstance(origin, Vector3) else Vector3(origin)
        self.front_position = self.origin.clone()
        self.direction = direction if isinstance(direction, Vector3) else Vector3(direction)
        self.up = up if isinstance(up, Vector3) else Vector3(up)
        self.right = right if isinstance(right, Vector3) else Vector3(right)
        self.velocity = velocity if isinstance(velocity, Vector3) else Vector3(velocity)
        self.cosines = cosines # Horizontal, then vertical
        self.area_factor = area_factor # Multiply by distance_travelled^2, and that is the area of the wave_front
        self.energy = energy
        self.beam_type = beam_type

        self.speed = self.velocity.length()
        self.distance_travelled = 0

        # We'll use a threshold energy coming back as 10^-10, or a tenth of a nanoJoule
        self.max_distance = sqrt(self.energy / (self.area_factor * Beam.solid_angle_factor * 10E1))

    @staticmethod
    def collide(b, obj, dt):
        # ### TODO ### Take radius into account

        # Move the object position to a point relative to the beam's origin.
        # Then scale the velocity by dt, and add it to the position to get the
        # start, end, and difference vectors.

        ps = obj.position.clone()
        ps.sub(b.origin)

        v = obj.velocity.clone()
        v.scale(dt)

        pe = Vector3.combine([[1, ps], [1, v]])

        proj_s = [ ps.project_down_n(b.up), ps.project_down_n(b.right) ]
        proj_s[0].normalize()
        proj_s[1].normalize()
        
        proj_e = [ pe.project_down_n(b.up), pe.project_down_n(b.right) ]
        proj_e[0].normalize()
        proj_e[1].normalize()

        # Project the position down the up vector and dot with the direction for
        # the cosine fo the horizontal angle, and project it down the right and dot
        # with direction to get the vertical angle.

        # Note that cosine decreses from 1 to -1 as the angle goes from 0 to 180.
        # We are inside, if we are > than the cosine of our beam.
        
        current = [ proj_s[0].dot(b.direction), proj_s[1].dot(b.direction) ]
        future = [ proj_s[0].dot(b.direction), proj_s[1].dot(b.direction) ]
        delta = [ future[0] - current[0], future[1] - current[1] ]

        current_b = [ int(current[0] >= b.cosines[0]), int(current[1] >= b.cosines[1]) ]
        future_b = [ int(future[0] >= b.cosines[0]), int(future[1] >= b.cosines[1]) ]
        
        entering = 0.0
        leaving = 1.0

        for i in [0,1]:
            if current_b[i] == 0:
                if future_b[i] == 1:
                    # if we're out, and coming in
                    entering = max(entering, (b.cosines[i] - current[i]) / delta[i])
                else:
                    # if we're out, and staying out
                    return -1
            elif current_b[i] == 1:
                if future_b[i] == 0:
                    # if we're in, and going out
                    leaving = min(leaving, (current[i] - b.cosines[i]) / delta[i])
                else:
                    # if we're in and staying in
                    pass

        # If we have to enter some planes, and are leaving other planes, but
        # we don't enter until after we leave, we can bail.
        if entering > leaving:
            return -1

        # ### NOTE ### I'm not sure if this hunch is right, but I think it is a
        # close enough approximation. The magic of linear situations might actually
        # make me right...
        # If there are still candidate values of t, then just take the middle of
        # the interval as the 'collision' time that maximizes the energy transfer.
        # This only might work for spheres...
        t = (entering + leaving) / 2.0

        collision_point = Vector3.combine([[1, ps], [t, v]])
        collision_dist = abs(collision_point.dot(b.direction))

        # If we want collisions for as long as the bounding sphere is intersecting
        # the front, use:
        #
        # if ((collision_dist + obj.radius) >= b.distance_travelled) and ((collision_dist - obj.radius) <= (b.distance_travelled + b.speed * dt)):
        if collision_dist >= b.distance_travelled and collision_dist <= (b.distance_travelled + b.speed * dt):
            wave_front_area = b.area_factor * collision_dist * collision_dist
            object_surface = pi * obj.radius * obj.radius

            energy_factor = 1 if (wave_front_area < object_surface) else (object_surface / wave_front_area)
            energy = b.energy * energy_factor
            
            return [t, energy, b.direction, collision_point, None]
        else:
            return -1

    def make_return_beam(self, energy, origin):
        direction = (self.origin - origin).unit()

        velocity = Vector3.combine([[self.speed, direction]])

        right = self.right.clone()
        right.scale(-1)

        return Beam(self.universe, origin, direction, self.up, self.right, self.cosines, self.area_factor, velocity, energy, None)

    def collision(self, obj, energy, position, shadow):
        # ### TODO ### Beam occlusion
        pass

    def tick(self, dt):
        self.distance_travelled += self.speed * dt
        self.front_position.add(self.velocity, dt)

        if self.distance_travelled > self.max_distance:
            self.universe.expired.append(self)

    @staticmethod
    def build(msg, universe):
        direction = Vector3(msg.velocity)
        speed = direction.length()
        direction.scale(1/speed)
        up = Vector3(msg.up)
        up.scale(up.length())
        right = direction.cross(up)

        area_factor = Beam.solid_angle_factor * (msg.spread_h * msg.spread_v)

        cosh = cos(msg.spread_h / 2)
        cosv = cos(msg.spread_v / 2)

        return Beam(universe, Vector3(msg.origin), direction, up, right, [cosh, cosv], area_factor, Vector3(msg.velocity), msg.energy, msg.beam_type)
