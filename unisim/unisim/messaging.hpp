#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <stdlib.h>
#include <stdint.h>

#include "vector.hpp"

class BSONReader;

// The general superclass of all messages
class BSONMessage
{
public:
    // A enumeration of all message types, and their IDs.
    enum MessageType
    {
        Reservedx00 = 0, Hello = 1, PhysicalProperties = 2, VisualProperties = 3, VisualDataEnable = 4,
        VisualMetaDataEnable = 5, VisualMetaData = 6, VisualData = 7, Beam = 8, Collision = 9,
        Spawn = 10, ScanResult = 11, ScanQuery = 12, ScanResponse = 13, Goodbye = 14,
        Directory = 15, Name = 16, Ready = 17, Thrust = 18, Velocity = 19, Jump = 20, 
        InfoUpdate = 21, RequestUpdate = 22
    };

    // Type of message, stored in the first field, an Int32 field with the name MsgType
    // of every message.
    MessageType msg_type;

    // The referece ID on the 'server' as appropriate for the message
    // For OSIM-Unisim messages, this is a phys_id, for end-user-client to OSIM
    // messages, this is an OSIM ID.
    int64_t server_id;
    
    // The referece ID on the 'client' as appropriate for the message
    // For OSIM-Unisim messages, this is an OSIM ID, and for end-user to OSIM messages
    // this will be an ID that is determined local to the end-user client.
    int64_t client_id;
    
    // Read a BSON message form a socket and return a pointer to a newly allocated object.
    static BSONMessage* ReadMessage(int sock);
    BSONMessage() { }
    virtual ~BSONMessage();
    virtual int enable_all();

protected:
    BSONReader* br;
    BSONMessage(BSONReader* _br, MessageType _msg_type);

    bool* specced;
    int num_el;
};

class HelloMsg : public BSONMessage
{
public:
    HelloMsg();
    HelloMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class PhysicalPropertiesMsg : public BSONMessage
{
public:
    PhysicalPropertiesMsg();
    PhysicalPropertiesMsg(BSONReader* _br, MessageType _msg_type);
    ~PhysicalPropertiesMsg();
    int64_t send(int sock);

    char* obj_type;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class VisualPropertiesMsg : public BSONMessage
{
public:
    VisualPropertiesMsg();
    VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class VisualDataEnableMsg : public BSONMessage
{
public:
    VisualDataEnableMsg();
    VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);

    bool enabled;
};

class VisualMetaDataEnableMsg : public BSONMessage
{
public:
    VisualMetaDataEnableMsg();
    VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);

    bool enabled;
};

class VisualMetaDataMsg : public BSONMessage
{
public:
    VisualMetaDataMsg();
    VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class VisualDataMsg : public BSONMessage
{
public:
    VisualDataMsg();
    VisualDataMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);

    int64_t phys_id;
    double radius;
    struct Vector3 position;
    struct Vector4 orientation;
};

class BeamMsg : public BSONMessage
{
public:
    BeamMsg();
    BeamMsg(BSONReader* _br, MessageType _msg_type);
    ~BeamMsg();
    int64_t send(int sock);

    char beam_type[5];
    char* comm_msg;
    double spread_h, spread_v, energy;
    struct Vector3 origin, velocity, up;
};

class CollisionMsg : public BSONMessage
{
public:
    CollisionMsg();
    CollisionMsg(BSONReader* _br, MessageType _msg_type);
    ~CollisionMsg();
    int64_t send(int sock);
    void set_colltype(char* type);

    char coll_type[5];
    char* comm_msg;
    double energy;
    struct Vector3 position, direction;
};

class SpawnMsg : public BSONMessage
{
public:
    SpawnMsg();
    SpawnMsg(BSONReader* _br, MessageType _msg_type);
    ~SpawnMsg();
    int64_t send(int sock);

    char* obj_type;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class ScanResultMsg : public BSONMessage
{
public:
    ScanResultMsg();
    ScanResultMsg(BSONReader* _br, MessageType _msg_type);
    ~ScanResultMsg();
    int64_t send(int sock);

    char* obj_type;
    char* data;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class ScanQueryMsg : public BSONMessage
{
public:
    ScanQueryMsg();
    ScanQueryMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);

    int64_t scan_id;
    double energy;
    struct Vector3 direction;
};

class ScanResponseMsg : public BSONMessage
{
public:
    ScanResponseMsg();
    ScanResponseMsg(BSONReader* _br, MessageType _msg_type);
    ~ScanResponseMsg();
    int64_t send(int sock);

    char* data;
    int64_t scan_id;
};

class GoodbyeMsg : public BSONMessage
{
public:
    GoodbyeMsg();
    GoodbyeMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class DirectoryMsg : public BSONMessage
{
public:
    struct DirectoryItem
    {
        char* name;
        int64_t id;
    };
    DirectoryMsg();
    DirectoryMsg(BSONReader* _br, MessageType _msg_type);
    ~DirectoryMsg();
    int64_t send(int sock);

    int64_t item_count;
    char* item_type;
    struct DirectoryItem* items;
};

class NameMsg : public BSONMessage
{
public:
    NameMsg();
    NameMsg(BSONReader* _br, MessageType _msg_type);
    ~NameMsg();
    int64_t send(int sock);

    char* name;
};

class ReadyMsg : public BSONMessage
{
public:
    ReadyMsg();
    ReadyMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);

    bool ready;
};

class ThrustMsg : public BSONMessage
{
public:
    ThrustMsg();
    ThrustMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class VelocityMsg : public BSONMessage
{
public:
    VelocityMsg();
    VelocityMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class JumpMsg : public BSONMessage
{
public:
    JumpMsg();
    JumpMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class InfoUpdateMsg : public BSONMessage
{
public:
    InfoUpdateMsg();
    InfoUpdateMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

class RequestUpdateMsg : public BSONMessage
{
public:
    RequestUpdateMsg();
    RequestUpdateMsg(BSONReader* _br, MessageType _msg_type);
    int64_t send(int sock);
};

#endif
