//
//  definitions.h
//  Fox
//
//  Created by Denver Lacey on 17/10/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "String.h"
#include "value.h"
#include "typedefs.h"
#include "codelocation.h"

#include <unordered_map>

struct Module;

struct Function_Definition {
    bool varargs;
    UUID uuid;
    Module *module;
    String name;
    Value_Type type;
    std::vector<String> param_names;
    std::vector<uint8_t> instructions;
};

struct Struct_Field {
    Size offset;
    String id;
    Value_Type type;
};

struct Method {
    bool is_static;
    UUID uuid;
};

struct Struct_Definition {
    Size size;
    // Struct_Definition *super;
    UUID uuid;
    Module *module;
    String name;
    std::vector<Struct_Field> fields;
    std::unordered_map<std::string, Method> methods;
    // std::vector<Ref<Typed_AST>> initializer;
    
    bool has_field(String id);
    Struct_Field *find_field(String id);
    bool has_method(String id);
    bool find_method(String id, Method &out_method);
};

struct Enum_Payload_Field {
    Size offset;
    Value_Type type;
};

struct Enum_Variant {
    runtime::Int tag;
    String id;
    std::vector<Enum_Payload_Field> payload;
};

struct Enum_Definition {
    bool is_sumtype;
    Size size;
    UUID uuid;
    Module *module;
    String name;
    std::vector<Enum_Variant> variants;
    std::unordered_map<std::string, Method> methods;
    
    Enum_Variant *find_variant(String id);
    Enum_Variant *find_variant_by_tag(runtime::Int tag);
    bool has_method(String id);
    bool find_method(String id, Method &out_method);
};
