#include "messaging.hpp"
#include "bson.hpp"
#include "MIMOServer.hpp"
#include <limits>
#include <functional>

// Send a single element named by variable name to the BSONWriter. If the specced value is false, push a NOP.
#define SEND_ELEMENT(var) if (specced[spec_check++]) { bw.push(var); } else { bw.push(); }
// Send a 3D vector to the writer.
#define SEND_VECTOR3(var) SEND_ELEMENT(var.x) SEND_ELEMENT(var.y) SEND_ELEMENT(var.z)
// Send a 4D vector to the writer.
#define SEND_VECTOR4(var) SEND_ELEMENT(var.w) SEND_ELEMENT(var.x) SEND_ELEMENT(var.y) SEND_ELEMENT(var.z)
// Prepare the necessary writers and index variables for writing elements.
#define SEND_PROLOGUE() BSONWriter bw; bw.push((char*)"", (int32_t)msg_type); int spec_check = 0; SEND_ELEMENT(server_id); SEND_ELEMENT(client_id);
// Finish off by pushing an end, and writing the bytes to the socket.
#define SEND_EPILOGUE() uint8_t* bytes = bw.push_end(); return MIMOServer::socket_write(sock, (char*)bytes, *(int32_t*)bytes);

//! @todo Move these to be static in the source file, so they aren't build for every message read. Leave that until later, because premature optimization=bad.
// Read a single value of the specified type from the BSONReader, and assign it to the variable specified.
#define READ_ELEMENT(var, val) [this, &el](int i){ this->var = val; this->specced[i] = true; },
// Insert arbitrary code into the lambda to operate on the lement, and modify the object.
#define READ_ELEMENT_IP(call) [this, &el](int i){ call; this->specced[i] = true; },
// Create a lambda reader for a 3D vector from the BSONReader, element by element.
#define READ_VECTOR3(var, val) READ_ELEMENT(var.x, val) READ_ELEMENT(var.y, val) READ_ELEMENT(var.z, val) 
// Create a lambda reader for a 4D vector from the BSONReader, element by element.
#define READ_VECTOR4(var, val) READ_ELEMENT(var.w, val) READ_ELEMENT(var.x, val) READ_ELEMENT(var.y, val) READ_ELEMENT(var.z, val) 
// Perform prologue setup, and boilerplate stuff, including keeping track of the number of elements, and other such.
#define READ_PROLOGUE(num_el_lit) num_el = 2 + num_el_lit; \
    struct BSONReader::Element el = br->get_next_element(); \
    specced = (bool*)calloc(num_el, sizeof(bool)); \
    if (specced == NULL) { throw "OOM"; }\
    std::function<void(int)> fps[num_el_lit + 2 + 1] = { \
        READ_ELEMENT(server_id, el.i64_val) READ_ELEMENT(client_id, el.i64_val)
// Begin the reading of objects from the BSONReader, calling the lambdas as we go, calling the 'last' lambda for every element that has an index too large.
#define READ_BEGIN() [](int i){}}; while (el.type != BSONReader::ElementType::NoMoreData) { \
    if ((el.name[0] > 0) && (el.name[0] <= num_el)) { fps[el.name[0] - 1](el.name[0] - 1); } el = br->get_next_element(); }

//! @todo Should we set doubles to a NaN value as a sentinel for those unset from the message?
const double dbl_nan = std::numeric_limits<double>::quiet_NaN();
const struct Vector3 v3_nan = { dbl_nan, dbl_nan, dbl_nan };
const struct Vector4 v4_nan = { dbl_nan, dbl_nan, dbl_nan, dbl_nan };

//! @todo Support things other than C strings
void ReadString(BSONReader::Element& el, char* dst, int32_t dstlen)
{
    //struct BSONReader::Element el;
    //el = br->get_next_element();
    int32_t copy_len = (el.str_len <= dstlen ? el.str_len : dstlen);
    memcpy(dst, el.str_val, copy_len);
    dst[dstlen - 1] = 0;
}

//! @todo Support things other than C strings
char* ReadString(BSONReader::Element& el)
{
    // Because we might actually be copying from the binary array segment, recall that Python's
    // bson is poorly behaved for strings, so check for, and if necessary, add a 0x00
    // Note that the str_len (if legit), includes the length of the string including the terminating 0x00
    bool null_missing = (el.str_val[el.str_len - 1] != 0);
    char* ret = (char*)malloc(el.str_len + (null_missing ? 1 : 0));
    if (ret == NULL)
    {
        throw "OOM you twat";
    }
    memcpy(ret, el.str_val, el.str_len);
    if (null_missing)
    {
        ret[el.str_len] = 0;
    }
    return ret;
}

BSONMessage::BSONMessage(BSONReader* _br, MessageType _msg_type)
{
    // Note that we don't have to delete the BSONReader, since it's
    // being allocated (probably on the stack) outside of this class.
    this->br = _br;
    this->msg_type = _msg_type;
    server_id = -1;
    client_id = -1;
    specced = NULL;
}

BSONMessage::~BSONMessage()
{
    free(specced);
}

int BSONMessage::spec_all(bool spec)
{
    for (int i = 0; i < num_el; i++)
    {
        specced[i] = spec;
    }
    return num_el;
}

bool BSONMessage::all_specced(int start_index, int stop_index)
{
    if (stop_index == -1)
    {
        stop_index = num_el - 1;
    }
    bool ret = true;
    for (int i = start_index; i <= stop_index; i++)
    {
        ret &= specced[i];
    }
    return ret;
}

BSONMessage* BSONMessage::ReadMessage(int sock)
{
    // First start by reading the whole BSON message
    int32_t message_len = 0;
    int64_t nbytes = MIMOServer::socket_read(sock, (char*)(&message_len), 4);

    //! @todo Do the right thing, and check that nbytes matches what it should.

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
    // The element should be an int32 (type 16), and a name of "".
    // Switch on the value to hand off further parsing.
    if ((el.type != BSONReader::ElementType::Int32) || (strcmp(el.name, "") != 0))
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

#define HELLO_MSG_LEN 0
HelloMsg::HelloMsg()
{
    msg_type = Hello;
    num_el = 2 + HELLO_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

HelloMsg::HelloMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(HELLO_MSG_LEN)
        READ_BEGIN();
}

int64_t HelloMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_EPILOGUE();
}

#define PHYSICALPROPERTIES_MSG_LEN 1 * 3 + 3 * 3 + 4
PhysicalPropertiesMsg::PhysicalPropertiesMsg()
{
    msg_type = PhysicalProperties;
    num_el = 2 + PHYSICALPROPERTIES_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

PhysicalPropertiesMsg::PhysicalPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = NULL;
    READ_PROLOGUE(PHYSICALPROPERTIES_MSG_LEN)
        READ_ELEMENT(obj_type, ReadString(el))
        READ_ELEMENT(mass, el.dbl_val)
        READ_VECTOR3(position, el.dbl_val)
        READ_VECTOR3(velocity, el.dbl_val)
        READ_VECTOR4(orientation, el.dbl_val)
        READ_VECTOR3(thrust, el.dbl_val)
        READ_ELEMENT(radius, el.dbl_val)
        READ_BEGIN();
}

PhysicalPropertiesMsg::~PhysicalPropertiesMsg()
{
    free(obj_type);
}

int64_t PhysicalPropertiesMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(obj_type);
    SEND_ELEMENT(mass);
    SEND_VECTOR3(position);
    SEND_VECTOR3(velocity);
    SEND_VECTOR4(orientation);
    SEND_VECTOR3(thrust);
    SEND_ELEMENT(radius);
    SEND_EPILOGUE();
}

#define VISUALPROPERTIES_MSG_LEN 
VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t VisualPropertiesMsg::send(int sock)
{
    throw "NotImplemented";
}

#define VISUALDATAENABLE_MSG_LEN 1
VisualDataEnableMsg::VisualDataEnableMsg()
{
    msg_type = VisualDataEnable;
    num_el = 2 + VISUALDATAENABLE_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(VISUALDATAENABLE_MSG_LEN)
        READ_ELEMENT(enabled, el.bln_val)
        READ_BEGIN()
}

int64_t VisualDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(enabled);
    SEND_EPILOGUE();
}

#define VISUALMETADATAENABLE_MSG_LEN 1
VisualMetaDataEnableMsg::VisualMetaDataEnableMsg()
{
    msg_type = VisualDataEnable;
    num_el = 2 + VISUALMETADATAENABLE_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(VISUALMETADATAENABLE_MSG_LEN)
        READ_ELEMENT(enabled, el.bln_val)
        READ_BEGIN()
}

int64_t VisualMetaDataEnableMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(enabled);
    SEND_EPILOGUE();
}

#define VISUALMETADATA_MSG_LEN
VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
}

int64_t VisualMetaDataMsg::send(int sock)
{
    throw "NotImplemented";
}

#define VISUALDATA_MSG_LEN 1 + 1 + 3 + 4
VisualDataMsg::VisualDataMsg()
{
    msg_type = VisualData;
    num_el = 2 + VISUALDATA_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

VisualDataMsg::VisualDataMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(VISUALDATA_MSG_LEN)
        READ_ELEMENT(phys_id, el.i64_val)
        READ_ELEMENT(radius, el.dbl_val)
        READ_VECTOR3(position, el.dbl_val)
        READ_VECTOR4(orientation, el.dbl_val)
        READ_BEGIN()
}

int64_t VisualDataMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(phys_id);
    SEND_ELEMENT(radius);
    SEND_VECTOR3(position);
    SEND_VECTOR4(orientation);
    SEND_EPILOGUE();
}

#define BEAM_MSG_LEN 3 * 3 + 5 * 1
BeamMsg::BeamMsg()
{
    msg_type = Beam;
    num_el = 2 + BEAM_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

BeamMsg::BeamMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    comm_msg = NULL;
    READ_PROLOGUE(BEAM_MSG_LEN)
        READ_VECTOR3(origin, el.dbl_val)
        READ_VECTOR3(velocity, el.dbl_val)
        READ_VECTOR3(up, el.dbl_val)
        READ_ELEMENT(spread_h, el.dbl_val)
        READ_ELEMENT(spread_v, el.dbl_val)
        READ_ELEMENT(energy, el.dbl_val)
        READ_ELEMENT_IP(ReadString(el, this->beam_type, 5)) //! @todo Move these to an enum?
        READ_ELEMENT(comm_msg, ReadString(el))
        READ_BEGIN()
}

BeamMsg::~BeamMsg()
{
    free(comm_msg);
}

int64_t BeamMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_VECTOR3(origin);
    SEND_VECTOR3(velocity);
    SEND_VECTOR3(up);
    SEND_ELEMENT(spread_h);
    SEND_ELEMENT(spread_v);
    SEND_ELEMENT(energy);
    SEND_ELEMENT(beam_type);
    SEND_ELEMENT(comm_msg);
    SEND_EPILOGUE();
}

#define COLLISION_MSG_LEN 3 * 2 + 1 * 3
CollisionMsg::CollisionMsg()
{
    msg_type = Collision;
    num_el = 2 + COLLISION_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

CollisionMsg::CollisionMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    comm_msg = NULL;
    READ_PROLOGUE(COLLISION_MSG_LEN)
        READ_VECTOR3(position, el.dbl_val)
        READ_VECTOR3(direction, el.dbl_val)
        READ_ELEMENT(energy, el.dbl_val)
        READ_ELEMENT_IP(ReadString(el, this->coll_type, 5))
        READ_ELEMENT(comm_msg, ReadString(el))
        READ_BEGIN()
}

CollisionMsg::~CollisionMsg()
{
    free(comm_msg);
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
    SEND_PROLOGUE();
    SEND_VECTOR3(position);
    SEND_VECTOR3(direction);
    SEND_ELEMENT(energy);
    SEND_ELEMENT(coll_type);
    SEND_ELEMENT(comm_msg);
    SEND_EPILOGUE();
}

#define SPAWN_MSG_LEN 3 * 3 + 4 + 4 * 1
SpawnMsg::SpawnMsg()
{
    msg_type = Spawn;
    num_el = 2 + SPAWN_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}
SpawnMsg::SpawnMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = NULL;
    READ_PROLOGUE(SPAWN_MSG_LEN)
        READ_ELEMENT(is_smart, el.bln_val)
        READ_ELEMENT(obj_type, ReadString(el))
        READ_ELEMENT(mass, el.dbl_val)
        READ_VECTOR3(position, el.dbl_val)
        READ_VECTOR3(velocity, el.dbl_val)
        READ_VECTOR4(orientation, el.dbl_val)
        READ_VECTOR3(thrust, el.dbl_val)
        READ_ELEMENT(radius, el.dbl_val)
        READ_BEGIN()
}

SpawnMsg::~SpawnMsg()
{
    free(obj_type);
}

int64_t SpawnMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(is_smart);
    SEND_ELEMENT(obj_type);
    SEND_ELEMENT(mass);
    SEND_VECTOR3(position);
    SEND_VECTOR3(velocity);
    SEND_VECTOR4(orientation);
    SEND_VECTOR3(thrust);
    SEND_ELEMENT(radius);
    SEND_EPILOGUE();
}

#define SCANRESULT_MSG_LEN 3 * 3 + 4 + 4 * 1
ScanResultMsg::ScanResultMsg()
{
    msg_type = ScanResult;
    num_el = 2 + SCANRESULT_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

ScanResultMsg::ScanResultMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    obj_type = NULL;
    data = NULL;
    READ_PROLOGUE(SCANRESULT_MSG_LEN)
        READ_ELEMENT(obj_type, ReadString(el))
        READ_ELEMENT(mass, el.dbl_val)
        READ_VECTOR3(position, el.dbl_val)
        READ_VECTOR3(velocity, el.dbl_val)
        READ_VECTOR4(orientation, el.dbl_val)
        READ_VECTOR3(thrust, el.dbl_val)
        READ_ELEMENT(radius, el.dbl_val)
        READ_ELEMENT(data, ReadString(el))
        READ_BEGIN()
}

ScanResultMsg::~ScanResultMsg()
{
    free(obj_type);
    free(data);
}

int64_t ScanResultMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(obj_type);
    SEND_ELEMENT(mass);
    SEND_VECTOR3(position);
    SEND_VECTOR3(velocity);
    SEND_VECTOR4(orientation);
    SEND_VECTOR3(thrust);
    SEND_ELEMENT(radius);
    SEND_ELEMENT(data);
    SEND_EPILOGUE();
}

#define SCANQUERY_MSG_LEN 1 + 1 + 3
ScanQueryMsg::ScanQueryMsg()
{
    msg_type = ScanQuery;
    num_el = 2 + SCANQUERY_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

ScanQueryMsg::ScanQueryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(SCANQUERY_MSG_LEN)
        READ_ELEMENT(scan_id, el.i64_val)
        READ_ELEMENT(energy, el.dbl_val)
        READ_VECTOR3(direction, el.dbl_val)
        READ_BEGIN()
}

int64_t ScanQueryMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(scan_id);
    SEND_ELEMENT(energy);
    SEND_VECTOR3(direction);
    SEND_EPILOGUE();
}

#define SCANRESPONSE_MSG_LEN 1 + 1
ScanResponseMsg::ScanResponseMsg()
{
    msg_type = ScanResponse;
    num_el = 2 + SCANRESPONSE_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

ScanResponseMsg::ScanResponseMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    data = NULL;
    READ_PROLOGUE(SCANRESPONSE_MSG_LEN)
        READ_ELEMENT(scan_id, el.i64_val)
        READ_ELEMENT(data, ReadString(el))
        READ_BEGIN()
}

ScanResponseMsg::~ScanResponseMsg()
{
    free(data);
}

int64_t ScanResponseMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(data);
    SEND_EPILOGUE();
}

#define GOODBYE_MSG_LEN 0
GoodbyeMsg::GoodbyeMsg()
{
    msg_type = Goodbye;
    num_el = 2 + GOODBYE_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

GoodbyeMsg::GoodbyeMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
    READ_PROLOGUE(GOODBYE_MSG_LEN)
        READ_BEGIN()
}

int64_t GoodbyeMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_EPILOGUE();
}

#define DIRECTORY_MSG_LEN 4
DirectoryMsg::DirectoryMsg()
{
    msg_type = Directory;
    num_el = 2 + DIRECTORY_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

DirectoryMsg::DirectoryMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    throw "NotImplemented";
    item_type = NULL;
    READ_PROLOGUE(DIRECTORY_MSG_LEN)
        READ_ELEMENT(item_type, ReadString(el))
        READ_ELEMENT(item_count, el.i64_val; \
        this->items = (struct DirectoryItem*)malloc((size_t)(this->item_count * sizeof(struct DirectoryItem)));)
        // Items 5 and 6 are, respetively, BSON arrays of ids and names that will fill in the items array.
        READ_ELEMENT_IP(;)
        READ_ELEMENT_IP(;)
        READ_BEGIN()
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
    SEND_PROLOGUE();
    SEND_ELEMENT(item_type);
    SEND_ELEMENT(item_count);
    bw.push_array();
    //for (int i = 0; i < item_count; i++)
    //{
    //    bw.push(items[i].id);
    //}
    bw.push_end();
    bw.push_array();
    //for (int i = 0; i < item_count; i++)
    //{
    //    bw.push(items[i].name);
    //}
    bw.push_end();
    
    SEND_EPILOGUE();
}

#define NAME_MSG_LEN 1
NameMsg::NameMsg()
{
    msg_type = Name;
    num_el = 2 + NAME_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

NameMsg::NameMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    name = NULL;
    READ_PROLOGUE(NAME_MSG_LEN)
        READ_ELEMENT(name, ReadString(el))
        READ_BEGIN()
}

NameMsg::~NameMsg()
{
    free(name);
}

int64_t NameMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(name);
    SEND_EPILOGUE();
}

#define READY_MSG_LEN 1
ReadyMsg::ReadyMsg()
{
    msg_type = Ready;
    num_el = 2 + READY_MSG_LEN;
    specced = (bool*)calloc(num_el, sizeof(bool));
}

ReadyMsg::ReadyMsg(BSONReader* _br, MessageType _msg_type) : BSONMessage(_br, _msg_type)
{
    READ_PROLOGUE(READY_MSG_LEN)
        READ_ELEMENT(ready, el.bln_val)
        READ_BEGIN()
}

int64_t ReadyMsg::send(int sock)
{
    SEND_PROLOGUE();
    SEND_ELEMENT(ready);
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
