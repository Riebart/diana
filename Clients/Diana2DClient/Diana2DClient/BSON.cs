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
            bw.Write((Int32)0);
            this.is_array = is_array;
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
                switch (type)
                {
                    case BSONReader.ElementType.Int64:
                    case BSONReader.ElementType.UTCDateTime:
                    case BSONReader.ElementType.MongoTimeStamp:
                        name = (is_array != -1 ? PrintArrayName() : name);
                        bw.Write((byte)type);
                        WriteCString(name);
                        bw.Write(v);
                        return true;
                    default:
                        return false;
                }
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

        public bool PushString(String name, String v, BSONReader.ElementType type = BSONReader.ElementType.String)
        {
            if (child != null)
            {
                return child.PushString(name, v);
            }
            else
            {
                switch (type)
                {
                    case BSONReader.ElementType.String:
                    case BSONReader.ElementType.JavaScript:
                    case BSONReader.ElementType.Deprecatedx0E:
                        name = (is_array != -1 ? PrintArrayName() : name);
                        byte[] str_bytes = Encoding.UTF8.GetBytes(v.ToCharArray());
                        bw.Write((byte)BSONReader.ElementType.String);
                        WriteCString(name);
                        bw.Write((Int32)(str_bytes.Length) + 1); // Plus the 0x00 tail.
                        bw.Write(str_bytes);
                        bw.Write((byte)0);
                        return true;
                    default:
                        return false;
                }
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
                bw.Write((Int32)v.Length);
                bw.Write(subtype);
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
                bw.Write((byte)BSONReader.ElementType.SubDocument);
                WriteCString(name);

                child = new BSONWriter();
                return true;
            }
        }

        public byte[] PushEnd()
        {
            if (child != null)
            {
                byte[] child_bytes = child.PushEnd();
                if (child_bytes != null)
                {
                    bw.Write(child_bytes);
                    child = null;
                }
                return null;
            }
            else
            {
                bw.Write((byte)0);
                bw.Seek(0, SeekOrigin.Begin);
                bw.Write((Int32)bytes.Length);
                return bytes.ToArray();
            }
        }

        BSONWriter child;
        int is_array;
        MemoryStream bytes;
        BinaryWriter bw;
    }
}

//// f30000000461727200a90000000530000900000000537472756e676f757408310001103200f6ffffff123300cce32320fdffffff013400bbbdd7d9df7cdb3d04350038000000083000001031009cffffff12320000b21184170000000133005a62d7d718e774690534000a0000000053747261696768747570000336003400000005610004000000006273747210693300fa0500000862000001640047e51aaeab44e93f1269360001f05a2b17ffffff00000164626c00333333333333144010693332005000000012693634000010a5d4e800000008626c6e000005737472000d00000000537472696e6779737472696e6700
//// {'arr': ['Strungout', True, -10, -12345678900, 1e-10, [False, -100, 101000000000, 1e+200, 'Straightup'], {'a': 'bstr', 'i3': 1530, 'b': False, 'd': 0.7896326447, 'i6': -999999999999}], 'dbl': 5.05, 'i32': 80, 'i64': 1000000000000, 'bln': False, 'str': 'Stringystring'}
//BSONWriter bw = new BSONWriter();
//bw.PushArray("arr");
//bw.PushBinary(Encoding.ASCII.GetBytes("Strungout".ToCharArray()));
//bw.PushBool(true);
//bw.PushInt32(-10);
//bw.PushInt64(-12345678900);
//bw.PushDouble(1e-10);
//bw.PushArray();
//bw.PushBool(false);
//bw.PushInt32(-100);
//bw.PushInt64(101000000000);
//bw.PushDouble(1e200);
//bw.PushBinary(Encoding.ASCII.GetBytes("Straightup".ToCharArray()));
//bw.PushEnd();
//bw.PushSubdoc();
//bw.PushBinary("a", Encoding.ASCII.GetBytes("bstr".ToCharArray()));
//bw.PushInt32("i3", 1530);
//bw.PushBool("b", false);
//bw.PushDouble("d", 0.7896326447);
//bw.PushInt64("i6", -999999999999);
//bw.PushEnd();
//bw.PushEnd();
//bw.PushDouble("dbl", 5.05);
//bw.PushInt32("i32", 80);
//bw.PushInt64("i64", 1000000000000);
//bw.PushBool("bln", false);
//bw.PushBinary("str", Encoding.ASCII.GetBytes("Stringystring".ToCharArray()));
//byte[] bytes = bw.PushEnd();
//bw = new BSONWriter();
//bw.PushString("This is a string.");
//bytes = bw.PushEnd();
