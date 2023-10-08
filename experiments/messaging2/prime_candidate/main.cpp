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
    PhysicalPropertiesMsg msg{}, msg2{};
    
    std::cout << sizeof(msg) << std::endl;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.radius = 102020.1;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.mass = 0.1;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.object_type = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.position = Vector3<double>(1.1, 2.2, 3.3);
    msg.orientation = Vector4<double>(-100.1,-200.2,-300.3,-400.4);
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    std::cout << msg.json() << std::endl;

    std::uint8_t buf[4096];
    std::size_t binary_write_count = msg.binary_write(buf);
    std::cout << msg.binary_size() << " " << binary_write_count << std::endl;

    std::size_t binary_read_count = msg2.binary_read(buf);
    std::cout << binary_read_count << std::endl;
    std::cout << msg2.json() << std::endl;
    return 0;
}
