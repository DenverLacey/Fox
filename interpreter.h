//
//  interpreter.h
//  Fox
//
//  Created by Denver Lacey on 20/9/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "vm.h"

#include "Bucket_Array.h"

#include <unordered_map>
#include <string>

struct Module {
    String module_path;
    std::unordered_map<UUID, size_t> structs;
    std::unordered_map<UUID, size_t> enums;
};

struct Type_Book {
    Bucket_Array<Struct_Definition> structs;
    Bucket_Array<Enum_Definition> enums;
    
    size_t add_struct(const Struct_Definition &defn);
    size_t add_enum(const Enum_Definition &defn);
};

struct Interpreter {
    UUID current_uuid = 0;
    Type_Book typebook;
    Bucket_Array<Module> modules;
    
    void interpret(const char *filepath);
    Module *create_module(String module_path);
    UUID next_uuid();
};
