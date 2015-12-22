#include "messaging.hpp"
#include "bson.hpp"
#include "MIMOServer.hpp"

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

BSONMessage::BSONMessage(BSONReader* _br, MessageType _msg_type)
{
    this->br = _br;
    this->msg_type = _msg_type;
    ReadIDs();
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
        MessageType mt = (MessageType)el.i32_val;
        switch (mt)
        {
        case MessageType::Hello:
            return new HelloMsg(&br, mt);
        case MessageType::PhysicalProperties:
            return new PhysicalPropertiesMsg(&br, mt);
        case MessageType::VisualProperties:
            return new VisualPropertiesMsg(&br, mt);
        case MessageType::VisualDataEnable:
            return new VisualDataEnableMsg(&br, mt);
        case MessageType::VisualMetaDataEnable:
            return new VisualMetaDataEnableMsg(&br, mt);
        case MessageType::VisualMetaData:
            return new VisualMetaDataMsg(&br, mt);
        case MessageType::VisualData:
            return new VisualDataMsg(&br, mt);
        case MessageType::Beam:
            return new BeamMsg(&br, mt);
        case MessageType::Collision:
            return new CollisionMsg(&br, mt);
        case MessageType::Spawn:
            return new SpawnMsg(&br, mt);
        case MessageType::ScanResult:
            return new ScanResultMsg(&br, mt);
        case MessageType::ScanQuery:
            return new ScanQueryMsg(&br, mt);
        case MessageType::ScanResponse:
            return new ScanResponseMsg(&br, mt);
        case MessageType::Goodbye:
            return new GoodbyeMsg(&br, mt);
        case MessageType::Directory:
            return new DirectoryMsg(&br, mt);
        case MessageType::Name:
            return new NameMsg(&br, mt);
        case MessageType::Ready:
            return new ReadyMsg(&br, mt);
        case MessageType::Thrust:
            return new ThrustMsg(&br, mt);
        case MessageType::Velocity:
            return new VelocityMsg(&br, mt);
        case MessageType::Jump:
            return new JumpMsg(&br, mt);
        case MessageType::InfoUpdate:
            return new InfoUpdateMsg(&br, mt);
        case MessageType::RequestUpdate:
            return new RequestUpdateMsg(&br, mt);
        default:
            return NULL;
        }
    }
}

HelloMsg::HelloMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    // This is just a token SYN, for ID establishing on both sides.
}

PhysicalPropertiesMsg::PhysicalPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = ReadString();
    mass = ReadDouble();
    position = ReadVector3();
    velocity = ReadVector3();
    orientation = ReadVector4();
    thrust = ReadVector3();
    radius = ReadDouble();
}

PhysicalPropertiesMsg::~PhysicalPropertiesMsg()
{
    free(obj_type);
}

VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool();
}

VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool();
}

VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

VisualDataMsg::VisualDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    phys_id = ReadInt64();
    radius = ReadDouble();
    position = ReadVector3();
    orientation = ReadVector4();
}

BeamMsg::BeamMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
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

CollisionMsg::CollisionMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
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

SpawnMsg::SpawnMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
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

ScanResultMsg::ScanResultMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
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

ScanQueryMsg::ScanQueryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    scan_id = ReadInt64();
    energy = ReadDouble();
    direction = ReadVector3();
}

ScanResponseMsg::ScanResponseMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    scan_id = ReadInt64();
    data = ReadString();
}

ScanResponseMsg::~ScanResponseMsg()
{
    free(data);
}

GoodbyeMsg::GoodbyeMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
}

DirectoryMsg::DirectoryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
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

NameMsg::NameMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    name = ReadString();
}

NameMsg::~NameMsg()
{
    free(name);
}

ReadyMsg::ReadyMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    ready = ReadBool();
}

ThrustMsg::ThrustMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

VelocityMsg::VelocityMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

JumpMsg::JumpMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

InfoUpdateMsg::InfoUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

RequestUpdateMsg::RequestUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}
