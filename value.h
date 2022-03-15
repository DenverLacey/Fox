//
//  value.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stdint.h>
#include "Array.h"
#include "String.h"
#include "mem.h"
#include "codelocation.h"

struct Untyped_AST_Symbol;
struct Struct_Definition;
struct Enum_Definition;
struct Trait_Definition;

enum class Value_Type_Kind : uint8_t {
    None,
    Unresolved_Type,
    Void,
    Bool,
    Char,
    Int,
    Float,
    Str,
    Ptr,
    Array,
    Slice,
    Tuple,
    Range,
    Struct,
    Enum,
    Trait,
    Function,
    Type,
};

namespace runtime {
using Bool = bool;
using Char = char32_t;
using Int = int64_t;
using Float = double;
using Pointer = void *;

struct String {
    char *s;
    Int len;
};

struct Slice {
    void *data;
    Int count;
};
}  // namesapce runtime

struct Value_Type;

struct Unresolved_Type_Data {
    Untyped_AST_Symbol *symbol;
};

struct Ptr_Type_Data {
    Value_Type *child_type;
};

struct Array_Type_Data {
    size_t count;
    Value_Type *element_type;
};

struct Slice_Type_Data {
    Value_Type *element_type;
};

struct Tuple_Type_Data {
    Array<Value_Type> child_types;
    
    Size offset_of_type(size_t idx);
};

struct Range_Type_Data {
    bool inclusive;
    Value_Type *child_type;
};

struct Struct_Type_Data {
    Struct_Definition *defn;
};

struct Enum_Type_Data {
    Enum_Definition *defn;
};

struct Trait_Type_Data {
    Trait_Definition *defn;
    Value_Type *real_type; // optional
};

struct Function_Type_Data {
    Value_Type *return_type;
    Array<Value_Type> arg_types;
    
    Size arg_size() const;
};

struct Type_Type_Data {
    Value_Type *type;
};

union Value_Type_Data {
    Ptr_Type_Data ptr;
    Unresolved_Type_Data unresolved;
    Array_Type_Data array;
    Slice_Type_Data slice;
    Tuple_Type_Data tuple;
    Range_Type_Data range;
    Struct_Type_Data struct_;
    Enum_Type_Data enum_;
    Trait_Type_Data trait;
    Function_Type_Data func;
    Type_Type_Data type;
};

struct Value_Type {
    Value_Type_Kind kind;
    bool is_mut = false;
    Value_Type_Data data;
    
    Size size() const;
    char *debug_str() const;
    char *display_str() const;
    Value_Type *child_type();
    const Value_Type *child_type() const;
    Value_Type clone() const;
    
    bool eq(const Value_Type &other);
    bool eq_ignoring_mutability(const Value_Type &other);
    bool assignable_from(const Value_Type &other);
    bool is_resolved() const;
    bool is_partially_mutable() const;
};

namespace value_types {
inline const Value_Type None = { Value_Type_Kind::None };
inline const Value_Type Void = { Value_Type_Kind::Void };
inline const Value_Type Bool = { Value_Type_Kind::Bool };
inline const Value_Type Char = { Value_Type_Kind::Char };
inline const Value_Type Int = { Value_Type_Kind::Int };
inline const Value_Type Float = { Value_Type_Kind::Float };
inline const Value_Type Str = { Value_Type_Kind::Str };
inline const Value_Type Ptr = { Value_Type_Kind::Ptr };
inline const Value_Type Array = { Value_Type_Kind::Array };
inline const Value_Type Slice = { Value_Type_Kind::Slice };
inline const Value_Type Tuple = { Value_Type_Kind::Tuple };
inline const Value_Type Range = { Value_Type_Kind::Range };

Value_Type unresolved(Untyped_AST_Symbol *symbol);
Value_Type unresolved(String id, Code_Location location);
Value_Type ptr_to(Value_Type *child_type);
Value_Type array_of(size_t count, Value_Type *element_type);
Value_Type slice_of(Value_Type *element_type);
Value_Type range_of(bool inclusive, Value_Type *child_type);
Value_Type tup_from(size_t count, Value_Type *child_types);
Value_Type trait(Trait_Definition *defn, Value_Type *real_type);
Value_Type func(Value_Type *return_type, size_t arg_count, Value_Type *arg_types);
Value_Type type_of(Value_Type *type);

template<typename ...Ts>
Value_Type tup_of(Ts ...child_types) {
    size_t num_types = sizeof...(Ts);
    
    Value_Type *buffer = nullptr;
    if (num_types != 0) {
        buffer = new Value_Type[num_types];
        std::initializer_list<Value_Type> subtype_list = { child_types... };
        Value_Type *it = buffer;
        for (auto &ty : subtype_list) {
            *it = ty;
            it++;
        }
    }
    
    return tup_from(num_types, buffer);
}

template<typename ...Value_Types>
Value_Type func(Value_Type return_type, Value_Types ...param_types) {
    Value_Type *ret = Mem.make<Value_Type>().as_ptr();
    *ret = return_type;
    
    size_t num_params = sizeof...(Value_Types);
    Value_Type *params = unpack(param_types...);
    
    return func(ret, num_params, params);
}
} // namespace value_types
