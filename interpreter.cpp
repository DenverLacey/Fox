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
    
    auto source = String::with_size(size);
    file.read(source.c_str(), size);
    verify(file.good(), "Could not read from '%s'.", path);
    
    // guarantee null terminator
    source.c_str()[size] = 0;
    
    return source;
}

void Interpreter::interpret(const char *path) {
    String source = read_entire_file(path);
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

#if TYPECHECK
    auto typed_ast = typecheck(*this, ast);
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    typed_ast->print(this);
#endif
    
#if COMPILE_AST
    Function_Definition program;
    auto global = Compiler { this, constants, str_constants, &program };
    global.compile(typed_ast);
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    printf("<MAIN>:\n");
    print_code(program.bytecode, constants, str_constants);
    
    for (auto &[_, fn] : funcbook.funcs) {
        printf("\n%.*s#%zu%s:\n", fn.name.size(), fn.name.c_str(), fn.uuid, fn.type.debug_str());
        print_code(fn.bytecode, constants, str_constants);
    }
#endif
    
    Mem.clear();
    SMem.clear();
    
#if RUN_VIRTUAL_MACHINE
    auto vm = VM { constants, str_constants };
    vm.call(&program, 0);
    vm.run();
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("------\n");
    vm.print_stack();
#endif
    
#endif // RUN_VIRTUAL_MACHINE
#endif // COMPILE_AST
#endif // TYPECHECK
}

Module *Interpreter::create_module(String module_path) {
    Module mod;
    mod.uuid = next_uuid();
    mod.module_path = module_path;
    modules.add(mod);
    return &modules.last();
}

UUID Interpreter::next_uuid() {
    return current_uuid++;
}

Struct_Definition *Type_Book::add_struct(const Struct_Definition &defn) {
    internal_verify(structs.find(defn.uuid) == structs.end(), "Struct with duplicate UUID detected: #%zu", defn.uuid);
    structs[defn.uuid] = std::move(defn);
    return &structs[defn.uuid];
}

Enum_Definition *Type_Book::add_enum(const Enum_Definition &defn) {
    internal_verify(enums.find(defn.uuid) == enums.end(), "Enum with duplicate UUID detected: #%zu", defn.uuid);
    enums[defn.uuid] = std::move(defn);
    return &enums[defn.uuid];
}

Struct_Definition *Type_Book::get_struct_by_uuid(UUID uuid) {
    auto it = structs.find(uuid);
    if (it == structs.end()) return nullptr;
    return &structs[uuid];
}

Enum_Definition *Type_Book::get_enum_by_uuid(UUID uuid) {
    auto it = enums.find(uuid);
    if (it == enums.end()) return nullptr;
    return &enums[uuid];
}

Function_Definition *Function_Book::add_func(const Function_Definition &defn) {
    internal_verify(funcs.find(defn.uuid) == funcs.end(), "Function with duplicate UUID detected: #%zu", defn.uuid);
    funcs[defn.uuid] = std::move(defn);
    return &funcs[defn.uuid];
}

Function_Definition *Function_Book::get_func_by_uuid(UUID uuid) {
    auto it = funcs.find(uuid);
    if (it == funcs.end()) return nullptr;
    return &funcs[uuid];
}
