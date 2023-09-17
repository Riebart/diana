#include <iostream>
#include <cstdint>
#include <cfloat>
#include <string>

#include "elements.hpp"

// #pragma pack(1)
template <typename T, typename TNext>
struct ElementC
{
    struct Element<T> element;
    TNext next;
};

// #pragma pack(1)
template <typename T, typename TNext>
struct OptionalElementC
{
    struct OptionalElement<T> element;
    TNext next;
};

template <typename PhysicsType, typename CoordinateType>
class PhysicalPropertiesMsg
{
public:
    std::int64_t& server_id() { return this->msg.element.value; }
    std::int64_t& client_id() { return this->msg.next.element.value; }
    char* object_type() { return this->msg.next.next.element.value; }
    PhysicsType mass() { return this->msg.next.next.next.element.value; }
    PhysicsType radius() { return this->msg.next.next.next.next.element.value; }
    struct Vector3<CoordinateType> position() { return this->msg.next.next.next.next.next.element.value; }
    struct Vector3<CoordinateType> velcoity() { return this->msg.next.next.next.next.next.next.element.value; }
    struct Vector3<PhysicsType> thrust() { return this->msg.next.next.next.next.next.next.next.element.value; }
    struct Vector4<PhysicsType> orientation() { return this->msg.next.next.next.next.next.next.next.next.element.value; }

private:
    struct ElementC<std::int64_t,
               struct ElementC<std::int64_t,
                   struct OptionalElementC<char*,
                       struct OptionalElementC<PhysicsType,
                           struct OptionalElementC<PhysicsType,
                               struct OptionalElementC<struct Vector3<CoordinateType>,
                                       struct OptionalElementC<struct Vector3<CoordinateType>,
                                               struct OptionalElementC<struct Vector3<PhysicsType>,
                                                       struct OptionalElement<struct Vector4<PhysicsType>
                                                               >>>>>>>>> msg;


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
    std::cout << "double+optional double " << sizeof(struct ElementC<double, struct OptionalElement<double>>) << std::endl;

    std::cout << "PhysProps Message " << sizeof(struct PhysicalPropertiesMsg<double, double>) << std::endl;

    struct PhysicalPropertiesMsg<double, double> msg;
    std::cout << "IDs before: " << msg.server_id() << " " << msg.client_id() << std::endl;
    msg.server_id() = 10;
    msg.client_id() = 1089;
    std::cout << "IDs after: "<< msg.server_id() << " " << msg.client_id() << std::endl;

    return 0;
}