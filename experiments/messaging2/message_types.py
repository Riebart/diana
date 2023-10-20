#!/usr/bin/env python3

class TypeMapper(object):
    def __init__(self):
        pass

class BSONType(object):
    cxx_type = "NULL"
    python_type = "None"
    num_elements = 1

class Double(BSONType):
    cpp_type = "double"
    python_type = "float"
    num_elements = 1

class Integer(BSONType):
    cpp_type = "int64_t"
    python_type = "int"
    num_elements = 1

class String(BSONType):
    cpp_type = "char*"
    python_type = "str"
    num_elements = 1

class Vector3(BSONType):
    def __init__(self, t: BSONType):
        # self.cpp_type = f"Vector3<{t.cpp_type}>"
        self.cpp_type = "struct Vector3"
        self.python_type = "Vector3"
        self.num_elements = 3

class Vector4(BSONType):
    def __init__(self, t: BSONType):
        # self.cpp_type = f"Vector4<{t.cpp_type}>"
        self.cpp_type = "struct Vector4"
        self.python_type = "Vector4"
        self.num_elements = 4

class Spectrum(BSONType):
    cpp_type = "struct Spectrum*"
    python_type = "Spectrum"
    num_elements = 10

class BSONMessage(object):
    """
    A BSONMessage has properties of specific types, and a number of properties that can be read.
    """
    properties = None
    name = None

    def gen_cpp_header(self):
        num_elements = sum([t.num_elements for p, t in self.properties.items()])

        properties = " ".join([
                f"{t.cpp_type} {k};" for k, t in self.properties.items()
            ])

        return  f"""
        class {self.name}Msg : public BSONMessage
        {{
            public:
            {self.name}Msg(BSONReader* _br = NULL) : BSONMessage(_br, {num_elements}, handlers(), {self.name}) {{ }}
            ~{self.name}Msg();
            int64_t send(sock_t sock);
            {properties}
        }}
        """

    def gen_cpp_constructor(self):
        # The reader in the constructor of the objects takes in a BSONReader object, which is assumed
        # to be a correct BSON reader
        pass

class HelloMsg(BSONMessage):
    properties = {}

class PhysicalPropertiesMessage(BSONMessage):
    name = "PhysicalProperties"
    properties = {
        "client_id": Integer(),
        "server_id": Integer(),
        "object_type": String(),
        "mass": Double(),
        "radius": Double(),
        "position": Vector3(Double()),
        "velocity": Vector3(Double()),
        "thrust": Vector3(Double()),
        "orientation": Vector4(Double()),
        "spectrum": Spectrum()
    }

if __name__ == "__main__":
    print(PhysicalPropertiesMessage().gen_cpp_header())
