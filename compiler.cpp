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
    scopes.push_front({ 0, nullptr });
}

Compiler::Compiler(Compiler *parent, Function_Definition *function) {
    stack_top = parent->stack_top;
    this->parent = parent;
    global = parent->global;
    this->function = function;
    scopes.push_front({ 0, nullptr });
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

Variable &Compiler::emit_variable(String id, Typed_AST *initializer) {
    std::string sid(id.c_str(), id.size());
    Scope &s = current_scope();
    Address address = (Address)stack_top;
    initializer->compile(this);
    Variable v = { initializer->type, address };
    return s.variables[sid] = v;
}

Variable *Compiler::find_variable(String id) {
    std::string sid(id.c_str(), id.size());
    for (Scope *s = &current_scope(); s != nullptr; s = s->parent) {
        auto it = s->variables.find(sid);
        if (it != s->variables.end()) {
            return &it->second;
        }
    }
    return nullptr;
}

size_t Compiler::add_constant(void *data, size_t size) {
    size_t alligned_size = (((size + ConstantsAllignment - 1)) / ConstantsAllignment) * ConstantsAllignment;
    
    // search for identical constant
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
    
    // no identical so add in the new one
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
    // search for identical string
    size_t i = 0;
    while (i < str_constants.size()) {
        size_t index = i;
        size_t len = *(size_t *)&str_constants[i];
        i += sizeof(size_t);
        char *str = (char *)&str_constants[i];
        if (memcmp(source.c_str(), str, len) == 0) {
            return index;
        }
        i += len;
    }
    
    // no identical so add new string
    size_t index = str_constants.size();
    size_t len = source.size();
    char *str = source.c_str();
    
    // write 64 bit length into buffer
    for (int i = 0; i < sizeof(size_t); i++) {
        str_constants.push_back(*(((uint8_t *)&len) + i));
    }
    
    // write string into buffer
    for (size_t i = 0; i < len; i++) {
        str_constants.push_back(str[i]);
    }
    
    return index;
}

Scope &Compiler::current_scope() {
    return scopes.front();
}

void Compiler::begin_scope() {
    Scope &parent = current_scope();
    Scope next;
    next.stack_bottom = stack_top;
    next.parent = &parent;
    scopes.push_front(next);
}

void Compiler::end_scope() {
    scopes.pop_front();
}

void Typed_AST_Bool::compile(Compiler *c) {
    c->emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
    c->stack_top += type.size();
}

void Typed_AST_Char::compile(Compiler *c) {
    c->emit_opcode(Opcode::Lit_Char);
    c->emit_value<runtime::Char>(value);
    c->stack_top += type.size();
}

void Typed_AST_Float::compile(Compiler *c) {
    size_t constant = c->add_constant<runtime::Float>(value);
    c->emit_opcode(Opcode::Load_Const_Float);
    c->emit_value<size_t>(constant);
    c->stack_top += type.size();
}

void Typed_AST_Ident::compile(Compiler *c) {
    Variable *v = c->find_variable(id);
    verify(v, "Unresolved identifier '%s'.", id.c_str());
    internal_verify(v->type == type, "In Typed_AST_Ident::compile(), v->type (%s) != type (%s).", v->type.debug_str(), type.debug_str());
    c->emit_opcode(Opcode::Push_Value);
    c->emit_size(v->type.size());
    c->emit_address(v->address);
    c->stack_top += v->type.size();
}

void Typed_AST_Int::compile(Compiler *c) {
    size_t constant = c->add_constant<runtime::Int>(value);
    c->emit_opcode(Opcode::Load_Const_Int);
    c->emit_value<size_t>(constant);
    c->stack_top += type.size();
}

void Typed_AST_Str::compile(Compiler *c) {
    size_t constant = c->add_str_constant(value);
    c->emit_opcode(Opcode::Load_Const_String);
    c->emit_value<size_t>(constant);
    c->stack_top += type.size();
}

void Typed_AST_Unary::compile(Compiler *c) {
    int stack_top = c->stack_top;
    sub->compile(c);
    switch (kind) {
        case Typed_AST_Kind::Not:
            c->emit_opcode(Opcode::Not);
            break;
            
        default:
            internal_error("Kind is not a valid unary operation: %d.", kind);
            break;
    }
    c->stack_top = stack_top + type.size();
}

void Typed_AST_Binary::compile(Compiler *c) {
    assert(false);
    int stack_top = c->stack_top;
    c->stack_top = stack_top + type.size();
}

void Typed_AST_Ternary::compile(Compiler *c) {
    assert(false);
    int stack_top = c->stack_top;
    c->stack_top = stack_top + type.size();
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
    int stack_top = c->stack_top;
    c->emit_variable(id, initializer.get());
    c->stack_top = stack_top + initializer->type.size();
}
