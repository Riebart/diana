#include <iostream>
#include <cstdint>
#include <cfloat>
#include <string>

#include "v6_generative_with_structs.hpp"

template <typename PhysicsType, typename CoordinateType>
class PhysicalPropertiesMsg
{
public:
    struct Element<std::int64_t>& server_id() { return this->msg.element; }
    struct Element<std::int64_t>& client_id() { return this->msg.next.element; }
    struct OptionalElement<const char*>& object_type() { return this->msg.next.next.element; }
    struct OptionalElement<PhysicsType>& mass() { return this->msg.next.next.next.element; }
    struct OptionalElement<PhysicsType>& radius() { return this->msg.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& position() { return this->msg.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& velocity() { return this->msg.next.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<PhysicsType>>& thrust() { return this->msg.next.next.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector4<PhysicsType>>& orientation() { return this->msg.next.next.next.next.next.next.next.next.element; }

    std::size_t dump_size() { return this->msg.dump_size(); }
    std::uint8_t* binary_write() { std::uint8_t* buf = new std::uint8_t[this->dump_size()]; this->binary_write(buf); return buf; }
    void binary_write(std::uint8_t* buf) { msg.binary_write(buf); }
    std::string json() { std::string s("{"); msg.json(&s, 0); s.append("}"); return s; }

private:
    struct NamedElementC<std::int64_t, "server_id",
        struct NamedElementC<std::int64_t, "client_id",
            struct NamedOptionalElementC<const char*, "object_type",
                struct NamedOptionalElementC<PhysicsType, "mass",
                    struct NamedOptionalElementC<PhysicsType, "radius",
                        struct NamedOptionalElementC<struct Vector3<CoordinateType>, "position",
                                struct NamedOptionalElementC<struct Vector3<CoordinateType>, "velocity",
                                        struct NamedOptionalElementC<struct Vector3<PhysicsType>, "thrust",
                                                struct NamedOptionalElement<struct Vector4<PhysicsType>, "orientation"
                                                        >>>>>>>>> msg;
};

int main(int argc, char** argv)
{
    std::cout << "double " << sizeof(struct Element<double>) << std::endl;
    std::cout << "std::int64_t " << sizeof(struct Element<std::int64_t>) << std::endl;
    std::cout << "optional std::int64_t " << sizeof(struct OptionalElement<std::int64_t>) << std::endl;
    std::cout << "optional double " << sizeof(struct OptionalElement<double>) << std::endl;
    std::cout << "optional char* " << sizeof(struct OptionalElement<const char*>) << std::endl;
    std::cout << "optional V3<d> " << sizeof(struct OptionalElement<struct Vector3<double>>) << std::endl;
    std::cout << "optional V4<d> " << sizeof(struct OptionalElement<struct Vector4<double>>) << std::endl;
    std::cout << "double+optional double " << sizeof(struct ElementC<double, struct OptionalElement<double>>) << std::endl;

    std::cout << "PhysProps Message " << sizeof(struct PhysicalPropertiesMsg<double, double>) << std::endl;

    struct PhysicalPropertiesMsg<double, double> msg;
    std::cout << "IDs before: " << (std::int64_t)msg.server_id() << " " << (std::int64_t)msg.client_id() << std::endl;
    msg.server_id() = 10;
    msg.client_id() = 1089;
    msg.mass() = 1.0;
    msg.radius() = 2.2;
    msg.position() = Vector3(1.1, 2.1, 3.1);
    msg.velocity() = Vector3(10.1, 20.1, 30.1);
    msg.thrust() = Vector3(11.1, 22.1, 33.1);
    msg.orientation() = Vector4(111.1, 222.1, 333.1, 10.0);
    msg.object_type() = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << "IDs after: "<< (std::int64_t)msg.server_id() << " " << (std::int64_t)msg.client_id() << std::endl;
    std::cout << (const char*)msg.object_type() << std::endl;
    std::cout << msg.dump_size() << std::endl;

    int loop_count = 100000;
    for (int i = 1 ; i <= loop_count ; i++)
    {
        msg.mass().present = i & 1;
        msg.radius().present = i & 2;
        msg.position().present = i & 4;
        msg.velocity().present = i & 8;
        msg.thrust().present = i & 16;
        msg.object_type().present = i & 32;
        msg.orientation().present = i & 64;
        // std::uint8_t* bin = msg.binary_write();
        // delete bin;
        std::cout << "loop " << i << "/" << loop_count << " (" <<
                  (i&64)/64 << " " <<
                  (i&32)/32 << " " <<
                  (i&16)/16 << " " <<
                  (i&8)/8 << " " <<
                  (i&4)/4 << " " <<
                  (i&2)/2 << " " <<
                  (i&1)/1 << ") " <<
                  msg.dump_size() << " " << msg.json().length() << "\r";
    }

    std::cout << std::endl;

    return 0;
}