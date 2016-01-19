#include <limits>
#include <functional>

#include "messaging.hpp"
#include "bson.hpp"

#ifdef UE_NETWORKING
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "SocketTypes.h"

#define SOCKET_WRITE socket_write
#define SOCKET_READ socket_read

namespace Diana
{
    int32 socket_write(FSocket* s, char* srcC, int32_t countS)
    {
        int32 count = (uint32)countS;
        uint8* src = (uint8*)srcC;
        int32 nbytes = 0;
        int32 curbytes;
        bool read_success;
        while (nbytes < count)
        {
            //! @todo Maybe use recvmsg()?
            // Try to wait for all of the data, if possible.
            read_success = s->Send(src + nbytes, count - nbytes, curbytes);
            //UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketSend::Post::(%d,%d,%d)"), count, curbytes, (int32)read_success);
            if (!read_success)
            {
                // Flip the sign of the bytes returnd, as a reminder to check up on errno and errmsg
                nbytes *= -1;
                if (nbytes == 0)
                {
                    nbytes = -1;
                }
                break;
            }
            nbytes += curbytes;
        }

        return nbytes;
    }

    int32 socket_read(FSocket* s, char* dstC, int32_t countS)
    {
        uint32 count = (uint32)countS;
        uint8* dst = (uint8*)dstC;
        uint32 nbytes = 0;
        bool read_success;
        uint32 pending_bytes;

        // See if there's pending data
        read_success = s->HasPendingData(pending_bytes);
        if (!read_success || (pending_bytes < count))
        {
            return 0;
        }
        //UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketRead::PostPendingCheck::(%d,%u,%d)"), count, pending_bytes, (int32)read_success);

        int32 curbytes = 0;
        while (nbytes < count)
        {
            //! @todo Maybe use recvmsg()?
            // Try to wait for all of the data, if possible.
            read_success = s->Recv(dst + nbytes, count - nbytes, curbytes, ESocketReceiveFlags::Type::None);
            //UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketRead::Post::(%d,%d,%d,%d)"), count, nbytes, curbytes, (int32)read_success);
            if (!read_success)
            {
                // Flip the sign of the bytes returnd, as a reminder to check up on errno and errmsg
                nbytes *= -1;
                if (nbytes == 0)
                {
                    nbytes = -1;
                }
                break;
            }
            nbytes += curbytes;
        }

        return nbytes;
    }
}
#else
#include "MIMOServer.hpp"
#define SOCKET_WRITE MIMOServer::socket_write
#define SOCKET_READ MIMOServer::socket_read
#endif

namespace Diana
{
    // Send a single element named by variable name to the BSONWriter. If the specced value is false, push a NOP.
#define SEND_ELEMENT(var) if (specced[spec_check++]) { bw.push(var); } else { bw.push(); }
    // Send a 3D vector to the writer.
#define SEND_VECTOR3(var) SEND_ELEMENT(var.x) SEND_ELEMENT(var.y) SEND_ELEMENT(var.z)
    // Send a 4D vector to the writer.
#define SEND_VECTOR4(var) SEND_ELEMENT(var.w) SEND_ELEMENT(var.x) SEND_ELEMENT(var.y) SEND_ELEMENT(var.z)
    // Prepare the necessary writers and index variables for writing elements.
#define SEND_PROLOGUE() BSONWriter bw; bw.push((char*)"", (int32_t)msg_type); int spec_check = 0; SEND_ELEMENT(server_id); SEND_ELEMENT(client_id);
    // Finish off by pushing an end, and writing the bytes to the socket.
#define SEND_EPILOGUE() uint8_t* bytes = bw.push_end(); return SOCKET_WRITE(sock, (char*)bytes, *(int32_t*)bytes);

    // Read a single element from the reader, into the specified member variable of a message
    // that is of the specified message type.
#define READER_LAMBDA(var, val, type) [](uint32_t i, BSONMessage* msg, struct BSONReader::Element& el) {((type*)msg)->var = val; msg->specced[i] = true; }
    // Read a 3D vector from the reader, into the specified member variable of a message
    // that is of the specified message type.
#define READER_LAMBDA3(var, val, type) READER_LAMBDA(var.x, val, type), READER_LAMBDA(var.y, val, type), READER_LAMBDA(var.z, val, type)
    // Read a 4D vector from the reader, into the specified member variable of a message
    // that is of the specified message type.
#define READER_LAMBDA4(var, val, type) READER_LAMBDA(var.w, val, type), READER_LAMBDA(var.x, val, type), READER_LAMBDA(var.y, val, type), READER_LAMBDA(var.z, val, type)

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
    if ((el.name[0] > 0) && ((uint32_t)el.name[0] <= num_el)) { fps[el.name[0] - 1](el.name[0] - 1); } el = br->get_next_element(); }

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

    BSONMessage::BSONMessage(BSONReader* _br, uint32_t _num_el)
    {
        // Note that we don't have to delete the BSONReader, since it's
        // being allocated (probably on the stack) outside of this class.
        this->br = _br;
        server_id = -1;
        client_id = -1;

        this->num_el = num_el;
        specced = new bool[num_el + 2]();
    }

    // Static lambdas for reading IDs from the message, to supplement the 
    static std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>
        id_readers[2] = { READER_LAMBDA(server_id, el.i64_val, BSONMessage), READER_LAMBDA(client_id, el.i64_val, BSONMessage) };

    void BSONMessage::ReadElements()
    {
        if (br != NULL)
        {
            int8_t i;
            struct BSONReader::Element el = br->get_next_element();;
            while (el.type != BSONReader::ElementType::NoMoreData)
            {
                i = el.name[0];
                if ((i > 0) && ((uint32_t)i <= num_el))
                {
                    (i < 2 ? id_readers[i](i, this, el) : this->handlers[i](i, this, el));
                }
                el = br->get_next_element();
            }
        }
    }

    BSONMessage::~BSONMessage()
    {
        delete specced;
    }

    int BSONMessage::spec_all(bool spec)
    {
        for (uint32_t i = 0; i < num_el; i++)
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

    BSONMessage* BSONMessage::ReadMessage(sock_t sock)
    {
        // First start by reading the whole BSON message
        int32_t message_len = 0;
        int64_t nbytes = SOCKET_READ(sock, (char*)(&message_len), 4);


        if (nbytes != 4)
        {
            //! @todo Do the right thing, and check that nbytes matches what it should.
        }

        char* buf = (char*)malloc(message_len);
        if (buf == NULL)
        {
            throw "OOM You twat";
        }
        *(int32_t*)buf = message_len;
        nbytes = SOCKET_READ(sock, buf + 4, message_len - 4);

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
                return new HelloMsg(&br);
            case MessageType::PhysicalProperties:
                return new PhysicalPropertiesMsg(&br);
            case MessageType::VisualProperties:
                return new VisualPropertiesMsg(&br);
            case MessageType::VisualDataEnable:
                return new VisualDataEnableMsg(&br);
            case MessageType::VisualMetaDataEnable:
                return new VisualMetaDataEnableMsg(&br);
            case MessageType::VisualMetaData:
                return new VisualMetaDataMsg(&br);
            case MessageType::VisualData:
                return new VisualDataMsg(&br);
            case MessageType::Beam:
                return new BeamMsg(&br);
            case MessageType::Collision:
                return new CollisionMsg(&br);
            case MessageType::Spawn:
                return new SpawnMsg(&br);
            case MessageType::ScanResult:
                return new ScanResultMsg(&br);
            case MessageType::ScanQuery:
                return new ScanQueryMsg(&br);
            case MessageType::ScanResponse:
                return new ScanResponseMsg(&br);
            case MessageType::Goodbye:
                return new GoodbyeMsg(&br);
            case MessageType::Directory:
                return new DirectoryMsg(&br);
            case MessageType::Name:
                return new NameMsg(&br);
            case MessageType::Ready:
                return new ReadyMsg(&br);
            case MessageType::Thrust:
                return new ThrustMsg(&br);
            case MessageType::Velocity:
                return new VelocityMsg(&br);
            case MessageType::Jump:
                return new JumpMsg(&br);
            case MessageType::InfoUpdate:
                return new InfoUpdateMsg(&br);
            case MessageType::RequestUpdate:
                return new RequestUpdateMsg(&br);
            default:
                return NULL;
            }
        }
    }

    // ================================================================================
    // ================================================================================

    HelloMsg::HelloMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        ReadElements();
    }

    int64_t HelloMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>
        physicalproperties_readers[16] = {
        READER_LAMBDA(obj_type, ReadString(el), PhysicalPropertiesMsg),
        READER_LAMBDA(mass, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(position, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(velocity, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA4(orientation, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(thrust, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA(radius, el.dbl_val, PhysicalPropertiesMsg)
    };

    PhysicalPropertiesMsg::PhysicalPropertiesMsg(BSONReader* _br) : BSONMessage(_br, 16)
    {
        handlers = physicalproperties_readers;
        ReadElements();
    }

    PhysicalPropertiesMsg::~PhysicalPropertiesMsg()
    {
        free(obj_type);
        free(spectrum);
    }

    int64_t PhysicalPropertiesMsg::send(sock_t sock)
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

    // ================================================================================
    // ================================================================================

    VisualPropertiesMsg::VisualPropertiesMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t VisualPropertiesMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    // ================================================================================
    // ================================================================================

    static std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>
        visualdataenable_readers[1] = {
        READER_LAMBDA(enabled, el.bln_val, VisualDataEnableMsg)
    };

    VisualDataEnableMsg::VisualDataEnableMsg(BSONReader* _br) : BSONMessage(_br, 1)
    {
        handlers = visualdataenable_readers;
        ReadElements();
    }

    int64_t VisualDataEnableMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(enabled);
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>
        visualmetadataenable_readers[1] = {
        READER_LAMBDA(enabled, el.bln_val, VisualDataEnableMsg)
    };

    VisualMetaDataEnableMsg::VisualMetaDataEnableMsg(BSONReader* _br) : BSONMessage(_br, 1)
    {
        handlers = visualmetadataenable_readers;
        ReadElements();
    }

    int64_t VisualMetaDataEnableMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(enabled);
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>
        visualmetadata_readers[1] = {
        READER_LAMBDA(enabled, el.bln_val, VisualDataEnableMsg)
    };

    VisualMetaDataMsg::VisualMetaDataMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        handlers = visualmetadata_readers;
        throw "NotImplemented";
        ReadElements();
    }

    int64_t VisualMetaDataMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

#define VISUALDATA_MSG_LEN 1 + 1 + 3 + 4
    VisualDataMsg::VisualDataMsg(BSONReader* _br) : BSONMessage(_br, VISUALDATA_MSG_LEN)
    {
        READ_PROLOGUE(VISUALDATA_MSG_LEN)
            READ_ELEMENT(phys_id, el.i64_val)
            READ_ELEMENT(radius, el.dbl_val)
            READ_VECTOR3(position, el.dbl_val)
            READ_VECTOR4(orientation, el.dbl_val)
            READ_BEGIN()
    }

    int64_t VisualDataMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(phys_id);
        SEND_ELEMENT(radius);
        SEND_VECTOR3(position);
        SEND_VECTOR4(orientation);
        SEND_EPILOGUE();
    }

#define BEAM_MSG_LEN 3 * 3 + 5 * 1
    BeamMsg::BeamMsg(BSONReader* _br) : BSONMessage(_br, BEAM_MSG_LEN)
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
        free(spectrum);
    }

    int64_t BeamMsg::send(sock_t sock)
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
    CollisionMsg::CollisionMsg(BSONReader* _br) : BSONMessage(_br, COLLISION_MSG_LEN)
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
        free(spectrum);
    }

    void CollisionMsg::set_colltype(char* type)
    {
        coll_type[0] = type[0];
        coll_type[1] = type[1];
        coll_type[2] = type[2];
        coll_type[3] = type[3];
        coll_type[4] = type[4];
    }

    int64_t CollisionMsg::send(sock_t sock)
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
    SpawnMsg::SpawnMsg(BSONReader* _br) : BSONMessage(_br, SPAWN_MSG_LEN)
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
        free(spectrum);
    }

    int64_t SpawnMsg::send(sock_t sock)
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
    ScanResultMsg::ScanResultMsg(BSONReader* _br) : BSONMessage(_br, SCANRESULT_MSG_LEN)
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

    int64_t ScanResultMsg::send(sock_t sock)
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
    ScanQueryMsg::ScanQueryMsg(BSONReader* _br) : BSONMessage(_br, SCANQUERY_MSG_LEN)
    {
        READ_PROLOGUE(SCANQUERY_MSG_LEN)
            READ_ELEMENT(scan_id, el.i64_val)
            READ_ELEMENT(energy, el.dbl_val)
            READ_VECTOR3(direction, el.dbl_val)
            READ_BEGIN()
    }

    int64_t ScanQueryMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(scan_id);
        SEND_ELEMENT(energy);
        SEND_VECTOR3(direction);
        SEND_EPILOGUE();
    }

    ScanQueryMsg::~ScanQueryMsg()
    {
        free(spectrum);
    }

#define SCANRESPONSE_MSG_LEN 1 + 1
    ScanResponseMsg::ScanResponseMsg(BSONReader* _br) : BSONMessage(_br, SCANRESPONSE_MSG_LEN)
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

    int64_t ScanResponseMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(data);
        SEND_EPILOGUE();
    }

#define GOODBYE_MSG_LEN 0
    GoodbyeMsg::GoodbyeMsg(BSONReader* _br) : BSONMessage(_br, GOODBYE_MSG_LEN)
    {
        // A generic FIN, indicating a smarty is disconnecting, or otherwise leaving.
        READ_PROLOGUE(GOODBYE_MSG_LEN)
            READ_BEGIN()
    }

    int64_t GoodbyeMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_EPILOGUE();
    }

#define DIRECTORY_MSG_LEN 4
    DirectoryMsg::DirectoryMsg(BSONReader* _br) : BSONMessage(_br, DIRECTORY_MSG_LEN)
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

    int64_t DirectoryMsg::send(sock_t sock)
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
    NameMsg::NameMsg(BSONReader* _br) : BSONMessage(_br, NAME_MSG_LEN)
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

    int64_t NameMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(name);
        SEND_EPILOGUE();
    }

#define READY_MSG_LEN 1
    ReadyMsg::ReadyMsg(BSONReader* _br) : BSONMessage(_br, READY_MSG_LEN)
    {
        READ_PROLOGUE(READY_MSG_LEN)
            READ_ELEMENT(ready, el.bln_val)
            READ_BEGIN()
    }

    int64_t ReadyMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(ready);
        SEND_EPILOGUE();
    }

    ThrustMsg::ThrustMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t ThrustMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    VelocityMsg::VelocityMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t VelocityMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    JumpMsg::JumpMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t JumpMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    InfoUpdateMsg::InfoUpdateMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t InfoUpdateMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    RequestUpdateMsg::RequestUpdateMsg(BSONReader* _br) : BSONMessage(_br, 0)
    {
        throw "NotImplemented";
    }

    int64_t RequestUpdateMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }
}
