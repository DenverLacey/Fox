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
#include <unordered_set>
#include <string>

struct Module {
    String module_path;
    std::unordered_set<UUID> structs;
    std::unordered_set<UUID> enums;
    std::unordered_set<UUID> funcs;
};

struct Type_Book {
    std::unordered_map<UUID, Struct_Definition> structs;
    std::unordered_map<UUID, Enum_Definition> enums;
    
    Struct_Definition *add_struct(const Struct_Definition &defn);
    Enum_Definition *add_enum(const Enum_Definition &defn);
    Struct_Definition *get_struct_by_uuid(UUID id);
    Enum_Definition *get_enum_by_uuid(UUID id);
};

struct Function_Book {
    std::unordered_map<UUID, Function_Definition> funcs;
    
    Function_Definition *add_func(const Function_Definition &defn);
    Function_Definition *get_func_by_uuid(UUID id);
};

struct Interpreter {
    UUID current_uuid = 0;
    Type_Book typebook;
    Function_Book funcbook;
    Bucket_Array<Module> modules;
    
    Data_Section constants;
    Data_Section str_constants;
    
    void interpret(const char *filepath);
    Module *create_module(String module_path);
    UUID next_uuid();
};
