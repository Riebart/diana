#include "messaging.hpp"
#include "bson.hpp"
#include "MIMOServer.hpp"

// There's some boilerplate stuff that happens for ever send, so here's the bookends that do it.
#define SEND_PROLOGUE(msg_type) BSONWriter bw; bw.push((char*)"MsgType", msg_type); bw.push(server_id); bw.push(client_id);
#define SEND_EPILOGUE() uint8_t* bytes = bw.push_end(); return MIMOServer::socket_write(sock, (char*)bytes, *(int32_t*)bytes);

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
    if (ret == NULL)
    {
        throw "OOM you twat";
    }
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
    bw->push(v->x);
    bw->push(v->y);
    bw->push(v->z);
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
    bw->push(v->w);
    bw->push(v->x);
    bw->push(v->y);
    bw->push(v->z);
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
    int64_t nbytes = MIMOServer::socket_read(sock, (char*)(&message_len), 4);

    char* buf = (char*)malloc(message_len);
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
    if ((el.type != BSONReader::ElementType::Int32) || (strcmp(el.name, "MsgType") != 0))
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

int64_t HelloMsg::send(int sock)
{    
    SEND_PROLOGUE(Hello);
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

int64_t PhysicalPropertiesMsg::send(int sock)
{
    SEND_PROLOGUE(PhysicalProperties);
    bw.push(obj_type);
    bw.push(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push(radius);
    SEND_EPILOGUE();
}

VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t VisualPropertiesMsg::send(int sock)
{
    throw "NotImplemented";
}

VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool(br);
}

int64_t VisualDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE(VisualDataEnable);
    bw.push(enabled);
    SEND_EPILOGUE();
}

VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    enabled = ReadBool(br);
}

int64_t VisualMetaDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE(VisualMetaDataEnable);
    bw.push(enabled);
    SEND_EPILOGUE();
}

VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t VisualMetaDataMsg::send(int sock)
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

int64_t VisualDataMsg::send(int sock)
{
    SEND_PROLOGUE(VisualData);
    bw.push(phys_id);
    bw.push(radius);
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
        comm_msg = ReadString(br);
    }
    else
    {
        comm_msg = NULL;
    }
}

BeamMsg::~BeamMsg()
{
    if (comm_msg != NULL)
    {
        free(comm_msg);
    }
}

int64_t BeamMsg::send(int sock)
{
    SEND_PROLOGUE(Beam);
    PushVector3(&bw, &origin);
    PushVector3(&bw, &velocity);
    PushVector3(&bw, &up);
    bw.push(spread_h);
    bw.push(spread_v);
    bw.push(energy);
    bw.push(beam_type);
    if (strcmp(beam_type, "COMM") == 0)
    {
        bw.push(comm_msg);
    }
    SEND_EPILOGUE();
}

CollisionMsg::CollisionMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    position = ReadVector3(br);
    direction = ReadVector3(br);
    energy = ReadDouble(br);
    ReadString(br, coll_type, 4);
    //! @todo Move these to an enum? It is at the top of physics_hpp, and used in the physics *_init() functions for POs and Beams.
    if (strcmp(coll_type, "COMM") == 0)
    {
        comm_msg = ReadString(br);
    }
    else
    {
        comm_msg = NULL;
    }
}

CollisionMsg::~CollisionMsg()
{
    if (comm_msg != NULL)
    {
        free(comm_msg);
    }
}

void CollisionMsg::set_colltype(char* type)
{
    coll_type[0] = type[0];
    coll_type[1] = type[1];
    coll_type[2] = type[2];
    coll_type[3] = type[3];
    coll_type[4] = type[4];
}

int64_t CollisionMsg::send(int sock)
{
    SEND_PROLOGUE(Collision);
    PushVector3(&bw, &position);
    PushVector3(&bw, &direction);
    bw.push(energy);
    bw.push(coll_type);
    if (strcmp(coll_type, "COMM") == 0)
    {
        bw.push(comm_msg);
    }
    else
    {
        bw.push("");
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

int64_t SpawnMsg::send(int sock)
{
    SEND_PROLOGUE(Spawn);
    bw.push(obj_type);
    bw.push(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push(radius);
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

int64_t ScanResultMsg::send(int sock)
{
    SEND_PROLOGUE(ScanResult);
    bw.push(obj_type);
    bw.push(mass);
    PushVector3(&bw, &position);
    PushVector3(&bw, &velocity);
    PushVector4(&bw, &orientation);
    PushVector3(&bw, &thrust);
    bw.push(radius);
    if (data != NULL)
    {
        bw.push(data);
    }
    else
    {
        bw.push("");
    }
    SEND_EPILOGUE();
}

ScanQueryMsg::ScanQueryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    scan_id = ReadInt64(br);
    energy = ReadDouble(br);
    direction = ReadVector3(br);
}

int64_t ScanQueryMsg::send(int sock)
{
    SEND_PROLOGUE(ScanQuery);
    bw.push(scan_id);
    bw.push(energy);
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

int64_t ScanResponseMsg::send(int sock)
{
    SEND_PROLOGUE(ScanResponse);
    if (data != NULL)
    {
        bw.push(data);
    }
    else
    {
        bw.push("");
    }
    SEND_EPILOGUE();
}

GoodbyeMsg::GoodbyeMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
}

int64_t GoodbyeMsg::send(int sock)
{
    SEND_PROLOGUE(Goodbye);
    SEND_EPILOGUE();
}

DirectoryMsg::DirectoryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    item_type = ReadString(br);
    item_count = ReadInt64(br);
    items = (struct DirectoryItem*)malloc((size_t)(item_count * sizeof(struct DirectoryItem)));
    for (int i = 0; i < item_count; i++)
    {
        items[i].id = ReadInt64(br);
        items[i].name = ReadString(br);
    }
}

DirectoryMsg::~DirectoryMsg()
{
    for (int i = 0; i < item_count; i++)
    {
        free(items[i].name);
    }
    free(items);
    free(item_type);
}

int64_t DirectoryMsg::send(int sock)
{
    SEND_PROLOGUE(Directory);
    bw.push(item_type);
    bw.push(item_count);
    for (int i = 0; i < item_count; i++)
    {
        bw.push(items[i].id);
        bw.push(items[i].name);
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

int64_t NameMsg::send(int sock)
{
    SEND_PROLOGUE(Name);
    bw.push(name);
    SEND_EPILOGUE();
}

ReadyMsg::ReadyMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    ready = ReadBool(br);
}

int64_t ReadyMsg::send(int sock)
{
    SEND_PROLOGUE(Ready);
    bw.push(ready);
    SEND_EPILOGUE();
}

ThrustMsg::ThrustMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t ThrustMsg::send(int sock)
{
    throw "NotImplemented";
}

VelocityMsg::VelocityMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t VelocityMsg::send(int sock)
{
    throw "NotImplemented";
}

JumpMsg::JumpMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t JumpMsg::send(int sock)
{
    throw "NotImplemented";
}

InfoUpdateMsg::InfoUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t InfoUpdateMsg::send(int sock)
{
    throw "NotImplemented";
}

RequestUpdateMsg::RequestUpdateMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t RequestUpdateMsg::send(int sock)
{
    throw "NotImplemented";
}
