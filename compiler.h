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
#include <forward_list>

#include "ast.h"
#include "typer.h"
#include "vm.h"
#include "definitions.h"

struct Variable {
    bool is_const;
    Value_Type type;
    Address address;
};

struct Compiler_Scope {
    Address stack_bottom;
    Compiler_Scope *parent;
    std::unordered_map<std::string, Variable> variables;
};

struct Find_Variable_Result {
    enum {
        Not_Found,
        Found,
        Found_Global,
        Found_Constant,
    } status;
    Variable *variable;
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

struct Compiler {
    Address stack_top;
    
    Compiler *parent = nullptr;
    Compiler *global;
    Function_Definition *function;
    
    Interpreter *interp;
    
    Compiler_Scope *global_scope;
    std::forward_list<Compiler_Scope> scopes;
    
    static constexpr size_t Constants_Allignment = 8;
    Data_Section &constants;
    Data_Section &str_constants;
//    int wb_top;
//    AST *ret_statement = nullptr;
//    std::vector<Loop> loops;
//    std::vector<LoopJump> breaks;
//    std::vector<LoopJump> continues;
    
    Compiler(Interpreter *interp, Data_Section &constants, Data_Section &str_constants, Function_Definition *function);
    Compiler(Compiler *parent, Function_Definition *function);
    
    Function_Definition *compile(Ref<Typed_AST_Multiary> node);
    
    void emit_byte(uint8_t byte);
    void emit_opcode(Opcode op);
    void emit_size(Size size);
    void emit_address(Address address);
    size_t emit_jump(Opcode jump_code, bool update_stack_top = true);
    void patch_jump(size_t jump);
    void emit_loop(size_t loop_start);
    Variable &put_variable(String id, Value_Type type, Address address, bool is_const = false);
    void put_variables_from_pattern(Typed_AST_Processed_Pattern &pp, Address address);
    Find_Variable_Result find_variable(String id);
    
    template<typename T>
    void emit_value(T value) {
        for (int i = 0; i < sizeof(T); i++) {
            emit_byte(*(reinterpret_cast<uint8_t *>(&value) + i));
        }
    }
    
    void declare_constant(Typed_AST_Let &let);
    void compile_constant(Variable constant);
    
    size_t add_constant(void *data, size_t size);
    size_t add_slice_constant(size_t size, char *source);
    void *get_constant(size_t constant);
    
    template<typename T>
    size_t add_constant(T constant) {
        return add_constant(&constant, sizeof(T));
    }
    
    template<typename T>
    T get_constant(size_t constant) {
        return *reinterpret_cast<T *>(get_constant(constant));
    }
    
    Compiler_Scope &current_scope();
    void begin_scope();
    void end_scope();
    
    template<typename T>
    bool evaluate(Ref<Typed_AST> expression, T &out_result) {
        if (!expression->is_constant(*this)) {
            return false;
        }
        evaluate_unchecked(expression, out_result);
        return true;
    }
    
    template<typename T>
    void evaluate_unchecked(Ref<Typed_AST> expression, T &out_result) {
        Function_Definition code;
        Function_Definition *old_func = function;
        function = &code;
        expression->compile(*this);
        function = old_func;
        
        auto vm = VM { constants, str_constants };
        vm.call(&code, 0);
        vm.run();
        
        if constexpr (std::is_pointer_v<T>) {
            void *result = vm.stack.get(0);
            memcpy(out_result, result, expression->type.size());
        } else {
            out_result = *reinterpret_cast<T *>(vm.stack.get(0));
        }
    }
};
