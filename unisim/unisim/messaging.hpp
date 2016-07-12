#ifndef MESSAGING_HPP
#define MESSAGING_HPP

// Branch if we're using UE networking or OS networking.
#ifdef UCLASS
class FSocket;
typedef FSocket* sock_t;
#else
typedef int sock_t;
#endif

#include <stdlib.h>
#include <stdint.h>
#include <functional>

// For Vector3
#include "vector.hpp"
// For Spectrum/SpectrumComponent
#include "physics.hpp"
// For BSONReader::Element
#include "bson.hpp"

namespace Diana
{
    //! @todo A message relay that will replicate/pass messages off to other pieces of code
    // that ask for them. Think about multiplexing multiple clients over a socket, and handling
    // the message relaying that's required.

    class BSONMessage;
    typedef std::function<void(uint32_t, BSONMessage*, struct BSONReader::Element*)> read_lambda;

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
            InfoUpdate = 21, RequestUpdate = 22, SystemUpdate = 23, Command = 24
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

        // A pointer to an externally allocated/destroyed BSONReader object.
        BSONReader* br;

        // Read a BSON message form a socket and return a pointer to a newly allocated object.
        static BSONMessage* ReadMessage(sock_t sock);
        BSONMessage() { }
        virtual ~BSONMessage();
        virtual int spec_all(bool spec = true);
        virtual bool all_specced(int start_index = 0, int stop_index = -1, int except = -1);

    protected:
        BSONMessage(BSONReader* _br, uint32_t _num_el, const read_lambda* handlers, MessageType _msg_type = Reservedx00);
        virtual const read_lambda* handlers() { return NULL; };
    };

    class HelloMsg : public BSONMessage
    {
    public:
        HelloMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), Hello) { }
        int64_t send(sock_t sock);

    protected:
        const read_lambda* handlers();
    };

    class PhysicalPropertiesMsg : public BSONMessage
    {
    public:
        PhysicalPropertiesMsg(BSONReader* _br = NULL) : BSONMessage(_br, 19, handlers(), PhysicalProperties) { }
        ~PhysicalPropertiesMsg();
        int64_t send(sock_t sock);

        char* obj_type;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4T<double> orientation;
        struct Spectrum* spectrum;

    protected:
        const read_lambda* handlers();
    };

    class VisualPropertiesMsg : public BSONMessage
    {
    public:
        VisualPropertiesMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), VisualProperties) { }
        int64_t send(sock_t sock);
    protected:
        const read_lambda* handlers();
    };

    class VisualDataEnableMsg : public BSONMessage
    {
    public:
        VisualDataEnableMsg(BSONReader* _br = NULL) : BSONMessage(_br, 1, handlers(), VisualDataEnable) { }
        int64_t send(sock_t sock);

        bool enabled;

    protected:
        const read_lambda* handlers();
    };

    class VisualMetaDataEnableMsg : public BSONMessage
    {
    public:
        VisualMetaDataEnableMsg(BSONReader* _br = NULL) : BSONMessage(_br, 1, handlers(), VisualMetaDataEnable) { }
        int64_t send(sock_t sock);

        bool enabled;

    protected:
        const read_lambda* handlers();
    };

    class VisualMetaDataMsg : public BSONMessage
    {
    public:
        VisualMetaDataMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), VisualMetaData) { }
        int64_t send(sock_t sock);

    protected:
        const read_lambda* handlers();
    };

    class VisualDataMsg : public BSONMessage
    {
    public:
        VisualDataMsg(BSONReader* _br = NULL) : BSONMessage(_br, 9, handlers(), VisualData) { }
        int64_t send(sock_t sock);

        int64_t phys_id;
        double radius;
        struct Vector3 position;
        struct Vector4T<double> orientation;
        double red, green, blue;

    protected:
        const read_lambda* handlers();
    };

    class BeamMsg : public BSONMessage
    {
    public:
        BeamMsg(BSONReader* _br = NULL) : BSONMessage(_br, 17, handlers(), Beam) { }
        ~BeamMsg();
        int64_t send(sock_t sock);

        char beam_type[5];
        char* comm_msg;
        double spread_h, spread_v, energy;
        struct Vector3 origin, velocity;
        struct Vector3T<double> up;
        struct Spectrum* spectrum;

    protected:
        const read_lambda* handlers();
    };

    class CollisionMsg : public BSONMessage
    {
    public:
        CollisionMsg(BSONReader* _br = NULL) : BSONMessage(_br, 12, handlers(), Collision) { }
        ~CollisionMsg();
        int64_t send(sock_t sock);
        void set_colltype(char* type);

        char coll_type[5];
        char* comm_msg;
        double energy;
        struct Vector3 position, direction;
        struct Spectrum* spectrum;

    protected:
        const read_lambda* handlers();
    };

    class SpawnMsg : public BSONMessage
    {
    public:
        SpawnMsg(BSONReader* _br = NULL) : BSONMessage(_br, 20, handlers(), Spawn) { }
        ~SpawnMsg();
        int64_t send(sock_t sock);

        bool is_smart;
        char* obj_type;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4T<double> orientation;
        struct Spectrum* spectrum;

    protected:
        const read_lambda* handlers();
    };

    class ScanResultMsg : public BSONMessage
    {
    public:
        ScanResultMsg(BSONReader* _br = NULL) : BSONMessage(_br, 23, handlers(), ScanResult) { }
        ~ScanResultMsg();
        int64_t send(sock_t sock);

        char* obj_type;
        //! @todo Promote this to a dict with full text keys. Can probably just store the BSON dict to spit back.
        char* data;
        double mass, radius;
        struct Vector3 position, velocity, thrust;
        struct Vector4T<double> orientation;
        struct Spectrum* obj_spectrum;
        struct Spectrum* beam_spectrum;

    protected:
        const read_lambda* handlers();
    };

    class ScanQueryMsg : public BSONMessage
    {
    public:
        ScanQueryMsg(BSONReader* _br = NULL) : BSONMessage(_br, 8, handlers(), ScanQuery) { }
        ~ScanQueryMsg();
        int64_t send(sock_t sock);

        int64_t scan_id;
        double energy;
        struct Vector3 direction;
        struct Spectrum* spectrum;

    protected:
        const read_lambda* handlers();
    };

    //! @todo Allow the response to specify the return energy/spread of the beam. I think this is in the wiki, but I'll put it here too.
    class ScanResponseMsg : public BSONMessage
    {
    public:
        ScanResponseMsg(BSONReader* _br = NULL) : BSONMessage(_br, 2, handlers(), ScanResponse) { }
        ~ScanResponseMsg();
        int64_t send(sock_t sock);

        //! @todo Promote this to a dict with full text keys. Can probably just store the BSON dict to spit back. This is tied to the ScanQueryResponse message data field.
        char* data;
        int64_t scan_id;

    protected:
        const read_lambda* handlers();
    };

    class GoodbyeMsg : public BSONMessage
    {
    public:
        GoodbyeMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), Goodbye) { }
        int64_t send(sock_t sock);

    protected:
        const read_lambda* handlers();
    };

    class DirectoryMsg : public BSONMessage
    {
    public:
        DirectoryMsg(BSONReader* _br = NULL) : BSONMessage(_br, 4, handlers(), Directory) { }
        ~DirectoryMsg();
        int64_t send(sock_t sock);

        struct DirectoryItem
        {
            char* name;
            int64_t id;
        };

        int64_t item_count;
        char* item_type;
        struct DirectoryItem* items;

        void read_parts(std::function<void(struct DirectoryItem&, struct BSONReader::Element*)> set);

    protected:
        const read_lambda* handlers();
    };

    class NameMsg : public BSONMessage
    {
    public:
        NameMsg(BSONReader* _br = NULL) : BSONMessage(_br, 1, handlers(), Name) { }
        ~NameMsg();
        int64_t send(sock_t sock);

        char* name;

    protected:
        const read_lambda* handlers();
    };

    class ReadyMsg : public BSONMessage
    {
    public:
        ReadyMsg(BSONReader* _br = NULL) : BSONMessage(_br, 1, handlers(), Ready) { }
        int64_t send(sock_t sock);

        bool ready;

    protected:
        const read_lambda* handlers();
    };

    class ThrustMsg : public BSONMessage
    {
    public:
        ThrustMsg(BSONReader* _br = NULL) : BSONMessage(_br, 3, handlers(), Thrust) { }
        int64_t send(sock_t sock);

        Vector3 thrust;

    protected:
        const read_lambda* handlers();
    };

    class VelocityMsg : public BSONMessage
    {
    public:
        VelocityMsg(BSONReader* _br = NULL) : BSONMessage(_br, 3, handlers(), Velocity) { }
        int64_t send(sock_t sock);

        Vector3 velocity;

    protected:
        const read_lambda* handlers();
    };

    class JumpMsg : public BSONMessage
    {
    public:
        JumpMsg(BSONReader* _br = NULL) : BSONMessage(_br, 3, handlers(), Jump) { }
        int64_t send(sock_t sock);

        Vector3 destination;

    protected:
        const read_lambda* handlers();
    };

    class InfoUpdateMsg : public BSONMessage
    {
    public:
        InfoUpdateMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), InfoUpdate) { }
        int64_t send(sock_t sock);

    protected:
        const read_lambda* handlers();
    };

    class RequestUpdateMsg : public BSONMessage
    {
    public:
        RequestUpdateMsg(BSONReader* _br = NULL) : BSONMessage(_br, 0, handlers(), RequestUpdate) { }
        int64_t send(sock_t sock);

    protected:
        const read_lambda* handlers();
    };

    class SystemUpdateMsg : public BSONMessage
    {
    public:
        SystemUpdateMsg(BSONReader* _br = NULL) : BSONMessage(_br, 1, handlers(), SystemUpdate) { }
        ~SystemUpdateMsg();
        int64_t send(sock_t sock);
        struct BSONReader::Element* properties;

    protected:
        const read_lambda* handlers();
    };

    class CommandMsg : public BSONMessage
    {
    public:
        CommandMsg(BSONReader* _br = NULL) : BSONMessage(_br, 2, handlers(), Command) { }
        ~CommandMsg();
        int64_t send(sock_t sock);

        int64_t system_id;
        struct BSONReader::Element* command;

    protected:
        const read_lambda* handlers();
    };
}
#endif
