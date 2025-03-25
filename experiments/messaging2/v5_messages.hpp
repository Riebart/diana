#include "v5_elements.hpp"

class Message
{
public:
    std::size_t dump_size() { return this->msg.dump_size(); }
    void* dump() { return NULL; }

protected:
    struct Element<char> msg;
};

#define PhysicsType double
#define CoordinateType double
// template <typename PhysicsType, typename CoordinateType>
class PhysicalPropertiesMsg : virtual public Message
{
public:
    struct Element<std::int64_t>& server_id() { return this->msg.element; }
    struct Element<std::int64_t>& client_id() { return this->msg.next.element; }
    struct OptionalElement<const char*>& object_type() { return this->msg.next.next.element; }
    struct OptionalElement<PhysicsType>& mass() { return this->msg.next.next.next.element; }
    struct OptionalElement<PhysicsType>& radius() { return this->msg.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& position() { return this->msg.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<CoordinateType>>& velcoity() { return this->msg.next.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector3<PhysicsType>>& thrust() { return this->msg.next.next.next.next.next.next.next.element; }
    struct OptionalElement<struct Vector4<PhysicsType>>& orientation() { return this->msg.next.next.next.next.next.next.next.next; }

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