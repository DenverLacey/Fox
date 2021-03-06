//
//  interpreter.h
//  Fox
//
//  Created by Denver Lacey on 20/9/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "builtins.h"
#include "definitions.h"
#include "vm.h"

#include <unordered_map>
#include <string>

struct Module {
    struct Member {
        enum : uint8_t {
            Struct,
            Enum,
            Function,
            Submodule
        } kind;
        UUID uuid;
    };
    
    UUID uuid;
    Function_Definition top_level;
    String module_path;
    std::unordered_map<std::string, Member> members;
    
    void add_struct_member(Struct_Definition *defn);
    void add_enum_member(Enum_Definition *defn);
    void add_func_member(Function_Definition *defn);
    void add_submodule(const std::string &id, Module *module);
    bool find_member_by_id(const std::string &id, Member &out_member);
    void merge(Module *other);
};

struct Types {
    std::unordered_map<UUID, Struct_Definition> structs;
    std::unordered_map<UUID, Enum_Definition> enums;
    std::unordered_map<UUID, Trait_Definition> traits;
    
    Struct_Definition *add_struct(const Struct_Definition &defn);
    Enum_Definition *add_enum(const Enum_Definition &defn);
    Trait_Definition *add_trait(const Trait_Definition &defn);
    Struct_Definition *get_struct_by_uuid(UUID uuid);
    Enum_Definition *get_enum_by_uuid(UUID uuid);
    Trait_Definition *get_trait_by_uuid(UUID uuid);
};

struct Functions {
    std::unordered_map<UUID, Function_Definition> funcs;
    
    Function_Definition *add_func(const Function_Definition &defn);
    Function_Definition *get_func_by_uuid(UUID uuid);
};

struct Modules {
    std::unordered_map<UUID, Module> modules;
    std::unordered_map<std::string, Module *> path_map;
    
    Module *add_module(const Module &mod);
    Module *get_module_by_uuid(UUID uuid);
    Module *get_module_by_path(String path);
};

struct Builtins {
    std::unordered_map<std::string, Builtin_Definition> builtins;
    
    void add_builtin(const std::string &id, Builtin_Definition builtin);
    Builtin_Definition *get_builtin(const std::string &id);
};

struct Interpreter {
    UUID current_uuid = 0;
    Types types;
    Functions functions;
    Builtins builtins;
    Modules modules;
    
    Data_Section constants;
    Data_Section str_constants;
    
    Interpreter();
    
    void interpret(const char *filepath);
    Module *create_module(String module_path);
    Module *get_module(String module_path);
    Module *get_or_create_module(String module_path);
    Module *compile_module(String module_path);
    UUID next_uuid();
};
