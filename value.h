//
//  value.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stdint.h>
#include "String.h"
#include "mem.h"

enum class Value_Type_Kind : uint8_t {
    Unresolved_Type,
    None,
    Void,
    Bool,
    Char,
    Int,
    Float,
    Str,
    Ptr,
    Struct,
    Enum,
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

union Value {
    runtime::Bool b;
    runtime::Char c;
    runtime::Int i;
    runtime::Float f;
    runtime::String s;
    runtime::Pointer p;
};

struct Unresolved_Type_Data {
    String id;
};

struct Value_Type;
struct Ptr_Type_Data {
    Value_Type *subtype;
};

struct Struct_Type_Data {
    
};

struct Enum_Type_Data {
    
};

union Value_Type_Data {
    Ptr_Type_Data ptr;
    Unresolved_Type_Data unresolved;
    Struct_Type_Data struct_;
    Enum_Type_Data enum_;
};

struct Value_Type {
    Value_Type_Kind kind;
    bool is_mut = false;
    Value_Type_Data data;
    
    Size size() const;
    char *debug_str() const;
};

bool operator==(const Value_Type &a, const Value_Type &b);

namespace value_types {
inline const Value_Type None = { Value_Type_Kind::None };
inline const Value_Type Void = { Value_Type_Kind::Void };
inline const Value_Type Bool = { Value_Type_Kind::Bool };
inline const Value_Type Char = { Value_Type_Kind::Char };
inline const Value_Type Int = { Value_Type_Kind::Int };
inline const Value_Type Float = { Value_Type_Kind::Float };
inline const Value_Type Str = { Value_Type_Kind::Str };
inline const Value_Type Ptr = { Value_Type_Kind::Ptr };
}
