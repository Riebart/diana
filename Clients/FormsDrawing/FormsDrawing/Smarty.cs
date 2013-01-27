using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;

namespace FormsDrawing
{
    class Smarty
    {
        TcpClient client;
        Stream s;
        internal int osim_id;
        internal int client_id;

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
            this.client_id = msg.client_id;
            return true;
        }

        internal int CaptainNewShip(int id, string name)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "CLASS", new int[] { id }, new string[] { name });
            Message helloBack = Message.GetMessage(s);
            osim_id = helloBack.osim_id;
            return osim_id;
        }

        internal void ListShips(out int[] ids, out string[] names)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "SHIP", null, null);
            DirectoryMessage dmsg = (DirectoryMessage)Message.GetMessage(s);

            ids = dmsg.ids;
            names = dmsg.names;
        }

        internal void ListClasses(out int[] ids, out string[] names)
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

        internal int JoinShip(int id, string name)
        {
            DirectoryMessage.Send(s, osim_id, client_id, "SHIP", new int[] { id }, new string[] { name });
            Message helloBack = Message.GetMessage(s);
            osim_id = helloBack.osim_id;
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

        internal int ConnectToVisData(Stream s)
        {
            HelloMessage.Send(s, -1, -1);
            Message helloback = (HelloMessage)Message.GetMessage(s);

            int visClientID = helloback.client_id;

            DirectoryMessage.Send(s, -1, visClientID, "SHIP", new int[] { osim_id }, new string[] { "" });
            Message.GetMessage(s); // Eat up the hello that comes back.
            VisDataEnableMessage.Send(s, osim_id, visClientID, true);

            return visClientID;
        }
    }
}
