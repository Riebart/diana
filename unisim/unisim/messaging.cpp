#include <limits>
#include <functional>
#include <stdexcept>

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
#define READER_LAMBDA(var, val, type) [](uint32_t i, const BSONMessage* msg, struct BSONReader::Element& el) {((type*)msg)->var = val; msg->specced[i] = true; }
    // Run a particular piece of code on the element and message object.
#define READER_LAMBDA_IP(call) [](uint32_t i, const BSONMessage* msg, struct BSONReader::Element& el) { call; msg->specced[i] = true; }
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

    // Static lambdas for reading IDs from the message, to supplement the 
    static read_lambda id_readers[2] = { 
        READER_LAMBDA(server_id, el.i64_val, BSONMessage),
        READER_LAMBDA(client_id, el.i64_val, BSONMessage) };

    BSONMessage::BSONMessage(BSONReader* _br, uint32_t _num_el, const read_lambda* handlers, BSONMessage::MessageType _msg_type)
    {
        // Note that we don't have to delete the BSONReader, since it's
        // being allocated (probably on the stack) outside of this class.
        this->br = _br;
        this->msg_type = _msg_type;
        server_id = -1;
        client_id = -1;

        this->num_el = _num_el + 2;
        specced = new bool[num_el]();

        if (br != NULL)
        {
            int8_t i;
            struct BSONReader::Element el = br->get_next_element();;
            while (el.type != BSONReader::ElementType::NoMoreData)
            {
                i = el.name[0] - 1;
                if ((i >= 0) && ((uint32_t)i < num_el))
                {
                    (i < 2 ? id_readers[i](i, this, el) : handlers[i - 2](i, this, el));
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

    const read_lambda* handlers()
    {
        return NULL;
    }

    // ================================================================================
    // ================================================================================

    static const read_lambda* HelloMsg_handlers = NULL;
    const read_lambda* HelloMsg::handlers()
    {
        return HelloMsg_handlers;
    }
    
    int64_t HelloMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static const read_lambda PhysicalPropertiesMsg_handlers[] = {
        READER_LAMBDA(obj_type, ReadString(el), PhysicalPropertiesMsg),
        READER_LAMBDA(mass, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(position, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(velocity, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA4(orientation, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA3(thrust, el.dbl_val, PhysicalPropertiesMsg),
        READER_LAMBDA(radius, el.dbl_val, PhysicalPropertiesMsg)
    };
    const read_lambda* PhysicalPropertiesMsg::handlers()
    {
        return PhysicalPropertiesMsg_handlers;
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

    static const read_lambda* VisualPropertiesMsg_handlers = NULL;
    const read_lambda* VisualPropertiesMsg::handlers()
    {
        throw std::runtime_error("VisualPropertiesMsg::NotImplemented");
        return VisualPropertiesMsg_handlers;
    }

    int64_t VisualPropertiesMsg::send(sock_t sock)
    {
        throw std::runtime_error("VisualPropertiesMsg::NotImplemented");
    }

    // ================================================================================
    // ================================================================================

    static const read_lambda VisualDataEnableMsg_handlers[] = {
        READER_LAMBDA(enabled, el.bln_val, VisualDataEnableMsg)
    };
    const read_lambda* VisualDataEnableMsg::handlers()
    {
        return VisualDataEnableMsg_handlers;
    }

    int64_t VisualDataEnableMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(enabled);
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static const read_lambda VisualMetaDataEnableMsg_handlers[] = {
        READER_LAMBDA(enabled, el.bln_val, VisualMetaDataEnableMsg)
    };
    const read_lambda* VisualMetaDataEnableMsg::handlers()
    {
        return VisualMetaDataEnableMsg_handlers;
    }

    int64_t VisualMetaDataEnableMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(enabled);
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static read_lambda* VisualMetaDataMsg_handlers = NULL;
    const read_lambda* VisualMetaDataMsg::handlers()
    {
        throw std::runtime_error("VisualMetaDataMsg::NotImplemented");
        return VisualMetaDataMsg_handlers;
    }

    int64_t VisualMetaDataMsg::send(sock_t sock)
    {
        throw std::runtime_error("VisualMetaDataMsg::NotImplemented");
    }

    // ================================================================================
    // ================================================================================

    static read_lambda VisualDataMsg_handlers[] = {
        READER_LAMBDA(phys_id, el.i64_val, VisualDataMsg),
        READER_LAMBDA(radius, el.dbl_val, VisualDataMsg),
        READER_LAMBDA3(position, el.dbl_val, VisualDataMsg),
        READER_LAMBDA4(orientation, el.dbl_val, VisualDataMsg)
    };
    const read_lambda* VisualDataMsg::handlers()
    {
        return VisualDataMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda BeamMsg_handlers[] = {
        READER_LAMBDA3(origin, el.dbl_val, BeamMsg),
        READER_LAMBDA3(velocity, el.dbl_val, BeamMsg),
        READER_LAMBDA3(up, el.dbl_val, BeamMsg),
        READER_LAMBDA(spread_h, el.dbl_val, BeamMsg),
        READER_LAMBDA(spread_v, el.dbl_val, BeamMsg),
        READER_LAMBDA(energy, el.dbl_val, BeamMsg),
        READER_LAMBDA_IP(ReadString(el, ((BeamMsg*)msg)->beam_type, 5)),
        READER_LAMBDA(comm_msg, ReadString(el), BeamMsg)
    };
    const read_lambda* BeamMsg::handlers()
    {
        return BeamMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda CollisionMsg_handlers[] = {
        READER_LAMBDA3(position, el.dbl_val, CollisionMsg),
        READER_LAMBDA3(direction, el.dbl_val, CollisionMsg),
        READER_LAMBDA(energy, el.dbl_val, CollisionMsg),
        READER_LAMBDA_IP(ReadString(el, ((CollisionMsg*)msg)->coll_type, 5)),
        READER_LAMBDA(comm_msg, ReadString(el), CollisionMsg)
    };
    const read_lambda* CollisionMsg::handlers()
    {
        return CollisionMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda SpawnMsg_handlers[] = {
        READER_LAMBDA(is_smart, el.bln_val, SpawnMsg),
        READER_LAMBDA(obj_type, ReadString(el), SpawnMsg),
        READER_LAMBDA(mass, el.dbl_val, SpawnMsg),
        READER_LAMBDA3(position, el.dbl_val, SpawnMsg),
        READER_LAMBDA3(velocity, el.dbl_val, SpawnMsg),
        READER_LAMBDA4(orientation, el.dbl_val, SpawnMsg),
        READER_LAMBDA3(thrust, el.dbl_val, SpawnMsg),
        READER_LAMBDA(radius, el.dbl_val, SpawnMsg)
    };
    const read_lambda* SpawnMsg::handlers()
    {
        return SpawnMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda ScanResultMsg_handlers[] = {
        READER_LAMBDA(obj_type, ReadString(el), ScanResultMsg),
        READER_LAMBDA(mass, el.dbl_val, ScanResultMsg),
        READER_LAMBDA3(position, el.dbl_val, ScanResultMsg),
        READER_LAMBDA3(velocity, el.dbl_val, ScanResultMsg),
        READER_LAMBDA4(orientation, el.dbl_val, ScanResultMsg),
        READER_LAMBDA3(thrust, el.dbl_val, ScanResultMsg),
        READER_LAMBDA(radius, el.dbl_val, ScanResultMsg),
        READER_LAMBDA(data, ReadString(el), ScanResultMsg)
    };
    const read_lambda* ScanResultMsg::handlers()
    {
        return ScanResultMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda ScanQueryMsg_handlers[] = {
        READER_LAMBDA(scan_id, el.i64_val, ScanQueryMsg),
        READER_LAMBDA(energy, el.dbl_val, ScanQueryMsg),
        READER_LAMBDA3(direction, el.dbl_val, ScanQueryMsg)
    };
    const read_lambda* ScanQueryMsg::handlers()
    {
        return ScanQueryMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda ScanResponseMsg_handlers[] = {
        READER_LAMBDA(scan_id, el.i64_val, ScanResponseMsg),
        READER_LAMBDA(data, ReadString(el), ScanResponseMsg)
    };
    const read_lambda* ScanResponseMsg::handlers()
    {
        return ScanResponseMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda* GoodbyeMsg_handlers = NULL;
    const read_lambda* GoodbyeMsg::handlers()
    {
        return GoodbyeMsg_handlers;
    }
    
    int64_t GoodbyeMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static read_lambda* DirectoryMsg_handlers = NULL;
    const read_lambda* DirectoryMsg::handlers()
    {
        throw std::runtime_error("DirectoryMsg::NotImplemented");
        return DirectoryMsg_handlers;
    }

    //DirectoryMsg::DirectoryMsg(BSONReader* _br) : BSONMessage(_br, DIRECTORY_MSG_LEN)
    //{
    //    throw "NotImplemented";
    //    item_type = NULL;
    //    READ_PROLOGUE(DIRECTORY_MSG_LEN)
    //        READ_ELEMENT(item_type, ReadString(el))
    //        READ_ELEMENT(item_count, el.i64_val; \
    //            this->items = (struct DirectoryItem*)malloc((size_t)(this->item_count * sizeof(struct DirectoryItem)));)
    //        // Items 5 and 6 are, respetively, BSON arrays of ids and names that will fill in the items array.
    //        READ_ELEMENT_IP(;)
    //        READ_ELEMENT_IP(;)
    //        READ_BEGIN()
    //}

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

    // ================================================================================
    // ================================================================================

    static read_lambda NameMsg_handlers[] = {
        READER_LAMBDA(name, ReadString(el), NameMsg)
    };
    const read_lambda* NameMsg::handlers()
    {
        return NameMsg_handlers;
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

    // ================================================================================
    // ================================================================================

    static read_lambda ReadyMsg_handlers[] = {
        READER_LAMBDA(ready, el.bln_val, ReadyMsg)
    };
    const read_lambda* ReadyMsg::handlers()
    {
        return ReadyMsg_handlers;
    }

    int64_t ReadyMsg::send(sock_t sock)
    {
        SEND_PROLOGUE();
        SEND_ELEMENT(ready);
        SEND_EPILOGUE();
    }

    // ================================================================================
    // ================================================================================

    static read_lambda ThrustMsg_handlers[] = {
        READER_LAMBDA3(thrust, el.dbl_val, ThrustMsg)
    };
    const read_lambda* ThrustMsg::handlers()
    {
        return ThrustMsg_handlers;
    }

    int64_t ThrustMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    // ================================================================================
    // ================================================================================

    static read_lambda VelocityMsg_handlers[] = {
        READER_LAMBDA3(velocity, el.dbl_val, VelocityMsg)
    };
    const read_lambda* VelocityMsg::handlers()
    {
        return VelocityMsg_handlers;
    }

    int64_t VelocityMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    // ================================================================================
    // ================================================================================

    static read_lambda JumpMsg_handlers[] = {
        READER_LAMBDA3(destination, el.dbl_val, JumpMsg)
    };
    const read_lambda* JumpMsg::handlers()
    {
        return JumpMsg_handlers;
    }

    int64_t JumpMsg::send(sock_t sock)
    {
        throw "NotImplemented";
    }

    // ================================================================================
    // ================================================================================

    static read_lambda* InfoUpdateMsg_handlers = NULL;
    const read_lambda* InfoUpdateMsg::handlers()
    {
        throw std::runtime_error("InfoUpdateMsg::NotImplemented");
        return InfoUpdateMsg_handlers;
    }

    int64_t InfoUpdateMsg::send(sock_t sock)
    {
        throw std::runtime_error("InfoUpdateMsg::NotImplemented");
    }

    // ================================================================================
    // ================================================================================

    static read_lambda* RequestUpdateMsg_handlers = NULL;
    const read_lambda* RequestUpdateMsg::handlers()
    {
        throw std::runtime_error("RequestUpdateMsg::NotImplemented");
        return RequestUpdateMsg_handlers;
    }

    int64_t RequestUpdateMsg::send(sock_t sock)
    {
        throw std::runtime_error("RequestUpdateMsg::NotImplemented");
    }
}
