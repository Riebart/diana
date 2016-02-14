#ifndef BSON_HPP
#define BSON_HPP

#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdexcept>

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
        MinKey = -1, MaxKey = 0x7F, NoMoreData = -32768
    };

    // Every element is returned in this structure, with the type field and the relevant
    // value member filled. Other values that don't map to the type are not guaranteed to
    // have any reasonable values.
    struct Element
    {
        Element() :
            bin_val(NULL), str_val(NULL), map_val(NULL),
            managed_pointers(false) {}

        Element(struct Element* src) : Element()
        {
            copy(src);
        }

        ~Element()
        {
            if (managed_pointers)
            {
                delete name;
                delete bin_val;
                delete str_val;
                delete map_val;

                name = NULL;
                bin_val = NULL;
                str_val = NULL;
                map_val = NULL;
                managed_pointers = false;
            }
        }

        void copy(struct Element* src)
        {
            *this = *src;
            // Name isn't an optional field.
            name = new char[strlen(src->name) + 1];
            memcpy(name, src->name, strlen(src->name) + 1);

            if (src->bin_val != NULL)
            {
                bin_val = new uint8_t[bin_len];
                memcpy(bin_val, src->bin_val, bin_len);
            }

            if (src->str_val != NULL)
            {
                str_val = new char[str_len + 1];
                memcpy(str_val, src->str_val, str_len);
                // It's possible that the bin and str values are the same pointer,
                // so since we have the luxury, make the string value properly ended.
                str_val[str_len] = 0;
            }

            managed_pointers = true;
        }

        bool managed_pointers;

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

        std::map<std::string, struct Element>* map_val;
    };

    BSONReader(char* _msg)
    {
        this->msg = _msg;
        len = *(int32_t*)(msg);
        pos = 4;
    }

    struct Element* get_next_element(bool read_complex = false, bool from_here = false)
    {
        // Return a sentinel NoMoreData type when we reach the end of the message
        if ((int32_t)pos >= len)
        {
            el.type = ElementType::NoMoreData;
            return &el;
        }

        // If we're not trying to re-read from here, or if we are, but we're neither an Array or a Subdoc,
        // just continue as normal.
        if ((!from_here) || ((el.type != ElementType::Array) && (el.type != ElementType::SubDocument)))
        {
            el = Element();

            el.type = (ElementType)(*(int8_t*)(msg + pos));
            pos += 1;

            // If it's an EOF document, it doesn't have a name, so don't try that. Just return.
            if (el.type == ElementType::EndOfDocument)
            {
                el.name = msg + pos - 1;
                return &el;
            }
            else
            {
                // Otherwise, read the name.
                el.name = msg + pos;
                pos += strlen(msg + pos) + 1;
            }
        }
        else
        {
            // For complex types trying to re-read from here, backtrack. For all other times, the above block
            // applies instead.
            //
            // We only need to backtrack the amount that the below switch cases move it forward.
            pos -= 4;
        }

        // Switch on the element types.
        switch (el.type)
        {
        case ElementType::Double:
            el.dbl_val = *(double*)(msg + pos);
            el.i32_val = (int32_t)el.dbl_val;
            el.i64_val = (int64_t)el.dbl_val;
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
            // Eat the 'length' field for the subdocument, we don't care.
            pos += 4;
            if (read_complex)
            {
                // Allocate a new element to return once we're done reading the subdoc.
                struct Element root_el(&el);
                struct Element i_el;
                struct Element* elp;
                root_el.map_val = new std::map<std::string, struct Element>();
                elp = get_next_element(read_complex);
                while (el.type != EndOfDocument)
                {
                    i_el.copy(elp);
                    root_el.map_val->operator[](std::string(i_el.name)) = i_el;

                    // Because of recursion, don't throw away the mapped value (array/subdoc), since that's
                    // now being tracked in this higher level value. Set the mapped value to NULL to prevent
                    // this before we get the next value, since that process calls the destructor.
                    elp->map_val = NULL;
                    elp = get_next_element(read_complex);
                }

                // Note that i_el is about to go out of scope, so it's destructor will be called, despite the
                // fact that it's last value is stored in the map. Same with root_el.
                i_el.name = NULL;
                i_el.bin_val = NULL;
                i_el.str_val = NULL;
                i_el.map_val = NULL;
                el = root_el;
                root_el = i_el;
            }
            break;

        case ElementType::Binary:
            el.bin_len = *(int32_t*)(msg + pos);
            pos += 4;
            el.subtype = *(int8_t*)(msg + pos);
            pos += 1;
            el.bin_val = (uint8_t*)(msg + pos);
            pos += el.bin_len;
            // The Python BSON library has a habit of encoding strings as binary arrays.
            // Fill in the string pointer and length in case we were expecting a string.
            el.str_len = el.bin_len;
            el.str_val = (char*)el.bin_val;
            break;

        case ElementType::Deprecatedx06:
        case ElementType::Null:
        case ElementType::MinKey:
        case ElementType::MaxKey:
        case ElementType::EndOfDocument:
            break;

        case ElementType::ObjectId:
            el.bin_len = 12;
            el.bin_val = (uint8_t*)(msg + pos);
            pos += 12;
            break;

        case ElementType::Boolean:
            el.bln_val = *(bool*)(msg + pos);
            el.i32_val = (int32_t)el.bln_val;
            el.i64_val = (int64_t)el.bln_val;
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
            // The Python BSON library has a habit of not obeying integer types
            // Fill in the i32_val from the parsed value, as best we can, in case we were expected an i32
            el.i32_val = (int32_t)el.i64_val;
            el.dbl_val = (double)el.i64_val;
            el.bln_val = (el.i64_val != 0);
            pos += 8;
            break;

        case ElementType::Int32:
            el.i32_val = *(int32_t*)(msg + pos);
            // The Python BSON library has a habit of not obeying integer types
            // Fill in the i64_val from the parsed value, in case we were expected an i64
            el.i64_val = el.i32_val;
            el.dbl_val = (double)el.i32_val;
            el.bln_val = (el.i32_val != 0);
            pos += 4;
            break;
        default:
            throw new std::runtime_error("BSONReader::UnrecognizedType");
            break;
        }

        return &el;
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
            throw new std::runtime_error("OOM you twat");
        }

        is_array = false;
        tag_index = 0;

        pos = 4;
        child = NULL;
    }

    // Push a NOP, simply increment the tag_index.
    bool push()
    {
        print_array_name(NULL);
        return true;
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
            if (v == NULL)
            {
                return false;
            }

            if (len < 0)
            {
                len = (int32_t)strlen(v);
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

    bool push(std::map<std::string, struct BSONReader::Element>* map, BSONReader::ElementType type)
    {
        return push((char*)NULL, map, type);
    }

    bool push(char* name, std::map<std::string, struct BSONReader::Element>* map, BSONReader::ElementType type)
    {
        if (map == NULL)
        {
            return false;
        }

        switch (type)
        {
        case BSONReader::ElementType::Array:
            push_array(name);
            break;
        case BSONReader::ElementType::SubDocument:
            push_subdoc(name);
            break;
        default:
            return false;
        }

        struct BSONReader::Element* el;
        std::map<std::string, struct BSONReader::Element>::iterator it;
        for (it = map->begin(); it != map->end(); it++)
        {
            el = &(it->second);
            push(el);
        }

        push_end();

        return true;
    }

    bool push(struct BSONReader::Element* el)
    {
        // Switch on the element types.
        switch (el->type)
        {
        case BSONReader::ElementType::Double:
            push(el->name, el->dbl_val);
            break;

        case BSONReader::ElementType::String:
        case BSONReader::ElementType::JavaScript:
        case BSONReader::ElementType::Deprecatedx0E:
            push(el->name, el->str_val, el->type);
            break;

        case BSONReader::ElementType::SubDocument:
        case BSONReader::ElementType::Array:
            push(el->name, el->map_val, el->type);
            break;

        case BSONReader::ElementType::Binary:
            push(el->name, el->bin_val, el->bin_len);
            break;

        case BSONReader::ElementType::Deprecatedx06:
        case BSONReader::ElementType::Null:
        case BSONReader::ElementType::MinKey:
        case BSONReader::ElementType::MaxKey:
            break;

        case BSONReader::ElementType::ObjectId:
            break;

        case BSONReader::ElementType::Boolean:
            push(el->name, el->bln_val);
            break;

        case BSONReader::ElementType::Regex:
            break;

        case BSONReader::ElementType::UTCDateTime:
        case BSONReader::ElementType::MongoTimeStamp:
        case BSONReader::ElementType::Int64:
            push(el->name, el->i64_val, el->type);
            break;

        case BSONReader::ElementType::Int32:
            push(el->name, el->i32_val);
            break;

        default:
            throw new std::runtime_error("BSONWriter::UnrecognizedType");
            break;
        }

        return true;
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
            throw new std::runtime_error("OOM you twat");
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
#ifdef _WIN32
            sprintf_s(array_name, "%d", tag_index);
#else
            sprintf(array_name, "%d", tag_index);
#endif
            tag_index++;
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
                throw new std::runtime_error("OutOfMinimalTags");
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
