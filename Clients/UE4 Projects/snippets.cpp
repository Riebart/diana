UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SendVisDataEnableMsg::%d"), enable);

// -----------------------------------------------------------------------------

PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Networking", "Sockets" });

// -----------------------------------------------------------------------------

UFUNCTION(BlueprintCallable, Category = "Diana Messagig")
void RegisterForVisData(FString host, int32 port, bool enable);

// See: https://answers.unrealengine.com/questions/98206/missing-support-for-uint32-int64-uint64.html
UFUNCTION(BlueprintImplementableEvent, Category = "Messages From Diana", meta = (FriendlyName = "Received Vis Data Message"))
void ReceivedVisData(int32 PhysID, FVector Position);

private:
bool ConnectSocket();
FString host = "";
int32 port = -1;
FSocket* sock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Address")
    FString host;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Address")
    int32 port;

    UFUNCTION(BlueprintCallable, Category = "Diana Messaging")
    void SendVisDataEnableMsg(bool enable);

// -----------------------------------------------------------------------------

    #include "Networking.h"
    #include "Sockets.h"
    #include "SocketSubsystem.h"

    bool UDianaMessagingComponent::ConnectSocket()
    {
        if (sock == NULL)
        {
            UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::ConnectSocket"));
            sock = FTcpSocketBuilder(TEXT("xSIMConn"));
            FIPv4Address ip;
            FIPv4Address::Parse(host, ip);
            FIPv4Endpoint endpoint(ip, (uint16)port);
            bool connected = sock->Connect(*endpoint.ToInternetAddr());
            return connected;

            //// See: https://wiki.unrealengine.com/Third_Party_Socket_Server_Connection
            //sock = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("xSIMConn"), false);
            //FIPv4Address ip;
            //FIPv4Address::Parse(host, ip);

            //TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
            //addr->SetIp(ip.GetValue());
            //addr->SetPort(port);
            //
            //return sock->Connect(*addr);
        }
        else
        {
            return true;
        }
    }

// -----------------------------------------------------------------------------

#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "SocketTypes.h"

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
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketSend::Post::(%d,%d,%d)"), count, curbytes, (int32)read_success);
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
    UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketRead::PrePendingCheck::(%d,%u,%d)"), count, pending_bytes, (int32)read_success);
    if (!read_success || (pending_bytes < count))
    {
        return 0;
    }

    int32 curbytes = 0;
    while (nbytes < count)
    {
        //! @todo Maybe use recvmsg()?
        // Try to wait for all of the data, if possible.
        read_success = s->Recv(dst + nbytes, count - nbytes, curbytes, ESocketReceiveFlags::Type::None);
        UE_LOG(LogTemp, Warning, TEXT("DianaMessaging::SocketRead::Post::(%d,%d,%d,%d)"), count, nbytes, curbytes, (int32)read_success);
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

// -----------------------------------------------------------------------------