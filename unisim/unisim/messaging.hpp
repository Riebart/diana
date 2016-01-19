#ifndef MESSAGING_HPP
#define MESSAGING_HPP

// Branch if we're using US networking or OS networking.
#ifdef UE_NETWORKING
typedef FSocket* sock_t;
#else
typedef int sock_t;
#endif

#include <stdlib.h>
#include <stdint.h>

// For Vector3
#include "vector.hpp"
// For Spectrum/SpectrumComponent
#include "physics.hpp"
// For BSONReader::Element
#include "bson.hpp"

namespace Diana
{
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

        // Type of message, stored in the first field, an Int32 field with the name "" (empty string)
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

        // An array of boolean values indicating whether a value was filled or left blank.
        bool* specced;

        // Number of elements, equals the length of the 'specced' and 'handlers' arrays, minus
        // 2. This number doesn't account for the server/client IDs at the head of the message,
        // just the message specific fields. Can be 0.
        uint32_t num_el;

        // Read a BSON message form a socket and return a pointer to a newly allocated object.
        static BSONMessage* ReadMessage(sock_t sock);
        BSONMessage() { }
        virtual ~BSONMessage();
        virtual int spec_all(bool spec = true);
        virtual bool all_specced(int start_index = 0, int stop_index = -1);

    protected:
        BSONReader* br;
        BSONMessage(BSONReader* _br, uint32_t _num_el);

        void ReadElements();
        std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element&)>* handlers;
    };

    class HelloMsg : public BSONMessage
    {
    public:
        HelloMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);
        MessageType msg_type = Hello;
    };

    class PhysicalPropertiesMsg : public BSONMessage
    {
    public:
        PhysicalPropertiesMsg(BSONReader* _br = NULL);
        ~PhysicalPropertiesMsg();
        int64_t send(sock_t sock);
        MessageType msg_type = PhysicalProperties;

        char* obj_type;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4 orientation;
        struct Spectrum* spectrum = NULL;  //! @todo convert the spectrum into a subdocument BSON dict.
    };

    class VisualPropertiesMsg : public BSONMessage
    {
    public:
        VisualPropertiesMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = VisualProperties;
    };

    class VisualDataEnableMsg : public BSONMessage
    {
    public:
        VisualDataEnableMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = VisualDataEnable;

        bool enabled;
    };

    class VisualMetaDataEnableMsg : public BSONMessage
    {
    public:
        VisualMetaDataEnableMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = VisualMetaDataEnable;

        bool enabled;
    };

    class VisualMetaDataMsg : public BSONMessage
    {
    public:
        VisualMetaDataMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = VisualMetaData;
    };

    class VisualDataMsg : public BSONMessage
    {
    public:
        VisualDataMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = VisualData;

        int64_t phys_id;
        double radius;
        struct Vector3 position;
        struct Vector4 orientation;

        double red, green, blue;
    };

    class BeamMsg : public BSONMessage
    {
    public:
        BeamMsg(BSONReader* _br = NULL);
        ~BeamMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = Beam;

        char beam_type[5];
        char* comm_msg;
        double spread_h, spread_v, energy;
        struct Vector3 origin, velocity, up;
        struct Spectrum* spectrum = NULL;
    };

    class CollisionMsg : public BSONMessage
    {
    public:
        CollisionMsg(BSONReader* _br = NULL);
        ~CollisionMsg();
        int64_t send(sock_t sock);
        void set_colltype(char* type);

        MessageType msg_type = Collision;

        char coll_type[5];
        char* comm_msg;
        double energy;
        struct Vector3 position, direction;
        struct Spectrum* spectrum = NULL;
    };

    class SpawnMsg : public BSONMessage
    {
    public:
        SpawnMsg(BSONReader* _br = NULL);
        ~SpawnMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = Spawn;

        bool is_smart;
        char* obj_type;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4 orientation;
        struct Spectrum* spectrum = NULL;
    };

    class ScanResultMsg : public BSONMessage
    {
    public:
        ScanResultMsg(BSONReader* _br = NULL);
        ~ScanResultMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = ScanResult;

        char* obj_type;
        char* data;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4 orientation;
        struct Spectrum* spectrum = NULL;
    };

    class ScanQueryMsg : public BSONMessage
    {
    public:
        ScanQueryMsg(BSONReader* _br = NULL);
        ~ScanQueryMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = ScanQuery;

        int64_t scan_id;
        double energy;
        struct Vector3 direction;
        struct Spectrum* spectrum = NULL;
    };

    class ScanResponseMsg : public BSONMessage
    {
    public:
        ScanResponseMsg(BSONReader* _br = NULL);
        ~ScanResponseMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = ScanResponse;

        char* data;
        int64_t scan_id;
    };

    class GoodbyeMsg : public BSONMessage
    {
    public:
        GoodbyeMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = Goodbye;
    };

    class DirectoryMsg : public BSONMessage
    {
    public:
        struct DirectoryItem
        {
            char* name;
            int64_t id;
        };
        DirectoryMsg(BSONReader* _br = NULL);
        ~DirectoryMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = Directory;

        int64_t item_count;
        char* item_type;
        struct DirectoryItem* items;
    };

    class NameMsg : public BSONMessage
    {
    public:
        NameMsg(BSONReader* _br = NULL);
        ~NameMsg();
        int64_t send(sock_t sock);

        MessageType msg_type = Name;

        char* name;
    };

    class ReadyMsg : public BSONMessage
    {
    public:
        ReadyMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = Ready;

        bool ready;
    };

    class ThrustMsg : public BSONMessage
    {
    public:
        ThrustMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = Thrust;
    };

    class VelocityMsg : public BSONMessage
    {
    public:
        VelocityMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = Velocity;
    };

    class JumpMsg : public BSONMessage
    {
    public:
        JumpMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = Jump;
    };

    class InfoUpdateMsg : public BSONMessage
    {
    public:
        InfoUpdateMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = InfoUpdate;
    };

    class RequestUpdateMsg : public BSONMessage
    {
    public:
        RequestUpdateMsg(BSONReader* _br = NULL);
        int64_t send(sock_t sock);

        MessageType msg_type = RequestUpdate;
    };
}
#endif
