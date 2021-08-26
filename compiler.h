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

struct Variable {
    Value_Type type;
    Address address;
//    String id;
};

struct Compiler_Scope {
    int stack_bottom;
    Compiler_Scope *parent;
    std::unordered_map<std::string, Variable> variables;
};

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
    
    Compiler *parent = nullptr;
    Compiler *global;
    Function_Definition *function;
    
    Compiler_Scope *global_scope;
    std::list<Compiler_Scope> scopes;
    
    static constexpr size_t ConstantsAllignment = 8;
    Data_Section constants;
    Data_Section str_constants;
//    int wb_top;
//    Compiler *child  = nullptr;
//    bool has_return;
    // Compiler_Scope global_scope;
    // Compiler_Scope *current_scope;
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
    
    Function_Definition *compile(Typed_AST_Block *node);
    
    void emit_byte(uint8_t byte);
    void emit_opcode(Opcode op);
    void emit_size(Size size);
    void emit_address(Address address);
    size_t emit_jump(Opcode jump_code);
    void patch_jump(size_t jump);
    Variable &emit_variable(String id, Typed_AST *initializer);
    bool find_variable(String id, Variable * &out_v);
    
    template<typename T>
    void emit_value(T value) {
        for (int i = 0; i < sizeof(T); i++) {
            emit_byte(*(((uint8_t *)&value) + i));
        }
    }
    
    size_t add_constant(void *data, size_t size);
    size_t add_str_constant(String source);
    
    template<typename T>
    size_t add_constant(T constant) {
        return add_constant(&constant, sizeof(T));
    }
    
    Compiler_Scope &current_scope();
    void begin_scope();
    void end_scope();
};
