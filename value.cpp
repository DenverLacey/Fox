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
        case Value_Type_Kind::Array:
            return data.array.count * data.array.element_type->size();
        case Value_Type_Kind::Slice:
            return sizeof(runtime::Slice);
        case Value_Type_Kind::Tuple: {
            Size size = 0;
            for (size_t i = 0; i < data.tuple.child_types.size(); i++) {
                size += data.tuple.child_types[i].size();
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
            s << data.ptr.child_type->debug_str();
            break;
        case Value_Type_Kind::Array:
            s << "[" << data.array.count << "]";
            s << data.array.element_type->debug_str();
            break;
        case Value_Type_Kind::Slice:
            s << "[]";
            s << data.slice.element_type->debug_str();
            break;
        case Value_Type_Kind::Tuple:
            s << "(";
            for (size_t i = 0; i < data.tuple.child_types.size(); i++) {
                s << data.tuple.child_types[i].debug_str();
                if (i + 1 < data.tuple.child_types.size()) s << ", ";
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
    
    char *debug_str = SMem.duplicate(s.str().c_str(), s.str().size());
    return debug_str;
}

Value_Type *Value_Type::child_type() {
    switch (kind) {
        case Value_Type_Kind::Ptr:
            return data.ptr.child_type;
        case Value_Type_Kind::Array:
            return data.array.element_type;
        case Value_Type_Kind::Slice:
            return data.slice.element_type;
        case Value_Type_Kind::Tuple:
            return data.tuple.child_types.data();
    }
    
    return nullptr;
}

Value_Type Value_Type::clone() const {
    Value_Type ty;
    ty.kind = kind;
    switch (kind) {
        case Value_Type_Kind::Unresolved_Type:
            ty.data.unresolved.id = data.unresolved.id.clone();
            break;
        case Value_Type_Kind::Ptr: {
            Value_Type *child = Mem.make<Value_Type>().as_ptr();
            *child = data.ptr.child_type->clone();
            ty.data.ptr.child_type = child;
        } break;
        case Value_Type_Kind::Array: {
            Value_Type *child = Mem.make<Value_Type>().as_ptr();
            *child = data.array.element_type->clone();
            ty.data.array.element_type = child;
        } break;
        case Value_Type_Kind::Slice: {
            Value_Type *child = Mem.make<Value_Type>().as_ptr();
            *child = data.array.element_type->clone();
            ty.data.slice.element_type = child;
        } break;
        case Value_Type_Kind::Tuple:
            ty.data.tuple.child_types = data.tuple.child_types.clone();
            break;
            
        case Value_Type_Kind::None:  break;
        case Value_Type_Kind::Void:  break;
        case Value_Type_Kind::Bool:  break;
        case Value_Type_Kind::Char:  break;
        case Value_Type_Kind::Int:   break;
        case Value_Type_Kind::Float: break;
        case Value_Type_Kind::Str:   break;
            
        case Value_Type_Kind::Struct:
        case Value_Type_Kind::Enum:
            internal_error("Struct and Enum can't be cloned yet.");
            break;
            
        default:
            internal_error("Unknown Value_Type_Kind: %d.", kind);
            break;
    }
    
    return ty;
}

bool Value_Type::eq(const Value_Type &other) {
    if (is_mut != other.is_mut) {
        return false;
    }
    return eq_ignoring_mutability(other);
}

bool Value_Type::eq_ignoring_mutability(const Value_Type &other) {
    if (kind != other.kind) {
        return false;
    }
    
    bool match = true;
    switch (kind) {
        case Value_Type_Kind::Ptr:
            match = data.ptr.child_type->eq(*other.data.ptr.child_type);
            break;
        case Value_Type_Kind::Array:
            match = data.array.count == other.data.array.count && data.array.element_type->eq_ignoring_mutability(*other.data.array.element_type);
            break;
        case Value_Type_Kind::Slice:
            match = data.slice.element_type->eq(*other.data.slice.element_type);
            break;
        case Value_Type_Kind::Tuple:
            if (data.tuple.child_types.size() != other.data.tuple.child_types.size()) {
                match = false;
            } else {
                for (size_t i = 0; i < data.tuple.child_types.size(); i++) {
                    auto ai = data.tuple.child_types[i];
                    auto bi = other.data.tuple.child_types[i];
                    if (!ai.eq_ignoring_mutability(bi)) {
                        match = false;
                        break;
                    }
                }
            }
            break;
        case Value_Type_Kind::Struct:
            assert(false);
        case Value_Type_Kind::Enum:
            assert(false);
    }
    
    return match;
}

bool Value_Type::assignable_from(const Value_Type &other) {
    if (kind != other.kind) {
        return false;
    }
    
    bool match = true;
    switch (kind) {
        case Value_Type_Kind::Ptr:
            if (data.ptr.child_type->is_mut) {
                match = data.ptr.child_type->eq(*other.data.ptr.child_type);
            } else {
                match = data.ptr.child_type->eq_ignoring_mutability(*other.data.ptr.child_type);
            }
            break;
        case Value_Type_Kind::Array:
            match = data.array.count == other.data.array.count;
            if (!match) break;
            
            if (data.array.element_type->is_mut) {
                match = data.array.element_type->eq(*other.data.array.element_type);
            } else {
                match = data.array.element_type->eq_ignoring_mutability(*other.data.array.element_type);
            }
            break;
        case Value_Type_Kind::Slice:
            if (data.slice.element_type->is_mut) {
                match = data.slice.element_type->eq(*other.data.slice.element_type);
            } else {
                match = data.slice.element_type->eq_ignoring_mutability(*other.data.slice.element_type);
            }
            break;
        case Value_Type_Kind::Tuple:
            if (data.tuple.child_types.size() != other.data.tuple.child_types.size()) {
                match = false;
            } else {
                for (size_t i = 0; i < data.tuple.child_types.size(); i++) {
                    auto ai = data.tuple.child_types[i];
                    auto bi = other.data.tuple.child_types[i];
                    if (!ai.eq_ignoring_mutability(bi)) {
                        match = false;
                        break;
                    }
                }
            }
            break;
        case Value_Type_Kind::Struct:
            assert(false);
        case Value_Type_Kind::Enum:
            assert(false);
    }
    
    return match;
}

Size Tuple_Type_Data::offset_of_type(size_t idx) {
    Size offset = 0;
    for (size_t i = 0; i < idx; i++) {
        offset += child_types[i].size();
    }
    return offset;
}

namespace value_types {
Value_Type ptr_to(Value_Type *child_type) {
    auto pty = Ptr;
    pty.data.ptr.child_type = child_type;
    return pty;
}

Value_Type array_of(size_t count, Value_Type *element_type) {
    Value_Type ty;
    ty.kind = Value_Type_Kind::Array;
    ty.data.array = { count, element_type };
    return ty;
}

Value_Type slice_of(Value_Type *element_type) {
    Value_Type ty;
    ty.kind = Value_Type_Kind::Slice;
    ty.data.slice = { element_type };
    return ty;
}
    
Value_Type tup_from(size_t count, Value_Type *child_types) {
    Value_Type ty;
    ty.kind = Value_Type_Kind::Tuple;
    ty.data.tuple.child_types = ::Array { count, child_types };
    return ty;
}
} // namespace value_types
