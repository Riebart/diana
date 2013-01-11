#!/usr/bin/env python

import sys
from math import sin, cos, pi, sqrt
from mimosrv import MIMOServer
from message import Message
from message import HelloMsg, PhysicalPropertiesMsg, VisualPropertiesMsg
from message import VisualDataEnableMsg, VisualMetaDataEnableMsg
from message import BeamMsg

class Vector3:
    def __init__(self, v, y=None, z=None):
        if y==None:
            self.x = v[0]
            self.y = v[1]
            self.z = v[2]
        else:
            self.x = v
            self.y = y
            self.z = z

    def clone(self):
        return Vector3(self.x, self.y, self.z)

    #def __init__(self, x, y, z):
        #self.x = x
        #self.y = y
        #self.z = z

    def length(self):
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return sqrt(r)

    def length2(self):
        r = self.x * self.x + self.y * self.y + self.z * self.z
        return r

    def dist(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return sqrt(x * x + y * y + z * z)

    def dist2(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        return x * x + y * y + z * z

    # Returns a unit vector that originates at v and goes to this vector.
    def ray(self, v):
        x = v.x - self.x
        y = v.y - self.y
        z = v.z - self.z
        m = sqrt(x * x + y * y + z * z)
        return Vector3([x / m, y / m, z / m])

    def unit(self):
        l = self.length()
        return Vector3([self.x / l, self.y / l, self.z / l])

    # In this case, self is the first vector in the cross product, because order
    # matters here
    def cross(self, v):
        rx = self.y * v.z - self.z * v.y
        ry = self.z * v.x - self.x * v.z
        rz = self.x * v.y - self.y * v.x
        return Vector3([rx, ry, rz])

    @staticmethod
    # Combines a linear combination of a bunch of vectors
    def combine(vecs):
        rx = 0.0
        ry = 0.0
        rz = 0.0
        
        for v in vecs:
            rx += v[0] * v[1].x
            ry += v[0] * v[1].y
            rz += v[0] * v[1].z

        return Vector3([rx, ry, rz])

    def normalize(self):
        l = self.length()
        self.x /= l
        self.y /= l
        self.z /= l

    def scale(self, c):
        self.x *= c
        self.y *= c
        self.z *= c

    # Adds v to self.
    def add(self, v, s = 1):
        self.x += s * v.x
        self.y += s * v.y
        self.z += s * v.z

    #override +
    def __add__(self, other):
        return Vector3([self.x+other.x, self.y+other.y, self.z+other.z])

    def sub(self, v):
        self.x -= v.x
        self.y -= v.y
        self.z -= v.z

    #override -
    def __sub__(self, other):
        return Vector3([self.x-other.x, self.y-other.y, self.z-other.z])

    def dot(self, v):
        return self.x * v.x + self.y * v.y + self.z * v.z

    @staticmethod
    def almost_zeroS(v):
        # Python doesn't seem to be able to distinguish exponents below -300,
        # So we'll cut off at -150
        if -1e-150 < v and v < 1e-150:
            return 1
        else:
            return 0

    def almost_zero(self):
        if Vector3.almost_zeroS(self.x) and Vector3.almost_zeroS(self.y) and Vector3.almost_zeroS(self.z):
            return 1
        else:
            return 0

    #overrid []
    def __getitem__(self, index):
        if index == 0:
            return self.x
        if index == 1:
            return self.y
        if index == 2:
            return self.z

        raise IndexError('Vector3 has only 3 dimensions')

    def __setitem__(self, index, value):
        if index == 0:
            self.x = value
        elif index == 1:
            self.y = value
        elif index == 2:
            self.z = value
        else:
            raise IndexError('Vector3 has only 3 dimensions')

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
                    position = [ 0.0, 0.0, 0 ],
                    velocity = [ 0.0, 0.0, 0 ],
                    orientation = [ 0.0, 0.0, 0.0 ],
                    mass = 10.0,
                    radius = 1.0,
                    thrust = [0.0, 0.0, 0.0],
                    object_type = "Unknown"):
        self.phys_id = universe.get_id()
        self.position = Vector3(position)
        self.velocity = Vector3(velocity)
        self.orientation = Vector3(orientation)
        self.mass = mass
        self.radius = radius
        self.thrust = Vector3(thrust)
        self.object_type = object_type
        self.art_id = None
        self.emits_gravity = PhysicsObject.is_big_enough(self.mass, self.radius)

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

        ## FYI: Mathematica comes back with about 20,000 arithmetic operations per
        ## parameter... I don't think I'm going to bother. If errors start to get
        ## too high (as in, noticed in gameplay), then forcibly slow down time to
        ## keep the physics ticks fast enough to keep collision error in check.

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

        ## Notes on shorthand here. v1 = (x, y, z), and v2 = (a, b, c). '.' means
        ## dot product, and 'x' means cross product.

        ## t = ((o1.v1-o2.v1) (v2.v2)-(o1.v2-o2.v2)(v1.v2))
        ##     --------------------------------------------
        ##              ((v1.v2)^2-(v1.v1) (v2.v2))

        ## v = (o1-o2).(c{-x z,-y z,x^2+y^2}+b{-x y,x^2+z^2,-y z}+a{y^2+z^2,-x y,-x z})
        ##     ------------------------------------------------------------------------
        ##                          (v2 x v1).(v2 x v1)

        ## I'm going to call the right-hand operand of the dot product in v's numerator 's'

        ## Some values we can precompute to make things faster (in order in the array):

        ## All together, they save a total of seven additions and eighteen multiplications,

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

        ##t = ((o1.dot(v1) - o2.dot(v1)) * pre[0] - (o1.dot(v2) - o2.dot(v2)) * pre[1]) / ((pre[1] * pre[1] - v1.length2()) * pre[0])
        ##s = Vector3([
                    ##v2.x * (v1.y * v1.y + v1.z * v1.z) - (
                        ##v2.y * v1.x * v1.y +
                        ##v2.z * v1.x * v1.z),
                    ##v2.y * (v1.x * v1.x + v1.z * v1.z) - (
                        ##v2.x * v1.x * v1.y +
                        ##v2.z * v1.y * v1.z),
                    ##v2.z * (v1.x * v1.x + v1.y * v1.y) - (
                        ##v2.x * v1.x * v1.z +
                        ##v2.y * v1.y * v1.z)
                    ##])
        ##v = (o1 - o2).dot(s) / pre[2]

        t = min(1, max(0, t))

        o1.add(v1, t)
        o2.add(v2, t)

        r = (obj1.radius + obj2.radius)
        r *= r
        
        if o1.dist2(o2) <= r:
            # ### TODO ### Relativistic kinetic energy
            v = (o1.velocity - o2.velocity).length2()
            e1 = 0.5 * o2.mass * v
            e2 = 0.5 * o1.mass * v

            d1 = o2.velocity.unit()
            d2 = o1.velocity.unit()

            p1 = o2 - o1
            p2 = p1.clone()
            p2.scale(-1)

            return [t, [e1, e2], [d1, d2], [p1, p2]]
        else:
            return -1

    def resolve_damage(self, energy):
        pass

    def resolve_phys_collision(self, energy, direction, location):
        # ### TODO ### Angular velocity
        pass

    def collision(self, obj, energy, d, p):
        if isinstance(obj, PhysicsObject):
            self.resolve_phys_collision(energy, d, p)
            self.resolve_damage(energy)
        elif isinstance(obj, Beam):
            if obj.beam_type == "WEAP":
                self.resolve_damage(energy)
            elif obj.beam_type == "SCAN":
                result_beam = obj.make_return_beam(energy, p)
                result_beam.beam_type = "SCANRESULT"
                result_beam.scan_target = self

class SmartPhysicsObject(PhysicsObject):
    def __init__(self, universe, client,
                    position = [ 0.0, 0.0, 0 ],
                    velocity = [ 0.0, 0.0, 0 ],
                    orientation = [ 0.0, 0.0, 0.0 ],
                    mass = 10.0,
                    radius = 1.0,
                    thrust = [0.0, 0.0, 0.0],
                    object_type = "Unknown"):
        PhysicsObject.__init__(self, universe, position, velocity, orientation, mass, radius, thrust, object_type)
        self.client = client
        self.universe = universe
        self.sim_id = None
        self.vis_data = 0
        self.vis_meta_data = 0

    def handle(self, client):
        msg = Message.get_message(client)

        if isinstance(msg, HelloMsg):
            if msg.endpoint_id == None:
                return

            self.sim_id = msg.endpoint_id
            HelloMsg.send(client, self.phys_id)

        # If we don't have a sim_id by this point, we can't accept any of the
        # following in good conscience...
        if self.sim_id == None:
            return

        if isinstance(msg, PhysicalPropertiesMsg):
            if msg.object_type:
                self.object_type = msg.object_type

            if msg.mass:
                self.mass = msg.mass

            if msg.position:
                self.position = Vector3(msg.position)

            if msg.velocity:
                self.velocity = Vector3(msg.velocity)

            if msg.orientation:
                self.orientation = Vector3(msg.orientation)

            if msg.thrust:
                self.thrust = Vector3(msg.thrust)

            if msg.radius:
                self.radius = msg.radius

            new_emits = PhysicsObject.is_big_enough(self.mass, self.radius)
            diff = self.emits_gravity - new_emits

            if diff != 0:
                self.universe.update_attractor(self)

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
            beam = Beam.build(msg, self.universe)

            if msg.beam_type == "COMM":
                beam.message = msg.msg

            self.universe.add_beam(beam)

    # I was in a collision with the other object, with a certain amount of energy,
    # the other object came from d and hit me at p
    def collision(self, obj, energy, d, p):
        if isinstance(obj, PhysicsObject):
            CollisionMessage.send(self.client, [p.x, p.y, p.z, d.x, d.y, d.x, energy, "PHYS"])
            self.resolve_phys_collision(energy, d, p)
        elif isinstance(obj, Beam):
            if obj.beam_type == "SCANRESULT":
                ScanResultMsg.send(self.client, PhysicalPropertiesMsg.make_from_object(obj.scan_target))
            if obj.beam_type == "COMM":
                CollisionMessage.send(self.client, [p.x, p.y, p.z, d.x, d.y, d.x, energy, obj.beam_type] + obj.message)
            else:
                CollisionMessage.send(self.client, [p.x, p.y, p.z, d.x, d.y, d.x, energy, obj.beam_type])

class Beam:
    def __init__(self, universe,
                    origin, normals,
                    direction, velocity,
                    energy, beam_type):
        self.universe = universe
        self.origin = origin if isinstance(origin, Vector3) else Vector3(origin)
        self.front_position = self.origin.clone()

        self.normals = []
        for n in normals:
            self.normals.append(n if isinstance(n, Vector3) else Vector3(n))
            
        self.direction = direction if isinstance(direction, Vector3) else Vector3(direction)
        self.velocity = velocity if isinstance(velocity, Vector3) else Vector3(velocity)
        self.energy = energy
        self.beam_type = beam_type
        
        self.speed = self.velocity.length()
        self.distance_travelled = 0

    @staticmethod
    def collide(b, obj, dt):
        # ### TODO ### Properly handle forces here for the physical object

        # To detect a collition, we grab the normals from the beam, and translate
        # the object's position to 'beam-space' by subtracting the beam's origin
        # from the object's position.

        # Since the object could be positioned outside of the beam's frustrum,
        # but still extend into the beam based on its radius, we check that each
        # of the dot procuts is >= -obj.radius. If that is satisfied for all
        # of the dot products, then the object extends (at least partially)
        # into the beam.

        # Now we need to check that it is actually in the portion of space that
        # the beam will be carving out during dt. To do this we take the beam's
        # front_position, or the centre of the wave-front at the start of the tick
        # and verify that the object's position is between front_position and
        # front_position + dt * beam velocity. Do this by grabbing a unit vector
        # along velocity (direction), dotting the (object's position - front_position)
        # with it, and then comparing the result with dt * beam's speed. The dot
        # product should be in [-obj.radius, dt * beam_speed + obj.radius]
        # (the closed interval)

        # For each plane, find out which side the object is on, and whether it
        # will cross the plane in this tick. If for any plane it is neither in
        # inside of it, nor will it cross it, then bail. Take the radius into
        # account here

        # For all planes where the object crosses it, find the t value, and note
        # whether the object is crossing in or out.

        # Using the t values we found above, determine the potential values for t
        # where the object is still (at least partially) inside the beam. Do this
        # by finding the earliest time the beam will leave the beam, and the latest
        # time that the object will enter the beam. Note that t must be in [0,1]
        # for these considerations. If the beam leaves before it gets in, bail.
        # No hit.

        # ### NOTE ### I'm not sure if this hunch is right, but I think it is a
        # close enough approximation. The magic of linear situations might actually
        # make me right...
        # If there are still candidate values of t, then just take the middle of
        # the interval as the 'collision' time that maximizes the energy transfer.
        # This only might work for spheres...

        ps = obj.position.clone()
        ps.sub(b.origin)

        v = obj.velocity.clone()
        v.scale(dt)

        pe = Vector3.combine([[1, ps], [1, v]])

        entering = 0.0
        leaving = 1.0
        
        for n in b.normals:
            inout = n.dot(ps)
            willbe = n.dot(pe)

            if inout < -obj.radius:
                if willbe < -obj.radius:
                    # staying out
                    return -1
                else:
                    # leaving
                    param = n.dot(ps) / n.dot(v)
                    leaving = min(leaving, param)
            else:
                if willbe < -obj.radius:
                    # entering
                    param = (n.dot(ps) / n.dot(v))
                    entering = max(entering, param)
                else:
                    # staying in
                    pass

        # If we have to enter some planes, and are leaving other planes, but
        # we don't enter until after we leave, we can bail.
        if entering > leaving:
            return -1

        t = (entering + leaving) / 2.0

        pc = Vector3.combine([[1, ps], [t, v]]).dot(b.direction)

        # If we want collisions for as long as the bounding sphere is intersecting
        # the front, use:
        #
        # if ((pc + obj.radius) >= b.distance_travelled) and ((pc - obj.radius) <= (b.distance_travelled + b.speed * dt)):
        if pc >= b.distance_travelled and pc <= (b.distance_travelled + b.speed * dt):
            return [t, None]
        else:
            return -1

    def make_return_beam(self, energy, origin):
        direction = self.direction.clone()
        direction.scale(-1)

        velocity = self.velocity.clone()
        velocity.scale(-1)

        normals = []
        for n in self.normals:
            tmp = n.clone()
            tmp.scale(-1)
            normals.append(tmp)

        return Beam(self.universe, origin, normals, direction, velocity, energy, None)

    def collision(self, obj, position, energy):
        # ### TODO ### Beam occlusion
        pass

    def tick(self, dt):
        self.distance_travelled += self.speed * dt
        self.front_position.add(self.velocity, dt)

    @staticmethod
    def build(msg, universe):
        direction = Vector3(msg.velocity)
        direction.scale(direction.length())
        up = Vector3(msg.up)
        up.scale(up.length())
        normals = []

        # Ok, so now we need to convert the direction, up, and spread angles
        # into plane normals. Yay!

        # First we cross the direction and up to get a vector pointing 'horizontally'.
        # this vector points 'right', or clockwise, when looking down the up vector.
        # The order of the cross product here doesn't really matter, but it
        # helps to follow some reasoning.
        right = direction.cross(up)

        # Once we have right, we can find vectors inside of the planes, thanks
        # to some basic trig. Here, cosine is along the direction, and sine
        # is along the right vector.

        sh = sin(msg.spread_h / 2)
        ch = cos(msg.spread_h / 2)
        sv = sin(msg.spread_v / 2)
        cv = cos(msg.spread_v / 2)

        # The vertical planes (those on the right and left boundaries of the beam)
        # are obtained by combining the direction and horizontal vector.
        # That gets us a vector inside the plane
        plane_r = Vector3.combine([[ch, direction], [sh, right]])
        plane_l = Vector3.combine([[ch, direction], [-sh, right]])

        # Similarly, the up vector gets us the top and bottom in-plane vectors
        plane_t = Vector3.combine([[ch, direction], [sh, up]])
        plane_b = Vector3.combine([[ch, direction], [-sh, up]])

        # We can get the normal vectors through another cross product, this time
        # between the in-plane vectors, and the right/up vectors.

        # The order of vectors in the cross product is important here. Reversing the
        # order reverses the direction of the resulting vector, pointing outside of
        # bounded volume, instead of inside.

        # Following the right-hand rule, point your index finger along the first
        # vector, middle finger along the second, and the result is your thumb

        # This means that we need the in-plane vectors first. For the left and right
        # planes, the up vector should be in them, so we can cross them to
        # get a normal. Similarly, the right vector is in the top and bottom planes.

        normals.append(plane_r.cross(up))
        normals.append(plane_l.cross(up))
        normals.append(plane_b.cross(right))
        normals.append(plane_t.cross(right))

        # Now we have enough information to build our beam object.

        return Beam(universe, Vector3(msg.origin), normals, direction, Vector3(msg.velocity), msg.energy, msg.beam_type)
        