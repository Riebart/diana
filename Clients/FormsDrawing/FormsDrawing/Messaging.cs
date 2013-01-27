using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace FormsDrawing
{
    class Message
    {
        internal int osim_id;
        internal int client_id;

        static Dictionary<string, Func<string[], Message>> messageMap = new Dictionary<string, Func<string[], Message>>()
        {
            { "VISDATA", VisDataMessage.Build },
            { "HELLO", HelloMessage.Build },
            { "GOODBYE", GoodbyeMessage.Build },
            { "DIRECTORY", DirectoryMessage.Build },
            { "NAME", NameMessage.Build },
            { "READY", ReadyMessage.Build }
        };

        internal static Message GetMessage(Stream s)
        {
            byte[] buf = new byte[10];
            int numRead = -1;

            try
            {
                numRead = s.Read(buf, 0, 10);
            }
            catch (System.IO.IOException)
            {
                return null;
            }

            if (numRead == 0)
            {
                return null;
            }

            int numBytes = int.Parse(System.Text.Encoding.Default.GetString(buf));

            buf = new byte[numBytes];
            try
            {
                numRead = s.Read(buf, 0, numBytes);
            }
            catch (System.IO.IOException)
            {
                return null;
            }

            if (numRead == 0)
            {
                return null;
            }

            string[] msg = System.Text.Encoding.Default.GetString(buf).Split(new char[] { '\n' });
            Message msgObj = Message.messageMap[msg[2]](msg);

            return msgObj;
        }

        internal static bool SendMessage(Stream s, string msg)
        {
            string full_msg = msg.Length.ToString("000000000") + "\n" + msg;
            byte[] bytes = System.Text.Encoding.ASCII.GetBytes(full_msg);
            s.Write(bytes, 0, bytes.Length);
            s.Flush();

            return true;
        }
    }

    class VisDataEnableMessage : Message
    {
        internal static bool Send(Stream s, int osim_id, int client_id, bool enable)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                (client_id != -1 ? client_id + "\n" : "\n") + "VISDATAENABLE\n" + (enable ? 1 : 0) + "\n");
        }
    }

    class VisDataMessage : Message
    {
        static byte[] buffer = new byte[4096];
        public int id;
        public double radius, pX, pY, pZ;

        internal VisDataMessage(string[] msg)
        {
           if (msg[3] == "-1")
            {
                Init(-1, 0.0, 0.0, 0.0, 0.0);
            }
            else
            {
                Init(int.Parse(msg[3]), double.Parse(msg[4]), double.Parse(msg[5]), double.Parse(msg[6]), double.Parse(msg[7]));
            }
        }

        internal static Message Build(string[] msg)
        {
            return new VisDataMessage(msg);
        }

        void Init(int id, double radius, double pX, double pY, double pZ)
        {
            this.id = id;
            this.radius = radius;
            this.pX = pX;
            this.pY = pY;
            this.pZ = pZ;
        }

        internal static bool Send(Stream s)
        {
            throw new NotImplementedException();
        }
    }

    class HelloMessage : Message
    {
        internal HelloMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));
        }

        internal static Message Build(string[] msg)
        {
            return new HelloMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                                          (client_id != -1 ? client_id + "\n" : "\n") + "HELLO\n");
        }
    }

    class GoodbyeMessage : Message
    {
        internal GoodbyeMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));
        }

        internal static Message Build(string[] msg)
        {
            return new GoodbyeMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                              (client_id != -1 ? client_id + "\n" : "\n") + "GOODBYE\n");
        }
    }

    class DirectoryMessage : Message
    {
        bool is_ships;
        internal int[] ids;
        internal string[] names;

        internal DirectoryMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));

            is_ships = (msg[3] == "SHIP");
            ids = new int[(msg.Length - 4) / 2];
            names = new string[(msg.Length - 4) / 2];

            for (int i = 4; i < msg.Length - (msg.Length % 2); i += 2)
            {
                ids[(i - 4) / 2] = int.Parse(msg[i]);
                names[(i - 4) / 2] = msg[i + 1];
            }
        }

        internal static Message Build(string[] msg)
        {
            return new DirectoryMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id, string type, int[] ids, string[] names)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append((osim_id != -1 ? osim_id + "\n" : "\n") +
                              (client_id != -1 ? client_id + "\n" : "\n") + "DIRECTORY\n" + type + "\n");

            if ((ids != null) && (names != null))
            {
                for (int i = 0; i < ids.Length; i++)
                {
                    sb.Append(ids[i] + "\n" + names[i] + "\n");
                }
            }

            return Message.SendMessage(s, sb.ToString());
        }
    }

    class NameMessage : Message
    {
        string name;

        internal NameMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));
            name = msg[3];
        }

        internal static Message Build(string[] msg)
        {
            return new NameMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id, string name)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                                          (client_id != -1 ? client_id + "\n" : "\n") + "NAME\n" + name + "\n");
        }
    }

    class ReadyMessage : Message
    {
        bool ready;

        internal ReadyMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));
            ready = (msg[3] == "1" ? true : false);
        }

        internal static Message Build(string[] msg)
        {
            return new ReadyMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id, bool ready)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                (client_id != -1 ? client_id + "\n" : "\n") + "READY\n" + (ready ? "1" : "0") + "\n");
        }
    }

    class PhysicalPropertiesMessage : Message
    {
        string objectType;
        double mass, radius;
        Vector3D position, velocity, thrust, orientation;

        internal PhysicalPropertiesMessage(string[] msg)
        {
            osim_id = (msg[0] == "" ? -1 : int.Parse(msg[0]));
            client_id = (msg[1] == "" ? -1 : int.Parse(msg[1]));
            objectType = msg[3];

            mass = (msg[4] == "" ? double.NaN : double.Parse(msg[4]));

            if (msg[5] == "")
            {
                position = null;
            }
            else
            {
                position = new Vector3D(double.Parse(msg[5]), double.Parse(msg[6]), double.Parse(msg[7]));
            }

            if (msg[5] == "")
            {
                velocity = null;
            }
            else
            {
                velocity = new Vector3D(double.Parse(msg[8]), double.Parse(msg[9]), double.Parse(msg[10]));
            }

            if (msg[5] == "")
            {
                orientation = null;
            }
            else
            {
                orientation = new Vector3D(double.Parse(msg[11]), double.Parse(msg[12]), double.Parse(msg[13]));
            }

            if (msg[5] == "")
            {
                thrust = null;
            }
            else
            {
                thrust = new Vector3D(double.Parse(msg[14]), double.Parse(msg[15]), double.Parse(msg[16]));
            }

            radius = (msg[16] == "" ? double.NaN : double.Parse(msg[17]));
        }

        internal static Message Build(string[] msg)
        {
            return new PhysicalPropertiesMessage(msg);
        }

        internal static bool Send(Stream s, int osim_id, int client_id, string object_type, double mass, double radius, 
                                  Vector3D position, Vector3D velocity, Vector3D orientation, Vector3D thrust)
        {
            return Message.SendMessage(s, (osim_id != -1 ? osim_id + "\n" : "\n") +
                (client_id != -1 ? client_id + "\n" : "\n") + "PHYSPROPS\n" + object_type + "\n" +
                (double.IsNaN(mass) ? "" : mass.ToString()) + "\n" +
                (position == null ? "\n\n\n" : position.X + "\n" + position.Y + "\n" + position.Z + "\n") +
                (velocity == null ? "\n\n\n" : velocity.X + "\n" + velocity.Y + "\n" + velocity.Z + "\n") +
                (orientation == null ? "\n\n\n" : orientation.X + "\n" + orientation.Y + "\n" + orientation.Z + "\n") +
                (thrust == null ? "\n\n\n" : thrust.X + "\n" + thrust.Y + "\n" + thrust.Z + "\n") +
                (double.IsNaN(radius) ? "" : radius.ToString()) + "\n");
        }
    }
}
