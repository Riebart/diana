#include "messaging.hpp"
#include "bson.hpp"
#include "MIMOServer.hpp"

#define READ_EPILOGUE() { if }
#define SEND_INTRO() BSONWriter bw; bw.push_int32(server_id); bw.push_int32(client_id);
#define PUSH_VECTOR3(bw, vec) { bw.push_double(vec.x); bw.push_double(vec.y); bw.push_double(vec.z); }
#define PUSH_VECTOR4(bw, vec) { bw.push_double(vec.w); bw.push_double(vec.x); bw.push_double(vec.y); bw.push_double(vec.z); }

void BSONMessage::ReadIDs()
{
    server_id = ReadInt64();
    client_id = ReadInt64();
}

double BSONMessage::ReadDouble()
{
    return br->get_next_element().dbl_val;
}

bool BSONMessage::ReadBool()
{
    return br->get_next_element().bln_val;
}

int32_t BSONMessage::ReadInt32()
{
    return br->get_next_element().i32_val;
}

int64_t BSONMessage::ReadInt64()
{
    return br->get_next_element().i64_val;
}

void BSONMessage::ReadString(char* dst, int32_t dstlen)
{
    struct BSONReader::Element el;
    el = br->get_next_element();
    int32_t copy_len = (el.str_len <= dstlen ? el.str_len : dstlen);
    memcpy(dst, el.str_val, copy_len);
}

char* BSONMessage::ReadString()
{
    struct BSONReader::Element el;
    el = br->get_next_element();
    char* ret = (char*)malloc(el.str_len);
    memcpy(ret, el.str_val, el.str_len);
    return ret;
}

struct Vector3 BSONMessage::ReadVector3()
{
    return Vector3{
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val
    };
}

struct Vector4 BSONMessage::ReadVector4()
{
    return Vector4{
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val
    };
}

BSONMessage::~BSONMessage()
{
    // Note that we don't have to delete the BSONReader, since it's
    // being allocated (probably on the stack) outside of this class.
}

BSONMessage* BSONMessage::ReadMessage(int sock)
{
    // First start by reading the whole BSON message
    int32_t message_len = 0;
    int nbytes = MIMOServer::socket_read(sock, (char*)(&message_len), 4);

    char* buf = (char*)malloc(message_len + 4);
    if (buf == NULL)
    {
        throw "OOM You twat";
    }
    *(int32_t*)buf = message_len;
    nbytes = MIMOServer::socket_read(sock, buf + 4, message_len - 4);

    BSONReader br(buf);
    struct BSONReader::Element el = br.get_next_element();

    // Sanity checks:
    // The element should be an int32 (type 16), and a name of "MsgType".
    // Switch on the value to hand off further parsing.
    if ((el.type != 16) || (strcmp(el.name, "MsgType") != 0))
    {
        return NULL;
    }
    else
    {
        switch (el.i32_val)
        {
        case MessageTypes::Hello:
            return new HelloMsg(&br, el.i32_val);
        case MessageTypes::PhysicalProperties:
            return new PhysicalPropertiesMsg(&br, el.i32_val);
        case MessageTypes::VisualProperties:
            return new VisualPropertiesMsg(&br, el.i32_val);
        case MessageTypes::VisualDataEnable:
            return new VisualDataEnableMsg(&br, el.i32_val);
        case MessageTypes::VisualMetaDataEnable:
            return new VisualMetaDataEnableMsg(&br, el.i32_val);
        case MessageTypes::VisualMetaData:
            return new VisualMetaDataMsg(&br, el.i32_val);
        case MessageTypes::VisualData:
            return new VisualDataMsg(&br, el.i32_val);
        case MessageTypes::Beam:
            return new BeamMsg(&br, el.i32_val);
        case MessageTypes::Collision:
            return new CollisionMsg(&br, el.i32_val);
        case MessageTypes::Spawn:
            return new SpawnMsg(&br, el.i32_val);
        case MessageTypes::ScanResult:
            return new ScanResultMsg(&br, el.i32_val);
        case MessageTypes::ScanQuery:
            return new ScanQueryMsg(&br, el.i32_val);
        case MessageTypes::ScanResponse:
            return new ScanResponseMsg(&br, el.i32_val);
        case MessageTypes::Goodbye:
            return new GoodbyeMsg(&br, el.i32_val);
        case MessageTypes::Directory:
            return new DirectoryMsg(&br, el.i32_val);
        case MessageTypes::Name:
            return new NameMsg(&br, el.i32_val);
        case MessageTypes::Ready:
            return new ReadyMsg(&br, el.i32_val);
        case MessageTypes::Thrust:
            return new ThrustMsg(&br, el.i32_val);
        case MessageTypes::Velocity:
            return new VelocityMsg(&br, el.i32_val);
        case MessageTypes::Jump:
            return new JumpMsg(&br, el.i32_val);
        case MessageTypes::InfoUpdate:
            return new InfoUpdateMsg(&br, el.i32_val);
        case MessageTypes::RequestUpdate:
            return new RequestUpdateMsg(&br, el.i32_val);
        default:
            return NULL;
        }
    }
}

HelloMsg::HelloMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    // This is just a token SYN, for ID establishing on both sides.
}

uint8_t* HelloMsg::send(int32_t server_id, int32_t client_id)
{
    SEND_INTRO();
    return bw.push_end();
}

PhysicalPropertiesMsg::PhysicalPropertiesMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    obj_type = ReadString();
    mass = ReadDouble();
    position = ReadVector3();
    velocity = ReadVector3();
    orientation = ReadVector4();
    thrust = ReadVector3();
    radius = ReadDouble();
}

//uint8_t* PhysicalPropertiesMsg::send(int32_t client_id, struct PhysicsObject* obj, struct PhysicsObject* ref)
//{
//}

uint8_t* PhysicalPropertiesMsg::send(int32_t server_id, int32_t client_id, char* obj_type,
    double mass,
struct Vector3 position,
struct Vector3 velocity,
struct Vector4 orientation,
struct Vector3 thrust,
    double radius)
{
    SEND_INTRO();
    bw.push_string(obj_type);
    bw.push_double(mass);
    PUSH_VECTOR3(bw, position);
    PUSH_VECTOR3(bw, velocity);
    PUSH_VECTOR4(bw, orientation);
    PUSH_VECTOR3(bw, thrust);
    bw.push_double(radius);
    return bw.push_end();
}

PhysicalPropertiesMsg::~PhysicalPropertiesMsg()
{
    free(obj_type);
}

VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    enabled = ReadBool();
}

uint8_t* VisualDataEnableMsg::send(int32_t server_id, int32_t client_id, bool enabled)
{
    SEND_INTRO();
    bw.push_bool(enabled);
    return bw.push_end();
}

VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    enabled = ReadBool();
}

uint8_t* VisualMetaDataEnableMsg::send(int32_t server_id, int32_t client_id, bool enabled)
{
    SEND_INTRO();
    bw.push_bool(enabled);
    return bw.push_end();
}

VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

VisualDataMsg::VisualDataMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    phys_id = ReadInt64();
    radius = ReadDouble();
    position = ReadVector3();
    orientation = ReadVector4();
}

uint8_t* VisualDataMsg::send(int32_t server_id, int32_t client_id, int64_t phys_id, double radius,
struct Vector3 position, struct Vector4 orientation)
{
    SEND_INTRO();
    bw.push_int64(phys_id);
    bw.push_double(radius);
    PUSH_VECTOR3(bw, position);
    PUSH_VECTOR4(bw, orientation);
    return bw.push_end();
}

BeamMsg::BeamMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    origin = ReadVector3();
    velocity = ReadVector3();
    up = ReadVector3();
    spread_h = ReadDouble();
    spread_v = ReadDouble();
    energy = ReadDouble();
    ReadString(type, 4);
    if (strcmp(type, "COMM") == 0)
    {
        msg = ReadString();
    }
    else
    {
        msg = NULL;
    }
}

BeamMsg::~BeamMsg()
{
    if (msg != NULL)
    {
        free(msg);
    }
}

CollisionMsg::CollisionMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    position = ReadVector3();
    direction = ReadVector3();
    energy = ReadDouble();
    ReadString(type, 4);
    if (strcmp(type, "COMM") == 0)
    {
        msg = ReadString();
    }
    else
    {
        msg = NULL;
    }
}

CollisionMsg::~CollisionMsg()
{
    if (msg != NULL)
    {
        free(msg);
    }
}

SpawnMsg::SpawnMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    obj_type = ReadString();
    mass = ReadDouble();
    position = ReadVector3();
    velocity = ReadVector3();
    orientation = ReadVector4();
    thrust = ReadVector3();
    radius = ReadDouble();
}

SpawnMsg::~SpawnMsg()
{
    free(obj_type);
}

ScanResultMsg::ScanResultMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    obj_type = ReadString();
    mass = ReadDouble();
    position = ReadVector3();
    velocity = ReadVector3();
    orientation = ReadVector4();
    thrust = ReadVector3();
    radius = ReadDouble();
    data = ReadString();
}

ScanResultMsg::~ScanResultMsg()
{
    free(obj_type);
    free(data);
}

ScanQueryMsg::ScanQueryMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    scan_id = ReadInt64();
    energy = ReadDouble();
    direction = ReadVector3();
}

ScanResponseMsg::ScanResponseMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    scan_id = ReadInt64();
    data = ReadString();
}

ScanResponseMsg::~ScanResponseMsg()
{
    free(data);
}

GoodbyeMsg::GoodbyeMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
}

DirectoryMsg::DirectoryMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    item_type = ReadString();
    item_count = ReadInt64();
    items = (char**)malloc((size_t)(item_count * sizeof(char*)));
    for (int i = 0; i < item_count; i++)
    {
        items[i] = ReadString();
    }
}

DirectoryMsg::~DirectoryMsg()
{
    for (int i = 0; i < item_count; i++)
    {
        free(items[i]);
    }
    free(items);
}

NameMsg::NameMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    name = ReadString();
}

NameMsg::~NameMsg()
{
    free(name);
}

ReadyMsg::ReadyMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    ready = ReadBool();
}

ThrustMsg::ThrustMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

VelocityMsg::VelocityMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

JumpMsg::JumpMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

InfoUpdateMsg::InfoUpdateMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}

RequestUpdateMsg::RequestUpdateMsg(BSONReader* _br, int32_t _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
    throw "NotImplemented";
}
