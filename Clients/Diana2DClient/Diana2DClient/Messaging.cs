using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace Diana2DClient
{
    class Message
    {
        public enum MessageType : int
        {
            Reservedx00 = 0, Hello = 1, PhysicalProperties = 2, VisualProperties = 3, VisualDataEnable = 4,
            VisualMetaDataEnable = 5, VisualMetaData = 6, VisualData = 7, Beam = 8, Collision = 9,
            Spawn = 10, ScanResult = 11, ScanQuery = 12, ScanResponse = 13, Goodbye = 14,
            Directory = 15, Name = 16, Ready = 17, Thrust = 18, Velocity = 19, Jump = 20,
            InfoUpdate = 21, RequestUpdate = 22
        };

        internal Int64 osim_id;
        internal Int64 client_id;

        static Dictionary<MessageType, Func<BSONReader, Message>> messageMap = new Dictionary<MessageType, Func<BSONReader, Message>>()
        {
            { MessageType.VisualData, VisDataMessage.Build },
            { MessageType.Hello, HelloMessage.Build },
            { MessageType.Goodbye, GoodbyeMessage.Build },
            { MessageType.Directory, DirectoryMessage.Build },
            { MessageType.Name, NameMessage.Build },
            { MessageType.Ready, ReadyMessage.Build }
        };

        internal static Message GetMessage(Stream s)
        {
            byte[] buf = new byte[4];
            
            try
            {
                s.Read(buf, 0, 4);
            }
            catch (System.IO.IOException)
            {
                return null;
            }
            
            Int32 msg_len = BitConverter.ToInt32(buf, 0);
            buf = new byte[msg_len];
            s.Read(buf, 4, msg_len - 4);
            Array.Copy(BitConverter.GetBytes(msg_len), 0, buf, 0, 4);

            BSONReader br = new BSONReader(buf);
            BSONReader.Element el = br.GetNextElement();

            if ((el.type != BSONReader.ElementType.Int32) || (el.name != "MsgType"))
            {
                return null;
            }
            
            return messageMap[(MessageType)el.i32_val](br);
        }
        
        internal static bool SendMessage(Stream s, byte[] msg)
        {
            s.Write(msg, 0, msg.Length);
            s.Flush();
            return true;
        }
    }

    class VisDataEnableMessage : Message
    {
        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id, bool enable)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.VisualDataEnable);
            bw.Push(osim_id);
            bw.Push(client_id);
            bw.Push(enable);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class VisDataMessage : Message
    {
        static byte[] buffer = new byte[4096];
        public Int64 id;
        public double radius;
        public Vector3D position;
        public Vector4D orientation;

        internal VisDataMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
            id = br.GetNextElement().i64_val;
            radius = br.GetNextElement().dbl_val;

            position = new Vector3D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );
            
            orientation = new Vector4D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );
        }

        internal static Message Build(BSONReader br)
        {
            return new VisDataMessage(br);
        }

        internal static bool Send(Stream s)
        {
            throw new NotImplementedException();
        }
    }

    class HelloMessage : Message
    {
        internal HelloMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
        }

        internal static Message Build(BSONReader br)
        {
            return new HelloMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.Hello);
            bw.Push(osim_id);
            bw.Push(client_id);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class GoodbyeMessage : Message
    {
        internal GoodbyeMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
        }

        internal static Message Build(BSONReader br)
        {
            return new GoodbyeMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.Goodbye);
            bw.Push(osim_id);
            bw.Push(client_id);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class DirectoryMessage : Message
    {
        string item_type;
        bool is_ships;
        internal Int64[] ids;
        internal string[] names;

        internal DirectoryMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
            item_type = br.GetNextElement().str_val;
            is_ships = item_type.Equals("SHIP");
            Int64 item_count = br.GetNextElement().i64_val;
            ids = new Int64[item_count];
            names = new string[item_count];

            for (int i = 0; i < item_count; i++)
            {
                ids[i] = br.GetNextElement().i64_val;
                names[i] = br.GetNextElement().str_val;
                
            }
        }

        internal static Message Build(BSONReader br)
        {
            return new DirectoryMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id, string type, Int64[] ids, string[] names)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.Directory);
            bw.Push(osim_id);
            bw.Push(client_id);
            bw.Push(type);
            for (int i = 0; i < ids.Length; i++)
            {
                bw.Push(ids[i]);
                bw.Push(names[i]);
            }
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class NameMessage : Message
    {
        string name;

        internal NameMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
            name = br.GetNextElement().str_val;
        }

        internal static Message Build(BSONReader br)
        {
            return new NameMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id, string name)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.Name);
            bw.Push(osim_id);
            bw.Push(client_id);
            bw.Push(name);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class ReadyMessage : Message
    {
        bool ready;

        internal ReadyMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
            ready = br.GetNextElement().bln_val;
        }

        internal static Message Build(BSONReader br)
        {
            return new ReadyMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id, bool ready)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.Name);
            bw.Push(osim_id);
            bw.Push(client_id);
            bw.Push(ready);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }

    class PhysicalPropertiesMessage : Message
    {
        string object_type;
        double mass, radius;
        Vector3D position, velocity, thrust;
        Vector4D orientation;

        internal PhysicalPropertiesMessage(BSONReader br)
        {
            osim_id = br.GetNextElement().i64_val;
            client_id = br.GetNextElement().i64_val;
            object_type = br.GetNextElement().str_val;
            mass = br.GetNextElement().dbl_val;

            position = new Vector3D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );

            velocity = new Vector3D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );

            thrust = new Vector3D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );

            orientation = new Vector4D(
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val,
                br.GetNextElement().dbl_val
                );

            radius = br.GetNextElement().dbl_val;
        }

        internal static Message Build(BSONReader br)
        {
            return new PhysicalPropertiesMessage(br);
        }

        internal static bool Send(Stream s, Int64 osim_id, Int64 client_id, string object_type, double mass, double radius,
                                  Vector3D position, Vector3D velocity, Vector4D orientation, Vector3D thrust)
        {
            BSONWriter bw = new BSONWriter();
            bw.Push("MsgType", (Int32)Message.MessageType.PhysicalProperties);
            bw.Push(osim_id);
            bw.Push(client_id);
            bw.Push(object_type);
            bw.Push(mass);

            bw.Push(position.X);
            bw.Push(position.Y);
            bw.Push(position.Z);

            bw.Push(velocity.X);
            bw.Push(velocity.Y);
            bw.Push(velocity.Z);

            bw.Push(orientation.W);
            bw.Push(orientation.X);
            bw.Push(orientation.Y);
            bw.Push(orientation.Z);

            bw.Push(thrust.X);
            bw.Push(thrust.Y);
            bw.Push(thrust.Z);

            bw.Push(radius);
            return Message.SendMessage(s, bw.PushEnd());
        }
    }
}
