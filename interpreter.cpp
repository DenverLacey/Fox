//
//  interpreter.cpp
//  Fox
//
//  Created by Denver Lacey on 20/9/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "interpreter.h"

#include "compiler.h"
#include "error.h"
#include "tokenizer.h"
#include "parser.h"

#include <assert.h>
#include <fstream>

#define PRINT_DEBUG_DIAGNOSTICS 1
#define TYPECHECK 1
#define COMPILE_AST 1
#define RUN_VIRTUAL_MACHINE 1

static String read_entire_file(const char *path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    verify(file.is_open(), "'%s' could not be opened.", path);
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    String source = { SMem.allocate(size + 1), static_cast<size_t>(size) };
    file.read(source.c_str(), size);
    verify(file.good(), "Could not read from '%s'.", path);
    
    // guarantee null terminator
    source.c_str()[size] = 0;
    
    return source;
}

void Interpreter::interpret(const char *path) {
    Module *module = compile_module(const_cast<char *>(path));
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    printf("<MAIN>:\n");
    print_code(module->top_level.bytecode, constants, str_constants);
    
    for (auto &[_, fn] : functions.funcs) {
        printf("\n%.*s#%zu%s:\n", fn.name.size(), fn.name.c_str(), fn.uuid, fn.type.debug_str());
        print_code(fn.bytecode, constants, str_constants);
    }
#endif
    
    Mem.clear();
    SMem.clear();
    
#if RUN_VIRTUAL_MACHINE
    auto vm = VM { constants, str_constants };
    vm.call(&module->top_level, 0);
    vm.run();
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    vm.print_stack();
#endif
    
#endif // RUN_VIRTUAL_MACHINE
}

Module *Interpreter::create_module(String module_path) {
    Module mod;
    mod.uuid = next_uuid();
    mod.module_path = module_path;
    Module *new_mod = modules.add_module(mod);
    internal_verify(new_mod, "Module couldn't be successfully added to registry.");
    return new_mod;
}

Module *Interpreter::get_or_create_module(String module_path) {
    if (Module *mod = modules.get_module_by_path(module_path)) {
        return mod;
    }
    return create_module(module_path);
}

Module *Interpreter::compile_module(String module_path) {
    String source = read_entire_file(module_path.c_str());
    auto tokens = tokenize(source);
    
#if PRINT_DEBUG_DIAGNOSTICS
    for (size_t i = 0; i < tokens.size(); i++) {
        printf("%04zu: ", i);
        tokens[i].print();
    }
#endif
    
    auto ast = parse(tokens);
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    ast->print();
#endif

    Module *module = get_or_create_module(module_path);
    
#if TYPECHECK
    auto typed_ast = typecheck(*this, module, ast);
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    typed_ast->print(this);
#endif
    
#if COMPILE_AST
    auto global = Compiler { this, constants, str_constants, &module->top_level };
    global.compile(typed_ast);

#endif // COMPILE_AST
#endif // TYPECHECK
    
    return module;
}

UUID Interpreter::next_uuid() {
    return current_uuid++;
}

void Module::add_struct_member(Struct_Definition *defn) {
    std::string sid = defn->name.str();
    internal_verify(members.find(sid) == members.end(), "Attempted to add struct member with a duplicate name '%s'", sid.c_str());
    
    members[sid] = { Member::Struct, defn->uuid };
}

void Module::add_enum_member(Enum_Definition *defn) {
    std::string sid = defn->name.str();
    internal_verify(members.find(sid) == members.end(), "Attempted to add enum member with a duplicate name '%s'", sid.c_str());
    
    members[sid] = { Member::Enum, defn->uuid };
}

void Module::add_func_member(Function_Definition *defn) {
    std::string sid = defn->name.str();
    internal_verify(members.find(sid) == members.end(), "Attempted to add function member with a duplicate name '%s'", sid.c_str());
    
    members[sid] = { Member::Function, defn->uuid };
}

bool Module::find_member_by_id(const std::string &id, Member &out_member) {
    auto it = members.find(id);
    if (it == members.end()) return false;
    out_member = it->second;
    return true;
}

Struct_Definition *Types::add_struct(const Struct_Definition &defn) {
    internal_verify(structs.find(defn.uuid) == structs.end(), "Struct with duplicate UUID detected: #%zu", defn.uuid);
    structs[defn.uuid] = std::move(defn);
    return &structs[defn.uuid];
}

Enum_Definition *Types::add_enum(const Enum_Definition &defn) {
    internal_verify(enums.find(defn.uuid) == enums.end(), "Enum with duplicate UUID detected: #%zu", defn.uuid);
    enums[defn.uuid] = std::move(defn);
    return &enums[defn.uuid];
}

Struct_Definition *Types::get_struct_by_uuid(UUID uuid) {
    auto it = structs.find(uuid);
    if (it == structs.end()) return nullptr;
    return &structs[uuid];
}

Enum_Definition *Types::get_enum_by_uuid(UUID uuid) {
    auto it = enums.find(uuid);
    if (it == enums.end()) return nullptr;
    return &enums[uuid];
}

Function_Definition *Functions::add_func(const Function_Definition &defn) {
    internal_verify(funcs.find(defn.uuid) == funcs.end(), "Function with duplicate UUID detected: #%zu", defn.uuid);
    funcs[defn.uuid] = std::move(defn);
    return &funcs[defn.uuid];
}

Function_Definition *Functions::get_func_by_uuid(UUID uuid) {
    auto it = funcs.find(uuid);
    if (it == funcs.end()) return nullptr;
    return &funcs[uuid];
}

Module *Modules::add_module(const Module &mod) {
    internal_verify(modules.find(mod.uuid) == modules.end(), "Module with duplicate UUID detected: #%zu", mod.uuid);
    
    modules[mod.uuid] = std::move(mod);
    
    auto sid = mod.module_path.str();
    path_map[sid] = &modules[mod.uuid];
    
    return &modules[mod.uuid];
}

Module *Modules::get_module_by_uuid(UUID uuid) {
    auto it = modules.find(uuid);
    if (it == modules.end()) return nullptr;
    return &modules[uuid];
}

Module *Modules::get_module_by_path(String path) {
    auto sid = path.str();
    auto it = path_map.find(sid);
    if (it == path_map.end()) return nullptr;
    return it->second;
}
