import json
import yaml

class SimpleJson(json.JSONEncoder):
    @classmethod
    def default(self, o):
        return vars(o)

class Vector3(object):
    x: float
    y: float
    z: float

    def __init__(self, x : float, y : float, z : float):
        self.x = x
        self.y = y
        self.z = z

class Vector4(Vector3):
    w : float

    def __init__(self, w : float, x : float, y : float, z : float):
        Vector3.__init__(self, x, y, z)
        self. w = w

class Motion(SimpleJson):
    position: Vector3
    velocity: Vector3
    acceleration: Vector3

    def __init__(self, position: Vector3, velocity: Vector3, acceleration: Vector3):
        self.position = position
        self.velocity = velocity
        self.acceleration = acceleration

if __name__ == "__main__":
    m = Motion(
        Vector3(1.1, 2.2, 3.3),
        Vector3(10.1, 20.2, 30.3),
        Vector3(101, 202, 303)
    )

import yaml, json
yaml.emitter.Emitter.process_tag = lambda *args, **kwargs: None

print(json.dumps(yaml.safe_load(yaml.dump({
    "name": "test",
    "motion": m,
    "orientation": Vector4(-1, -2, -3, -4)
    }))))