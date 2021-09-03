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
    global_scope = nullptr;
    this->function = function;
}

Compiler::Compiler(Compiler *parent, Function_Definition *function) {
    stack_top = parent->stack_top;
    this->parent = parent;
    global = parent->global;
    global_scope = parent->global_scope;
    this->function = function;
}

Function_Definition *Compiler::compile(Ref<Typed_AST_Multiary> multi) {
    begin_scope();
    global_scope = &current_scope();
    for (auto &n : multi->nodes) {
        n->compile(*this);
    }
    
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
    if (jump_code == Opcode::Jump_True ||
          jump_code == Opcode::Jump_False) {
        stack_top -= value_types::Bool.size();
    }
    return jump;
}

void Compiler::patch_jump(size_t jump) {
    size_t to = function->bytecode.size();
    *(size_t *)&function->bytecode[jump] = to - jump - sizeof(size_t);
}

void Compiler::emit_loop(size_t loop_start) {
    emit_opcode(Opcode::Loop);
    size_t jump = function->bytecode.size() - loop_start + sizeof(size_t);
    emit_value<size_t>(jump);
}

Variable &Compiler::put_variable(String id, Value_Type type) {
    std::string sid(id.c_str(), id.size());
    Compiler_Scope &s = current_scope();
    Address address = (Address)stack_top;
    Variable v = { type, address };
    s.variables[sid] = v;
    return s.variables[sid];
}

std::pair<bool, Variable *> Compiler::find_variable(String id) {
    std::string sid(id.c_str(), id.size());
    for (Compiler_Scope *s = &current_scope(); s != nullptr; s = s->parent) {
        auto it = s->variables.find(sid);
        if (it != s->variables.end()) {
            return { false, &s->variables[sid] };
        }
    }
    
    // check global scope
    auto it = global_scope->variables.find(sid);
    if (it != global_scope->variables.end()) {
        return { true, &global_scope->variables[sid] };
    }
    
    // nothing was found
    return { false, nullptr };
}

size_t Compiler::add_constant(void *data, size_t size) {
    size_t alligned_size = (((size + Constants_Allignment - 1)) / Constants_Allignment) * Constants_Allignment;
    
    // search for identical constant
    for (size_t i = 0; i < constants.size(); i += Constants_Allignment) {
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
        if (source.size() == len && memcmp(source.c_str(), str, len) == 0) {
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

Compiler_Scope &Compiler::current_scope() {
    return scopes.front();
}

void Compiler::begin_scope() {
    Compiler_Scope *parent = scopes.size() > 0 ? &current_scope() : nullptr;
    Compiler_Scope next{};
    next.stack_bottom = stack_top;
    next.parent = parent;
    scopes.push_front(next);
}

void Compiler::end_scope() {
    Address flush_point = current_scope().stack_bottom;
    emit_opcode(Opcode::Flush);
    emit_address(flush_point);
    scopes.pop_front();
}

void Typed_AST_Bool::compile(Compiler &c) {
    c.emit_opcode(value ? Opcode::Lit_True : Opcode::Lit_False);
    c.stack_top += type.size();
}

void Typed_AST_Char::compile(Compiler &c) {
    c.emit_opcode(Opcode::Lit_Char);
    c.emit_value<runtime::Char>(value);
    c.stack_top += type.size();
}

void Typed_AST_Float::compile(Compiler &c) {
    c.emit_opcode(Opcode::Lit_Float);
    c.emit_value<runtime::Float>(value);
    c.stack_top += type.size();
}

void Typed_AST_Ident::compile(Compiler &c) {
    auto [is_global, v] = c.find_variable(id);
    verify(v, "Unresolved identifier '%s'.", id.c_str());
    internal_verify(v->type == type, "In Typed_AST_Ident::compile(), v->type (%s) != type (%s).", v->type.debug_str(), type.debug_str());
    c.emit_opcode(is_global ? Opcode::Push_Global_Value : Opcode::Push_Value);
    c.emit_size(v->type.size());
    c.emit_address(v->address);
    c.stack_top += v->type.size();
}

void Typed_AST_Int::compile(Compiler &c) {
    if (value == 0) {
        c.emit_opcode(Opcode::Lit_0);
    } else if (value == 1) {
        c.emit_opcode(Opcode::Lit_1);
    } else {
        c.emit_opcode(Opcode::Lit_Int);
        c.emit_value<runtime::Int>(value);
    }
    c.stack_top += type.size();
}

void Typed_AST_Str::compile(Compiler &c) {
    size_t constant = c.add_str_constant(value);
    c.emit_opcode(Opcode::Load_Const_String);
    c.emit_value<size_t>(constant);
    c.stack_top += type.size();
}

struct Find_Static_Address_Result {
    enum {
        Not_Found,
        Found,
        Found_Global
    } status;
    Address address;
};

static Find_Static_Address_Result find_static_address(Compiler &c, Typed_AST &node) {
    auto status = Find_Static_Address_Result::Found;
    Address address;
    
    switch (node.kind) {
        case Typed_AST_Kind::Ident: {
            auto id = dynamic_cast<Typed_AST_Ident *>(&node);
            internal_verify(id, "Failed to cast node to an Ident* in find_static_address.");
            
            auto [is_global, v] = c.find_variable(id->id);
            verify(v, "Unresolved identifier '%.*s'.", id->id.size(), id->id.c_str());
            
            if (is_global) {
                status = Find_Static_Address_Result::Found_Global;
            }
            
            address = v->address;
        } break;
//        case AST_SUBSCRIPT: {
//            Binary *sub = (Binary *)ast;
//            auto lhs_result = find_static_address(c, sub->lhs);
//
//            if (lhs_result.type == FSAR_NOT_FOUND)
//                { type = FSAR_NOT_FOUND; break; }
//
//            // @TODO: can we make this more constant?
//            if (!types_match(c, sub->lhs->inferred_type, TYPE_ARRAY))
//                { type = FSAR_NOT_FOUND; break; }
//
//            // @TODO: implement constant evaluation
//            if (sub->rhs->type != AST_INTEGER)
//                { type = FSAR_NOT_FOUND; break; }
//
//            if (lhs_result.type == FSAR_FOUND_GLOBAL) type = FSAR_FOUND_GLOBAL;
//
//            Integer idx = ((Literal *)sub->rhs)->as.integer;
//            addr = lhs_result.address + idx * size_of_type(c, sub->inferred_type);
//        } break;
//        case AST_DOT: {
//            Binary *dot = (Binary *)ast;
//            AST *lhs = dot->lhs;
//            auto lhs_result = find_static_address(c, lhs);
//
//            if (lhs_result.type == FSAR_NOT_FOUND)
//                { type = FSAR_NOT_FOUND; break; }
//
//            if (lhs_result.type == FSAR_FOUND_GLOBAL) type = FSAR_FOUND_GLOBAL;
//
//            StructDefinition *defn = lhs->inferred_type.data.struct_defn;
//            Identifier *mem_id = (Identifier *)dot->rhs;
//
//            auto member = std::find_if(defn->members.begin(), defn->members.end(),
//            [mem_id] (const StructMember &m) {
//                return mem_id->len == m.len && memcmp(mem_id->s, m.ident, m.len) == 0;
//            });
//            verify(member != defn->members.end(), mem_id->location, "'%.*s' is not a member of '%.*s'.", mem_id->len, mem_id->s, defn->len_ident, defn->ident);
//
//            addr = lhs_result.address + member->offset;
//        } break;
            
        default:
            status = Find_Static_Address_Result::Not_Found;
            break;
    }
    
    return { status, address };
}

static bool emit_dynamic_address_code(Compiler &c, Typed_AST &node) {
    switch (node.kind) {
        case Typed_AST_Kind::Ident: {
            auto id = dynamic_cast<Typed_AST_Ident *>(&node);
            internal_verify(id, "Failed to cast node to Ident* in emit_dynamic_address_code().");
            
            auto [is_global, v] = c.find_variable(id->id);
            verify(v, "Unresolved identifier '%.*s'.", id->id.size(), id->id.c_str());
            
            c.emit_opcode(is_global ? Opcode::Push_Global_Pointer : Opcode::Push_Pointer);
            c.emit_address(v->address);
        } break;
        case Typed_AST_Kind::Deref: {
            auto deref = dynamic_cast<Typed_AST_Unary *>(&node);
            internal_verify(deref, "Failed to cast node to Unary* in emit_dynamic_address_code().");
            deref->sub->compile(c);
        } break;
//        case AST_SUBSCRIPT: {
//            Binary *sub = (Binary *)ast;
//
//            ValueType *element_type = get_subtype(c, sub->lhs->inferred_type);
//            TypeSize size = size_of_type(c, *element_type);
//
//            if (!emit_address_bytecode(c, sub->lhs)) return false;
//            if (types_match(c, sub->lhs->inferred_type, TYPE_SLICE)) {
//                emit_byte(c, BYTE_LOAD);
//                emit_size(c, size_of_type(PTR_TYPE));
//            }
//
//            emit_byte(c, BYTE_LOAD_CONST);
//            emit_size(c, size_of_type(INT_TYPE));
//            emit_value<Integer>(c, size);
//            emit_bytecode(c, sub->rhs);
//            emit_byte(c, BYTE_INTEGER_MUL);
//
//            emit_byte(c, BYTE_INTEGER_ADD);
//        } break;
//        case AST_DOT: {
//            Binary *dot = (Binary *)ast;
//
//            verify(types_match(c, dot->lhs->inferred_type, TYPE_STRUCT), dot->lhs->location, "Type mismatch! Expected a struct type but was given '%s'.", type_to_string(dot->lhs->inferred_type));
//
//            StructDefinition *defn = dot->lhs->inferred_type.data.struct_defn;
//            Identifier *mem_id = (Identifier *)dot->rhs;
//
//            auto member = std::find_if(defn->members.begin(), defn->members.end(),
//            [mem_id] (const StructMember &m) {
//                return mem_id->len == m.len && memcmp(mem_id->s, m.ident, m.len) == 0;
//            });
//
//            verify(member != defn->members.end(), mem_id->location, "'%.*s' is not a member of '%s'.", mem_id->len, mem_id->s, type_to_string(dot->lhs->inferred_type));
//
//            emit_address_bytecode(c, dot->lhs);
//            emit_byte(c, BYTE_LOAD_CONST);
//            emit_size(c, sizeof(Integer));
//            emit_value<Integer>(c, member->offset);
//            emit_byte(c, BYTE_INTEGER_ADD);
//        } break;
//        case AST_DEREF_DOT:
//            assert(false);
            
            
        default:
            return false;
    }
    
    return true;
}

static bool emit_address_code(Compiler &c, Typed_AST &node) {
    int stack_top = c.stack_top;
    bool success = true;
    
    auto [status, address] = find_static_address(c, node);
    
    auto push_type = Opcode::Push_Pointer;
    switch (status) {
        case Find_Static_Address_Result::Found_Global:
            push_type = Opcode::Push_Global_Pointer;
        case Find_Static_Address_Result::Found:
            c.emit_opcode(push_type);
            c.emit_address(address);
            break;
        case Find_Static_Address_Result::Not_Found:
            success = emit_dynamic_address_code(c, node);
            break;
            
        default:
            internal_error("Invalid status in emit_address_code(): %d.", status);
            break;
    }
    
    c.stack_top = stack_top + value_types::Ptr.size();
    return success;
}

void Typed_AST_Unary::compile(Compiler &c) {
    int stack_top = c.stack_top;
    switch (kind) {
        case Typed_AST_Kind::Negation:
            sub->compile(c);
            if (sub->type.kind == Value_Type_Kind::Int)
                c.emit_opcode(Opcode::Int_Neg);
            else
                c.emit_opcode(Opcode::Float_Neg);
            break;
        case Typed_AST_Kind::Not:
            sub->compile(c);
            c.emit_opcode(Opcode::Not);
            break;
        case Typed_AST_Kind::Address_Of:
        case Typed_AST_Kind::Address_Of_Mut:
            emit_address_code(c, *sub);
            break;
        case Typed_AST_Kind::Deref: {
            Size size = sub->type.child_type()->size();
            sub->compile(c);
            c.emit_opcode(Opcode::Load);
            c.emit_size(size);
        } break;
            
        default:
            internal_error("Kind is not a valid unary operation: %d.", kind);
            break;
    }
    c.stack_top = stack_top + type.size();
}

static void compile_assignment(Compiler &c, Typed_AST_Binary &b) {
    int stack_top = c.stack_top;
    
    b.rhs->compile(c);
    bool success = emit_address_code(c, *b.lhs);
    verify(success, "Cannot assign to this kind of expression.");
        
    Size size = b.rhs->type.size();
    c.emit_opcode(Opcode::Move);
    c.emit_size(size);
    
    c.stack_top = stack_top;
}

void compile_while_loop(Compiler &c, Typed_AST_Binary &b) {
    size_t loop_start = c.function->bytecode.size();
    int stack_top = c.stack_top;
    
    b.lhs->compile(c);
    size_t exit_jump = c.emit_jump(Opcode::Jump_False);
    
    b.rhs->compile(c);
    c.emit_loop(loop_start);
    c.patch_jump(exit_jump);
    
    c.stack_top = stack_top;
}

void compile_logical_operator(Compiler &c, Typed_AST_Binary &b) {
    int stack_top = c.stack_top;
    Size size_of_bool = value_types::Bool.size();
    
    b.lhs->compile(c);
    
    size_t jump;
    switch (b.kind) {
        case Typed_AST_Kind::And:
            jump = c.emit_jump(Opcode::Jump_False_No_Pop);
            c.emit_opcode(Opcode::Pop);
            c.emit_size(size_of_bool);
            break;
        case Typed_AST_Kind::Or:
            jump = c.emit_jump(Opcode::Jump_True_No_Pop);
            c.emit_opcode(Opcode::Pop);
            c.emit_size(size_of_bool);
            break;
        default:
            internal_error("Invalid node passed to compile_logical_operator(): %d.", b.kind);
            break;
    }
    
    b.rhs->compile(c);
    c.patch_jump(jump);
    
    c.stack_top = stack_top + size_of_bool;
}

void compile_tuple_dot_operator(Compiler &c, Typed_AST_Binary &dot) {
    int stack_top = c.stack_top;
    
    auto i = dot.rhs.cast<Typed_AST_Int>();
    Size offset = dot.lhs->type.data.tuple.offset_of_type(i->value);
    
    auto [status, address] = find_static_address(c, *dot.lhs);
    switch (status) {
        case Find_Static_Address_Result::Found:
            c.emit_opcode(Opcode::Push_Value);
            c.emit_size(dot.type.size());
            c.emit_address(address + offset);
            break;
        case Find_Static_Address_Result::Found_Global:
            c.emit_opcode(Opcode::Push_Global_Value);
            c.emit_size(dot.type.size());
            c.emit_address(address + offset);
            break;
        case Find_Static_Address_Result::Not_Found:
            bool success = emit_dynamic_address_code(c, *dot.lhs);
            verify(success, "Cannot access this value.");
            if (offset == 0) {
                // do nothing
            } else if (offset == 1) {
                c.emit_opcode(Opcode::Lit_1);
                c.emit_opcode(Opcode::Int_Add);
            } else {
                c.emit_opcode(Opcode::Lit_Int);
                c.emit_value<runtime::Int>(offset);
                c.emit_opcode(Opcode::Int_Add);
            }
            break;
    }
    
    c.stack_top = stack_top + dot.type.size();
}

void Typed_AST_Binary::compile(Compiler &c) {
    int stack_top = c.stack_top;
    
    switch (kind) {
        case Typed_AST_Kind::Assignment:
            compile_assignment(c, *this);
            return;
        case Typed_AST_Kind::While:
            compile_while_loop(c, *this);
            return;
        case Typed_AST_Kind::Equal:
            lhs->compile(c);
            rhs->compile(c);
            if (lhs->type.kind == Value_Type_Kind::Str) {
                c.emit_opcode(Opcode::Str_Equal);
            } else {
                c.emit_opcode(Opcode::Equal);
                c.emit_size(lhs->type.size());
            }
            c.stack_top = stack_top + value_types::Bool.size();
            return;
        case Typed_AST_Kind::Not_Equal:
            lhs->compile(c);
            rhs->compile(c);
            if (lhs->type.kind == Value_Type_Kind::Str) {
                c.emit_opcode(Opcode::Str_Not_Equal);
            } else {
                c.emit_opcode(Opcode::Not_Equal);
                c.emit_size(lhs->type.size());
            }
            c.stack_top = stack_top + value_types::Bool.size();
            return;
        case Typed_AST_Kind::And:
        case Typed_AST_Kind::Or:
            compile_logical_operator(c, *this);
            return;
        case Typed_AST_Kind::Dot_Tuple:
            compile_tuple_dot_operator(c, *this);
            return;
    }
    
    Opcode op;
    switch (kind) {
        case Typed_AST_Kind::Addition:
            if (type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Add;
            else if (type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Add;
            else if (type.kind == Value_Type_Kind::Str)
                op = Opcode::Str_Add;
            break;
        case Typed_AST_Kind::Subtraction:
            if (type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Sub;
            else if (type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Sub;
            break;
        case Typed_AST_Kind::Multiplication:
            if (type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Mul;
            else if (type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Mul;
            break;
        case Typed_AST_Kind::Division:
            if (type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Div;
            else if (type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Div;
            break;
        case Typed_AST_Kind::Mod:
            op = Opcode::Mod;
            break;
            
        case Typed_AST_Kind::Less:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Less_Than;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Less_Than;
            break;
        case Typed_AST_Kind::Less_Eq:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Less_Equal;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Less_Equal;
            break;
        case Typed_AST_Kind::Greater:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Greater_Than;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Greater_Than;
            break;
        case Typed_AST_Kind::Greater_Eq:
            if (lhs->type.kind == Value_Type_Kind::Int)
                op = Opcode::Int_Greater_Equal;
            else if (lhs->type.kind == Value_Type_Kind::Float)
                op = Opcode::Float_Greater_Equal;
            break;
            
        default:
            internal_error("Invalid binary operation: %d.", kind);
            break;
    }
    
    lhs->compile(c);
    rhs->compile(c);
    c.emit_opcode(op);
    
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Ternary::compile(Compiler &c) {
    assert(false);
    int stack_top = c.stack_top;
    c.stack_top = stack_top + type.size();
}

void Typed_AST_Multiary::compile(Compiler &c) {
    if (kind == Typed_AST_Kind::Block) c.begin_scope();
    for (auto &node : nodes) {
        node->compile(c);
    }
    if (kind == Typed_AST_Kind::Block) c.end_scope();
}

void Typed_AST_Array::compile(Compiler &c) {
    if (kind == Typed_AST_Kind::Array) {
        element_nodes->compile(c);
    } else {
        internal_error("Slices not yet compilable.");
    }
}

void Typed_AST_If::compile(Compiler &c) {
    cond->compile(c);
    size_t else_jump = c.emit_jump(Opcode::Jump_False);
    size_t exit_jump;
    
    then->compile(c);
    if (else_) exit_jump = c.emit_jump(Opcode::Jump);
    c.patch_jump(else_jump);
    
    if (else_) {
        else_->compile(c);
        c.patch_jump(exit_jump);
    }
}

void Typed_AST_Type_Signiture::compile(Compiler &c) {
    internal_error("Call to Typed_AST_Type_Signiture::compile() is disallowed.");
}

void Typed_AST_Let::compile(Compiler &c) {
    int stack_top = c.stack_top;
    
    Value_Type type = initializer ? initializer->type : *specified_type->value_type;
    c.put_variable(id, type);
    
    if (initializer) {
        initializer->compile(c);
    } else {
        c.emit_opcode(Opcode::Clear_Allocate);
        c.emit_size(type.size());
    }
    
    c.stack_top = stack_top + type.size();
}
