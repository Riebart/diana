using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace Diana2DClient
{
    class BSONReader
    {
        public enum ElementType : byte
        {
            EndOfDocument = 0, Double = 1, String = 2, SubDocument = 3, Array = 4,
            Binary = 5, Deprecatedx06 = 6, ObjectId = 7, Boolean = 8, UTCDateTime = 9,
            Null = 10, Regex = 11, DBPointer = 12, JavaScript = 13, Deprecatedx0E = 14,
            JavaScriptWScope = 15, Int32 = 16, MongoTimeStamp = 17, Int64 = 18,
            MinKey = (int)0xFF, MaxKey = 0x7F
        }

        public struct Element
        {
            public ElementType type;
            public int subtype;
            public String name;

            public bool bln_val;
            public Int32 i32_val;
            public Int64 i64_val;
            public double dbl_val;

            public byte[] bin_val;
            public String str_val;
        }

        public BSONReader(byte[] msg)
        {
            this.msg = msg;
            len = BitConverter.ToInt32(msg, 0);
            pos = 4;
        }

        String ReadCString()
        {
            StringBuilder sb = new StringBuilder();
            while (msg[pos] != 0)
            {
                sb.Append((char)msg[pos]);
                pos += 1;
            }
            pos += 1; // Also skip the Cstring part.
            return sb.ToString();
        }

        String ReadString(int len)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < len; i++)
            {
                sb.Append((char)msg[pos + i]);
            }
            pos += 1; // Also skip the trailing 0x00
            return sb.ToString();

        }

        public Element GetNextElement()
        {
            // Return a sentinel MinKey type when we reach the end of the message.
            if (pos >= len)
            {
                el.type = ElementType.MinKey;
                return el;
            }

            el.type = (ElementType)msg[pos];
            pos += 1;

            if (el.type == ElementType.EndOfDocument)
            {
                return el;
            }
            else
            {
                el.name = ReadCString();
            }

            int item_len;
            switch (el.type)
            {
                case ElementType.Double:
                    el.dbl_val = BitConverter.ToDouble(msg, pos);
                    pos += 8;
                    break;

                case ElementType.String:
                case ElementType.JavaScript:
                case ElementType.Deprecatedx0E:
                    item_len = BitConverter.ToInt32(msg, pos);
                    pos += 4;
                    el.str_val = ReadString(item_len);
                    pos += item_len;
                    break;

                case ElementType.SubDocument:
                case ElementType.Array:
                    // Just eat the 'length' field for the subdocument, we don't care.
                    pos += 4;
                    break;

                case ElementType.Binary:
                    item_len = BitConverter.ToInt32(msg, pos);
                    pos += 4;
                    el.subtype = (int)msg[pos];
                    pos += 1;
                    el.bin_val = msg.Skip(pos).Take(item_len).ToArray();
                    pos += item_len;
                    break;

                case ElementType.Deprecatedx06:
                case ElementType.Null:
                case ElementType.MinKey:
                case ElementType.MaxKey:
                    break;

                case ElementType.ObjectId:
                    el.bin_val = msg.Skip(pos).Take(12).ToArray();
                    pos += 12;
                    break;

                case ElementType.Boolean:
                    el.bln_val = BitConverter.ToBoolean(msg, pos);
                    pos += 1;
                    break;

                case ElementType.Regex:
                    ReadCString();
                    ReadCString();
                    break;

                case ElementType.UTCDateTime:
                case ElementType.MongoTimeStamp:
                case ElementType.Int64:
                    el.i64_val = BitConverter.ToInt64(msg, pos);
                    pos += 8;
                    break;

                case ElementType.Int32:
                    el.i32_val = BitConverter.ToInt32(msg, pos);
                    pos += 4;
                    break;
            }

            return el;
        }

        Element el;
        byte[] msg;
        Int64 len;
        int pos;
    }

    class BSONWriter
    {
        public BSONWriter(int is_array = -1)
        {
            bytes = new MemoryStream();
            bw = new BinaryWriter(bytes);
            this.is_array = is_array;
            pos = 4;
            child = null;
        }

        String PrintArrayName()
        {
            String r = is_array.ToString();
            is_array += 1;
            return r;
        }

        void WriteCString(String s)
        {
            foreach (char c in s)
            {
                bw.Write(c);
            }
            bw.Write((byte)0);
        }

        public bool PushBool(bool v)
        {
            return PushBool("", v);
        }

        public bool PushBool(String name, bool v)
        {
            if (child != null)
            {
                return child.PushBool(name, v);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Boolean);
                WriteCString(name);
                bw.Write(v);
                return true;
            }
        }

        public bool PushInt32(Int32 v)
        {
            return PushInt32("", v);
        }

        public bool PushInt32(String name, Int32 v)
        {
            if (child != null)
            {
                return child.PushInt32(name, v);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Int32);
                WriteCString(name);
                bw.Write(v);
                return true;
            }
        }

        public bool PushInt64(Int64 v)
        {
            return PushInt64("", v);
        }

        public bool PushInt64(String name, Int64 v, BSONReader.ElementType type = BSONReader.ElementType.Int64)
        {
            if (child != null)
            {
                return child.PushInt64(name, v);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)type);
                WriteCString(name);
                bw.Write(v);
                return true;
            }
        }

        public bool PushDouble(double v)
        {
            return PushDouble("", v);
        }

        public bool PushDouble(String name, double v)
        {
            if (child != null)
            {
                return child.PushDouble(name, v);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Double);
                WriteCString(name);
                bw.Write(v);
                return true;
            }
        }

        public bool PushString(String v)
        {
            return PushString("", v);
        }

        public bool PushString(String name, String v)
        {
            if (child != null)
            {
                return child.PushString(name, v);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                byte[] str_bytes = Encoding.Unicode.GetBytes(v.ToCharArray());
                bw.Write((byte)BSONReader.ElementType.String);
                WriteCString(name);
                bw.Write((Int32)(str_bytes.Length));
                bw.Write(str_bytes);
                return true;
            }
        }

        public bool PushBinary(byte[] v)
        {
            return PushBinary("", v);
        }

        public bool PushBinary(String name, byte[] v, byte subtype = 0)
        {
            if (child != null)
            {
                return child.PushBinary(name, v, subtype);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Binary);
                WriteCString(name);
                bw.Write(subtype);
                bw.Write((Int32)v.Length);
                bw.Write(v);
                return true;
            }
        }

        public bool PushArray()
        {
            return PushArray("");
        }

        public bool PushArray(String name)
        {
            if (child != null)
            {
                return child.PushArray(name);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Array);
                WriteCString(name);

                child = new BSONWriter(0);
                return true;
            }
        }

        public bool PushSubdoc()
        {
            return PushSubdoc("");
        }

        public bool PushSubdoc(String name)
        {
            if (child != null)
            {
                return child.PushSubdoc(name);
            }
            else
            {
                name = (is_array != -1 ? PrintArrayName() : name);
                bw.Write((byte)BSONReader.ElementType.Array);
                WriteCString(name);

                child = new BSONWriter();
                return true;
            }
        }

        BSONWriter child;
        int pos;
        int is_array;
        MemoryStream bytes;
        BinaryWriter bw;
    }
}
