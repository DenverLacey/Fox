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

#define PRINT_DEBUG_DIAGNOSTICS  1 && defined(DEBUG)
#define PRINT_STACK              1 && defined(DEBUG)
#define TYPECHECK                1 || defined(NDEBUG)
#define COMPILE_AST              1 || defined(NDEBUG)
#define RUN_VIRTUAL_MACHINE      1 || defined(NDEBUG)

Interpreter::Interpreter() {
    load_builtins(this);
}

static String read_entire_file(const char *path) {
    auto dummy_loc = Code_Location{ 0, 0, "<read_entire_file>" };

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    verify(file.is_open(), dummy_loc, "'%s' could not be opened.", path);
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    String source = String::with_null_terminator(size);
    file.read(source.c_str(), size);
    verify(file.good(), dummy_loc, "Could not read from '%s'.", path);
    
    return source;
}

void Interpreter::interpret(const char *path) {
    Module *module = compile_module(const_cast<char *>(path));
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    printf("<MAIN>:\n");
    print_code(module->top_level.instructions, constants, str_constants);
    
    for (auto &[_, fn] : functions.funcs) {
        printf("\n%.*s#%zu%s:\n", fn.name.size(), fn.name.c_str(), fn.uuid, fn.type.debug_str());
        print_code(fn.instructions, constants, str_constants);
    }
#endif
    
#if COMPILE_AST && RUN_VIRTUAL_MACHINE
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
#endif
    
    auto vm = VM { constants, str_constants };
    vm.call(&module->top_level, 0);
    vm.run();
    
#if PRINT_DEBUG_DIAGNOSTICS || PRINT_STACK
    printf("------\n");
    vm.print_stack();
#endif
    
#endif // COMPILE_AST && RUN_VIRTUAL_MACHINE

    Mem.clear();
    SMem.clear();
}

Module *Interpreter::create_module(String module_path) {
    Module mod;
    mod.uuid = next_uuid();
    mod.module_path = module_path;
    Module *new_mod = modules.add_module(mod);
    internal_verify(new_mod, "Module couldn't be successfully added to registry.");
    return new_mod;
}

Module *Interpreter::get_module(String module_path) {
    return modules.get_module_by_path(module_path);
}

Module *Interpreter::get_or_create_module(String module_path) {
    if (auto module = get_module(module_path)) {
        return module;
    }
    return create_module(module_path);
}

Module *Interpreter::compile_module(String module_path) {
    if (auto m = get_module(module_path)) {
        return m;
    }
    
    String source = read_entire_file(module_path.c_str());
    auto tokens = tokenize(source, module_path.c_str());
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
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

    Module *module = create_module(module_path);
    
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

void Module::add_submodule(const std::string &id, Module *module) {
    internal_verify(members.find(id) == members.end(), "Attempted to add submodule with a duplicate name '%s'", id.c_str());
    
    members[id] = { Member::Submodule, module->uuid };
}

bool Module::find_member_by_id(const std::string &id, Member &out_member) {
    auto it = members.find(id);
    if (it == members.end()) return false;
    out_member = it->second;
    return true;
}

void Module::merge(Module *other) {
    for (auto &[id, member] : other->members) {
        auto dummy_loc = Code_Location{ 0, 0, "<Module::merge()>" };
        verify(members.find(id) == members.end(), dummy_loc, "While merging 2 modules encountered name conflict. '%s'.", id.c_str());
        members[id] = member;
    }
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

Trait_Definition *Types::add_trait(const Trait_Definition &defn) {
    internal_verify(traits.find(defn.uuid) == traits.end(), "Trait with duplicate UUID detected: #%zu", defn.uuid);
    traits[defn.uuid] = std::move(defn);
    return &traits[defn.uuid];
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

Trait_Definition *Types::get_trait_by_uuid(UUID uuid) {
    auto it = traits.find(uuid);
    if (it == traits.end()) return nullptr;
    return &traits[uuid];
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

void Builtins::add_builtin(const std::string &id, Builtin_Definition builtin) {
    internal_verify(builtins.find(id) == builtins.end(), "Attempted to add a builtin with a duplicate name: '%s'.", id.c_str());
    builtins[id] = builtin;
}

Builtin_Definition *Builtins::get_builtin(const std::string &id) {
    auto it = builtins.find(id);
    if (it == builtins.end()) return nullptr;
    return &builtins[id];
}
