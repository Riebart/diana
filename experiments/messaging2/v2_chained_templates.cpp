#include <iostream>
#include <cstdint>
#include <cfloat>
#include <string>

#include "elements.hpp"

template <typename T> struct ChainedElement
{
    T next;
    std::size_t dump_size() { return next.dump_size(); }
};

template <typename T, typename TNext>
struct ElementC : ChainedElement<TNext>
{
    struct Element<T> element;

    std::size_t dump_size()
    {
        return element.dump_size() + ChainedElement<TNext>::dump_size();
    }
};

template <typename T, typename TNext>
struct OptionalElementC : ChainedElement<TNext>
{
    struct OptionalElement<T> element;

    std::size_t dump_size()
    {
        return element.dump_size() + ChainedElement<TNext>::dump_size();
    }
};

class Message
{
public:
    std::size_t dump_size() { return this->msg.dump_size(); }
    void* dump() { return NULL; }

protected:
    struct Element<char> msg;
};

/// OPERATIONAL AND DATA MODEL IN SINGLE STRUCTURE
//-----------------------------------------------------------------------------------------------------------
template <typename PhysicsType, typename CoordinateType>
class PhysicalPropertiesMsg : virtual public Message
{
public:
    struct Element<std::int64_t>& server_id() { return this->msg.element; }
    struct Element<std::int64_t>& client_id() { return this->msg.next.element; }
    struct OptionalElement<const char*>& object_type() { return this->msg.next.next.element; }
    struct OptionalElement<PhysicsType>& mass() { return this->msg.next.next.next.element; }
    struct OptionalElement<PhysicsType>& radius() { return this->msg.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& position() { return this->msg.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& velcoity() { return this->msg.next.next.next.next.next.next.value; }
    struct OptionalElement<struct Vector3<PhysicsType>>& thrust() { return this->msg.next.next.next.next.next.next.next.value; }
    struct OptionalElement<struct Vector4<PhysicsType>>& orientation() { return this->msg.next.next.next.next.next.next.next.next.value; }

    virtual std::size_t dump_size() { return this->msg.dump_size(); }

private:
    struct ElementC<std::int64_t,
               struct ElementC<std::int64_t,
                   struct OptionalElementC<const char*,
                       struct OptionalElementC<PhysicsType,
                           struct OptionalElementC<PhysicsType,
                               struct OptionalElementC<struct Vector3<CoordinateType>,
                                       struct OptionalElementC<struct Vector3<CoordinateType>,
                                               struct OptionalElementC<struct Vector3<PhysicsType>,
                                                       struct OptionalElement<struct Vector4<PhysicsType>
                                                               >>>>>>>>> msg;
};
//-----------------------------------------------------------------------------------------------------------

/// SEPARATE OPERATIONAL AND DATA MODELS SHARING A UNION
//-----------------------------------------------------------------------------------------------------------
template <typename PhysicsType, typename CoordinateType>
class PhysicalPropertiesMsgO : virtual public Message
{
public:
    virtual std::size_t dump_size() { return this->msg.dump_size(); }

private:
    struct ElementC<std::int64_t,
               struct ElementC<std::int64_t,
                   struct OptionalElementC<const char*,
                       struct OptionalElementC<PhysicsType,
                           struct OptionalElementC<PhysicsType,
                               struct OptionalElementC<struct Vector3<CoordinateType>,
                                       struct OptionalElementC<struct Vector3<CoordinateType>,
                                               struct OptionalElementC<struct Vector3<PhysicsType>,
                                                       struct OptionalElement<struct Vector4<PhysicsType>
                                                               >>>>>>>>> msg;
};

template <typename PhysicsType, typename CoordinateType>
struct PhysicalPropertiesMsgD
{
    std::int64_t server_id, client_id;
    struct OptionalElement<PhysicsType> mass, radius;
    struct OptionalElement<struct Vector3<PhysicsType>> position, velocity, thrust;
    struct OptionalElement<struct Vector4<CoordinateType>> orientation;
};

union PhysPropsMsg
{
    struct PhysicalPropertiesMsgD<double, double> d;
    PhysicalPropertiesMsgO<double, double> m;
};
//-----------------------------------------------------------------------------------------------------------

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
    msg.radius() = 2.0;
    msg.position() = Vector3(1.1, 2.1, 3.1);
    msg.object_type() = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << "IDs after: "<< (std::int64_t)msg.server_id() << " " << (std::int64_t)msg.client_id() << std::endl;
    std::cout << (const char*)msg.object_type() << std::endl;
    msg.dump();
    std::cout << msg.dump_size() << std::endl;

    return 0;
}