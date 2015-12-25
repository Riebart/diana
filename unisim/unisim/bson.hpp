#ifndef BSON_HPP
#define BSON_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Partial implementation of bson that is self-contained and simple, both implementation
// and API.
//
// Because of how it is architected, it is intended for smaller messages as it does not
// support streaming input mechanisms.
//
// It supports reading flat documents (documents without nested documents), and types:
//     1, 2, 3, 4, 5 (subtype 0), 9, 16, 17, 18
//
// It supports reading flat documents (documents without nested documents), and types:
//     (in their hex ID) 1, 2, 8, 9, 10, 11, 12
//
// See: http://bsonspec.org/spec.html and http://bsonspec.org/faq.html

// Implements an element-by-element BSON reader that ingests a pointer to a byte-array.
class BSONReader
{
public:
    enum ElementType
    {
        EndOfDocument = 0, Double = 1, String = 2, SubDocument = 3, Array = 4,
        Binary = 5, Deprecatedx06 = 6, ObjectId = 7, Boolean = 8, UTCDateTime = 9,
        Null = 10, Regex = 11, DBPointer = 12, JavaScript = 13, Deprecatedx0E = 14,
        JavaScriptWScope = 15, Int32 = 16, MongoTimeStamp = 17, Int64 = 18,
        MinKey = (int8_t)0xFF, MaxKey = 0x7F
    };

    // Every element is returned in this structure, with the type field and the relevant
    // value member filled. Other values that don't map to the type are not guaranteed to
    // have any reasonable values.
    struct Element
    {
        ElementType type;
        int8_t subtype;
        char* name;

        bool     bln_val;
        int32_t  i32_val;
        int64_t  i64_val;
        double   dbl_val;

        int32_t  bin_len;
        uint8_t* bin_val;

        char*    str_val;
        int32_t  str_len;
    };

    BSONReader(char* _msg)
    {
        this->msg = _msg;
        len = *(int32_t*)(msg);
        pos = 4;
    }

    struct Element get_next_element()
    {
        // Return a sentinel MinKey type when we reach the end of the message
        if ((int32_t)pos >= len)
        {
            el.type = ElementType::MinKey;
            return el;
        }

        el.type = (ElementType)(*(int8_t*)(msg + pos));
        pos += 1;

        // If it's an EOF document, it doesn't have a name, so don't try that. Just return.
        if (el.type == ElementType::EndOfDocument)
        {
            return el;
        }
        else
        {
            // Otherwise, read the name.
            el.name = msg + pos;
            pos += strlen(msg + pos) + 1;;
        }

        // Switch on the element types.
        switch (el.type)
        {
        case ElementType::Double:
            el.dbl_val = *(double*)(msg + pos);
            pos += 8;
            break;

        case ElementType::String:
        case ElementType::JavaScript:
        case ElementType::Deprecatedx0E:
            el.str_len = *(int32_t*)(msg + pos);
            pos += 4;
            el.str_val = msg + pos;
            pos += el.str_len;
            break;

        case ElementType::SubDocument:
        case ElementType::Array:
            // Just eat the 'length' field for the subdocument, we don't care.
            pos += 4;
            break;

        case ElementType::Binary:
            el.bin_len = *(int32_t*)(msg + pos);
            pos += 4;
            el.subtype = *(int8_t*)(msg + pos);
            pos += 1;
            el.bin_val = (uint8_t*)(msg + pos);
            pos += el.bin_len;
            break;

        case ElementType::Deprecatedx06:
        case ElementType::Null:
        case ElementType::MinKey:
        case ElementType::MaxKey:
            break;

        case ElementType::ObjectId:
            el.bin_len = 12;
            el.bin_val = (uint8_t*)(msg + pos);
            pos += 12;
            break;

        case ElementType::Boolean:
            el.bln_val = *(bool*)(msg + pos);
            pos += 1;
            break;

        case ElementType::Regex:
            pos += strlen(msg + pos) + 1;
            pos += strlen(msg + pos) + 1;
            break;

        case ElementType::UTCDateTime:
        case ElementType::MongoTimeStamp:
        case ElementType::Int64:
            el.i64_val = *(int64_t*)(msg + pos);
            pos += 8;
            break;

        case ElementType::Int32:
            el.i32_val = *(int32_t*)(msg + pos);
            pos += 4;
            break;
        default:
            throw "UnrecognizedType";
            break;
        }

        return el;
    }

private:
    struct Element el;
    int32_t len;
    uint64_t pos;
    char* msg;

    BSONReader()
    {
    }
};

class BSONWriter
{
public:
    BSONWriter()
    {
        out = NULL;
        child = NULL;
        reset();
    }

    void reset()
    {
        if (child != NULL)
        {
            delete child;
        }

        if (out != NULL)
        {
            free(out);
        }

        // Allocate enough memory for the length starting field.
        // Each time we push something, we will realloc the block a bit bigger.
        out = (uint8_t*)calloc(1, 4);
        if (out == NULL)
        {
            throw "OOM you twat";
        }

        is_array = false;
        tag_index = 0;
        
        pos = 4;
        child = NULL;
    }

    bool push(bool v)
    {
        return push((char*)NULL, v);
    }

    bool push(char* name, bool v)
    {
        if (child != NULL)
        {
            return child->push(name, v);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len + 1);
            out[pos] = BSONReader::ElementType::Boolean;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;
            *(bool*)(out + pos) = v;
            pos += 1;
            return true;
        }
    }

    bool push(int32_t v)
    {
        return push((char*)NULL, v);
    }
    
    bool push(char* name, int32_t v)
    {
        if (child != NULL)
        {
            return child->push(name, v);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len + 4);
            out[pos] = BSONReader::ElementType::Int32;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;
            *(int32_t*)(out + pos) = v;
            pos += 4;
            return true;
        }
    }

    bool push(int64_t v)
    {
        return push((char*)NULL, v);
    }
    
    bool push(char* name, int64_t v, int8_t type = BSONReader::ElementType::Int64)
    {
        if (child != NULL)
        {
            return child->push(name, v, type);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);

            switch (type)
            {
            case BSONReader::ElementType::Int64:
            case BSONReader::ElementType::UTCDateTime:
            case BSONReader::ElementType::MongoTimeStamp:
                enlarge(1 + name_len + 8);
                out[pos] = type;
                pos += 1;
                memcpy(out + pos, name, name_len);
                pos += name_len;
                *(int64_t*)(out + pos) = v;
                pos += 8;
                return true;
            default:
                return false;
            }
        }
    }

    bool push(double v)
    {
        return push((char*)NULL, v);
    }
    
    bool push(char* name, double v)
    {
        if (child != NULL)
        {
            return child->push(name, v);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len + 8);
            out[pos] = BSONReader::ElementType::Double;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;
            *(double*)(out + pos) = v;
            pos += 8;
            return true;
        }
    }

    bool push(char* v)
    {
        return push((char*)NULL, v);
    }
    
    bool push(char* name, char* v, int32_t len = -1, int8_t type = BSONReader::ElementType::String)
    {
        if (child != NULL)
        {
            return child->push(name, v, len, type);
        }
        else
        {
            if (len < 0)
            {
                len = strlen(v);
            }
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);

            switch (type)
            {
            case BSONReader::ElementType::String:
            case BSONReader::ElementType::JavaScript:
            case BSONReader::ElementType::Deprecatedx0E:
                enlarge(1 + name_len + 4 + len + 1); // Type, name, str_len, string, 0x00
                out[pos] = type;
                pos += 1;
                memcpy(out + pos, name, name_len);
                pos += name_len;
                *(int32_t*)(out + pos) = (int32_t)(len + 1); // string + 0x00
                pos += 4;
                memcpy(out + pos, v, len);
                pos += len;
                out[pos] = 0;
                pos += 1;
                return true;
            default:
                return false;
            }
        }
    }

    bool push(uint8_t* bin, int32_t len)
    {
        return push((char*)NULL, bin, len);
    }

    bool push(char* name, uint8_t* bin, int32_t len, uint8_t subtype = 0)
    {
        if (child != NULL)
        {
            return child->push(name, bin, len, subtype);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len + 4 + 1 + len); // Type, name, length, subtype, data
            out[pos] = BSONReader::ElementType::Binary;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;
            *(int32_t*)(out + pos) = len;
            pos += 4;
            *(uint8_t*)(out + pos) = subtype;
            pos += 1;
            memcpy(out + pos, bin, len);
            pos += len;
            return true;
        }
    }

    bool push_array()
    {
        return push_array((char*)NULL);
    }
    
    bool push_array(char* name)
    {
        if (child != NULL)
        {
            return child->push_array(name);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len);
            out[pos] = BSONReader::ElementType::Array;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;

            child = new BSONWriter(true);
            return true;
        }
    }

    bool push_subdoc()
    {
        return push_subdoc((char*)NULL);
    }
    
    bool push_subdoc(char* name)
    {
        if (child != NULL)
        {
            return child->push_subdoc(name);
        }
        else
        {
            name = print_array_name(name);
            int32_t name_len = (int32_t)(strlen(name) + 1);
            enlarge(1 + name_len);
            out[pos] = BSONReader::ElementType::SubDocument;
            pos += 1;
            memcpy(out + pos, name, name_len);
            pos += name_len;

            child = new BSONWriter();
            return true;
        }
    }

    uint8_t* push_end()
    {
        if (child != NULL)
        {
            uint8_t* child_out = child->push_end();
            if (child_out != NULL)
            {
                int32_t child_len = *(int32_t*)child_out;
                enlarge(child_len);
                memcpy(out + pos, child_out, child_len);
                pos += child_len;
                //free(child_out); // Note that this happens during the child's destructor.
                delete child;
                child = NULL;
            }
            return NULL;
        }
        else
        {
            enlarge(1);
            out[pos] = 0;
            *(int32_t*)out = (int32_t)(pos + 1); // Set the first 4 bytes to the length
            return out;
        }
    }

    ~BSONWriter()
    {
        free(out);
        if (child != NULL)
        {
            delete child;
        }
    }

private:
    BSONWriter(bool _is_array)
    {
        out = NULL;
        child = NULL;
        reset();
        is_array = _is_array;
    }
    
    bool is_array;
    int32_t tag_index = 0;
    char array_name[10];
    
    uint8_t* out;
    uint64_t pos;
    BSONWriter* child;

    void enlarge(size_t nbytes)
    {
        uint8_t* out_new = (uint8_t*)realloc(out, (size_t)(pos + nbytes));
        if (out_new == NULL)
        {
            throw "OOM you twat";
        }
        else
        {
            out = out_new;
        }
    }

    char* print_array_name(char* name)
    {
        // If we're an array, we don't care what the name is, print the numeric
        // value of the tag_index into the char buffer, and return that.
        if (is_array)
        {
            // Windows notes that sprintf_s is available.
#ifdef WIN32
            sprintf_s(array_name, "%d", tag_index);
#else
            sprintf(array_name, "%d", tag_index);
#endif
            tag_index += 1;
            return array_name;
        }
        // If the name is NULL, one was unspecified, so fill it in with a one-character
        // tag
        else if (name == NULL)
        {
            // Note that ANSI characters in the range of (decimal) 32 - 126 inclusive are
            // printable, but this just uses this as an 8-bit unsigned field.
            if (tag_index > 254)
            {
                throw "OutOfMinimalTags";
            }

            // Because we're using the actual integer values, we need to start at 1, otherwise we'll have a pair of NULL characters.
            array_name[0] = (char)(tag_index + 1);
            array_name[1] = 0;
            tag_index++;
            return array_name;
        }
        // If it's not an array, and the name isn't NULL (that is, it is specified), 
        // then use the name we got.
        else
        {
            return name;
        }
    }
};

#endif

//// f30000000461727200a90000000530000900000000537472756e676f757408310001103200f6ffffff123300cce32320fdffffff013400bbbdd7d9df7cdb3d04350038000000083000001031009cffffff12320000b21184170000000133005a62d7d718e774690534000a0000000053747261696768747570000336003400000005610004000000006273747210693300fa0500000862000001640047e51aaeab44e93f1269360001f05a2b17ffffff00000164626c00333333333333144010693332005000000012693634000010a5d4e800000008626c6e000005737472000d00000000537472696e6779737472696e6700
//// {'arr': ['Strungout', True, -10, -12345678900, 1e-10, [False, -100, 101000000000, 1e+200, 'Straightup'], {'a': 'bstr', 'i3': 1530, 'b': False, 'd': 0.7896326447, 'i6': -999999999999}], 'dbl': 5.05, 'i32': 80, 'i64': 1000000000000, 'bln': False, 'str': 'Stringystring'}
//// The Python bson library doesn't output strings as strings, but as binary tyoe 0 arrays.
//BSONWriter bw;
//bw.push_array("arr");
//bw.push_binary((uint8_t*)"Strungout", 9);
//bw.push_bool(true);
//bw.push_int32(-10);
//bw.push_int64(-12345678900);
//bw.push_double(1e-10);
//bw.push_array();
//bw.push_bool(false);
//bw.push_int32(-100);
//bw.push_int64(101000000000);
//bw.push_double(1e200);
//bw.push_binary((uint8_t*)"Straightup", 10);
//bw.push_end();
//bw.push_subdoc();
//bw.push_binary("a", (uint8_t*)"bstr", 4);
//bw.push_int32("i3", 1530);
//bw.push_bool("b", false);
//bw.push_double("d", 0.7896326447);
//bw.push_int64("i6", -999999999999);
//bw.push_end();
//bw.push_end();
//bw.push_double("dbl", 5.05);
//bw.push_int32("i32", 80);
//bw.push_int64("i64", 1000000000000);
//bw.push_bool("bln", false);
//bw.push_binary("str", (uint8_t*)"Stringystring", 13);
//uint8_t* out = bw.push_end();
//for (int i = 0; i < *(int32_t*)out; i++)
//{
//    printf("%02x", out[i]);
//}
//printf("\n");
//return 0;
//BSONWriter bw;
//bw.push_string("This is a string.");
//uint8_t* bytes = bw.push_end();
//for (int i = 0; i < *(int32_t*)bytes; i++)
//{
//    printf("%d\n", bytes[i]);
//}
//return 0;
