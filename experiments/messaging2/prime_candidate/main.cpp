#include "reflection.hpp"
#include "messaging.hpp"

REFLECTION_STRUCT(PhysicalPropertiesMsg,
    (std::int64_t) server_id,
    (std::int64_t) client_id,
    (Optional<const char*>) object_type,
    (Optional<double>) mass,
    (Optional<double>) radius,
    (Optional<Vector3<double>>) position,
    (Optional<Vector3<double>>) velocity,
    (Optional<Vector3<double>>) thrust,
    (Optional<Vector4<double>>) orientation
)

int main(int argc, char** argv)
{
    std::cout << "Physical Properties message testing..." << std::endl;
    PhysicalPropertiesMsg msg{};
    std::cout << sizeof(msg) << std::endl;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.radius = 0.0;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.mass = 0.0;
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;
    msg.object_type = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << msg.dump_size() << " " << msg.json() << std::endl;

    return 0;
}
