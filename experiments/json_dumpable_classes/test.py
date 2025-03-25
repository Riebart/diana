#!/usr/bin/env python3

import json

def to_json_handler(obj):
    return obj.json()

class Vector3(object):
    x: float
    y: float
    z: float

    def __init__(self, x : float, y : float, z : float):
        self.x = x
        self.y = y
        self.z = z

    def json(self):
        return vars(self)
    
class Vector4(Vector3):
    w : float

    def __init__(self, w : float, x : float, y : float, z : float):
        Vector3.__init__(self, x, y, z)
        self. w = w


class Motion(object):
    position: Vector3
    velocity: Vector3
    acceleration: Vector3

    def __init__(self, position: Vector3, velocity: Vector3, acceleration: Vector3):
        self.position = position
        self.velocity = velocity
        self.acceleration = acceleration
    
    def json(self):
        return vars(self)

if __name__ == "__main__":
    m = Motion(
        Vector3(1.1, 2.2, 3.3),
        Vector3(10.1, 20.2, 30.3),
        Vector3(101, 202, 303)
    )

    print(json.dumps({
        "name": "test",
        "motion": m,
        "orientation": Vector4(-1, -2, -3, -4)
        }, default=to_json_handler))

# mike@Ceres:~/working/diana/experiments/messaging2$ python3 test.py  | jq .motion
# {
#   "position": {
#     "x": 1.1,
#     "y": 2.2,
#     "z": 3.3
#   },
#   "velocity": {
#     "x": 10.1,
#     "y": 20.2,
#     "z": 30.3
#   },
#   "acceleration": {
#     "x": 101,
#     "y": 202,
#     "z": 303
#   }
# }