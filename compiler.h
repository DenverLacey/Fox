//
//  compiler.h
//  Fox
//
//  Created by Denver Lacey on 9/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <list> // replace this with something that doesn't totally suck

#include "ast.h"
#include "typer.h"
#include "vm.h"

//struct Variable {
//    Value_Type inferred_type;
//    Address address;
//    int len;
//    const char *name;
//};

//struct Scope {
//    int stack_bottom;
//    Scope *parent;
//    std::list<Scope> children;
//    std::list<Variable> variables;
//    std::vector<AST *> defers;
//};

//struct Loop {
//    int stack_bottom;
//    int loop_start;
//    int depth;
//    Identifier *label;
//};

//struct LoopJump {
//    int depth;
//    size_t jump;
//};

//using PolyTable = std::unordered_map<std::string, ValueType>;

struct Function_Definition;
struct Compiler {
    int stack_top;
//    int wb_top;
    Compiler *parent = nullptr;
//    Compiler *child  = nullptr;
    Compiler *global;
//    VM *vm;
    Function_Definition *function;
//    bool has_return;
    // Scope global_scope;
    // Scope *current_scope;
//    AST *ret_statement = nullptr;
//    std::vector<Loop> loops;
//    std::vector<LoopJump> breaks;
//    std::vector<LoopJump> continues;
//    std::list<StructDefinition> struct_defns; // see above
//    std::list<EnumDefinition> enum_defns; // see above
//    std::vector<FunctionDefinition *> fn_defns;
//    std::vector<FnDeclaration *> poly_fn_decls;
//    std::vector<PolyTable> poly_params;
//    std::list<FunctionDefinition> *fn_buffer; // see above
    
    Compiler(Function_Definition *function);
    Compiler(Compiler *parent, Function_Definition *function);
    
    Function_Definition *compile(Typed_AST *node);
    
    void emit_byte(uint8_t byte);
    void emit_opcode(Opcode op);
    void emit_size(Size size);
    void emit_address(Address address);
    
    template<typename T>
    void emit_value(T value) {
        for (int i = 0; i < sizeof(T); i++) {
            emit_byte(*(((uint8_t *)&value) + i));
        }
    }
};
