//
//  compiler.cpp
//  Fox
//
//  Created by Denver Lacey on 9/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "compiler.h"

#include "error.h"

Compiler::Compiler(Function_Definition *function) {
    stack_top = 0;
    parent = nullptr;
    global = this;
    this->function = function;
}

Compiler::Compiler(Compiler *parent, Function_Definition *function) {
    stack_top = parent->stack_top;
    this->parent = parent;
    global = parent->global;
    this->function = function;
}

Function_Definition *Compiler::compile(Typed_AST *node) {
    node->compile(this);
    return this->function;
}

void Compiler::emit_byte(uint8_t byte) {
    static_assert(sizeof(Opcode) == 1, "This function assumess Opcode is 8 bits large.");
    function->bytecode.push_back((Opcode)byte);
}

void Compiler::emit_opcode(Opcode op) {
    function->bytecode.push_back(op);
}

void Compiler::emit_size(Size size) {
    emit_value<Size>(size);
}

void Compiler::emit_address(Address address) {
    emit_value<Address>(address);
}

size_t Compiler::emit_jump(Opcode jump_code) {
    emit_opcode(jump_code);
    size_t jump = function->bytecode.size();
    emit_value<size_t>(-1);
    return jump;
}

void Compiler::patch_jump(size_t jump) {
    size_t to = function->bytecode.size();
    *(size_t *)&function->bytecode[jump] = to - jump - sizeof(size_t);
}

size_t Compiler::add_constant(void *data, size_t size) {
    size_t alligned_size = (((size + ConstantsAllignment - 1)) / ConstantsAllignment) * ConstantsAllignment;
    
    // search for potential duplicate
    for (size_t i = 0; i < constants.size(); i += ConstantsAllignment) {
        if (i + alligned_size > constants.size()) {
            // this constant can't fit and therefore can't already be in
            // the constants data section
            break;
        }
        
        if (memcmp(data, &constants[i], size) == 0) {
            return i;
        }
    }
    
    // no duplicate so add in the new one
    size_t index = constants.size();
    size_t i;
    for (i = 0; i < size; i++) {
        constants.push_back(*(((uint8_t *)data) + i));
    }
    for (; i < alligned_size; i++) {
        constants.push_back(0);
    }
    
    return index;
}

size_t Compiler::add_str_constant(String source) {
    return 0;
}

void Typed_AST_Bool::compile(Compiler *c) {
    c->emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
}

void Typed_AST_Char::compile(Compiler *c) {
    c->emit_opcode(Opcode::Lit_Char);
    c->emit_value<runtime::Char>(value);
}

void Typed_AST_Float::compile(Compiler *c) {
    size_t constant = c->add_constant<runtime::Float>(value);
    c->emit_opcode(Opcode::Load_Const_Float);
    c->emit_value<size_t>(constant);
}

void Typed_AST_Ident::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Int::compile(Compiler *c) {
    size_t constant = c->add_constant<runtime::Int>(value);
    c->emit_opcode(Opcode::Load_Const_Int);
    c->emit_value<size_t>(constant);
}

void Typed_AST_Str::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Unary::compile(Compiler *c) {
    sub->compile(c);
    switch (kind) {
        case Typed_AST_Kind::Not:
            c->emit_opcode(Opcode::Not);
            break;
            
        default:
            internal_error("Kind is not a valid unary operation: %d.", kind);
            break;
    }
}

void Typed_AST_Binary::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Ternary::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Block::compile(Compiler *c) {
    for (auto &node : nodes) {
        node->compile(c);
    }
}

void Typed_AST_If::compile(Compiler *c) {
    cond->compile(c);
    
    size_t else_jump = c->emit_jump(Opcode::Jump_False);
    size_t exit_jump;
    
    then->compile(c);
    
    if (else_) {
        exit_jump = c->emit_jump(Opcode::Jump);
    }
    c->patch_jump(else_jump);
    
    // else block
    if (else_) {
        else_->compile(c);
        c->patch_jump(exit_jump);
    }
}

void Typed_AST_Type_Signiture::compile(Compiler *c) {
    internal_error("Call to Typed_AST_Type_Signiture::compile() is disallowed.");
}

void Typed_AST_Let::compile(Compiler *c) {
    assert(false);
}
