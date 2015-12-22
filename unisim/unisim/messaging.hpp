#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <stdlib.h>
#include <stdint.h>

#include "vector.hpp"

class BSONReader;

class BSONMessage
{
public:
    enum MessageType
    {
        Hello, PhysicalProperties, VisualProperties, VisualDataEnable,
        VisualMetaDataEnable, VisualMetaData, VisualData, Beam, Collision,
        Spawn, ScanResult, ScanQuery, ScanResponse, Goodbye,
        Directory, Name, Ready, Thrust, Velocity,
        Jump, InfoUpdate, RequestUpdate
    };

    MessageType msg_type;
    int64_t server_id;
    int64_t client_id;

    static BSONMessage* ReadMessage(int sock);

protected:
    BSONReader* br;
    BSONMessage(BSONReader* _br, MessageType _msg_type);
};

class HelloMsg : public BSONMessage
{
public:
    HelloMsg(BSONReader* _br, MessageType _msg_type);
    size_t send(int sock);
};

class PhysicalPropertiesMsg : public BSONMessage
{
public:
    PhysicalPropertiesMsg(BSONReader* _br, MessageType _msg_type);
    ~PhysicalPropertiesMsg();
    size_t send(int sock);

    char* obj_type;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class VisualPropertiesMsg : public BSONMessage
{
public:
    VisualPropertiesMsg(BSONReader* _br, MessageType _msg_type);
};

class VisualDataEnableMsg : public BSONMessage
{
public:
    VisualDataEnableMsg(BSONReader* _br, MessageType _msg_type);

    bool enabled;
};

class VisualMetaDataEnableMsg : public BSONMessage
{
public:
    VisualMetaDataEnableMsg(BSONReader* _br, MessageType _msg_type);

    bool enabled;
};

class VisualMetaDataMsg : public BSONMessage
{
public:
    VisualMetaDataMsg(BSONReader* _br, MessageType _msg_type);
};

class VisualDataMsg : public BSONMessage
{
public:
    VisualDataMsg(BSONReader* _br, MessageType _msg_type);

    int64_t phys_id;
    double radius;
    struct Vector3 position;
    struct Vector4 orientation;
};

class BeamMsg : public BSONMessage
{
public:
    BeamMsg(BSONReader* _br, MessageType _msg_type);
    ~BeamMsg();

    char type[4];
    char* msg;
    double spread_h, spread_v, energy;
    struct Vector3 origin, velocity, up;
};

class CollisionMsg : public BSONMessage
{
public:
    CollisionMsg(BSONReader* _br, MessageType _msg_type);
    ~CollisionMsg();

    char type[4];
    char* msg;
    double energy;
    struct Vector3 position, direction;
};

class SpawnMsg : public BSONMessage
{
public:
    SpawnMsg(BSONReader* _br, MessageType _msg_type);
    ~SpawnMsg();

    char* obj_type;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class ScanResultMsg : public BSONMessage
{
public:
    ScanResultMsg(BSONReader* _br, MessageType _msg_type);
    ~ScanResultMsg();

    char* obj_type;
    char* data;
    double mass, radius;
    struct Vector3 position, velocity, thrust;
    struct Vector4 orientation;
};

class ScanQueryMsg : public BSONMessage
{
public:
    ScanQueryMsg(BSONReader* _br, MessageType _msg_type);

    int64_t scan_id;
    double energy;
    struct Vector3 direction;
};

class ScanResponseMsg : public BSONMessage
{
public:
    ScanResponseMsg(BSONReader* _br, MessageType _msg_type);
    ~ScanResponseMsg();

    char* data;
    int64_t scan_id;
};

class GoodbyeMsg : public BSONMessage
{
public:
    GoodbyeMsg(BSONReader* _br, MessageType _msg_type);
};

class DirectoryMsg : public BSONMessage
{
public:
    DirectoryMsg(BSONReader* _br, MessageType _msg_type);
    ~DirectoryMsg();

    int64_t item_count;
    char* item_type;
    char** items;
};

class NameMsg : public BSONMessage
{
public:
    NameMsg(BSONReader* _br, MessageType _msg_type);
    ~NameMsg();

    char* name;
};

class ReadyMsg : public BSONMessage
{
public:
    ReadyMsg(BSONReader* _br, MessageType _msg_type);

    bool ready;
};

class ThrustMsg : public BSONMessage
{
public:
    ThrustMsg(BSONReader* _br, MessageType _msg_type);
};

class VelocityMsg : public BSONMessage
{
public:
    VelocityMsg(BSONReader* _br, MessageType _msg_type);
};

class JumpMsg : public BSONMessage
{
public:
    JumpMsg(BSONReader* _br, MessageType _msg_type);
};

class InfoUpdateMsg : public BSONMessage
{
public:
    InfoUpdateMsg(BSONReader* _br, MessageType _msg_type);
};

class RequestUpdateMsg : public BSONMessage
{
public:
    RequestUpdateMsg(BSONReader* _br, MessageType _msg_type);
};

#endif
