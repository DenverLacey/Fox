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

void Typed_AST_Bool::compile(Compiler *c) {
    c->emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
}

void Typed_AST_Char::compile(Compiler *c) {
    c->emit_value<runtime::Char>(value);
}

void Typed_AST_Float::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Ident::compile(Compiler *c) {
    assert(false);
}

void Typed_AST_Int::compile(Compiler *c) {
    assert(false);
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

void Typed_AST_Type_Signiture::compile(Compiler *c) {
    internal_error("Call to Typed_AST_Type_Signiture::compile() is disallowed.");
}

void Typed_AST_Let::compile(Compiler *c) {
    assert(false);
}
