// This uses a struct (non-POD) and templates to define the message
// structures.

#include <iostream> // for std::cout
#include <cstdint> // This gives us C++ named std::int64_t
// #include <cfloat> // In theory this gives us std::float128, but... no

#include "elements.hpp"

template <typename PhysicsType, typename CoordinateType>
struct PhysicalPropertiesMsg
{
    struct Element<std::int64_t>
        server_id, client_id;
    struct OptionalElement<char*> object_type;
    struct OptionalElement<PhysicsType> mass;
    struct OptionalElement<PhysicsType> radius;
    struct OptionalElement<struct Vector3<CoordinateType>>
                    position, velocity, thrust;
    struct OptionalElement<struct Vector4<PhysicsType>> orientation;

    void* dump()
    {
        struct PhysicalPropertiesMsg* out = new struct PhysicalPropertiesMsg();
        *out = *this;

        server_id.hton();
        server_id.hton();
        object_type.hton();
        mass.hton();
        radius.hton();
        position.hton();
        velocity.hton();
        thrust.hton();
        orientation.hton();

        return (void*)out;
    }

    std::size_t dump_size()
    {
        return
            server_id.dump_size() +
            server_id.dump_size() +
            object_type.dump_size() +
            mass.dump_size() +
            radius.dump_size() +
            position.dump_size() +
            velocity.dump_size() +
            thrust.dump_size() +
            orientation.dump_size();
    }
};

int main(int argc, char** argv)
{
    std::cout << "double " << sizeof(struct Element<double>) << std::endl;
    std::cout << "std::int64_t " << sizeof(struct Element<std::int64_t>) << std::endl;
    std::cout << "optional std::int64_t " << sizeof(struct OptionalElement<std::int64_t>) << std::endl;
    std::cout << "optional double " << sizeof(struct OptionalElement<double>) << std::endl;
    std::cout << "optional char* " << sizeof(struct OptionalElement<char*>) << std::endl;
    std::cout << "optional V3<d> " << sizeof(struct OptionalElement<struct Vector3<double>>) << std::endl;
    std::cout << "optional V4<d> " << sizeof(struct OptionalElement<struct Vector4<double>>) << std::endl;

    std::cout << "PhysProps Message " << sizeof(struct PhysicalPropertiesMsg<double, double>) << std::endl;

    struct PhysicalPropertiesMsg<double, double> msg;
    msg.mass = 1.0;
    msg.radius = 2.0;
    msg.position = Vector3(1.1, 2.1, 3.1);
    msg.object_type = (char*)"This is some stuff!";
    msg.dump();
    std::cout << msg.dump_size() << std::endl;

    return 0;
}