#include "messaging.hpp"
#include "bson.hpp"
#include "MIMOServer.hpp"

// There's some boilerplate stuff that happens for ever send, so here's the bookends that do it.
#define SEND_PROLOGUE() BSONWriter bw; bw.push_int64(server_id); bw.push_int64(client_id);
#define SEND_EPILOGUE() uint8_t* bytes = bw.push_end(); return MIMOServer::socket_write(sock, bytes, *(int32_t*)bytes);

double ReadDouble(BSONReader* br)
{
    return br->get_next_element().dbl_val;
}

bool ReadBool(BSONReader* br)
{
    return br->get_next_element().bln_val;
}

int32_t ReadInt32(BSONReader* br)
{
    return br->get_next_element().i32_val;
}

int64_t ReadInt64(BSONReader* br)
{
    return br->get_next_element().i64_val;
}

//! @todo Support things other than C strings
void ReadString(BSONReader* br, char* dst, int32_t dstlen)
{
    struct BSONReader::Element el;
    el = br->get_next_element();
    int32_t copy_len = (el.str_len <= dstlen ? el.str_len : dstlen);
    memcpy(dst, el.str_val, copy_len);
}

//! @todo Support things other than C strings
char* ReadString(BSONReader* br)
{
    struct BSONReader::Element el;
    el = br->get_next_element();
    char* ret = (char*)malloc(el.str_len);
    memcpy(ret, el.str_val, el.str_len);
    return ret;
}

struct Vector3 ReadVector3(BSONReader* br)
{
    return Vector3{
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val
    };
}

void PushVector3(BSONWriter* bw, struct Vector3* v)
{
    bw->push_double(v->x);
    bw->push_double(v->y);
    bw->push_double(v->z);
}

struct Vector4 ReadVector4(BSONReader* br)
{
    return Vector4{
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val,
        br->get_next_element().dbl_val
    };
}

void PushVector4(BSONWriter* bw, struct Vector4* v)
{
    bw->push_double(v->w);
    bw->push_double(v->x);
    bw->push_double(v->y);
    bw->push_double(v->z);
}

BSONMessage::BSONMessage(BSONReader* _br, MessageType _msg_type)
{
    // Note that we don't have to delete the BSONReader, since it's
    // being allocated (probably on the stack) outside of this class.
    this->br = _br;
    this->msg_type = _msg_type;
    server_id = ReadInt64(_br);
    client_id = ReadInt64(_br);
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

size_t HelloMsg::send(int sock)
{    
    SEND_PROLOGUE();
    SEND_EPILOGUE();
}

PhysicalPropertiesMsg::PhysicalPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = ReadString(br);
    mass = ReadDouble(br);
    position = ReadVector3(br);
    velocity = ReadVector3(br);
    orientation = ReadVector4(br);
    thrust = ReadVector3(br);
    radius = ReadDouble(br);
}

PhysicalPropertiesMsg::~PhysicalPropertiesMsg()
{
    free(obj_type);
}

size_t PhysicalPropertiesMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(obj_type);
    bw.push_double(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push_double(radius);
    SEND_EPILOGUE();
}

VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t VisualPropertiesMsg::send(int sock)
{
    throw "NotImplemented";
}

VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool(br);
}

size_t VisualDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_bool(enabled);
    SEND_EPILOGUE();
}

VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool(br);
}

size_t VisualMetaDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_bool(enabled);
    SEND_EPILOGUE();
}

VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t VisualMetaDataMsg::send(int sock)
{
    throw "NotImplemented";
}

VisualDataMsg::VisualDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    phys_id = ReadInt64(br);
    radius = ReadDouble(br);
    position = ReadVector3(br);
    orientation = ReadVector4(br);
}

size_t VisualDataMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_int64(phys_id);
    bw.push_double(radius);
    PushVector3(&bw, &position);
    PushVector4(&bw, &orientation);
    SEND_EPILOGUE();
}

BeamMsg::BeamMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    origin = ReadVector3(br);
    velocity = ReadVector3(br);
    up = ReadVector3(br);
    spread_h = ReadDouble(br);
    spread_v = ReadDouble(br);
    energy = ReadDouble(br);
    ReadString(br, beam_type, 5);
    if (strcmp(beam_type, "COMM") == 0)
    {
        msg = ReadString(br);
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

size_t BeamMsg::send(int sock)
{
    SEND_PROLOGUE();
    PushVector3(&bw, &origin);
    PushVector3(&bw, &velocity);
    PushVector3(&bw, &up);
    bw.push_double(spread_h);
    bw.push_double(spread_v);
    bw.push_double(energy);
    bw.push_string(beam_type);
    if (strcmp(beam_type, "COMM") == 0)
    {
        bw.push_string(msg);
    }
    SEND_EPILOGUE();
}

CollisionMsg::CollisionMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    position = ReadVector3(br);
    direction = ReadVector3(br);
    energy = ReadDouble(br);
    ReadString(br, coll_type, 4);
    if (strcmp(coll_type, "COMM") == 0)
    {
        msg = ReadString(br);
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

size_t CollisionMsg::send(int sock)
{
    SEND_PROLOGUE();
    PushVector3(&bw, &position);
    PushVector3(&bw, &direction);
    bw.push_double(energy);
    bw.push_string(coll_type);
    if (strcmp(coll_type, "COMM") == 0)
    {
        bw.push_string(msg);
    }
    SEND_EPILOGUE();
}

SpawnMsg::SpawnMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = ReadString(br);
    mass = ReadDouble(br);
    position = ReadVector3(br);
    velocity = ReadVector3(br);
    orientation = ReadVector4(br);
    thrust = ReadVector3(br);
    radius = ReadDouble(br);
}

SpawnMsg::~SpawnMsg()
{
    free(obj_type);
}

size_t SpawnMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(obj_type);
    bw.push_double(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push_double(radius);
    SEND_EPILOGUE();
}

ScanResultMsg::ScanResultMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = ReadString(br);
    mass = ReadDouble(br);
    position = ReadVector3(br);
    velocity = ReadVector3(br);
    orientation = ReadVector4(br);
    thrust = ReadVector3(br);
    radius = ReadDouble(br);
    data = ReadString(br);
}

ScanResultMsg::~ScanResultMsg()
{
    free(obj_type);
    free(data);
}

size_t ScanResultMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(obj_type);
    bw.push_double(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push_double(radius);
    bw.push_string(data);
    SEND_EPILOGUE();
}

ScanQueryMsg::ScanQueryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    scan_id = ReadInt64(br);
    energy = ReadDouble(br);
    direction = ReadVector3(br);
}

size_t ScanQueryMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_int64(scan_id);
    bw.push_double(energy);
    PushVector3(&bw, &direction);
    SEND_EPILOGUE();
}

ScanResponseMsg::ScanResponseMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    scan_id = ReadInt64(br);
    data = ReadString(br);
}

ScanResponseMsg::~ScanResponseMsg()
{
    free(data);
}

size_t ScanResponseMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(data);
    SEND_EPILOGUE();
}

GoodbyeMsg::GoodbyeMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
}

size_t GoodbyeMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_EPILOGUE();
}

DirectoryMsg::DirectoryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    item_type = ReadString(br);
    item_count = ReadInt64(br);
    items = (char**)malloc((size_t)(item_count * sizeof(char*)));
    for (int i = 0; i < item_count; i++)
    {
        items[i] = ReadString(br);
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

size_t DirectoryMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(item_type);
    bw.push_int64(item_count);
    for (int i = 0; i < item_count; i++)
    {
        bw.push_string(items[i]);
    }
    SEND_EPILOGUE();
}

NameMsg::NameMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    name = ReadString(br);
}

NameMsg::~NameMsg()
{
    free(name);
}

size_t NameMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_string(name);
    SEND_EPILOGUE();
}

ReadyMsg::ReadyMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    ready = ReadBool(br);
}

size_t ReadyMsg::send(int sock)
{
    SEND_PROLOGUE();
    bw.push_bool(ready);
    SEND_EPILOGUE();
}

ThrustMsg::ThrustMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t ThrustMsg::send(int sock)
{
    throw "NotImplemented";
}

VelocityMsg::VelocityMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t VelocityMsg::send(int sock)
{
    throw "NotImplemented";
}

JumpMsg::JumpMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t JumpMsg::send(int sock)
{
    throw "NotImplemented";
}

InfoUpdateMsg::InfoUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t InfoUpdateMsg::send(int sock)
{
    throw "NotImplemented";
}

RequestUpdateMsg::RequestUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

size_t RequestUpdateMsg::send(int sock)
{
    throw "NotImplemented";
}