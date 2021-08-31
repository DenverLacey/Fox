//
//  value.cpp
//  Fox
//
//  Created by Denver Lacey on 8/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "value.h"

#include <iostream>
#include <sstream>
#include <assert.h>

#include "error.h"

Size Value_Type::size() const {
    switch (kind) {
        case Value_Type_Kind::Unresolved_Type:
            return 0;
        case Value_Type_Kind::None:
            return 0;
        case Value_Type_Kind::Void:
            return 0;
        case Value_Type_Kind::Bool:
            return sizeof(runtime::Bool);
        case Value_Type_Kind::Char:
            return sizeof(runtime::Char);
        case Value_Type_Kind::Int:
            return sizeof(runtime::Int);
        case Value_Type_Kind::Float:
            return sizeof(runtime::Float);
        case Value_Type_Kind::Str:
            return sizeof(runtime::String);
        case Value_Type_Kind::Ptr:
            return sizeof(runtime::Pointer);
        case Value_Type_Kind::Tuple: {
            Size size = 0;
            for (size_t i = 0; i < data.tuple.subtypes.size(); i++) {
                size += data.tuple.subtypes[i].size();
            }
            return size;
        }
            
        case Value_Type_Kind::Struct:
        case Value_Type_Kind::Enum:
            internal_error("Struct and Enums not yet implemented.");
            return 0;
            
        default:
            internal_error("Unknown value type: %d.", kind);
            return 0;
    }
}

char *Value_Type::debug_str() const {
    std::ostringstream s;
    
    if (is_mut) {
        s << "mut ";
    }
    
    switch (kind) {
        case Value_Type_Kind::Unresolved_Type:
            s << "<?>";
            break;
        case Value_Type_Kind::None:
            s << "<NONE>";
            break;
        case Value_Type_Kind::Void:
            s << "void";
            break;
        case Value_Type_Kind::Bool:
            s << "bool";
            break;
        case Value_Type_Kind::Char:
            s << "char";
            break;
        case Value_Type_Kind::Int:
            s << "int";
            break;
        case Value_Type_Kind::Float:
            s << "float";
            break;
        case Value_Type_Kind::Str:
            s << "str";
            break;
        case Value_Type_Kind::Ptr:
            s << "*";
            s << data.ptr.subtype->debug_str();
            break;
        case Value_Type_Kind::Tuple:
            s << "(";
            for (size_t i = 0; i < data.tuple.subtypes.size(); i++) {
                s << data.tuple.subtypes[i].debug_str();
                if (i + 1 < data.tuple.subtypes.size()) s << ", ";
            }
            s << ")";
            break;
        case Value_Type_Kind::Struct:
            assert(false);
            break;
        case Value_Type_Kind::Enum:
            assert(false);
            break;
    }
    
    char *debug_str = strndup(s.str().c_str(), s.str().size()); // @Leak
    return debug_str;
}

bool operator==(const Value_Type &a, const Value_Type &b) {
    if (a.kind != b.kind) {
        return false;
    }
    
    switch (a.kind) {
        case Value_Type_Kind::Ptr:
            return *a.data.ptr.subtype == *b.data.ptr.subtype;
        case Value_Type_Kind::Tuple:
            if (a.data.tuple.subtypes.size() != b.data.tuple.subtypes.size()) {
                return false;
            }
            for (size_t i = 0; i < a.data.tuple.subtypes.size(); i++) {
                auto ai = a.data.tuple.subtypes[i];
                auto bi = b.data.tuple.subtypes[i];
                if (ai != bi) {
                    return false;
                }
            }
            break;
        case Value_Type_Kind::Struct:
            assert(false);
        case Value_Type_Kind::Enum:
            assert(false);
    }
    
    return true;
}

bool operator!=(const Value_Type &a, const Value_Type &b) {
    return !(a == b);
}

Size Tuple_Type_Data::offset_of_type(size_t idx) {
    Size size = 0;
    for (size_t i = 0; i < idx; i++) {
        size += subtypes[i].size();
    }
    return size;
}

namespace value_types {
    Value_Type ptr_to(Value_Type *subtype) {
        auto pty = Ptr;
        pty.data.ptr.subtype = subtype;
        return pty;
    }
    
    void copy_subtypes_into_buffer(
        Value_Type *buffer,
        const std::initializer_list<Value_Type> &subtypes)
    {
        for (auto it = subtypes.begin(); it != subtypes.end(); it++, buffer++) {
            *buffer = *it;
        }
    }
    
    Value_Type tup_from(size_t count, Value_Type *subtypes) {
        Value_Type ty;
        ty.kind = Value_Type_Kind::Tuple;
        ty.data.tuple.subtypes = Array { count, subtypes };
        return ty;
    }
}
