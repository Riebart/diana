#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include <stdlib.h>
#include <stdint.h>

#include "vector.hpp"

class BSONReader;

class BSONMessage
{
public:
    static BSONMessage* ReadMessage(int sock);

    int32_t msg_type;
    int64_t server_id;
    int64_t client_id;

    enum MessageTypes
    {
        Hello, PhysicalProperties, VisualProperties, VisualDataEnable,
        VisualMetaDataEnable, VisualMetaData, VisualData, Beam, Collision,
        Spawn, ScanResult, ScanQuery, ScanResponse, Goodbye,
        Directory, Name, Ready, Thrust, Velocity,
        Jump, InfoUpdate, RequestUpdate
    };

protected:
    double ReadDouble();
    bool ReadBool();
    int32_t ReadInt32();
    int64_t ReadInt64();
    struct Vector3 ReadVector3();
    struct Vector4 ReadVector4();
    //! @todo Support things other than C-strings
    void ReadString(char* dst, int32_t dstlen);
    char* ReadString();

    void ReadIDs();

    ~BSONMessage();

    BSONReader* br;
};

class HelloMsg : public BSONMessage
{
public:
    HelloMsg(BSONReader* _br, int32_t _msg_type);
    static uint8_t* send(int32_t server_id, int32_t client_id);
};

class PhysicalPropertiesMsg : public BSONMessage
{
public:
    PhysicalPropertiesMsg(BSONReader* _br, int32_t _msg_type);
    static uint8_t* send(int32_t server_id, int32_t client_id, char* obj_type, double mass,
    struct Vector3 position,
    struct Vector3 velocity,
    struct Vector4 orientation,
    struct Vector3 thrust,
        double radius);
    ~PhysicalPropertiesMsg();
private:
    char* obj_type;
    double mass,
        radius;
    struct Vector3 position,
        velocity,
        thrust;
    struct Vector4 orientation;
};

class VisualPropertiesMsg : public BSONMessage
{
public:
    VisualPropertiesMsg(BSONReader* _br, int32_t _msg_type);
};

class VisualDataEnableMsg : public BSONMessage
{
public:
    VisualDataEnableMsg(BSONReader* _br, int32_t _msg_type);
    static uint8_t* send(int32_t server_id, int32_t client_id, bool enabled);
private:
    bool enabled;
};

class VisualMetaDataEnableMsg : public BSONMessage
{
public:
    VisualMetaDataEnableMsg(BSONReader* _br, int32_t _msg_type);
    static uint8_t* send(int32_t server_id, int32_t client_id, bool enabled);
private:
    bool enabled;
};

class VisualMetaDataMsg : public BSONMessage
{
public:
    VisualMetaDataMsg(BSONReader* _br, int32_t _msg_type);
};

class VisualDataMsg : public BSONMessage
{
public:
    VisualDataMsg(BSONReader* _br, int32_t _msg_type);
    static uint8_t* send(int32_t server_id, int32_t client_id, int64_t phys_id, double radius,
    struct Vector3 position, struct Vector4 orientation);
private:
    int64_t phys_id;
    double radius;
    struct Vector3 position;
    struct Vector4 orientation;
};

class BeamMsg : public BSONMessage
{
public:
    BeamMsg(BSONReader* _br, int32_t _msg_type);
    ~BeamMsg();
private:
    struct Vector3 origin,
        velocity,
        up;
    double spread_h,
        spread_v,
        energy;
    char type[4];
    char* msg;
};

class CollisionMsg : public BSONMessage
{
public:
    CollisionMsg(BSONReader* _br, int32_t _msg_type);
    ~CollisionMsg();
private:
    struct Vector3 position,
        direction;
    double energy;
    char type[4];
    char* msg;
};

class SpawnMsg : public BSONMessage
{
public:
    SpawnMsg(BSONReader* _br, int32_t _msg_type);
    ~SpawnMsg();
private:
    char* obj_type;
    double mass,
        radius;
    struct Vector3 position,
        velocity,
        thrust;
    struct Vector4 orientation;
};

class ScanResultMsg : public BSONMessage
{
public:
    ScanResultMsg(BSONReader* _br, int32_t _msg_type);
    ~ScanResultMsg();
private:
    char* obj_type;
    char* data;
    double mass,
        radius;
    struct Vector3 position,
        velocity,
        thrust;
    struct Vector4 orientation;
};

class ScanQueryMsg : public BSONMessage
{
public:
    ScanQueryMsg(BSONReader* _br, int32_t _msg_type);
private:
    int64_t scan_id;
    double energy;
    struct Vector3 direction;
};

class ScanResponseMsg : public BSONMessage
{
public:
    ScanResponseMsg(BSONReader* _br, int32_t _msg_type);
    ~ScanResponseMsg();
private:
    char* data;
    int64_t scan_id;
};

class GoodbyeMsg : public BSONMessage
{
public:
    GoodbyeMsg(BSONReader* _br, int32_t _msg_type);
};

class DirectoryMsg : public BSONMessage
{
public:
    DirectoryMsg(BSONReader* _br, int32_t _msg_type);
    ~DirectoryMsg();
private:
    char* item_type;
    int64_t item_count;
    char** items;
};

class NameMsg : public BSONMessage
{
public:
    NameMsg(BSONReader* _br, int32_t _msg_type);
    ~NameMsg();
private:
    char* name;
};

class ReadyMsg : public BSONMessage
{
public:
    ReadyMsg(BSONReader* _br, int32_t _msg_type);
private:
    bool ready;
};

class ThrustMsg : public BSONMessage
{
    friend BSONMessage;
protected:
    ThrustMsg(BSONReader* _br, int32_t _msg_type);
};

class VelocityMsg : public BSONMessage
{
    friend BSONMessage;
protected:
    VelocityMsg(BSONReader* _br, int32_t _msg_type);
};

class JumpMsg : public BSONMessage
{
    friend BSONMessage;
protected:
    JumpMsg(BSONReader* _br, int32_t _msg_type);
};

class InfoUpdateMsg : public BSONMessage
{
    friend BSONMessage;
protected:
    InfoUpdateMsg(BSONReader* _br, int32_t _msg_type);
};

class RequestUpdateMsg : public BSONMessage
{
    friend BSONMessage;
protected:
    RequestUpdateMsg(BSONReader* _br, int32_t _msg_type);
};

#endif
