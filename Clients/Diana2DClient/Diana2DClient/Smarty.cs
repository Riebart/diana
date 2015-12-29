using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;

namespace Diana2DClient
{
    class Smarty
    {
        TcpClient client;
        Stream s;
        internal Int64 osim_id;
        internal Int64 client_id;

        public Smarty()
        {
            osim_id = -1;
            client_id = -1;
        }

        internal bool Connect(IPEndPoint osimServer)
        {
            client = new TcpClient();
            client.ReceiveTimeout = 1000;
            try
            {
                client.Connect(osimServer);
            }
            catch (System.Net.Sockets.SocketException)
            {
                MessageBox.Show("Unable to connect");
                return false;
            }

            s = client.GetStream();

            HelloMessage.Send(s, -1, -1);

            Message msg = Message.GetMessage(s);
            this.osim_id = msg.server_id;
            return true;
        }

        internal Int64 CaptainNewShip(Int64 id, string name)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "CLASS", new Int64[] { id }, new string[] { name });
            Message helloBack = Message.GetMessage(s);
            osim_id = helloBack.server_id;
            return osim_id;
        }

        internal void ListShips(out Int64[] ids, out string[] names)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "SHIP", null, null);
            DirectoryMessage dmsg = (DirectoryMessage)Message.GetMessage(s);

            ids = dmsg.ids;
            names = dmsg.names;
        }

        internal void ListClasses(out Int64[] ids, out string[] names)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "CLASS", null, null);
            DirectoryMessage dmsg = (DirectoryMessage)Message.GetMessage(s);

            ids = dmsg.ids;
            names = dmsg.names;
        }

        internal void Disconnect(bool goodbye)
        {
            if (goodbye)
            {
                GoodbyeMessage.Send(s, osim_id, client_id);
            }

            s.Flush();
            s.Close();
            client.Close();
            client = null;
        }

        internal void RenameShip(string name)
        {
            NameMessage.Send(s, osim_id, client_id, name);
        }

        internal Int64 JoinShip(Int64 id, string name)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "SHIP", new Int64[] { id }, new string[] { name });
            Message helloBack = Message.GetMessage(s);
            osim_id = helloBack.server_id;
            return osim_id;
        }

        internal void Ready()
        {
            ReadyMessage.Send(s, osim_id, client_id, true);
        }

        internal bool Connected()
        {
            if (client == null)
            {
                return false;
            }

            bool hungup = (client.Client.Poll(1, SelectMode.SelectRead) && (client.Available == 0));

            if (hungup)
            {
                client = null;
                MessageBox.Show("The objectsim hung up on us.");
            }

            return !hungup;
        }

        internal void UpdateVelocity(double x, double y, double z)
        {
            PhysicalPropertiesMessage.Send(s, osim_id, client_id, "", double.NaN, double.NaN, null, new Vector3D(x, y, z), null, null);
        }

        internal void UpdateThrust(double x, double y, double z)
        {
            PhysicalPropertiesMessage.Send(s, osim_id, client_id, "", double.NaN, double.NaN, null, null, null, new Vector3D(x, y, z));
        }

        internal void UpdateThrust(Vector3D thrust)
        {
            PhysicalPropertiesMessage.Send(s, osim_id, client_id, "", double.NaN, double.NaN, null, null, null, thrust);
        }

        internal IPEndPoint ServerEndpoint()
        {
            return (IPEndPoint)client.Client.RemoteEndPoint;
        }

        internal Int64 GetConnection(out TcpClient client, out Stream s)
        {
            client = new TcpClient();
            client.ReceiveTimeout = 1000;
            client.Connect((IPEndPoint)this.client.Client.RemoteEndPoint);
            s = client.GetStream();

            HelloMessage.Send(s, -1, -1);
            Message helloback = (HelloMessage)Message.GetMessage(s);

            Int64 visClientID = helloback.client_id;

            return visClientID;
        }

        internal void Disconnect(TcpClient client, Stream s, Int64 clientID)
        {
            GoodbyeMessage.Send(s, osim_id, clientID);
            s.Flush();
            s.Close();
            client.Close();
        }

        internal Int64 ConnectToVisData(Stream s)
        {
            HelloMessage.Send(s, -1, -1);
            Message helloback = (HelloMessage)Message.GetMessage(s);

            Int64 visClientID = helloback.client_id;

            DirectoryMessage.Send(s, -1, visClientID, "SHIP", new Int64[] { osim_id }, new string[] { "" });
            Message.GetMessage(s); // Eat up the hello that comes back.
            VisDataEnableMessage.Send(s, osim_id, visClientID, true);

            return visClientID;
        }

        internal int ConnectToHelm(Stream s)
        {
            return -1;
        }
    }
}
